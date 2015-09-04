// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "content/browser/renderer_host/render_widget_host_view_mac_dictionary_helper.h"

#include "base/strings/sys_string_conversions.h"
#import "content/browser/renderer_host/render_widget_host_view_mac.h"

namespace content {

RenderWidgetHostViewMacDictionaryHelper::
    RenderWidgetHostViewMacDictionaryHelper(RenderWidgetHostView* view)
    : view_(static_cast<RenderWidgetHostViewMac*>(view)),
      target_view_(static_cast<RenderWidgetHostViewMac*>(view)) {
}

void RenderWidgetHostViewMacDictionaryHelper::SetTargetView(
    RenderWidgetHostView* target_view) {
  target_view_ = static_cast<RenderWidgetHostViewMac*>(target_view);
}

void RenderWidgetHostViewMacDictionaryHelper::ShowDefinitionForSelection() {
  NSRange selection_range = [view_->cocoa_view() selectedRange];
  NSRect rect =
      [view_->cocoa_view() firstViewRectForCharacterRange:selection_range
                                              actualRange:nil];
  rect.origin.x += offset_.x();
  rect.origin.y += NSHeight(rect) + offset_.y();
  [view_->cocoa_view() showLookUpDictionaryOverlayAtPoint:rect.origin];
}

}  // namespace content
