// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_PROXY_PDF_RESOURCE_H_
#define PPAPI_PROXY_PDF_RESOURCE_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/proxy/connection.h"
#include "ppapi/proxy/plugin_resource.h"
#include "ppapi/proxy/ppapi_proxy_export.h"
#include "ppapi/thunk/ppb_pdf_api.h"

namespace ppapi {
namespace proxy {

class PluginDispatcher;

class PPAPI_PROXY_EXPORT PDFResource
    : public PluginResource,
      public thunk::PPB_PDF_API {
 public:
  PDFResource(Connection connection, PP_Instance instance);
  ~PDFResource() override;

  // For unittesting with a given locale.
  void SetLocaleForTest(const std::string& locale) {
    locale_ = locale;
  }

  // Resource override.
  thunk::PPB_PDF_API* AsPPB_PDF_API() override;

  // PPB_PDF_API implementation.
  void SearchString(const unsigned short* input_string,
                    const unsigned short* input_term,
                    bool case_sensitive,
                    PP_PrivateFindResult** results,
                    int* count) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void SetContentRestriction(int restrictions) override;
  void UserMetricsRecordAction(const PP_Var& action) override;
  void HasUnsupportedFeature() override;
  void Print() override;
  void SaveAs() override;
  PP_Bool IsFeatureEnabled(PP_PDFFeature feature) override;
  void SetSelectedText(const char* selected_text) override;
  void SetLinkUnderCursor(const char* url) override;
  void GetV8ExternalSnapshotData(const char** natives_data_out,
                                 int* natives_size_out,
                                 const char** snapshot_data_out,
                                 int* snapshot_size_out) override;

 private:
  std::string locale_;

  DISALLOW_COPY_AND_ASSIGN(PDFResource);
};

}  // namespace proxy
}  // namespace ppapi

#endif  // PPAPI_PROXY_PDF_RESOURCE_H_
