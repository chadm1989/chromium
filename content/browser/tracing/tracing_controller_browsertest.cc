// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/tracing_controller.h"

#include <stdint.h>
#include <utility>

#include "base/files/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/run_loop.h"
#include "base/strings/pattern.h"
#include "build/build_config.h"
#include "content/browser/tracing/tracing_controller_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/trace_uploader.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/test_content_browser_client.h"

using base::trace_event::RECORD_CONTINUOUSLY;
using base::trace_event::RECORD_UNTIL_FULL;
using base::trace_event::TraceConfig;

namespace content {

namespace {

const char* kMetadataWhitelist[] = {
  "cpu-brand",
  "network-type",
  "os-name",
  "user-agent"
};

bool IsMetadataWhitelisted(const std::string& metadata_name) {
  for (auto key : kMetadataWhitelist) {
    if (base::MatchPattern(metadata_name, key)) {
      return true;
    }
  }
  return false;
}

bool IsTraceEventArgsWhitelisted(
    const char* category_group_name,
    const char* event_name,
    base::trace_event::ArgumentNameFilterPredicate* arg_filter) {
  if (base::MatchPattern(category_group_name, "benchmark") &&
      base::MatchPattern(event_name, "whitelisted")) {
    return true;
  }
  return false;
}

}  // namespace

class TracingControllerTestEndpoint : public TraceDataEndpoint {
 public:
  TracingControllerTestEndpoint(
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)> done_callback)
      : done_callback_(done_callback) {}

  void ReceiveTraceChunk(const std::string& chunk) override {
    EXPECT_FALSE(chunk.empty());
    trace_ += chunk;
  }

  void ReceiveTraceFinalContents(
      std::unique_ptr<const base::DictionaryValue> metadata,
      const std::string& contents) override {
    EXPECT_EQ(trace_, contents);

    std::string tmp = contents;
    scoped_refptr<base::RefCountedString> chunk_ptr =
        base::RefCountedString::TakeString(&tmp);

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(done_callback_, base::Passed(std::move(metadata)),
                   base::RetainedRef(chunk_ptr)));
  }

 protected:
  ~TracingControllerTestEndpoint() override {}

  std::string trace_;
  base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                      base::RefCountedString*)>
      done_callback_;
};

class TracingTestBrowserClient : public TestContentBrowserClient {
 public:
  TracingDelegate* GetTracingDelegate() override {
    return new TestTracingDelegate();
  };

 private:
  class TestTracingDelegate : public TracingDelegate {
   public:
    std::unique_ptr<TraceUploader> GetTraceUploader(
        net::URLRequestContextGetter* request_context) override {
      return nullptr;
    }
    MetadataFilterPredicate GetMetadataFilterPredicate() override {
      return base::Bind(IsMetadataWhitelisted);
    }
  };
};

class TracingControllerTest : public ContentBrowserTest {
 public:
  TracingControllerTest() {}

  void SetUp() override {
    get_categories_done_callback_count_ = 0;
    enable_recording_done_callback_count_ = 0;
    disable_recording_done_callback_count_ = 0;
    ContentBrowserTest::SetUp();
  }

  void TearDown() override { ContentBrowserTest::TearDown(); }

  void Navigate(Shell* shell) {
    NavigateToURL(shell, GetTestUrl("", "title.html"));
  }

  void GetCategoriesDoneCallbackTest(base::Closure quit_callback,
                                     const std::set<std::string>& categories) {
    get_categories_done_callback_count_++;
    EXPECT_TRUE(categories.size() > 0);
    quit_callback.Run();
  }

  void StartTracingDoneCallbackTest(base::Closure quit_callback) {
    enable_recording_done_callback_count_++;
    quit_callback.Run();
  }

  void StopTracingStringDoneCallbackTest(
      base::Closure quit_callback,
      std::unique_ptr<const base::DictionaryValue> metadata,
      base::RefCountedString* data) {
    disable_recording_done_callback_count_++;
    last_metadata_.reset(metadata.release());
    last_data_ = data->data();
    EXPECT_TRUE(data->size() > 0);
    quit_callback.Run();
  }

