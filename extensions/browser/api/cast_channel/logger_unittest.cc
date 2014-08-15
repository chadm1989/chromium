// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/simple_test_tick_clock.h"
#include "extensions/browser/api/cast_channel/cast_auth_util.h"
#include "extensions/browser/api/cast_channel/logger.h"
#include "extensions/browser/api/cast_channel/logger_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace core_api {
namespace cast_channel {

const int kTestNssErrorCode = -8164;

using proto::AggregatedSocketEvent;
using proto::EventType;
using proto::Log;
using proto::SocketEvent;

class CastChannelLoggerTest : public testing::Test {
 public:
  // |logger_| will take ownership of |clock_|.
  CastChannelLoggerTest()
      : clock_(new base::SimpleTestTickClock),
        logger_(new Logger(scoped_ptr<base::TickClock>(clock_),
                           base::TimeTicks())) {}
  virtual ~CastChannelLoggerTest() {}

 protected:
  base::SimpleTestTickClock* clock_;
  scoped_refptr<Logger> logger_;
};

TEST_F(CastChannelLoggerTest, BasicLogging) {
  logger_->LogSocketEvent(1, EventType::CAST_SOCKET_CREATED);
  clock_->Advance(base::TimeDelta::FromMicroseconds(1));
  logger_->LogSocketEventWithDetails(
      1, EventType::TCP_SOCKET_CONNECT, "TCP socket");
  clock_->Advance(base::TimeDelta::FromMicroseconds(1));
  logger_->LogSocketEvent(2, EventType::CAST_SOCKET_CREATED);
  clock_->Advance(base::TimeDelta::FromMicroseconds(1));
  logger_->LogSocketEventWithRv(1, EventType::SSL_SOCKET_CONNECT, -1);
  clock_->Advance(base::TimeDelta::FromMicroseconds(1));
  logger_->LogSocketEventForMessage(
      2, EventType::MESSAGE_ENQUEUED, "foo_namespace", "details");
  clock_->Advance(base::TimeDelta::FromMicroseconds(1));

  AuthResult auth_result =
      AuthResult::Create("No response", AuthResult::ERROR_NO_RESPONSE);

  logger_->LogSocketChallengeReplyEvent(2, auth_result);
  clock_->Advance(base::TimeDelta::FromMicroseconds(1));

  auth_result =
      AuthResult::CreateWithNSSError("Parsing failed",
                                     AuthResult::ERROR_NSS_CERT_PARSING_FAILED,
                                     kTestNssErrorCode);
  logger_->LogSocketChallengeReplyEvent(2, auth_result);

  std::string output;
  bool success = logger_->LogToString(&output);
  ASSERT_TRUE(success);

  Log log;
  success = log.ParseFromString(output);
  ASSERT_TRUE(success);

  LastErrors last_errors = logger_->GetLastErrors(2);
  EXPECT_EQ(last_errors.event_type, proto::AUTH_CHALLENGE_REPLY);
  EXPECT_EQ(last_errors.net_return_value, 0);
  EXPECT_EQ(last_errors.challenge_reply_error_type,
            proto::CHALLENGE_REPLY_ERROR_NSS_CERT_PARSING_FAILED);
  EXPECT_EQ(last_errors.nss_error_code, kTestNssErrorCode);

  ASSERT_EQ(2, log.aggregated_socket_event_size());
  {
    const AggregatedSocketEvent& aggregated_socket_event =
        log.aggregated_socket_event(0);
    EXPECT_EQ(1, aggregated_socket_event.id());
    EXPECT_EQ(3, aggregated_socket_event.socket_event_size());
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(0);
      EXPECT_EQ(EventType::CAST_SOCKET_CREATED, event.type());
      EXPECT_EQ(0, event.timestamp_micros());
    }
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(1);
      EXPECT_EQ(EventType::TCP_SOCKET_CONNECT, event.type());
      EXPECT_EQ(1, event.timestamp_micros());
      EXPECT_EQ("TCP socket", event.details());
    }
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(2);
      EXPECT_EQ(EventType::SSL_SOCKET_CONNECT, event.type());
      EXPECT_EQ(3, event.timestamp_micros());
      EXPECT_EQ(-1, event.net_return_value());
    }
  }
  {
    const AggregatedSocketEvent& aggregated_socket_event =
        log.aggregated_socket_event(1);
    EXPECT_EQ(2, aggregated_socket_event.id());
    EXPECT_EQ(4, aggregated_socket_event.socket_event_size());
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(0);
      EXPECT_EQ(EventType::CAST_SOCKET_CREATED, event.type());
      EXPECT_EQ(2, event.timestamp_micros());
    }
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(1);
      EXPECT_EQ(EventType::MESSAGE_ENQUEUED, event.type());
      EXPECT_EQ(4, event.timestamp_micros());
      EXPECT_FALSE(event.has_message_namespace());
      EXPECT_EQ("details", event.details());
    }
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(2);
      EXPECT_EQ(EventType::AUTH_CHALLENGE_REPLY, event.type());
      EXPECT_EQ(5, event.timestamp_micros());
      EXPECT_EQ(proto::CHALLENGE_REPLY_ERROR_NO_RESPONSE,
                event.challenge_reply_error_type());
      EXPECT_FALSE(event.has_net_return_value());
      EXPECT_FALSE(event.has_nss_error_code());
    }
    {
      const SocketEvent& event = aggregated_socket_event.socket_event(3);
      EXPECT_EQ(EventType::AUTH_CHALLENGE_REPLY, event.type());
      EXPECT_EQ(6, event.timestamp_micros());
      EXPECT_EQ(proto::CHALLENGE_REPLY_ERROR_NSS_CERT_PARSING_FAILED,
                event.challenge_reply_error_type());
      EXPECT_FALSE(event.has_net_return_value());
      EXPECT_EQ(kTestNssErrorCode, event.nss_error_code());
    }
  }
}

