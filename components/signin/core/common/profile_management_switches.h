// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These are functions to access various profile-management flags but with
// possible overrides from Experiements.  This is done inside chrome/common
// because it is accessed by files through the chrome/ directory tree.

#ifndef CHROME_COMMON_PROFILE_MANAGEMENT_SWITCHES_H_
#define CHROME_COMMON_PROFILE_MANAGEMENT_SWITCHES_H_

namespace switches {

// Enables the web-based sign in flow on Chrome desktop.
bool IsEnableWebBasedSignin();

// Whether the chrome.identity API should be multi-account.
bool IsExtensionsMultiAccount();

// Checks whether the flag for fast user switching is enabled.
bool IsFastUserSwitching();

// Enables using GAIA information to populate profile name and icon.
bool IsGoogleProfileInfo();

// Whether the new avatar menu is enabled, either because new profile management
// is enabled or because the new profile management preview UI is enabled.
bool IsNewAvatarMenu();

// Use new profile management system, including profile sign-out and new
// choosers.
bool IsNewProfileManagement();

// Whether the new profile management preview has been enabled.
bool IsNewProfileManagementPreviewEnabled();

}  // namespace switches

#endif  // CHROME_COMMON_PROFILE_MANAGEMENT_SWITCHES_H_
