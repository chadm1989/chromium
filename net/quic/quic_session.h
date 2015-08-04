// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A QuicSession, which demuxes a single connection to individual streams.

#ifndef NET_QUIC_QUIC_SESSION_H_
#define NET_QUIC_QUIC_SESSION_H_

#include <map>
#include <string>
#include <vector>

#include "build/build_config.h"

// TODO(rtenneti): Temporary while investigating crbug.com/473893.
//                 Note base::Debug::StackTrace() is not supported in NACL
//                 builds so conditionally disabled it there.
#ifndef OS_NACL
#define TEMP_INSTRUMENTATION_473893
#endif

#include "base/compiler_specific.h"
#include "base/containers/hash_tables.h"
#ifdef TEMP_INSTRUMENTATION_473893
#include "base/debug/stack_trace.h"
#endif
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_piece.h"
#include "net/base/ip_endpoint.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_crypto_stream.h"
#include "net/quic/quic_packet_creator.h"
#include "net/quic/quic_protocol.h"
#include "net/quic/quic_write_blocked_list.h"
#include "net/quic/reliable_quic_stream.h"

namespace net {

class QuicCryptoStream;
class QuicFlowController;
class ReliableQuicStream;
class VisitorShim;

namespace test {
class QuicSessionPeer;
}  // namespace test

class NET_EXPORT_PRIVATE QuicSession : public QuicConnectionVisitorInterface {
 public:
  // CryptoHandshakeEvent enumerates the events generated by a QuicCryptoStream.
  enum CryptoHandshakeEvent {
    // ENCRYPTION_FIRST_ESTABLISHED indicates that a full client hello has been
    // sent by a client and that subsequent packets will be encrypted. (Client
    // only.)
    ENCRYPTION_FIRST_ESTABLISHED,
    // ENCRYPTION_REESTABLISHED indicates that a client hello was rejected by
    // the server and thus the encryption key has been updated. Therefore the
    // connection should resend any packets that were sent under
    // ENCRYPTION_INITIAL. (Client only.)
    ENCRYPTION_REESTABLISHED,
    // HANDSHAKE_CONFIRMED, in a client, indicates the the server has accepted
    // our handshake. In a server it indicates that a full, valid client hello
    // has been received. (Client and server.)
    HANDSHAKE_CONFIRMED,
  };

  QuicSession(QuicConnection* connection, const QuicConfig& config);

  ~QuicSession() override;

  virtual void Initialize();

  // QuicConnectionVisitorInterface methods:
  void OnStreamFrame(const QuicStreamFrame& frame) override;
  void OnRstStream(const QuicRstStreamFrame& frame) override;
  void OnGoAway(const QuicGoAwayFrame& frame) override;
  void OnWindowUpdateFrame(const QuicWindowUpdateFrame& frame) override;
  void OnBlockedFrame(const QuicBlockedFrame& frame) override;
  void OnConnectionClosed(QuicErrorCode error, bool from_peer) override;
  void OnWriteBlocked() override {}
  void OnSuccessfulVersionNegotiation(const QuicVersion& version) override;
  void OnCanWrite() override;
  void OnCongestionWindowChange(QuicTime now) override {}
  bool WillingAndAbleToWrite() const override;
  bool HasPendingHandshake() const override;
  bool HasOpenDynamicStreams() const override;

  // Called by streams when they want to write data to the peer.
  // Returns a pair with the number of bytes consumed from data, and a boolean
  // indicating if the fin bit was consumed.  This does not indicate the data
  // has been sent on the wire: it may have been turned into a packet and queued
  // if the socket was unexpectedly blocked.  |fec_protection| indicates if
  // data is to be FEC protected. Note that data that is sent immediately
  // following MUST_FEC_PROTECT data may get protected by falling within the
  // same FEC group.
  // If provided, |ack_notifier_delegate| will be registered to be notified when
  // we have seen ACKs for all packets resulting from this call.
  virtual QuicConsumedData WritevData(
      QuicStreamId id,
      const QuicIOVector& iov,
      QuicStreamOffset offset,
      bool fin,
      FecProtection fec_protection,
      QuicAckNotifier::DelegateInterface* ack_notifier_delegate);

  // Called by streams when they want to close the stream in both directions.
  virtual void SendRstStream(QuicStreamId id,
                             QuicRstStreamErrorCode error,
                             QuicStreamOffset bytes_written);

  // Called when the session wants to go away and not accept any new streams.
  void SendGoAway(QuicErrorCode error_code, const std::string& reason);

  // Removes the stream associated with 'stream_id' from the active stream map.
  virtual void CloseStream(QuicStreamId stream_id);