TEST_F(CastChannelLoggerTest, TooManySockets) {
  for (int i = 0; i < kMaxSocketsToLog + 5; i++) {
    logger_->LogSocketEvent(i, EventType::CAST_SOCKET_CREATED);
  }

  std::string output;
  bool success = logger_->LogToString(&output);
  ASSERT_TRUE(success);

  Log log;
  success = log.ParseFromString(output);
  ASSERT_TRUE(success);

  ASSERT_EQ(kMaxSocketsToLog, log.aggregated_socket_event_size());
  EXPECT_EQ(5, log.num_evicted_aggregated_socket_events());
  EXPECT_EQ(5, log.num_evicted_socket_events());

  const AggregatedSocketEvent& aggregated_socket_event =
      log.aggregated_socket_event(0);
  EXPECT_EQ(5, aggregated_socket_event.id());
}

TEST_F(CastChannelLoggerTest, TooManyEvents) {
  for (int i = 0; i < kMaxEventsPerSocket + 5; i++) {
    logger_->LogSocketEvent(1, EventType::CAST_SOCKET_CREATED);
    clock_->Advance(base::TimeDelta::FromMicroseconds(1));
  }

  std::string output;
  bool success = logger_->LogToString(&output);
  ASSERT_TRUE(success);

  Log log;
  success = log.ParseFromString(output);
  ASSERT_TRUE(success);

  ASSERT_EQ(1, log.aggregated_socket_event_size());
  EXPECT_EQ(0, log.num_evicted_aggregated_socket_events());
  EXPECT_EQ(5, log.num_evicted_socket_events());

  const AggregatedSocketEvent& aggregated_socket_event =
      log.aggregated_socket_event(0);
  ASSERT_EQ(kMaxEventsPerSocket, aggregated_socket_event.socket_event_size());
  EXPECT_EQ(5, aggregated_socket_event.socket_event(0).timestamp_micros());
}

TEST_F(CastChannelLoggerTest, Reset) {
  logger_->LogSocketEvent(1, EventType::CAST_SOCKET_CREATED);

  std::string output;
  bool success = logger_->LogToString(&output);
  ASSERT_TRUE(success);

  Log log;
  success = log.ParseFromString(output);
  ASSERT_TRUE(success);

  EXPECT_EQ(1, log.aggregated_socket_event_size());

  logger_->Reset();

  success = logger_->LogToString(&output);
  ASSERT_TRUE(success);
  success = log.ParseFromString(output);
  ASSERT_TRUE(success);

  EXPECT_EQ(0, log.aggregated_socket_event_size());
}

}  // namespace cast_channel
}  // namespace api
}  // namespace extensions
