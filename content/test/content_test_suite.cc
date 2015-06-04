// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/content_test_suite.h"

#if defined(OS_ANDROID)
#include <android/native_window.h>
#include <android/native_window_jni.h>
#endif

#include "base/base_paths.h"
#include "base/logging.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_paths.h"
#include "content/public/test/test_content_client_initializer.h"
#include "gpu/config/gpu_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include "ui/gfx/win/dpi.h"
#endif

#if defined(OS_MACOSX)
#include "base/mac/scoped_nsautorelease_pool.h"
#if !defined(OS_IOS)
#include "base/containers/scoped_ptr_hash_map.h"
#include "base/mac/scoped_mach_port.h"
#include "base/memory/scoped_ptr.h"
#include "base/test/mock_chrome_application_mac.h"
#include "content/common/mac/io_surface_manager.h"
#endif
#endif

#if !defined(OS_IOS)
#include "base/base_switches.h"
#include "base/command_line.h"
#include "media/base/media.h"
#include "ui/gl/gl_surface.h"
#endif

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/scoped_ptr.h"
#include "content/common/android/surface_texture_manager.h"
#include "ui/gl/android/scoped_java_surface.h"
#include "ui/gl/android/surface_texture.h"
#endif

namespace content {
namespace {

class TestInitializationListener : public testing::EmptyTestEventListener {
 public:
  TestInitializationListener() : test_content_client_initializer_(NULL) {
  }

  void OnTestStart(const testing::TestInfo& test_info) override {
    test_content_client_initializer_ =
        new content::TestContentClientInitializer();
  }

  void OnTestEnd(const testing::TestInfo& test_info) override {
    delete test_content_client_initializer_;
  }

 private:
  content::TestContentClientInitializer* test_content_client_initializer_;

  DISALLOW_COPY_AND_ASSIGN(TestInitializationListener);
};

#if defined(OS_ANDROID)
class TestSurfaceTextureManager : public SurfaceTextureManager {
 public:
  // Overridden from SurfaceTextureManager:
  void RegisterSurfaceTexture(int surface_texture_id,
                              int client_id,
                              gfx::SurfaceTexture* surface_texture) override {
    surfaces_.add(surface_texture_id,
                  make_scoped_ptr(new gfx::ScopedJavaSurface(surface_texture)));
  }
  void UnregisterSurfaceTexture(int surface_texture_id,
                                int client_id) override {
    surfaces_.erase(surface_texture_id);
  }
  gfx::AcceleratedWidget AcquireNativeWidgetForSurfaceTexture(
      int surface_texture_id) override {
    JNIEnv* env = base::android::AttachCurrentThread();
    return ANativeWindow_fromSurface(
        env, surfaces_.get(surface_texture_id)->j_surface().obj());
  }

 private:
  using SurfaceMap =
      base::ScopedPtrHashMap<int, scoped_ptr<gfx::ScopedJavaSurface>>;
  SurfaceMap surfaces_;
};
#endif

#if defined(OS_MACOSX) && !defined(OS_IOS)
class TestIOSurfaceManager : public IOSurfaceManager {
 public:
  // Overridden from IOSurfaceManager:
  bool RegisterIOSurface(int io_surface_id,
                         int client_id,
                         IOSurfaceRef io_surface) override {
    io_surfaces_.add(io_surface_id,
                     make_scoped_ptr(new base::mac::ScopedMachSendRight(
                         IOSurfaceCreateMachPort(io_surface))));
    return true;
  }
  void UnregisterIOSurface(int io_surface_id, int client_id) override {
    io_surfaces_.erase(io_surface_id);
  }
  IOSurfaceRef AcquireIOSurface(int io_surface_id) override {
    return IOSurfaceLookupFromMachPort(io_surfaces_.get(io_surface_id)->get());
  }

 private:
  using IOSurfaceMap =
      base::ScopedPtrHashMap<int, scoped_ptr<base::mac::ScopedMachSendRight>>;
  IOSurfaceMap io_surfaces_;
};
#endif

}  // namespace

ContentTestSuite::ContentTestSuite(int argc, char** argv)
    : ContentTestSuiteBase(argc, argv) {
}

ContentTestSuite::~ContentTestSuite() {
}

void ContentTestSuite::Initialize() {
#if defined(OS_MACOSX)
  base::mac::ScopedNSAutoreleasePool autorelease_pool;
#if !defined(OS_IOS)
  mock_cr_app::RegisterMockCrApp();
#endif
#endif

#if defined(OS_WIN)
  gfx::InitDeviceScaleFactor(1.0f);
#endif

  ContentTestSuiteBase::Initialize();
  {
    ContentClient client;
    ContentTestSuiteBase::RegisterContentSchemes(&client);
  }
  RegisterPathProvider();
#if !defined(OS_IOS)
  media::InitializeMediaLibrary();
  // When running in a child process for Mac sandbox tests, the sandbox exists
  // to initialize GL, so don't do it here.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kTestChildProcess)) {
    gfx::GLSurface::InitializeOneOffForTests();
    gpu::ApplyGpuDriverBugWorkarounds(base::CommandLine::ForCurrentProcess());
  }
#endif
  testing::TestEventListeners& listeners =
      testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new TestInitializationListener);
#if defined(OS_ANDROID)
  SurfaceTextureManager::SetInstance(new TestSurfaceTextureManager);
#endif
#if defined(OS_MACOSX) && !defined(OS_IOS)
  IOSurfaceManager::SetInstance(new TestIOSurfaceManager);
#endif
}

}  // namespace content
