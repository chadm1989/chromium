// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/rtl_geometry.h"

#include <limits>
#import <UIKit/UIKit.h>

#include "base/ios/ios_util.h"
#include "base/logging.h"
#include "ios/chrome/browser/ui/ui_util.h"

bool UseRTLLayout() {
  return base::i18n::IsRTL() && base::ios::IsRunningOnIOS9OrLater();
}

base::i18n::TextDirection LayoutDirection() {
  return UseRTLLayout() ? base::i18n::RIGHT_TO_LEFT : base::i18n::LEFT_TO_RIGHT;
}

#pragma mark - LayoutRects.

const LayoutRectPosition LayoutRectPositionZero = {0.0, 0.0};

LayoutRectPosition LayoutRectPositionMake(CGFloat leading, CGFloat originY) {
  return {leading, originY};
}

BOOL LayoutRectPositionEqualToPosition(LayoutRectPosition a,
                                       LayoutRectPosition b) {
  CGFloat epsilon = std::numeric_limits<CGFloat>::epsilon();
  return fabs(a.leading - b.leading) <= epsilon &&
         fabs(a.originY - b.originY) <= epsilon;
}

LayoutRectPosition AlignLayoutRectPositionToPixel(LayoutRectPosition position) {
  return LayoutRectPositionMake(AlignValueToPixel(position.leading),
                                AlignValueToPixel(position.originY));
}

const LayoutRect LayoutRectZero = {0.0, LayoutRectPositionZero,
                                   CGSizeMake(0.0, 0.0)};

LayoutRect LayoutRectMake(CGFloat leading,
                          CGFloat boundingWidth,
                          CGFloat originY,
                          CGFloat width,
                          CGFloat height) {
  return {boundingWidth, LayoutRectPositionMake(leading, originY),
          CGSizeMake(width, height)};
}

BOOL LayoutRectEqualToRect(LayoutRect a, LayoutRect b) {
  CGFloat epsilon = std::numeric_limits<CGFloat>::epsilon();
  return fabs(a.boundingWidth - b.boundingWidth) <= epsilon &&
         LayoutRectPositionEqualToPosition(a.position, b.position) &&
         CGSizeEqualToSize(a.size, b.size);
}

CGRect LayoutRectGetRectUsingDirection(LayoutRect layout,
                                       base::i18n::TextDirection direction) {
  CGRect rect;
  if (direction == base::i18n::RIGHT_TO_LEFT) {
    CGFloat trailing =
        layout.boundingWidth - (layout.position.leading + layout.size.width);
    rect = CGRectMake(trailing, layout.position.originY, layout.size.width,
                      layout.size.height);
  } else {
    DCHECK_EQ(direction, base::i18n::LEFT_TO_RIGHT);
    rect = CGRectMake(layout.position.leading, layout.position.originY,
                      layout.size.width, layout.size.height);
  }
  return rect;
}

CGRect LayoutRectGetRect(LayoutRect layout) {
  return LayoutRectGetRectUsingDirection(layout, LayoutDirection());
}

CGRect LayoutRectGetBoundsRect(LayoutRect layout) {
  return CGRectMake(0, 0, layout.size.width, layout.size.height);
}

CGPoint LayoutRectGetPositionForAnchorUsingDirection(
    LayoutRect layout,
    CGPoint anchor,
    base::i18n::TextDirection direction) {
  CGRect rect = LayoutRectGetRectUsingDirection(layout, direction);
  return CGPointMake(CGRectGetMinX(rect) + (rect.size.width * anchor.x),
                     CGRectGetMinY(rect) + (rect.size.height * anchor.y));
}

CGPoint LayoutRectGetPositionForAnchor(LayoutRect layout, CGPoint anchor) {
  return LayoutRectGetPositionForAnchorUsingDirection(layout, anchor,
                                                      LayoutDirection());
}

LayoutRect LayoutRectForRectInBoundingRectUsingDirection(
    CGRect rect,
    CGRect contextRect,
    base::i18n::TextDirection direction) {
  LayoutRect layout;
  if (direction == base::i18n::RIGHT_TO_LEFT) {
    layout.position.leading = contextRect.size.width - CGRectGetMaxX(rect);
  } else {
    layout.position.leading = CGRectGetMinX(rect);
  }
  layout.boundingWidth = contextRect.size.width;
  layout.position.originY = rect.origin.y;
  layout.size = rect.size;
  return layout;
}

