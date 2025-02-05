// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BASE_DEMUXER_STREAM_FOR_TEST_H_
#define CHROMECAST_MEDIA_CMA_BASE_DEMUXER_STREAM_FOR_TEST_H_

#include <list>

#include "base/bind.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromecast/media/cma/base/demuxer_stream_adapter.h"
#include "media/base/decoder_buffer.h"
#include "media/base/demuxer_stream.h"

namespace chromecast {
namespace media {

class DemuxerStreamForTest : public ::media::DemuxerStream {
 public:
  // Creates a demuxer stream which provides frames either with a delay
  // or instantly.
  // - |total_frames| is the number of frames to generate before EOS frame.
  //   -1 means keep generating frames and never produce EOS.
  // The scheduling pattern is the following:
  // - provides |delayed_frame_count| frames with a delay,
  // - then provides the following |cycle_count| - |delayed_frame_count|
  //   instantly,
  // - then provides |delayed_frame_count| frames with a delay,
  // - ... and so on.
  // Special cases:
  // - all frames are delayed: |delayed_frame_count| = |cycle_count|
  // - all frames are provided instantly: |delayed_frame_count| = 0
  // |config_idx| is a list of frame index before which there is
  // a change of decoder configuration.
  DemuxerStreamForTest(int total_frames,
                       int cycle_count,
                       int delayed_frame_count,
                       const std::list<int>& config_idx);
  ~DemuxerStreamForTest() override;

  // ::media::DemuxerStream implementation.
  void Read(const ReadCB& read_cb) override;
  ::media::AudioDecoderConfig audio_decoder_config() override;
  ::media::VideoDecoderConfig video_decoder_config() override;
  Type type() const override;
  bool SupportsConfigChanges() override;
  ::media::VideoRotation video_rotation() override;
  bool enabled() const override;
  void set_enabled(bool enabled, base::TimeDelta time) override;
  void SetStreamRestartedCB(const StreamRestartedCB& cb) override;

  bool has_pending_read() const { return has_pending_read_; }

  // Frame duration
  static const int kDemuxerStreamForTestFrameDuration = 40;

 private:
  void DoRead(const ReadCB& read_cb);

  // Demuxer configuration.
  int total_frame_count_;
  const int cycle_count_;
  const int delayed_frame_count_;
  std::list<int> config_idx_;

  // Number of frames sent so far.
  int frame_count_;

  bool has_pending_read_;

  DISALLOW_COPY_AND_ASSIGN(DemuxerStreamForTest);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BASE_DEMUXER_STREAM_FOR_TEST_H_
