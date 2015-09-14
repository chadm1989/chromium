// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/syncapi_server_connection_manager.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "net/base/net_errors.h"
#include "sync/internal_api/public/base/cancelation_signal.h"
#include "sync/internal_api/public/http_post_provider_factory.h"
#include "sync/internal_api/public/http_post_provider_interface.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {
namespace {

using base::TimeDelta;

class BlockingHttpPost : public HttpPostProviderInterface {
 public:
  BlockingHttpPost() : wait_for_abort_(false, false) {}
  ~BlockingHttpPost() override {}

  void SetExtraRequestHeaders(const char* headers) override {}
  void SetURL(const char* url, int port) override {}
  void SetPostPayload(const char* content_type,
                      int content_length,
                      const char* content) override {}
  bool MakeSynchronousPost(int* error_code, int* response_code) override {
    wait_for_abort_.TimedWait(TestTimeouts::action_max_timeout());
    *error_code = net::ERR_ABORTED;
    return false;
  }
  int GetResponseContentLength() const override { return 0; }
  const char* GetResponseContent() const override { return ""; }
  const std::string GetResponseHeaderValue(
      const std::string& name) const override {
    return std::string();
  }
  void Abort() override { wait_for_abort_.Signal(); }
 private:
  base::WaitableEvent wait_for_abort_;
};

class BlockingHttpPostFactory : public HttpPostProviderFactory {
 public:
  ~BlockingHttpPostFactory() override {}
  void Init(const std::string& user_agent,
            const BindToTrackerCallback& bind_to_tracker_callback) override {}

  HttpPostProviderInterface* Create() override {
    return new BlockingHttpPost();
  }
  void Destroy(HttpPostProviderInterface* http) override {
    delete static_cast<BlockingHttpPost*>(http);
  }
};

}  // namespace

// Ask the ServerConnectionManager to stop before it is created.
TEST(SyncAPIServerConnectionManagerTest, VeryEarlyAbortPost) {
  CancelationSignal signal;
  signal.Signal();
  SyncAPIServerConnectionManager server(
      "server", 0, true, new BlockingHttpPostFactory(), &signal);

  ServerConnectionManager::PostBufferParams params;
  ScopedServerStatusWatcher watcher(&server, &params.response);

  bool result = server.PostBufferToPath(
      &params, "/testpath", "testauth", &watcher);

  EXPECT_FALSE(result);
  EXPECT_EQ(HttpResponse::CONNECTION_UNAVAILABLE,
            params.response.server_status);
}

// Ask the ServerConnectionManager to stop before its first request is made.
TEST(SyncAPIServerConnectionManagerTest, EarlyAbortPost) {
  CancelationSignal signal;
  SyncAPIServerConnectionManager server(
      "server", 0, true, new BlockingHttpPostFactory(), &signal);

  ServerConnectionManager::PostBufferParams params;
  ScopedServerStatusWatcher watcher(&server, &params.response);

  signal.Signal();
  bool result = server.PostBufferToPath(
      &params, "/testpath", "testauth", &watcher);

  EXPECT_FALSE(result);
  EXPECT_EQ(HttpResponse::CONNECTION_UNAVAILABLE,
            params.response.server_status);
}

// Ask the ServerConnectionManager to stop during a request.
TEST(SyncAPIServerConnectionManagerTest, AbortPost) {
  CancelationSignal signal;
  SyncAPIServerConnectionManager server(
      "server", 0, true, new BlockingHttpPostFactory(), &signal);

  ServerConnectionManager::PostBufferParams params;
  ScopedServerStatusWatcher watcher(&server, &params.response);

  base::Thread abort_thread("Test_AbortThread");
  ASSERT_TRUE(abort_thread.Start());
  abort_thread.message_loop()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&CancelationSignal::Signal,
                 base::Unretained(&signal)),
      TestTimeouts::tiny_timeout());

  bool result = server.PostBufferToPath(
      &params, "/testpath", "testauth", &watcher);

  EXPECT_FALSE(result);
  EXPECT_EQ(HttpResponse::CONNECTION_UNAVAILABLE,
            params.response.server_status);
  abort_thread.Stop();
}

}  // namespace syncer
