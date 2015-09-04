// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/large_icon_bridge.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/favicon/large_icon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon_base/fallback_icon_style.h"
#include "components/favicon_base/favicon_types.h"
#include "jni/LargeIconBridge_jni.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/codec/png_codec.h"

using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF16;

namespace {

const SkColor kDefaultBackgroundColor = SkColorSetRGB(0x78, 0x78, 0x78);

void OnLargeIconAvailable(
    ScopedJavaGlobalRef<jobject>* j_callback,
    const favicon_base::LargeIconResult& result) {
  JNIEnv* env = AttachCurrentThread();

  // Convert the result to a Java Bitmap.
  SkBitmap bitmap;
  ScopedJavaLocalRef<jobject> j_bitmap;
  if (result.bitmap.is_valid()) {
    gfx::PNGCodec::Decode(result.bitmap.bitmap_data->front(),
                          result.bitmap.bitmap_data->size(),
                          &bitmap);
    if (!bitmap.isNull())
      j_bitmap = gfx::ConvertToJavaBitmap(&bitmap);
  }

  jint background_color = kDefaultBackgroundColor;
  if (result.fallback_icon_style)
    background_color = result.fallback_icon_style->background_color;

  Java_LargeIconCallback_onLargeIconAvailable(env,
                                              j_callback->obj(),
                                              j_bitmap.obj(),
                                              background_color);
}

}  // namespace

static jlong Init(JNIEnv* env, const JavaParamRef<jclass>& clazz) {
  return reinterpret_cast<intptr_t>(new LargeIconBridge());
}

LargeIconBridge::LargeIconBridge() {
}

LargeIconBridge::~LargeIconBridge() {
}

void LargeIconBridge::Destroy(JNIEnv* env, jobject obj) {
  delete this;
}

jboolean LargeIconBridge::GetLargeIconForURL(
    JNIEnv* env,
    jobject obj,
    jobject j_profile,
    jstring j_page_url,
    jint min_source_size_px,
    jobject j_callback) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  if (!profile)
    return false;

  favicon::LargeIconService* large_icon_service =
      LargeIconServiceFactory::GetForBrowserContext(profile);
  if (!large_icon_service)
    return false;

  ScopedJavaGlobalRef<jobject>* j_global_callback =
      new ScopedJavaGlobalRef<jobject>();
  j_global_callback->Reset(env, j_callback);

  favicon_base::LargeIconCallback callback_runner =
      base::Bind(&OnLargeIconAvailable, base::Owned(j_global_callback));

  large_icon_service->GetLargeIconOrFallbackStyle(
      GURL(ConvertJavaStringToUTF16(env, j_page_url)),
      min_source_size_px,
      0,  // Do not resize.
      callback_runner,
      &cancelable_task_tracker_);

  return true;
}

// static
bool LargeIconBridge::RegisterLargeIconBridge(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