  // Returns true if outgoing packets will be encrypted, even if the server
  // hasn't confirmed the handshake yet.
  virtual bool IsEncryptionEstablished();

  // For a client, returns true if the server has confirmed our handshake. For
  // a server, returns true if a full, valid client hello has been received.
  virtual bool IsCryptoHandshakeConfirmed();

  // Called by the QuicCryptoStream when a new QuicConfig has been negotiated.
  virtual void OnConfigNegotiated();

  // Called by the QuicCryptoStream when the handshake enters a new state.
  //
  // Clients will call this function in the order:
  //   ENCRYPTION_FIRST_ESTABLISHED
  //   zero or more ENCRYPTION_REESTABLISHED
  //   HANDSHAKE_CONFIRMED
  //
  // Servers will simply call it once with HANDSHAKE_CONFIRMED.
  virtual void OnCryptoHandshakeEvent(CryptoHandshakeEvent event);

  // Called by the QuicCryptoStream when a handshake message is sent.
  virtual void OnCryptoHandshakeMessageSent(
      const CryptoHandshakeMessage& message);

  // Called by the QuicCryptoStream when a handshake message is received.
  virtual void OnCryptoHandshakeMessageReceived(
      const CryptoHandshakeMessage& message);

  // Returns mutable config for this session. Returned config is owned
  // by QuicSession.
  QuicConfig* config();

  // Returns true if the stream existed previously and has been closed.
  // Returns false if the stream is still active or if the stream has
  // not yet been created.
  bool IsClosedStream(QuicStreamId id);

  QuicConnection* connection() {
    // TODO(rtenneti): Temporary while investigating crbug.com/473893
    CrashIfInvalid();
    return connection_.get();
  }
  const QuicConnection* connection() const {
    // TODO(rtenneti): Temporary while investigating crbug.com/473893
    CrashIfInvalid();
    return connection_.get();
  }
  size_t num_active_requests() const { return dynamic_stream_map_.size(); }
  const IPEndPoint& peer_address() const {
    return connection_->peer_address();
  }
  QuicConnectionId connection_id() const {
    return connection_->connection_id();
  }

  // Returns the number of currently open streams, including those which have
  // been implicitly created, but excluding the reserved headers and crypto
  // streams.
  virtual size_t GetNumOpenStreams() const;

  // Add the stream to the session's write-blocked list because it is blocked by
  // connection-level flow control but not by its own stream-level flow control.
  // The stream will be given a chance to write when a connection-level
  // WINDOW_UPDATE arrives.
  void MarkConnectionLevelWriteBlocked(QuicStreamId id, QuicPriority priority);

  // Returns true if the session has data to be sent, either queued in the
  // connection, or in a write-blocked stream.
  bool HasDataToWrite() const;

  bool goaway_received() const { return goaway_received_; }

  bool goaway_sent() const { return goaway_sent_; }

  QuicErrorCode error() const { return error_; }

  Perspective perspective() const { return connection_->perspective(); }

  QuicFlowController* flow_controller() { return &flow_controller_; }

  // Returns true if connection is flow controller blocked.
  bool IsConnectionFlowControlBlocked() const;

  // Returns true if any stream is flow controller blocked.
  bool IsStreamFlowControlBlocked();

  // Returns true if this is a secure QUIC session.
  bool IsSecure() const { return connection()->is_secure(); }

  size_t get_max_open_streams() const { return max_open_streams_; }

  ReliableQuicStream* GetStream(const QuicStreamId stream_id);

  // Mark a stream as draining.
  void StreamDraining(QuicStreamId id);

 protected:
  typedef base::hash_map<QuicStreamId, ReliableQuicStream*> StreamMap;

  // Creates a new stream, owned by the caller, to handle a peer-initiated
  // stream.  Returns nullptr and does error handling if the stream can not be
  // created.
  virtual ReliableQuicStream* CreateIncomingDynamicStream(QuicStreamId id) = 0;

  // Create a new stream, owned by the caller, to handle a locally-initiated
  // stream.  Returns nullptr if max streams have already been opened.
  virtual ReliableQuicStream* CreateOutgoingDynamicStream() = 0;

  // Return the reserved crypto stream.
  virtual QuicCryptoStream* GetCryptoStream() = 0;

  // Adds 'stream' to the active stream map.
  virtual void ActivateStream(ReliableQuicStream* stream);

  // Returns the stream id for a new stream.
  QuicStreamId GetNextStreamId();

  ReliableQuicStream* GetIncomingDynamicStream(QuicStreamId stream_id);

  ReliableQuicStream* GetDynamicStream(const QuicStreamId stream_id);

