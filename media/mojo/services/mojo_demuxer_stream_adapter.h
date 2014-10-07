// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_DEMUXER_STREAM_ADAPTER_H_
#define MEDIA_MOJO_SERVICES_MOJO_DEMUXER_STREAM_ADAPTER_H_

#include <queue>

#include "base/memory/weak_ptr.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/demuxer_stream.h"
#include "media/base/video_decoder_config.h"
#include "media/mojo/interfaces/demuxer_stream.mojom.h"

namespace media {

// This class acts as a MojoRendererService-side stub for a real
// media::DemuxerStream that is part of a media::Pipeline in a remote
// application. Roughly speaking, it takes a mojo::DemuxerStreamPtr and exposes
// it as a media::DemuxerStream for use by media components.
class MojoDemuxerStreamAdapter : public media::DemuxerStream,
                                 public mojo::DemuxerStreamClient {
 public:
  // |demuxer_stream| is connected to the mojo::DemuxerStream that |this| will
  //     become the client of.
  // |stream_ready_cb| will be invoked when |stream| has fully initialized
  //     and |this| is ready for use.
  // NOTE: Illegal to call any methods until |stream_ready_cb| is invoked.
  MojoDemuxerStreamAdapter(mojo::DemuxerStreamPtr demuxer_stream,
                           const base::Closure& stream_ready_cb);
  virtual ~MojoDemuxerStreamAdapter();

  // media::DemuxerStream implementation.
  virtual void Read(const ReadCB& read_cb) override;
  virtual AudioDecoderConfig audio_decoder_config() override;
  virtual VideoDecoderConfig video_decoder_config() override;
  virtual Type type() override;
  virtual void EnableBitstreamConverter() override;
  virtual bool SupportsConfigChanges() override;
  virtual VideoRotation video_rotation() override;

  // mojo::DemuxerStreamClient implementation.
  virtual void OnStreamReady(mojo::ScopedDataPipeConsumerHandle pipe) override;
  virtual void OnAudioDecoderConfigChanged(
      mojo::AudioDecoderConfigPtr config) override;

 private:
  // The callback from |demuxer_stream_| that a read operation has completed.
  // |read_cb| is a callback from the client who invoked Read() on |this|.
  void OnBufferReady(mojo::DemuxerStream::Status status,
                     mojo::MediaDecoderBufferPtr buffer);

  // See constructor for descriptions.
  mojo::DemuxerStreamPtr demuxer_stream_;
  base::Closure stream_ready_cb_;

  // The last ReadCB received through a call to Read().
  // Used to store the results of OnBufferReady() in the event it is called
  // with DemuxerStream::Status::kConfigChanged and we don't have an up to
  // date AudioDecoderConfig yet. In that case we can't forward the results
  // on to the caller of Read() until OnAudioDecoderConfigChanged is observed.
  DemuxerStream::ReadCB read_cb_;

  // The front of the queue is the current config. We pop when we observe
  // DemuxerStatus::CONFIG_CHANGED.
  std::queue<media::AudioDecoderConfig> config_queue_;

  base::WeakPtrFactory<MojoDemuxerStreamAdapter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(MojoDemuxerStreamAdapter);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_DEMUXER_STREAM_ADAPTER_H_
