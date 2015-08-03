// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/channel_info.h"

#include "base/profiler/scoped_tracker.h"
#include "build/build_config.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/version_info/version_info.h"
#include "ui/base/l10n/l10n_util.h"

namespace chrome {

std::string GetVersionString() {
  // TODO(robliao): Remove ScopedTracker below once https://crbug.com/422460 is
  // fixed.
  tracked_objects::ScopedTracker tracking_profile(
      FROM_HERE_WITH_EXPLICIT_FUNCTION(
          "422460 VersionInfo::CreateVersionString"));

  return version_info::GetVersionStringWithModifier(GetChannelString());
}

}  // namespace chrome
