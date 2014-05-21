// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/local_discovery/cloud_print_printer_list.h"

#include <set>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/cloud_devices/common/cloud_devices_urls.h"
#include "content/public/test/test_browser_thread.h"
#include "google_apis/gaia/fake_oauth2_token_service.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::Mock;
using testing::NiceMock;
using testing::StrictMock;

namespace local_discovery {

namespace {

const char kSampleSuccessResponseOAuth[] = "{"
    "   \"success\": true,"
    "   \"printers\": ["
    "     {\"id\" : \"someID\","
    "      \"displayName\": \"someDisplayName\","
    "      \"description\": \"someDescription\"}"
    "    ]"
    "}";

class MockDelegate : public CloudDeviceListDelegate {
 public:
  MOCK_METHOD0(OnDeviceListUnavailable, void());
  MOCK_METHOD0(OnDeviceListReady, void());
};

class CloudPrintPrinterListTest : public testing::Test {
 public:
  CloudPrintPrinterListTest()
      : ui_thread_(content::BrowserThread::UI, &loop_),
        request_context_(new net::TestURLRequestContextGetter(
            base::MessageLoopProxy::current())),
        fetcher_factory_(NULL),
        printer_list_(request_context_.get(), &token_service_, "account_id",
                      &delegate_) {
  }

  virtual void SetUp() OVERRIDE {
    ui_thread_.Stop();  // HACK: Fake being on the UI thread
    token_service_.set_request_context(request_context_.get());
    token_service_.AddAccount("account_id");
  }

 protected:
  base::MessageLoopForUI loop_;
  content::TestBrowserThread ui_thread_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  net::FakeURLFetcherFactory fetcher_factory_;
  FakeOAuth2TokenService token_service_;
  StrictMock<MockDelegate> delegate_;
  CloudPrintPrinterList printer_list_;
};

TEST_F(CloudPrintPrinterListTest, List) {
  fetcher_factory_.SetFakeResponse(
      cloud_devices::GetCloudPrintRelativeURL("search"),
      kSampleSuccessResponseOAuth,
      net::HTTP_OK,
      net::URLRequestStatus::SUCCESS);

  GCDBaseApiFlow* cloudprint_flow = printer_list_.GetOAuth2ApiFlowForTests();

  printer_list_.Start();

  cloudprint_flow->OnGetTokenSuccess(NULL, "SomeToken", base::Time());

  EXPECT_CALL(delegate_, OnDeviceListReady());

  base::RunLoop().RunUntilIdle();

  Mock::VerifyAndClear(&delegate_);

  std::set<std::string> ids_found;
  std::set<std::string> ids_expected;
  ids_expected.insert("someID");

  for (CloudPrintPrinterList::iterator i = printer_list_.printer_list().begin();
       i != printer_list_.printer_list().end(); i++) {
    ids_found.insert(i->id);
  }

  ASSERT_EQ(ids_expected, ids_found);

  EXPECT_EQ("someID", printer_list_.printer_list()[0].id);
  EXPECT_EQ("someDisplayName", printer_list_.printer_list()[0].display_name);
  EXPECT_EQ("someDescription", printer_list_.printer_list()[0].description);
  EXPECT_EQ("printer", printer_list_.printer_list()[0].type);
}

}  // namespace

}  // namespace local_discovery
