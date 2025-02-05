// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/media/audio/cast_audio_mixer.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/run_loop.h"
#include "chromecast/media/audio/cast_audio_manager.h"
#include "chromecast/media/audio/cast_audio_output_stream.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromecast {
namespace media {
namespace {

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

// Utility functions
::media::AudioParameters GetAudioParams() {
  return ::media::AudioParameters(
      ::media::AudioParameters::AUDIO_PCM_LOW_LATENCY,
      ::media::CHANNEL_LAYOUT_STEREO, 48000, 16, 1024);
}

// Mock implementations
class MockAudioSourceCallback
    : public ::media::AudioOutputStream::AudioSourceCallback {
 public:
  MockAudioSourceCallback() {
    ON_CALL(*this, OnMoreData(_, _, _))
        .WillByDefault(Invoke(this, &MockAudioSourceCallback::OnMoreDataImpl));
  }

  MOCK_METHOD3(OnMoreData,
               int(::media::AudioBus* audio_bus,
                   uint32_t total_bytes_delay,
                   uint32_t frames_skipped));
  MOCK_METHOD1(OnError, void(::media::AudioOutputStream* stream));

 private:
  int OnMoreDataImpl(::media::AudioBus* audio_bus,
                     uint32_t total_bytes_delay,
                     uint32_t frames_skipped) {
    audio_bus->Zero();
    return audio_bus->frames();
  }
};

class MockCastAudioOutputStream : public CastAudioOutputStream {
 public:
  MockCastAudioOutputStream(const ::media::AudioParameters& audio_params,
                            CastAudioManager* audio_manager)
      : CastAudioOutputStream(audio_params, audio_manager) {}

  MOCK_METHOD0(Open, bool());
  MOCK_METHOD0(Close, void());
  MOCK_METHOD1(Start, void(AudioSourceCallback* source_callback));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD1(SetVolume, void(double volume));
  MOCK_METHOD1(GetVolume, void(double* volume));

  void SignalPull(AudioSourceCallback* source_callback,
                  uint32_t total_bytes_delay) {
    std::unique_ptr<::media::AudioBus> audio_bus =
        ::media::AudioBus::Create(GetAudioParams());
    source_callback->OnMoreData(audio_bus.get(), total_bytes_delay, 0);
  }

  void SignalError(AudioSourceCallback* source_callback) {
    source_callback->OnError(this);
  }
};

class MockCastAudioManager : public CastAudioManager {
 public:
  MockCastAudioManager(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                       CastAudioMixer* audio_mixer)
      : CastAudioManager(task_runner,
                         task_runner,
                         nullptr,
                         nullptr,
                         audio_mixer) {
    ON_CALL(*this, ReleaseOutputStream(_))
        .WillByDefault(
            Invoke(this, &MockCastAudioManager::ReleaseOutputStreamConcrete));
  }

  MOCK_METHOD1(
      MakeMixerOutputStream,
      ::media::AudioOutputStream*(const ::media::AudioParameters& params));
  MOCK_METHOD1(ReleaseOutputStream, void(::media::AudioOutputStream* stream));

 private:
  void ReleaseOutputStreamConcrete(::media::AudioOutputStream* stream) {
    CastAudioManager::ReleaseOutputStream(stream);
  }
};

class MockCastAudioMixer : public CastAudioMixer {
 public:
  MockCastAudioMixer(const RealStreamFactory& real_stream_factory)
      : CastAudioMixer(real_stream_factory) {
    ON_CALL(*this, MakeStream(_, _))
        .WillByDefault(Invoke(this, &MockCastAudioMixer::MakeStreamConcrete));
  }

  MOCK_METHOD2(
      MakeStream,
      ::media::AudioOutputStream*(const ::media::AudioParameters& params,
                                  CastAudioManager* audio_manager));

 private:
  ::media::AudioOutputStream* MakeStreamConcrete(
      const ::media::AudioParameters& params,
      CastAudioManager* audio_manager) {
    return CastAudioMixer::MakeStream(params, audio_manager);
  }
};

// Generates StrictMocks of Mixer, Manager, and Mixer OutputStream.
class CastAudioMixerTest : public ::testing::Test {
 public:
  CastAudioMixerTest() {}
  ~CastAudioMixerTest() override {}

 protected:
  void SetUp() override {
    // |this| will outlive |mock_mixer_|
    mock_mixer_ = new StrictMock<MockCastAudioMixer>(
        base::Bind(&CastAudioMixerTest::MakeMixerOutputStreamProxy,
                   base::Unretained(this)));
    mock_manager_.reset(new StrictMock<MockCastAudioManager>(
        message_loop_.task_runner(), mock_mixer_));
    mock_mixer_stream_.reset(new StrictMock<MockCastAudioOutputStream>(
        GetAudioParams(), mock_manager_.get()));
  }

