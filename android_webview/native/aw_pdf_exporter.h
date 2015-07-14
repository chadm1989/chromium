// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_NATIVE_AW_PDF_EXPORTER_H_
#define ANDROID_WEBVIEW_NATIVE_AW_PDF_EXPORTER_H_

#include <jni.h>

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "skia/ext/refptr.h"

namespace content {
class WebContents;
};

namespace printing {
class PrintSettings;
};

namespace android_webview {

class AwPdfExporter {
 public:
  AwPdfExporter(JNIEnv* env,
                jobject obj,
                content::WebContents* web_contents);

  ~AwPdfExporter();

  void ExportToPdf(JNIEnv* env,
                   jobject obj,
                   int fd,
                   jobject cancel_signal);

 private:
  void CreatePdfSettings(JNIEnv* env, jobject obj);
  void DidExportPdf(int fd, bool success);

  JavaObjectWeakGlobalRef java_ref_;
  content::WebContents* web_contents_;

  scoped_ptr<printing::PrintSettings> print_settings_;

  DISALLOW_COPY_AND_ASSIGN(AwPdfExporter);
};

bool RegisterAwPdfExporter(JNIEnv* env);

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_NATIVE_AW_PDF_EXPORTER_H_