  void StopTracingFileDoneCallbackTest(base::Closure quit_callback,
                                            const base::FilePath& file_path) {
    disable_recording_done_callback_count_++;
    EXPECT_TRUE(PathExists(file_path));
    int64_t file_size;
    base::GetFileSize(file_path, &file_size);
    EXPECT_TRUE(file_size > 0);
    quit_callback.Run();
    last_actual_recording_file_path_ = file_path;
  }

    int get_categories_done_callback_count() const {
    return get_categories_done_callback_count_;
  }

  int enable_recording_done_callback_count() const {
    return enable_recording_done_callback_count_;
  }

  int disable_recording_done_callback_count() const {
    return disable_recording_done_callback_count_;
  }

  base::FilePath last_actual_recording_file_path() const {
    return last_actual_recording_file_path_;
  }

  const base::DictionaryValue* last_metadata() const {
    return last_metadata_.get();
  }

  const std::string& last_data() const {
    return last_data_;
  }

  void TestStartAndStopTracingString() {
    Navigate(shell());

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this),
                     run_loop.QuitClosure());
      bool result = controller->StartTracing(
          TraceConfig(), callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)>
          callback = base::Bind(
              &TracingControllerTest::StopTracingStringDoneCallbackTest,
              base::Unretained(this), run_loop.QuitClosure());
      bool result = controller->StopTracing(
          TracingController::CreateStringSink(callback));
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
  }

  void TestStartAndStopTracingStringWithFilter() {
    TracingTestBrowserClient client;
    ContentBrowserClient* old_client = SetBrowserClientForTesting(&client);
    Navigate(shell());

    base::trace_event::TraceLog::GetInstance()->SetArgumentFilterPredicate(
        base::Bind(&IsTraceEventArgsWhitelisted));

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this),
                     run_loop.QuitClosure());

      TraceConfig config = TraceConfig();
      config.EnableArgumentFilter();

      bool result = controller->StartTracing(config, callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)>
          callback = base::Bind(
              &TracingControllerTest::StopTracingStringDoneCallbackTest,
              base::Unretained(this), run_loop.QuitClosure());

      scoped_refptr<TracingController::TraceDataSink> trace_data_sink =
          TracingController::CreateStringSink(callback);

      base::DictionaryValue metadata;
      metadata.SetString("not-whitelisted", "this_not_found");
      controller->AddMetadata(metadata);

      bool result = controller->StopTracing(trace_data_sink);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
    SetBrowserClientForTesting(old_client);
  }

  void TestStartAndStopTracingCompressed() {
    Navigate(shell());

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this), run_loop.QuitClosure());
      bool result = controller->StartTracing(TraceConfig(), callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Callback<void(std::unique_ptr<const base::DictionaryValue>,
                          base::RefCountedString*)>
          callback = base::Bind(
              &TracingControllerTest::StopTracingStringDoneCallbackTest,
              base::Unretained(this), run_loop.QuitClosure());
      bool result = controller->StopTracing(
          TracingControllerImpl::CreateCompressedStringSink(
              new TracingControllerTestEndpoint(callback)));
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
  }

  void TestStartAndStopTracingFile(
      const base::FilePath& result_file_path) {
    Navigate(shell());

    TracingController* controller = TracingController::GetInstance();

    {
      base::RunLoop run_loop;
      TracingController::StartTracingDoneCallback callback =
          base::Bind(&TracingControllerTest::StartTracingDoneCallbackTest,
                     base::Unretained(this),
                     run_loop.QuitClosure());
      bool result = controller->StartTracing(TraceConfig(), callback);
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(enable_recording_done_callback_count(), 1);
    }

    {
      base::RunLoop run_loop;
      base::Closure callback = base::Bind(
          &TracingControllerTest::StopTracingFileDoneCallbackTest,
          base::Unretained(this),
          run_loop.QuitClosure(),
          result_file_path);
      bool result = controller->StopTracing(
          TracingController::CreateFileSink(result_file_path, callback));
      ASSERT_TRUE(result);
      run_loop.Run();
      EXPECT_EQ(disable_recording_done_callback_count(), 1);
    }
  }

 private:
  int get_categories_done_callback_count_;
  int enable_recording_done_callback_count_;
  int disable_recording_done_callback_count_;
  base::FilePath last_actual_recording_file_path_;
  std::unique_ptr<const base::DictionaryValue> last_metadata_;
  std::string last_data_;
};