  void TearDown() override { mock_manager_.reset(); }

  MockCastAudioManager& mock_manager() { return *mock_manager_; }

  MockCastAudioMixer& mock_mixer() { return *mock_mixer_; }

  MockCastAudioOutputStream& mock_mixer_stream() { return *mock_mixer_stream_; }

  ::media::AudioOutputStream* CreateMixerStream() {
    EXPECT_CALL(*mock_mixer_, MakeStream(_, &mock_manager()));
    return mock_manager_->MakeAudioOutputStream(
        GetAudioParams(), "", ::media::AudioManager::LogCallback());
  }

 private:
  ::media::AudioOutputStream* MakeMixerOutputStreamProxy(
      const ::media::AudioParameters& audio_params) {
    return mock_manager_ ? mock_manager_->MakeMixerOutputStream(audio_params)
                         : nullptr;
  }

  base::MessageLoop message_loop_;
  MockCastAudioMixer* mock_mixer_;
  std::unique_ptr<MockCastAudioManager> mock_manager_;
  std::unique_ptr<MockCastAudioOutputStream> mock_mixer_stream_;
};

TEST_F(CastAudioMixerTest, Volume) {
  ::media::AudioOutputStream* stream = CreateMixerStream();
  ASSERT_TRUE(stream);

  double volume;
  stream->GetVolume(&volume);
  ASSERT_EQ(volume, 1.0);

  stream->SetVolume(.56);
  stream->GetVolume(&volume);
  ASSERT_EQ(volume, .56);

  EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
  stream->Close();
}

TEST_F(CastAudioMixerTest, MixerCallsCloseOnFailedOpen) {
  ::media::AudioOutputStream* stream = CreateMixerStream();
  ASSERT_TRUE(stream);

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(false));
  EXPECT_CALL(mock_mixer_stream(), Close());
  ASSERT_FALSE(stream->Open());

  EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
  stream->Close();
}

TEST_F(CastAudioMixerTest, StreamControlOrderMisuse) {
  MockAudioSourceCallback source;
  ::media::AudioOutputStream* stream = CreateMixerStream();
  ASSERT_TRUE(stream);

  // Close stream without first opening
  EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
  stream->Close();

  stream = CreateMixerStream();
  ASSERT_TRUE(stream);

  // Should not trigger mixer actions.
  stream->Stop();
  stream->Start(&source);

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(false));
  EXPECT_CALL(mock_mixer_stream(), Close());
  ASSERT_FALSE(stream->Open());

  // Should not trigger mixer actions.
  stream->Start(&source);
  stream->Stop();

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  ASSERT_TRUE(stream->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
  stream->Start(&source);
  stream->Start(&source);

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_mixer_stream(), Close());
  EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
  stream->Close();  // Close abruptly without Stop(), should not fail.
}

TEST_F(CastAudioMixerTest, SingleStreamCycle) {
  MockAudioSourceCallback source;
  ::media::AudioOutputStream* stream = CreateMixerStream();
  ASSERT_TRUE(stream);

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  ASSERT_TRUE(stream->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer())).Times(2);
  EXPECT_CALL(mock_mixer_stream(), Stop()).Times(2);
  stream->Start(&source);
  stream->Stop();
  stream->Start(&source);
  stream->Stop();

  EXPECT_CALL(mock_mixer_stream(), Close());
  EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
  stream->Close();
}

TEST_F(CastAudioMixerTest, MultiStreamCycle) {
  // This test will break if run with < 1 stream.
  std::vector<::media::AudioOutputStream*> streams(5);
  std::vector<std::unique_ptr<MockAudioSourceCallback>> sources(streams.size());
  std::generate(streams.begin(), streams.end(),
                [this] { return CreateMixerStream(); });
  std::generate(sources.begin(), sources.end(), [this] {
    return std::unique_ptr<MockAudioSourceCallback>(
        new StrictMock<MockAudioSourceCallback>());
  });

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  for (auto& stream : streams)
    ASSERT_TRUE(stream->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
  for (unsigned int i = 0; i < streams.size(); i++)
    streams[i]->Start(sources[i].get());

  // Individually pull out streams
  while (streams.size() > 1) {
    ::media::AudioOutputStream* stream = streams.front();
    stream->Stop();
    streams.erase(streams.begin());
    sources.erase(sources.begin());

    for (auto& source : sources)
      EXPECT_CALL(*source, OnMoreData(_, _, _));
    mock_mixer_stream().SignalPull(&mock_mixer(), 0);

    EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
    stream->Close();
  }

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_mixer_stream(), Close());
  EXPECT_CALL(mock_manager(), ReleaseOutputStream(streams.front()));
  streams.front()->Close();
}

