// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_CHROME_STABILITY_METRICS_PROVIDER_H_
#define CHROME_BROWSER_METRICS_CHROME_STABILITY_METRICS_PROVIDER_H_

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/metrics/user_metrics.h"
#include "base/process/kill.h"
#include "components/metrics/metrics_provider.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class PrefRegistrySimple;

namespace content {
class RenderProcessHost;
class WebContents;
}

class PrefService;

// ChromeStabilityMetricsProvider gathers and logs Chrome-specific stability-
// related metrics.
class ChromeStabilityMetricsProvider
    : public metrics::MetricsProvider,
      public content::BrowserChildProcessObserver,
      public content::NotificationObserver {
 public:
  explicit ChromeStabilityMetricsProvider(PrefService* local_state);
  ~ChromeStabilityMetricsProvider() override;

  // metrics::MetricsDataProvider:
  void OnRecordingEnabled() override;
  void OnRecordingDisabled() override;
  void ProvideStabilityMetrics(
      metrics::SystemProfileProto* system_profile_proto) override;
  void ClearSavedStabilityMetrics() override;

  // Registers local state prefs used by this class.
  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  FRIEND_TEST_ALL_PREFIXES(ChromeStabilityMetricsProviderTest,
                           BrowserChildProcessObserver);
  FRIEND_TEST_ALL_PREFIXES(ChromeStabilityMetricsProviderTest,
                           NotificationObserver);

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // content::BrowserChildProcessObserver:
  void BrowserChildProcessCrashed(
      const content::ChildProcessData& data,
      int exit_code) override;

  // Logs the initiation of a page load and uses |web_contents| to do
  // additional logging of the type of page loaded.
  void LogLoadStarted(content::WebContents* web_contents);

  // Records a renderer process crash.
  void LogRendererCrash(content::RenderProcessHost* host,
                        base::TerminationStatus status,
                        int exit_code);

  // Increment an Integer pref value specified by |path|
  void IncrementPrefValue(const char* path);

  // Increment a 64-bit Integer pref value specified by |path|
  void IncrementLongPrefsValue(const char* path);

  // Records a renderer process hang.
  void LogRendererHang();

  PrefService* local_state_;

  // Registrar for receiving stability-related notifications.
  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(ChromeStabilityMetricsProvider);
};

#endif  // CHROME_BROWSER_METRICS_CHROME_STABILITY_METRICS_PROVIDER_H_