IN_PROC_BROWSER_TEST_F(TracingControllerTest, GetCategories) {
  Navigate(shell());

  TracingController* controller = TracingController::GetInstance();

  base::RunLoop run_loop;
  TracingController::GetCategoriesDoneCallback callback =
      base::Bind(&TracingControllerTest::GetCategoriesDoneCallbackTest,
                 base::Unretained(this),
                 run_loop.QuitClosure());
  ASSERT_TRUE(controller->GetCategories(callback));
  run_loop.Run();
  EXPECT_EQ(get_categories_done_callback_count(), 1);
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest, EnableAndStopTracing) {
  TestStartAndStopTracingString();
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest, DisableRecordingStoresMetadata) {
  TestStartAndStopTracingString();
  // Check that a number of important keys exist in the metadata dictionary. The
  // values are not checked to ensure the test is robust.
  EXPECT_TRUE(last_metadata() != NULL);
  std::string network_type;
  last_metadata()->GetString("network-type", &network_type);
  EXPECT_TRUE(network_type.length() > 0);
  std::string user_agent;
  last_metadata()->GetString("user-agent", &user_agent);
  EXPECT_TRUE(user_agent.length() > 0);
  std::string os_name;
  last_metadata()->GetString("os-name", &os_name);
  EXPECT_TRUE(os_name.length() > 0);
  std::string cpu_brand;
  last_metadata()->GetString("cpu-brand", &cpu_brand);
  EXPECT_TRUE(cpu_brand.length() > 0);
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest, NotWhitelistedMetadataStripped) {
  TestStartAndStopTracingStringWithFilter();
  // Check that a number of important keys exist in the metadata dictionary.
  EXPECT_TRUE(last_metadata() != NULL);
  std::string cpu_brand;
  last_metadata()->GetString("cpu-brand", &cpu_brand);
  EXPECT_TRUE(cpu_brand.length() > 0);
  EXPECT_TRUE(cpu_brand != "__stripped__");
  std::string network_type;
  last_metadata()->GetString("network-type", &network_type);
  EXPECT_TRUE(network_type.length() > 0);
  EXPECT_TRUE(network_type != "__stripped__");
  std::string os_name;
  last_metadata()->GetString("os-name", &os_name);
  EXPECT_TRUE(os_name.length() > 0);
  EXPECT_TRUE(os_name != "__stripped__");
  std::string user_agent;
  last_metadata()->GetString("user-agent", &user_agent);
  EXPECT_TRUE(user_agent.length() > 0);
  EXPECT_TRUE(user_agent != "__stripped__");

  // Check that the not whitelisted metadata is stripped.
  std::string not_whitelisted;
  last_metadata()->GetString("not-whitelisted", &not_whitelisted);
  EXPECT_TRUE(not_whitelisted.length() > 0);
  EXPECT_TRUE(not_whitelisted == "__stripped__");

  // Also check the string data.
  EXPECT_TRUE(last_data().size() > 0);
  EXPECT_TRUE(last_data().find("cpu-brand") != std::string::npos);
  EXPECT_TRUE(last_data().find("network-type") != std::string::npos);
  EXPECT_TRUE(last_data().find("os-name") != std::string::npos);
  EXPECT_TRUE(last_data().find("user-agent") != std::string::npos);

  EXPECT_TRUE(last_data().find("not-whitelisted") != std::string::npos);
  EXPECT_TRUE(last_data().find("this_not_found") == std::string::npos);
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest,
                       EnableAndStopTracingWithFilePath) {
  base::FilePath file_path;
  base::CreateTemporaryFile(&file_path);
  TestStartAndStopTracingFile(file_path);
  EXPECT_EQ(file_path.value(), last_actual_recording_file_path().value());
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest,
                       EnableAndStopTracingWithCompression) {
  TestStartAndStopTracingCompressed();
}

IN_PROC_BROWSER_TEST_F(TracingControllerTest,
                       EnableAndStopTracingWithEmptyFileAndNullCallback) {
  Navigate(shell());

  TracingController* controller = TracingController::GetInstance();
  EXPECT_TRUE(controller->StartTracing(
      TraceConfig(),
      TracingController::StartTracingDoneCallback()));
  EXPECT_TRUE(controller->StopTracing(NULL));
  base::RunLoop().RunUntilIdle();
}

}  // namespace content