TEST_F(CastAudioMixerTest, TwoStreamRestart) {
  MockAudioSourceCallback source;
  ::media::AudioOutputStream *stream1, *stream2;

  for (int i = 0; i < 2; i++) {
    stream1 = CreateMixerStream();
    stream2 = CreateMixerStream();
    ASSERT_TRUE(stream1);
    ASSERT_TRUE(stream2);

    EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
        .WillOnce(Return(&mock_mixer_stream()));
    EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
    ASSERT_TRUE(stream1->Open());
    ASSERT_TRUE(stream2->Open());

    EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
    stream1->Start(&source);
    stream2->Start(&source);

    stream1->Stop();
    EXPECT_CALL(mock_mixer_stream(), Stop());
    EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream2));
    stream2->Close();
    EXPECT_CALL(mock_mixer_stream(), Close());
    EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream1));
    stream1->Close();
  }
}

TEST_F(CastAudioMixerTest, OnErrorRecovery) {
  MockAudioSourceCallback source;
  std::vector<::media::AudioOutputStream*> streams;

  streams.push_back(CreateMixerStream());
  streams.push_back(CreateMixerStream());
  for (auto stream : streams)
    ASSERT_TRUE(stream);

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  for (auto stream : streams)
    ASSERT_TRUE(stream->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
  streams.front()->Start(&source);

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_mixer_stream(), Close());
  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));

  // The MockAudioSourceCallback should not receive any call OnError.
  mock_mixer_stream().SignalError(&mock_mixer());

  // Try to add another stream.
  streams.push_back(CreateMixerStream());
  ASSERT_TRUE(streams.back());
  ASSERT_TRUE(streams.back()->Open());

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_mixer_stream(), Close());
  for (auto stream : streams) {
    EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
    stream->Close();
  }
}

TEST_F(CastAudioMixerTest, OnErrorNoRecovery) {
  MockAudioSourceCallback source;
  std::vector<::media::AudioOutputStream*> streams;

  streams.push_back(CreateMixerStream());
  streams.push_back(CreateMixerStream());
  for (auto stream : streams)
    ASSERT_TRUE(stream);

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  for (auto stream : streams)
    ASSERT_TRUE(stream->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
  streams.front()->Start(&source);

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(false));
  EXPECT_CALL(mock_mixer_stream(), Close()).Times(2);
  EXPECT_CALL(source, OnError(streams.front()));

  // The MockAudioSourceCallback should receive an OnError call.
  mock_mixer_stream().SignalError(&mock_mixer());

  // Try to add another stream.
  streams.push_back(CreateMixerStream());
  ASSERT_TRUE(streams.back());
  ASSERT_FALSE(streams.back()->Open());

  for (auto stream : streams) {
    EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
    stream->Close();
  }

  // Now that the state has been refreshed, attempt to open a stream.
  streams.clear();
  streams.push_back(CreateMixerStream());
  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  ASSERT_TRUE(streams.front()->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
  streams.front()->Start(&source);

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_mixer_stream(), Close());
  EXPECT_CALL(mock_manager(), ReleaseOutputStream(streams.front()));
  streams.front()->Close();
}

TEST_F(CastAudioMixerTest, ByteDelays) {
  MockAudioSourceCallback source;
  ::media::AudioOutputStream* stream = CreateMixerStream();

  EXPECT_CALL(mock_manager(), MakeMixerOutputStream(_))
      .WillOnce(Return(&mock_mixer_stream()));
  EXPECT_CALL(mock_mixer_stream(), Open()).WillOnce(Return(true));
  ASSERT_TRUE(stream->Open());

  EXPECT_CALL(mock_mixer_stream(), Start(&mock_mixer()));
  stream->Start(&source);

  // The |total_bytes_delay| is the same because the Mixer and stream are
  // using the same AudioParameters.
  EXPECT_CALL(source, OnMoreData(_, 100, 0));
  mock_mixer_stream().SignalPull(&mock_mixer(), 100);

  EXPECT_CALL(mock_mixer_stream(), Stop());
  EXPECT_CALL(mock_mixer_stream(), Close());
  EXPECT_CALL(mock_manager(), ReleaseOutputStream(stream));
  stream->Close();
}

}  // namespace
}  // namespace media
}  // namespace chromecast
