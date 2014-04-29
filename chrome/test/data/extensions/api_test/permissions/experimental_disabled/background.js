// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Calls to chrome.experimental.* functions should fail, since this extension
// has not declared that permission.

chrome.test.runTests([
  function experimental() {
    chrome.tabs.getSelected(null, function(tab) {
      try {
        // If/when chrome.experimental.accessibility is moved out of
        // experimental, this test needs to be updated.
        chrome.experimental.accessibility.getAlertsForTab(
          tab.id, function(alerts) {
          chrome.test.fail();
        });
      } catch (e) {
        chrome.test.succeed();
      }
    });
  }
]);
