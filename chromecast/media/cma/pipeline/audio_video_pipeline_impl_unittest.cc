// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "chromecast/media/base/decrypt_context_impl.h"
#include "chromecast/media/cdm/cast_cdm_context.h"
#include "chromecast/media/cma/backend/audio_decoder_default.h"
#include "chromecast/media/cma/backend/media_pipeline_backend_default.h"
#include "chromecast/media/cma/backend/video_decoder_default.h"
#include "chromecast/media/cma/pipeline/av_pipeline_client.h"
#include "chromecast/media/cma/pipeline/media_pipeline_impl.h"
#include "chromecast/media/cma/pipeline/video_pipeline_client.h"
#include "chromecast/media/cma/test/frame_generator_for_test.h"
#include "chromecast/media/cma/test/mock_frame_provider.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/media_util.h"
#include "media/base/video_decoder_config.h"
#include "media/cdm/player_tracker_impl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// Total number of frames generated by CodedFrameProvider.
// The first frame has config, while the last one is EOS.
const int kNumFrames = 100;
const int kFrameSize = 512;
const int kFrameDurationUs = 40 * 1000;
const int kLastFrameTimestamp = (kNumFrames - 2) * kFrameDurationUs;
}  // namespace

namespace chromecast {
namespace media {

class CastCdmContextForTest : public CastCdmContext {
 public:
  CastCdmContextForTest() : license_installed_(false) {}
  void SetLicenseInstalled() {
    license_installed_ = true;
    player_tracker_.NotifyNewKey();
  }

  // CastCdmContext implementation:
  int RegisterPlayer(const base::Closure& new_key_cb,
                     const base::Closure& cdm_unset_cb) override {
    return player_tracker_.RegisterPlayer(new_key_cb, cdm_unset_cb);
  }

  void UnregisterPlayer(int registration_id) override {
    return player_tracker_.UnregisterPlayer(registration_id);
  }

  std::unique_ptr<DecryptContextImpl> GetDecryptContext(
      const std::string& key_id) override {
    if (license_installed_) {
      return std::unique_ptr<DecryptContextImpl>(
          new DecryptContextImpl(KEY_SYSTEM_CLEAR_KEY));
    } else {
      return std::unique_ptr<DecryptContextImpl>();
    }
  }

  void SetKeyStatus(const std::string& key_id,
                    CastKeyStatus key_status,
                    uint32_t system_code) override {}

 private:
  bool license_installed_;
  base::Closure new_key_cb_;
  ::media::PlayerTrackerImpl player_tracker_;

  DISALLOW_COPY_AND_ASSIGN(CastCdmContextForTest);
};

// Helper class for managing pipeline setup, teardown, feeding data, stop/start
// etc in a simple API for tests to use.
class PipelineHelper {
 public:
  PipelineHelper(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                 bool audio,
                 bool video,
                 bool encrypted)
      : have_audio_(audio),
        have_video_(video),
        encrypted_(encrypted),
        pipeline_backend_(nullptr) {
    eos_[STREAM_AUDIO] = eos_[STREAM_VIDEO] = false;
  }

  void Setup() {
    if (encrypted_) {
      cdm_context_.reset(new CastCdmContextForTest());
    }
    std::unique_ptr<MediaPipelineBackendDefault> backend =
        base::WrapUnique(new MediaPipelineBackendDefault());
    pipeline_backend_ = backend.get();
    media_pipeline_.Initialize(kLoadTypeURL, std::move(backend));

    if (have_audio_) {
      ::media::AudioDecoderConfig audio_config(
          ::media::kCodecMP3, ::media::kSampleFormatS16,
          ::media::CHANNEL_LAYOUT_STEREO, 44100, ::media::EmptyExtraData(),
          ::media::Unencrypted());
      AvPipelineClient client;
      client.eos_cb = base::Bind(&PipelineHelper::OnEos, base::Unretained(this),
                                 STREAM_AUDIO);
      ::media::PipelineStatus status = media_pipeline_.InitializeAudio(
          audio_config, client, CreateFrameProvider());
      ASSERT_EQ(::media::PIPELINE_OK, status);
    }
    if (have_video_) {
      std::vector<::media::VideoDecoderConfig> video_configs;
      video_configs.push_back(::media::VideoDecoderConfig(
          ::media::kCodecH264, ::media::H264PROFILE_MAIN,
          ::media::PIXEL_FORMAT_I420, ::media::COLOR_SPACE_UNSPECIFIED,
          gfx::Size(640, 480), gfx::Rect(0, 0, 640, 480), gfx::Size(640, 480),
          ::media::EmptyExtraData(), ::media::EncryptionScheme()));
      VideoPipelineClient client;
      client.av_pipeline_client.eos_cb = base::Bind(
          &PipelineHelper::OnEos, base::Unretained(this), STREAM_VIDEO);
      ::media::PipelineStatus status = media_pipeline_.InitializeVideo(
          video_configs, client, CreateFrameProvider());
      ASSERT_EQ(::media::PIPELINE_OK, status);
    }
  }

