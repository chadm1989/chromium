// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/guest_view/web_view/context_menu_content_type_web_view.h"

#include "base/command_line.h"
#include "chrome/common/channel_info.h"
#include "chrome/common/chrome_switches.h"
#include "components/version_info/version_info.h"
#include "extensions/browser/guest_view/web_view/web_view_guest.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest.h"

using extensions::Extension;
using extensions::ProcessManager;

ContextMenuContentTypeWebView::ContextMenuContentTypeWebView(
    content::WebContents* web_contents,
    const content::ContextMenuParams& params)
    : ContextMenuContentType(web_contents, params, true) {
}

ContextMenuContentTypeWebView::~ContextMenuContentTypeWebView() {
}

const Extension* ContextMenuContentTypeWebView::GetExtension() const {
  ProcessManager* process_manager =
      ProcessManager::Get(source_web_contents()->GetBrowserContext());
  return process_manager->GetExtensionForWebContents(
      source_web_contents());
}

bool ContextMenuContentTypeWebView::SupportsGroup(int group) {
  switch (group) {
    case ITEM_GROUP_PAGE:
    case ITEM_GROUP_FRAME:
    case ITEM_GROUP_LINK:
    case ITEM_GROUP_SEARCHWEBFORIMAGE:
    case ITEM_GROUP_SEARCH_PROVIDER:
    case ITEM_GROUP_PRINT:
    case ITEM_GROUP_ALL_EXTENSION:
    case ITEM_GROUP_PRINT_PREVIEW:
      return false;
    case ITEM_GROUP_CURRENT_EXTENSION:
      // Show contextMenus API items.
      return true;
    case ITEM_GROUP_DEVELOPER:
      {
      if (chrome::GetChannel() >= version_info::Channel::DEV) {
          // Hide dev tools items in guests inside WebUI if we are not running
          // canary or tott.
          auto web_view_guest =
              extensions::WebViewGuest::FromWebContents(source_web_contents());
          if (web_view_guest &&
                  web_view_guest->owner_web_contents()->GetWebUI()) {
              return false;
          }
        }

        // TODO(lazyboy): Enable this for mac too when http://crbug.com/380405
        // is fixed.
#if !defined(OS_MACOSX)
        // Add dev tools for unpacked extensions.
        const extensions::Extension* embedder_platform_app = GetExtension();
        return !embedder_platform_app ||
               extensions::Manifest::IsUnpackedLocation(
                   embedder_platform_app->location()) ||
               base::CommandLine::ForCurrentProcess()->HasSwitch(
                   switches::kDebugPackedApps);
#else
        return ContextMenuContentType::SupportsGroup(group);
#endif
      }
    default:
      return ContextMenuContentType::SupportsGroup(group);
  }
}
