// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/downloads_counter.h"

#include "chrome/browser/download/download_history.h"
#include "chrome/browser/profiles/profile.h"
#include "components/browsing_data/pref_names.h"
#include "content/public/browser/download_manager.h"

DownloadsCounter::DownloadsCounter(Profile* profile)
    : BrowsingDataCounter(browsing_data::prefs::kDeleteDownloadHistory),
      profile_(profile) {}

DownloadsCounter::~DownloadsCounter() {
}

void DownloadsCounter::Count() {
  content::DownloadManager* download_manager =
      content::BrowserContext::GetDownloadManager(profile_);
  std::vector<content::DownloadItem*> downloads;
  download_manager->GetAllDownloads(&downloads);
  base::Time begin_time = GetPeriodStart();

  ReportResult(std::count_if(
      downloads.begin(),
      downloads.end(),
      [begin_time](const content::DownloadItem* item) {
        return item->GetStartTime() >= begin_time &&
            DownloadHistory::IsPersisted(item);
      }));
}