  void Start(const base::Closure& eos_cb) {
    eos_cb_ = eos_cb;
    eos_[STREAM_AUDIO] = !media_pipeline_.HasAudio();
    eos_[STREAM_VIDEO] = !media_pipeline_.HasVideo();
    base::TimeDelta start_time = base::TimeDelta::FromMilliseconds(0);
    media_pipeline_.StartPlayingFrom(start_time);
  }
  void SetCdm() { media_pipeline_.SetCdm(cdm_context_.get()); }
  void Flush(const base::Closure& flush_cb) { media_pipeline_.Flush(flush_cb); }
  void Stop() {
    media_pipeline_.Stop();
    base::MessageLoop::current()->QuitWhenIdle();
  }
  void SetCdmLicenseInstalled() { cdm_context_->SetLicenseInstalled(); }

  MediaPipelineBackendDefault* pipeline_backend() { return pipeline_backend_; }

 private:
  enum Stream { STREAM_AUDIO, STREAM_VIDEO };

  std::unique_ptr<CodedFrameProvider> CreateFrameProvider() {
    std::vector<FrameGeneratorForTest::FrameSpec> frame_specs;
    frame_specs.resize(kNumFrames);
    for (size_t k = 0; k < frame_specs.size() - 1; k++) {
      frame_specs[k].has_config = (k == 0);
      frame_specs[k].timestamp =
          base::TimeDelta::FromMicroseconds(kFrameDurationUs) * k;
      frame_specs[k].size = kFrameSize;
      frame_specs[k].has_decrypt_config = encrypted_;
    }
    frame_specs.back().is_eos = true;

    std::unique_ptr<FrameGeneratorForTest> frame_generator(
        new FrameGeneratorForTest(frame_specs));
    bool provider_delayed_pattern[] = {false, true};
    std::unique_ptr<MockFrameProvider> frame_provider(new MockFrameProvider());
    frame_provider->Configure(
        std::vector<bool>(
            provider_delayed_pattern,
            provider_delayed_pattern + arraysize(provider_delayed_pattern)),
        std::move(frame_generator));
    frame_provider->SetDelayFlush(true);
    return std::move(frame_provider);
  }

  void OnEos(Stream stream) {
    eos_[stream] = true;
    if (eos_[STREAM_AUDIO] && eos_[STREAM_VIDEO] && !eos_cb_.is_null())
      eos_cb_.Run();
  }

  bool have_audio_;
  bool have_video_;
  bool encrypted_;
  bool eos_[2];
  base::Closure eos_cb_;
  MediaPipelineImpl media_pipeline_;
  std::unique_ptr<CastCdmContextForTest> cdm_context_;
  MediaPipelineBackendDefault* pipeline_backend_;

  DISALLOW_COPY_AND_ASSIGN(PipelineHelper);
};

using AudioVideoTuple = ::testing::tuple<bool, bool>;

class AudioVideoPipelineImplTest
    : public ::testing::TestWithParam<AudioVideoTuple> {
 public:
  AudioVideoPipelineImplTest() {}

 protected:
  void SetUp() override {
    pipeline_helper_.reset(new PipelineHelper(
        message_loop_.task_runner(), ::testing::get<0>(GetParam()),
        ::testing::get<1>(GetParam()), false));
    pipeline_helper_->Setup();
  }

  base::MessageLoop message_loop_;
  std::unique_ptr<PipelineHelper> pipeline_helper_;

  DISALLOW_COPY_AND_ASSIGN(AudioVideoPipelineImplTest);
};

static void VerifyPlay(PipelineHelper* pipeline_helper) {
  // The backend must still be running.
  MediaPipelineBackendDefault* backend = pipeline_helper->pipeline_backend();
  EXPECT_TRUE(backend->running());

  // The decoders must have received a few frames.
  const AudioDecoderDefault* audio_decoder = backend->audio_decoder();
  const VideoDecoderDefault* video_decoder = backend->video_decoder();
  ASSERT_TRUE(audio_decoder || video_decoder);
  if (audio_decoder)
    EXPECT_EQ(kLastFrameTimestamp, audio_decoder->last_push_pts());
  if (video_decoder)
    EXPECT_EQ(kLastFrameTimestamp, video_decoder->last_push_pts());

  pipeline_helper->Stop();
}

TEST_P(AudioVideoPipelineImplTest, Play) {
  base::Closure verify_task =
      base::Bind(&VerifyPlay, base::Unretained(pipeline_helper_.get()));
  message_loop_.PostTask(
      FROM_HERE,
      base::Bind(&PipelineHelper::Start,
                 base::Unretained(pipeline_helper_.get()), verify_task));
  message_loop_.Run();
}

static void VerifyFlush(PipelineHelper* pipeline_helper) {
  // The backend must have been stopped.
  MediaPipelineBackendDefault* backend = pipeline_helper->pipeline_backend();
  EXPECT_FALSE(backend->running());

  // The decoders must not have received any frame.
  const AudioDecoderDefault* audio_decoder = backend->audio_decoder();
  const VideoDecoderDefault* video_decoder = backend->video_decoder();
  ASSERT_TRUE(audio_decoder || video_decoder);
  if (audio_decoder)
    EXPECT_LT(audio_decoder->last_push_pts(), 0);
  if (video_decoder)
    EXPECT_LT(video_decoder->last_push_pts(), 0);

  pipeline_helper->Stop();
}

static void VerifyNotReached() {
  EXPECT_TRUE(false);
}

TEST_P(AudioVideoPipelineImplTest, Flush) {
  base::Closure verify_task =
      base::Bind(&VerifyFlush, base::Unretained(pipeline_helper_.get()));
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::Start,
                                    base::Unretained(pipeline_helper_.get()),
                                    base::Bind(&VerifyNotReached)));
  message_loop_.PostTask(
      FROM_HERE,
      base::Bind(&PipelineHelper::Flush,
                 base::Unretained(pipeline_helper_.get()), verify_task));