  // This is called after every call other than OnConnectionClose from the
  // QuicConnectionVisitor to allow post-processing once the work has been done.
  // In this case, it deletes streams given that it's safe to do so (no other
  // operations are being done on the streams at this time)
  virtual void PostProcessAfterData();

  StreamMap& static_streams() { return static_stream_map_; }
  const StreamMap& static_streams() const { return static_stream_map_; }

  StreamMap& dynamic_streams() { return dynamic_stream_map_; }
  const StreamMap& dynamic_streams() const { return dynamic_stream_map_; }

  std::vector<ReliableQuicStream*>* closed_streams() {
    return &closed_streams_;
  }

  void set_max_open_streams(size_t max_open_streams);

  void set_largest_peer_created_stream_id(
      QuicStreamId largest_peer_created_stream_id) {
    largest_peer_created_stream_id_ = largest_peer_created_stream_id;
  }

 private:
  friend class test::QuicSessionPeer;
  friend class VisitorShim;

#ifdef TEMP_INSTRUMENTATION_473893
  // TODO(rtenneti): Temporary while investigating crbug.com/473893
  enum Liveness {
    ALIVE = 0xCA11AB13,
    DEAD = 0xDEADBEEF,
  };
#endif

  // Performs the work required to close |stream_id|.  If |locally_reset|
  // then the stream has been reset by this endpoint, not by the peer.
  void CloseStreamInner(QuicStreamId stream_id, bool locally_reset);

  // When a stream is closed locally, it may not yet know how many bytes the
  // peer sent on that stream.
  // When this data arrives (via stream frame w. FIN, or RST) this method
  // is called, and correctly updates the connection level flow controller.
  void UpdateFlowControlOnFinalReceivedByteOffset(
      QuicStreamId id, QuicStreamOffset final_byte_offset);

  // Called in OnConfigNegotiated when we receive a new stream level flow
  // control window in a negotiated config. Closes the connection if invalid.
  void OnNewStreamFlowControlWindow(QuicStreamOffset new_window);

  // Called in OnConfigNegotiated when we receive a new connection level flow
  // control window in a negotiated config. Closes the connection if invalid.
  void OnNewSessionFlowControlWindow(QuicStreamOffset new_window);

  // Called in OnConfigNegotiated when auto-tuning is enabled for flow
  // control receive windows.
  void EnableAutoTuneReceiveWindow();

  // TODO(rtenneti): Temporary while investigating crbug.com/473893
  void CrashIfInvalid() const;

  // Keep track of highest received byte offset of locally closed streams, while
  // waiting for a definitive final highest offset from the peer.
  std::map<QuicStreamId, QuicStreamOffset>
      locally_closed_streams_highest_offset_;

  scoped_ptr<QuicConnection> connection_;

  // A shim to stand between the connection and the session, to handle stream
  // deletions.
  scoped_ptr<VisitorShim> visitor_shim_;

  std::vector<ReliableQuicStream*> closed_streams_;

  QuicConfig config_;

  // Returns the maximum number of streams this connection can open.
  size_t max_open_streams_;

  // Static streams, such as crypto and header streams. Owned by child classes
  // that create these streams.
  StreamMap static_stream_map_;

  // Map from StreamId to pointers to streams that are owned by the caller.
  StreamMap dynamic_stream_map_;
  QuicStreamId next_stream_id_;

  // Set of stream ids that have been "implicitly created" by receipt
  // of a stream id larger than the next expected stream id.
  base::hash_set<QuicStreamId> implicitly_created_streams_;

  // Set of stream ids that are "draining" -- a FIN has been sent and received,
  // but the stream object still exists because not all the received data has
  // been consumed.
  base::hash_set<QuicStreamId> draining_streams_;

  // A list of streams which need to write more data.
  QuicWriteBlockedList write_blocked_streams_;

  QuicStreamId largest_peer_created_stream_id_;

  // The latched error with which the connection was closed.
  QuicErrorCode error_;

  // Used for connection-level flow control.
  QuicFlowController flow_controller_;

  // Whether a GoAway has been received.
  bool goaway_received_;
  // Whether a GoAway has been sent.
  bool goaway_sent_;

  // Indicate if there is pending data for the crypto stream.
  bool has_pending_handshake_;

#ifdef TEMP_INSTRUMENTATION_473893
  // TODO(rtenneti): Temporary while investigating crbug.com/473893
  Liveness liveness_ = ALIVE;
  base::debug::StackTrace stack_trace_;
#endif

  DISALLOW_COPY_AND_ASSIGN(QuicSession);
};

}  // namespace net

#endif  // NET_QUIC_QUIC_SESSION_H_
