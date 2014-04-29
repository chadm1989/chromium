// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_

#import <Cocoa/Cocoa.h>

// This protocol is used as a delegate for the NSView class used in the
// hierarchy. There are two ways to extend the view:
// - Implement the methods listed in the protocol below.
// - Implement any method, and if the view is requested to perform that method
//   and cannot, the delegate's implementation will be used.
//
// Like any Objective-C delegate, it is not retained by the delegator object.
// The delegator object will call the -viewGone: method when it is going away.

@class NSEvent;
@protocol RenderWidgetHostViewMacDelegate
@optional
// Notification that the view is gone.
- (void)viewGone:(NSView*)view;

// Handle an event. All incoming key and mouse events flow through this delegate
// method if implemented. Return YES if the event is fully handled, or NO if
// normal processing should take place.
- (BOOL)handleEvent:(NSEvent*)event;

// Notification that a wheel event was unhandled.
- (void)gotUnhandledWheelEvent;

// Notification of scroll offset pinning.
- (void)scrollOffsetPinnedToLeft:(BOOL)left toRight:(BOOL)right;

// Notification of whether the view has a horizontal scrollbar.
- (void)setHasHorizontalScrollbar:(BOOL)has_horizontal_scrollbar;

// Provides validation of user interface items. If the return value is NO, then
// the delegate is unaware of that item and |valid| is undefined.  Otherwise,
// |valid| contains the validity of the specified item.
- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item
                      isValidItem:(BOOL*)valid;

@required
// Notification of when a gesture begins/ends.
- (void)beginGestureWithEvent:(NSEvent*)event;
- (void)endGestureWithEvent:(NSEvent*)event;

// This is a low level API which provides touches associated with an event.
// It is used in conjunction with gestures to determine finger placement
// on the trackpad.
- (void)touchesMovedWithEvent:(NSEvent*)event;
- (void)touchesBeganWithEvent:(NSEvent*)event;
- (void)touchesCancelledWithEvent:(NSEvent*)event;
- (void)touchesEndedWithEvent:(NSEvent*)event;

// These methods control whether a given view is allowed to rubberband in the
// given direction. This is inversely related to whether the view is allowed to
// 2-finger history swipe in the given direction.
- (BOOL)canRubberbandLeft:(NSView*)view;
- (BOOL)canRubberbandRight:(NSView*)view;
@end

#endif  // CONTENT_PUBLIC_BROWSER_RENDER_WIDGET_HOST_VIEW_MAC_DELEGATE_H_
