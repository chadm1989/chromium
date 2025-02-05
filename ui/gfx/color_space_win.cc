// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/color_space.h"

#include <windows.h>
#include <stddef.h>
#include <map>

#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"

namespace gfx {

namespace {

void ReadBestMonitorICCProfile(std::vector<char>* profile) {
  HDC screen_dc = GetDC(NULL);
  DWORD path_len = MAX_PATH;
  WCHAR path[MAX_PATH + 1];

  BOOL result = GetICMProfile(screen_dc, &path_len, path);
  ReleaseDC(NULL, screen_dc);
  if (!result)
    return;
  std::string profile_data;
  if (!base::ReadFileToString(base::FilePath(path), &profile_data))
    return;
  size_t length = profile_data.size();
  if (!ColorSpace::IsValidProfileLength(length))
    return;
  profile->assign(profile_data.data(), profile_data.data() + length);
}

base::LazyInstance<base::Lock> g_best_monitor_color_space_lock =
    LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<gfx::ColorSpace> g_best_monitor_color_space =
    LAZY_INSTANCE_INITIALIZER;
bool g_has_initialized_best_monitor_color_space = false;

}  // namespace

// static
ColorSpace ColorSpace::FromBestMonitor() {
  base::AutoLock lock(g_best_monitor_color_space_lock.Get());
  return g_best_monitor_color_space.Get();
}

// static
bool ColorSpace::CachedProfilesNeedUpdate() {
  base::AutoLock lock(g_best_monitor_color_space_lock.Get());
  return !g_has_initialized_best_monitor_color_space;
}

// static
void ColorSpace::UpdateCachedProfilesOnBackgroundThread() {
  std::vector<char> icc_profile;
  ReadBestMonitorICCProfile(&icc_profile);
  gfx::ColorSpace color_space = FromICCProfile(icc_profile);

  base::AutoLock lock(g_best_monitor_color_space_lock.Get());
  g_best_monitor_color_space.Get() = color_space;
  g_has_initialized_best_monitor_color_space = true;
}

}  // namespace gfx
