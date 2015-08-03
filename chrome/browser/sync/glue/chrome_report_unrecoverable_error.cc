// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/chrome_report_unrecoverable_error.h"

#include "base/debug/dump_without_crashing.h"
#include "base/rand_util.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_constants.h"
#include "components/version_info/version_info.h"

namespace browser_sync {

void ChromeReportUnrecoverableError() {
  // Only upload on canary/dev builds to avoid overwhelming crash server.
  version_info::Channel channel = chrome::GetChannel();
  if (channel != version_info::Channel::CANARY &&
      channel != version_info::Channel::DEV) {
    return;
  }

  // We only want to upload |kErrorUploadRatio| ratio of errors.
  const double kErrorUploadRatio = 0.01;
  if (kErrorUploadRatio <= 0.0)
    return; // We are not allowed to upload errors.
  double random_number = base::RandDouble();
  if (random_number > kErrorUploadRatio)
    return;

  base::debug::DumpWithoutCrashing();
}

}  // namespace browser_sync