LayoutRect LayoutRectForRectInBoundingRect(CGRect rect, CGRect contextRect) {
  return LayoutRectForRectInBoundingRectUsingDirection(rect, contextRect,
                                                       LayoutDirection());
}

LayoutRect LayoutRectGetLeadingLayout(LayoutRect layout) {
  LayoutRect leadingLayout;
  leadingLayout.position.leading = 0;
  leadingLayout.boundingWidth = layout.boundingWidth;
  leadingLayout.position.originY = leadingLayout.position.originY;
  leadingLayout.size = CGSizeMake(layout.position.leading, layout.size.height);
  return leadingLayout;
}

LayoutRect LayoutRectGetTrailingLayout(LayoutRect layout) {
  LayoutRect leadingLayout;
  CGFloat trailing = LayoutRectGetTrailingEdge(layout);
  leadingLayout.position.leading = trailing;
  leadingLayout.boundingWidth = layout.boundingWidth;
  leadingLayout.position.originY = leadingLayout.position.originY;
  leadingLayout.size =
      CGSizeMake((layout.boundingWidth - trailing), layout.size.height);
  return leadingLayout;
}

CGFloat LayoutRectGetTrailingEdge(LayoutRect layout) {
  return layout.position.leading + layout.size.width;
}

#pragma mark - UIKit utilities

CGFloat CGRectGetLeadingEdgeUsingDirection(
    CGRect rect,
    base::i18n::TextDirection direction) {
  return direction == base::i18n::RIGHT_TO_LEFT ? CGRectGetMaxX(rect)
                                                : CGRectGetMinX(rect);
}

CGFloat CGRectGetTrailingEdgeUsingDirection(
    CGRect rect,
    base::i18n::TextDirection direction) {
  return direction == base::i18n::RIGHT_TO_LEFT ? CGRectGetMinX(rect)
                                                : CGRectGetMaxX(rect);
}

CGFloat CGRectGetLeadingEdge(CGRect rect) {
  return CGRectGetLeadingEdgeUsingDirection(rect, LayoutDirection());
}

CGFloat CGRectGetTrailingEdge(CGRect rect) {
  return CGRectGetTrailingEdgeUsingDirection(rect, LayoutDirection());
}

UIViewAutoresizing UIViewAutoresizingFlexibleLeadingMargin() {
  return base::i18n::IsRTL() && base::ios::IsRunningOnIOS9OrLater()
             ? UIViewAutoresizingFlexibleRightMargin
             : UIViewAutoresizingFlexibleLeftMargin;
}

UIViewAutoresizing UIViewAutoresizingFlexibleTrailingMargin() {
  return base::i18n::IsRTL() && base::ios::IsRunningOnIOS9OrLater()
             ? UIViewAutoresizingFlexibleLeftMargin
             : UIViewAutoresizingFlexibleRightMargin;
}

UIEdgeInsets UIEdgeInsetsMakeUsingDirection(
    CGFloat top,
    CGFloat leading,
    CGFloat bottom,
    CGFloat trailing,
    base::i18n::TextDirection direction) {
  if (direction == base::i18n::RIGHT_TO_LEFT) {
    using std::swap;
    swap(leading, trailing);
  }
  // At this point, |leading| == left, |trailing| = right.
  return UIEdgeInsetsMake(top, leading, bottom, trailing);
}

UIEdgeInsets UIEdgeInsetsMakeDirected(CGFloat top,
                                      CGFloat leading,
                                      CGFloat bottom,
                                      CGFloat trailing) {
  return UIEdgeInsetsMakeUsingDirection(top, leading, bottom, trailing,
                                        LayoutDirection());
}

CGFloat UIEdgeInsetsGetLeading(UIEdgeInsets insets) {
  return UseRTLLayout() ? insets.right : insets.left;
}

CGFloat UIEdgeInsetsGetTrailing(UIEdgeInsets insets) {
  return UseRTLLayout() ? insets.left : insets.right;
}

#pragma mark - autolayout utilities

NSLayoutFormatOptions LayoutOptionForRTLSupport() {
  if (UseRTLLayout()) {
    return NSLayoutFormatDirectionLeadingToTrailing;
  }
  return NSLayoutFormatDirectionLeftToRight;
}
