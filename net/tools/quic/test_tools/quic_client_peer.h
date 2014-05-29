// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_TEST_TOOLS_QUIC_CLIENT_PEER_H_
#define NET_TOOLS_QUIC_TEST_TOOLS_QUIC_CLIENT_PEER_H_

#include "base/basictypes.h"

namespace net {

class QuicCryptoClientConfig;

namespace tools {

class QuicClient;

namespace test {

class QuicClientPeer {
 public:
  static QuicCryptoClientConfig* GetCryptoConfig(QuicClient* client);
  static bool CreateUDPSocket(QuicClient* client);
  static void SetClientPort(QuicClient* client, int port);

 private:
  DISALLOW_COPY_AND_ASSIGN(QuicClientPeer);
};

}  // namespace test
}  // namespace tools
}  // namespace net

#endif  // NET_TOOLS_QUIC_TEST_TOOLS_QUIC_CLIENT_PEER_H_
