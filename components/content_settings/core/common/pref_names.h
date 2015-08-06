// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_PREF_NAMES_H_
#define COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_PREF_NAMES_H_

#include "base/logging.h"

namespace prefs {

// NOTE: This file does not contain all content settings related pref names as
// some of these are generated by WebsiteSettingsInfo from content settings
// names.

extern const char kBlockThirdPartyCookies[];
extern const char kContentSettingsVersion[];
extern const char kContentSettingsWindowLastTabIndex[];

extern const char kMigratedDefaultMediaStreamSetting[];

extern const char kManagedDefaultCookiesSetting[];
extern const char kManagedDefaultImagesSetting[];
extern const char kManagedDefaultJavaScriptSetting[];
extern const char kManagedDefaultPluginsSetting[];
extern const char kManagedDefaultPopupsSetting[];
extern const char kManagedDefaultGeolocationSetting[];
extern const char kManagedDefaultNotificationsSetting[];
extern const char kManagedDefaultMediaStreamSetting[];

extern const char kManagedCookiesAllowedForUrls[];
extern const char kManagedCookiesBlockedForUrls[];
extern const char kManagedCookiesSessionOnlyForUrls[];
extern const char kManagedImagesAllowedForUrls[];
extern const char kManagedImagesBlockedForUrls[];
extern const char kManagedJavaScriptAllowedForUrls[];
extern const char kManagedJavaScriptBlockedForUrls[];
extern const char kManagedPluginsAllowedForUrls[];
extern const char kManagedPluginsBlockedForUrls[];
extern const char kManagedPopupsAllowedForUrls[];
extern const char kManagedPopupsBlockedForUrls[];
extern const char kManagedNotificationsAllowedForUrls[];
extern const char kManagedNotificationsBlockedForUrls[];
extern const char kManagedAutoSelectCertificateForUrls[];

}  // namespace prefs

#endif  // COMPONENTS_CONTENT_SETTINGS_CORE_COMMON_PREF_NAMES_H_
