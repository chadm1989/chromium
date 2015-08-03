// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autofill/risk_util.h"

#include "base/base64.h"
#include "base/callback.h"
#include "base/prefs/pref_service.h"
#include "base/time/time.h"
#include "chrome/browser/apps/app_window_registry_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_content_client.h"
#include "chrome/common/pref_names.h"
#include "components/autofill/content/browser/risk/fingerprint.h"
#include "components/autofill/content/browser/risk/proto/fingerprint.pb.h"
#include "components/metrics/metrics_service.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/web_contents.h"

#if !defined(OS_ANDROID)
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "ui/base/base_window.h"
#endif

namespace autofill {

namespace {

void PassRiskData(const base::Callback<void(const std::string&)>& callback,
                  scoped_ptr<risk::Fingerprint> fingerprint) {
  std::string proto_data, risk_data;
  fingerprint->SerializeToString(&proto_data);
  base::Base64Encode(proto_data, &risk_data);
  callback.Run(risk_data);
}

#if !defined(OS_ANDROID)
// Returns the containing window for the given |web_contents|. The containing
// window might be a browser window for a Chrome tab, or it might be an app
// window for a platform app.
ui::BaseWindow* GetBaseWindowForWebContents(
    content::WebContents* web_contents) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (browser)
    return browser->window();

  gfx::NativeWindow native_window = web_contents->GetTopLevelNativeWindow();
  extensions::AppWindow* app_window =
      AppWindowRegistryUtil::GetAppWindowForNativeWindowAnyProfile(
          native_window);
  return app_window->GetBaseWindow();
}
#endif

}  // namespace

void LoadRiskData(uint64 obfuscated_gaia_id,
                  content::WebContents* web_contents,
                  const base::Callback<void(const std::string&)>& callback) {
  // No easy way to get window bounds on Android, and that signal isn't very
  // useful anyway (given that we're also including the bounds of the web
  // contents).
  gfx::Rect window_bounds;
#if !defined(OS_ANDROID)
  window_bounds = GetBaseWindowForWebContents(web_contents)->GetBounds();
#endif

  const PrefService* user_prefs =
      Profile::FromBrowserContext(web_contents->GetBrowserContext())
          ->GetPrefs();
  std::string charset = user_prefs->GetString(::prefs::kDefaultCharset);
  std::string accept_languages =
      user_prefs->GetString(::prefs::kAcceptLanguages);
  base::Time install_time = base::Time::FromTimeT(
      g_browser_process->metrics_service()->GetInstallDate());

  risk::GetFingerprint(obfuscated_gaia_id, window_bounds, web_contents,
                       version_info::GetVersionNumber(), charset,
                       accept_languages, install_time,
                       g_browser_process->GetApplicationLocale(),
                       GetUserAgent(), base::Bind(PassRiskData, callback));
}

}  // namespace autofill