  message_loop_.Run();
}

TEST_P(AudioVideoPipelineImplTest, FullCycle) {
  base::Closure stop_task = base::Bind(
      &PipelineHelper::Stop, base::Unretained(pipeline_helper_.get()));
  base::Closure eos_cb =
      base::Bind(&PipelineHelper::Flush,
                 base::Unretained(pipeline_helper_.get()), stop_task);

  message_loop_.PostTask(
      FROM_HERE, base::Bind(&PipelineHelper::Start,
                            base::Unretained(pipeline_helper_.get()), eos_cb));
  message_loop_.Run();
};

// Test all three types of pipeline: audio-only, video-only, audio-video.
INSTANTIATE_TEST_CASE_P(
    MediaPipelineImplTests,
    AudioVideoPipelineImplTest,
    ::testing::Values(AudioVideoTuple(true, false),   // Audio only.
                      AudioVideoTuple(false, true),   // Video only.
                      AudioVideoTuple(true, true)));  // Audio and Video.

// These tests verify that the pipeline handles encrypted media playback
// events (in particular, CDM and license installation) correctly.
class EncryptedAVPipelineImplTest : public ::testing::Test {
 public:
  EncryptedAVPipelineImplTest() {}

 protected:
  void SetUp() override {
    pipeline_helper_.reset(
        new PipelineHelper(message_loop_.task_runner(), true, true, true));
    pipeline_helper_->Setup();
  }

  base::MessageLoop message_loop_;
  std::unique_ptr<PipelineHelper> pipeline_helper_;

  DISALLOW_COPY_AND_ASSIGN(EncryptedAVPipelineImplTest);
};

// Sets a CDM with license already installed before starting the pipeline.
TEST_F(EncryptedAVPipelineImplTest, SetCdmWithLicenseBeforeStart) {
  base::Closure verify_task =
      base::Bind(&VerifyPlay, base::Unretained(pipeline_helper_.get()));
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::SetCdm,
                                    base::Unretained(pipeline_helper_.get())));
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::SetCdmLicenseInstalled,
                                    base::Unretained(pipeline_helper_.get())));
  message_loop_.PostTask(
      FROM_HERE,
      base::Bind(&PipelineHelper::Start,
                 base::Unretained(pipeline_helper_.get()), verify_task));
  message_loop_.Run();
}

// Start the pipeline, then set a CDM with existing license.
TEST_F(EncryptedAVPipelineImplTest, SetCdmWithLicenseAfterStart) {
  base::Closure verify_task =
      base::Bind(&VerifyPlay, base::Unretained(pipeline_helper_.get()));
  message_loop_.PostTask(
      FROM_HERE,
      base::Bind(&PipelineHelper::Start,
                 base::Unretained(pipeline_helper_.get()), verify_task));

  message_loop_.RunUntilIdle();
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::SetCdmLicenseInstalled,
                                    base::Unretained(pipeline_helper_.get())));
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::SetCdm,
                                    base::Unretained(pipeline_helper_.get())));
  message_loop_.Run();
}

// Start the pipeline, set a CDM, and then install the license.
TEST_F(EncryptedAVPipelineImplTest, SetCdmAndInstallLicenseAfterStart) {
  base::Closure verify_task =
      base::Bind(&VerifyPlay, base::Unretained(pipeline_helper_.get()));
  message_loop_.PostTask(
      FROM_HERE,
      base::Bind(&PipelineHelper::Start,
                 base::Unretained(pipeline_helper_.get()), verify_task));
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::SetCdm,
                                    base::Unretained(pipeline_helper_.get())));

  message_loop_.RunUntilIdle();
  message_loop_.PostTask(FROM_HERE,
                         base::Bind(&PipelineHelper::SetCdmLicenseInstalled,
                                    base::Unretained(pipeline_helper_.get())));
  message_loop_.Run();
}

}  // namespace media
}  // namespace chromecast
