// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/private/ppb_pdf.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_flash_font_file_api.h"
#include "ppapi/thunk/ppb_pdf_api.h"
#include "ppapi/thunk/resource_creation_api.h"
#include "ppapi/thunk/thunk.h"

namespace ppapi {
namespace thunk {

namespace {

PP_Resource GetFontFileWithFallback(
    PP_Instance instance,
    const PP_BrowserFont_Trusted_Description* description,
    PP_PrivateFontCharset charset) {
  // TODO(raymes): Eventually we should replace the use of this function with
  // either PPB_Flash_Font_File or PPB_TrueType_Font directly in the PDF code.
  // For now just call into PPB_Flash_Font_File which has the exact same API.
  EnterResourceCreation enter(instance);
  if (enter.failed())
    return 0;
  return enter.functions()->CreateFlashFontFile(instance, description, charset);
}

bool GetFontTableForPrivateFontFile(PP_Resource font_file,
                                    uint32_t table,
                                    void* output,
                                    uint32_t* output_length) {
  // TODO(raymes): Eventually we should replace the use of this function with
  // either PPB_Flash_Font_File or PPB_TrueType_Font directly in the PDF code.
  // For now just call into PPB_Flash_Font_File which has the exact same API.
  EnterResource<PPB_Flash_FontFile_API> enter(font_file, true);
  if (enter.failed())
    return PP_FALSE;
  return PP_ToBool(enter.object()->GetFontTable(table, output, output_length));
}

void SearchString(PP_Instance instance,
                  const unsigned short* string,
                  const unsigned short* term,
                  bool case_sensitive,
                  PP_PrivateFindResult** results,
                  int* count) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.failed())
    return;
  enter.functions()->SearchString(string, term, case_sensitive, results, count);
}

void DidStartLoading(PP_Instance instance) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->DidStartLoading();
}

void DidStopLoading(PP_Instance instance) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->DidStopLoading();
}

void SetContentRestriction(PP_Instance instance, int restrictions) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->SetContentRestriction(restrictions);
}

void UserMetricsRecordAction(PP_Instance instance, PP_Var action) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->UserMetricsRecordAction(action);
}

void HasUnsupportedFeature(PP_Instance instance) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->HasUnsupportedFeature();
}

void SaveAs(PP_Instance instance) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->SaveAs();
}

void Print(PP_Instance instance) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->Print();
}

PP_Bool IsFeatureEnabled(PP_Instance instance, PP_PDFFeature feature) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.failed())
    return PP_FALSE;
  return enter.functions()->IsFeatureEnabled(feature);
}

void SetSelectedText(PP_Instance instance,
                     const char* selected_text) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.succeeded())
    enter.functions()->SetSelectedText(selected_text);
}

void SetLinkUnderCursor(PP_Instance instance, const char* url) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.failed())
    return;
  enter.functions()->SetLinkUnderCursor(url);
}

void GetV8ExternalSnapshotData(PP_Instance instance,
                               const char** natives_data_out,
                               int* natives_size_out,
                               const char** snapshot_data_out,
                               int* snapshot_size_out) {
  EnterInstanceAPI<PPB_PDF_API> enter(instance);
  if (enter.failed())
    return;
  enter.functions()->GetV8ExternalSnapshotData(natives_data_out,
      natives_size_out, snapshot_data_out, snapshot_size_out);
}

const PPB_PDF g_ppb_pdf_thunk = {
  &GetFontFileWithFallback,
  &GetFontTableForPrivateFontFile,
  &SearchString,
  &DidStartLoading,
  &DidStopLoading,
  &SetContentRestriction,
  &UserMetricsRecordAction,
  &HasUnsupportedFeature,
  &SaveAs,
  &Print,
  &IsFeatureEnabled,
  &SetSelectedText,
  &SetLinkUnderCursor,
  &GetV8ExternalSnapshotData,
};

}  // namespace

const PPB_PDF* GetPPB_PDF_Thunk() {
  return &g_ppb_pdf_thunk;
}

}  // namespace thunk
}  // namespace ppapi
