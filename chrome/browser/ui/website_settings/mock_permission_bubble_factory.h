// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBSITE_SETTINGS_MOCK_PERMISSION_BUBBLE_FACTORY_H_
#define CHROME_BROWSER_UI_WEBSITE_SETTINGS_MOCK_PERMISSION_BUBBLE_FACTORY_H_

#include <memory>
#include <vector>

#include "chrome/browser/permissions/permission_request_manager.h"

class Browser;
class PermissionBubbleView;
class MockPermissionBubbleView;

// Provides a skeleton class for both unit and browser testing when trying to
// test the bubble manager logic. Should not be used for anything that requires
// actual UI.
// See example usage in
// chrome/browser/permissions/permission_request_manager_unittest.cc
class MockPermissionBubbleFactory {
 public:
  explicit MockPermissionBubbleFactory(PermissionRequestManager* manager);
  ~MockPermissionBubbleFactory();

  // Create method called by the PBM to show a bubble.
  std::unique_ptr<PermissionBubbleView> Create(Browser* browser);

  void SetCanUpdateUi(bool can_update_ui);

  void ResetCounts();

  void DocumentOnLoadCompletedInMainFrame();

  void set_response_type(PermissionRequestManager::AutoResponseType type) {
    response_type_ = type;
  }

  // If the current view is visible.
  bool is_visible();
  // Number of times |Show| was called on any bubble.
  int show_count() { return show_count_; }
  // Number of requests seen by the last |Show|.
  int request_count() { return requests_count_; }
  // Number of requests seen.
  int total_request_count() { return total_requests_count_; }

  void WaitForPermissionBubble();

 private:
  friend class MockPermissionBubbleView;

  // This shouldn't be called. Is here to fail tests that try to create a bubble
  // after the factory has been destroyed.
  static std::unique_ptr<PermissionBubbleView> DoNotCreate(Browser* browser);

  void UpdateResponseType();
  void ShowView(MockPermissionBubbleView* view);
  void HideView(MockPermissionBubbleView* view);

  bool can_update_ui_;
  int show_count_;
  int requests_count_;
  int total_requests_count_;
  std::vector<MockPermissionBubbleView*> views_;
  PermissionRequestManager::AutoResponseType response_type_;

  base::Closure show_bubble_quit_closure_;

  // The bubble manager that will be associated with this factory.
  PermissionRequestManager* manager_;

  DISALLOW_COPY_AND_ASSIGN(MockPermissionBubbleFactory);
};

#endif  // CHROME_BROWSER_UI_WEBSITE_SETTINGS_MOCK_PERMISSION_BUBBLE_FACTORY_H_
