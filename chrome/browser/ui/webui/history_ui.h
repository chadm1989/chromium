// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_HISTORY_UI_H_
#define CHROME_BROWSER_UI_WEBUI_HISTORY_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/base/layout.h"

class Profile;

namespace base {
class RefCountedMemory;
}

class HistoryUI : public content::WebUIController {
 public:
  explicit HistoryUI(content::WebUI* web_ui);
  ~HistoryUI() override;

  static base::RefCountedMemory* GetFaviconResourceBytes(
      ui::ScaleFactor scale_factor);

  // Returns a localized string warning about deleting history. Takes into
  // account whether or not incognito mode is available.
  static base::string16 GetDeleteWarningString(Profile* profile);

 private:
  DISALLOW_COPY_AND_ASSIGN(HistoryUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_HISTORY_UI_H_
