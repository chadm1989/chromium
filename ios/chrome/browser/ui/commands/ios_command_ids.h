// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_IOS_COMMAND_IDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_IOS_COMMAND_IDS_H_

// This file lists all the command IDs understood by e.g. the browser.
// It is used by NIB files.

// NIB files include ID numbers rather than the corresponding #define labels.
// If you change a given command's number, any NIB files that refer to it will
// also need to be updated.

// Do not use IDs below 40900 while the iOS build still depends on //chrome, to
// avoid conflicts.
// TODO(droger): Remove this comment once iOS no longer depends on //chrome.
#define IDC_SHOW_TOOLS_MENU                            40900
#define IDC_TOGGLE_TAB_SWITCHER                        40901
#define IDC_VOICE_SEARCH                               40902
#define IDC_NEW_INCOGNITO_TAB                          40903
#define IDC_CLOSE_ALL_TABS                             40904
#define IDC_SHOW_SIGNIN_IOS                            40905
#define IDC_SWITCH_BROWSER_MODES                       40906
#define IDC_FIND_CLOSE                                 40907
#define IDC_FIND_UPDATE                                40908
#define IDC_SHOW_PAGE_INFO                             40911
#define IDC_HIDE_PAGE_INFO                             40912
#define IDC_SHOW_SECURITY_HELP                         40913
#define IDC_SHOW_SYNC_SETTINGS                         40914
#define IDC_OPEN_URL                                   40915
#define IDC_SETUP_FOR_TESTING                          40916
#define IDC_SHOW_OTHER_DEVICES                         40917
#define IDC_CLOSE_ALL_INCOGNITO_TABS                   40918
#define IDC_CLOSE_SETTINGS_AND_OPEN_URL                40920
#define IDC_REQUEST_DESKTOP_SITE                       40921
#define IDC_CLEAR_BROWSING_DATA_IOS                    40924
#define IDC_SHOW_SIGN_IN_WITH_RESHARE_ON_COMPLETION    40925
#define IDC_SHOW_MAIL_COMPOSER                         40926
#define IDC_BACK_TO_CALLING_APP                        40927
#define IDC_RESET_ALL_WEBVIEWS                         40928
#define IDC_SHARE_PAGE                                 40929
#define IDC_BACK_FORWARD_IN_TAB_HISTORY                40930
#define IDC_SHOW_GOOGLE_APPS_SETTINGS                  40931
#define IDC_SHOW_TRANSLATE_SETTINGS                    40932
#define IDC_SHOW_SIGN_IN_WITH_PRINT_ON_COMPLETION      40933
#define IDC_SHOW_PRIVACY_SETTINGS                      40934
#define IDC_HIDE_SETTINGS_AND_SHOW_PRIVACY_SETTINGS    40935
#define IDC_REPORT_AN_ISSUE                            40936
#define IDC_PRELOAD_VOICE_SEARCH                       40937
#define IDC_SHOW_BACK_HISTORY                          40938
#define IDC_SHOW_FORWARD_HISTORY                       40939
#define IDC_SHOW_PROXY_SETTINGS                        40940
#define IDC_CLOSE_SETTINGS_AND_OPEN_NEW_INCOGNITO_TAB  40942
#define IDC_SHOW_ACCOUNTS_SETTINGS                     40943
#define IDC_CLOSE_SETTINGS                             40944
#define IDC_SHOW_SAVE_PASSWORDS_SETTINGS               40945
#define IDC_READER_MODE                                40947
// Do not use IDs above 40999 while the iOS build still depends on //chrome, to
// avoid conflicts.
// TODO(droger): Remove this comment once iOS no longer depends on //chrome.

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_IOS_COMMAND_IDS_H_
