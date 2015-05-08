// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_WIN_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_WIN_H_

#include "ui/aura/window_tree_host.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/win/hwnd_message_handler_delegate.h"
#include "ui/wm/public/animation_host.h"

namespace aura {
namespace client {
class DragDropClient;
class FocusClient;
class ScopedTooltipDisabler;
}
}

namespace views {
class DesktopCursorClient;
class DesktopDragDropClientWin;
class HWNDMessageHandler;

namespace corewm {
class TooltipWin;
}

class VIEWS_EXPORT DesktopWindowTreeHostWin
    : public DesktopWindowTreeHost,
      public aura::client::AnimationHost,
      public aura::WindowTreeHost,
      public ui::EventSource,
      public HWNDMessageHandlerDelegate {
 public:
  DesktopWindowTreeHostWin(
      internal::NativeWidgetDelegate* native_widget_delegate,
      DesktopNativeWidgetAura* desktop_native_widget_aura);
  ~DesktopWindowTreeHostWin() override;

  // A way of converting an HWND into a content window.
  static aura::Window* GetContentWindowForHWND(HWND hwnd);

 protected:
  // Overridden from DesktopWindowTreeHost:
  void Init(aura::Window* content_window,
            const Widget::InitParams& params) override;
  void OnNativeWidgetCreated(const Widget::InitParams& params) override;
  scoped_ptr<corewm::Tooltip> CreateTooltip() override;
  scoped_ptr<aura::client::DragDropClient> CreateDragDropClient(
      DesktopNativeCursorManager* cursor_manager) override;
  void Close() override;
  void CloseNow() override;
  aura::WindowTreeHost* AsWindowTreeHost() override;
  void ShowWindowWithState(ui::WindowShowState show_state) override;
  void ShowMaximizedWithBounds(const gfx::Rect& restored_bounds) override;
  bool IsVisible() const override;
  void SetSize(const gfx::Size& size) override;
  void StackAtTop() override;
  void CenterWindow(const gfx::Size& size) override;
  void GetWindowPlacement(gfx::Rect* bounds,
                          ui::WindowShowState* show_state) const override;
  gfx::Rect GetWindowBoundsInScreen() const override;
  gfx::Rect GetClientAreaBoundsInScreen() const override;
  gfx::Rect GetRestoredBounds() const override;
  gfx::Rect GetWorkAreaBoundsInScreen() const override;
  void SetShape(gfx::NativeRegion native_region) override;
  void Activate() override;
  void Deactivate() override;
  bool IsActive() const override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  bool HasCapture() const override;
  void SetAlwaysOnTop(bool always_on_top) override;
  bool IsAlwaysOnTop() const override;
  void SetVisibleOnAllWorkspaces(bool always_visible) override;
  bool SetWindowTitle(const base::string16& title) override;
  void ClearNativeFocus() override;
  Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      Widget::MoveLoopSource source,
      Widget::MoveLoopEscapeBehavior escape_behavior) override;
  void EndMoveLoop() override;
  void SetVisibilityChangedAnimationsEnabled(bool value) override;
  bool ShouldUseNativeFrame() const override;
  bool ShouldWindowContentsBeTransparent() const override;
  void FrameTypeChanged() override;
  void SetFullscreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetOpacity(unsigned char opacity) override;
  void SetWindowIcons(const gfx::ImageSkia& window_icon,
                      const gfx::ImageSkia& app_icon) override;
  void InitModalType(ui::ModalType modal_type) override;
  void FlashFrame(bool flash_frame) override;
  void OnRootViewLayout() override;
  void OnNativeWidgetFocus() override;
  void OnNativeWidgetBlur() override;
  bool IsAnimatingClosed() const override;
  bool IsTranslucentWindowOpacitySupported() const override;
  void SizeConstraintsChanged() override;

  // Overridden from aura::WindowTreeHost:
  ui::EventSource* GetEventSource() override;
  gfx::AcceleratedWidget GetAcceleratedWidget() override;
  void Show() override;
  void Hide() override;
  gfx::Rect GetBounds() const override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Point GetLocationOnNativeScreen() const override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void SetCursorNative(gfx::NativeCursor cursor) override;
  void OnCursorVisibilityChangedNative(bool show) override;
  void MoveCursorToNative(const gfx::Point& location) override;

  // Overridden frm ui::EventSource
  ui::EventProcessor* GetEventProcessor() override;

  // Overridden from aura::client::AnimationHost
  void SetHostTransitionOffsets(
      const gfx::Vector2d& top_left_delta,
      const gfx::Vector2d& bottom_right_delta) override;
  void OnWindowHidingAnimationCompleted() override;

  // Overridden from HWNDMessageHandlerDelegate:
  bool IsWidgetWindow() const override;
  bool IsUsingCustomFrame() const override;
  void SchedulePaint() override;
  void EnableInactiveRendering() override;
  bool IsInactiveRenderingDisabled() override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanMinimize() const override;
  bool CanActivate() const override;
  bool WidgetSizeIsClientSize() const override;
  bool IsModal() const override;
  int GetInitialShowState() const override;
  bool WillProcessWorkAreaChange() const override;
  int GetNonClientComponent(const gfx::Point& point) const override;
  void GetWindowMask(const gfx::Size& size, gfx::Path* path) override;
  bool GetClientAreaInsets(gfx::Insets* insets) const override;
  void GetMinMaxSize(gfx::Size* min_size, gfx::Size* max_size) const override;
  gfx::Size GetRootViewSize() const override;
  void ResetWindowControls() override;
  void PaintLayeredWindow(gfx::Canvas* canvas) override;
  gfx::NativeViewAccessible GetNativeViewAccessible() override;
  bool ShouldHandleSystemCommands() const override;
  InputMethod* GetInputMethod() override;
  void HandleAppDeactivated() override;
  void HandleActivationChanged(bool active) override;
  bool HandleAppCommand(short command) override;
  void HandleCancelMode() override;
  void HandleCaptureLost() override;
  void HandleClose() override;
  bool HandleCommand(int command) override;
  void HandleAccelerator(const ui::Accelerator& accelerator) override;
  void HandleCreate() override;
  void HandleDestroying() override;
  void HandleDestroyed() override;
  bool HandleInitialFocus(ui::WindowShowState show_state) override;
  void HandleDisplayChange() override;
  void HandleBeginWMSizeMove() override;
  void HandleEndWMSizeMove() override;
  void HandleMove() override;
  void HandleWorkAreaChanged() override;
  void HandleVisibilityChanging(bool visible) override;
  void HandleVisibilityChanged(bool visible) override;
  void HandleClientSizeChanged(const gfx::Size& new_size) override;
  void HandleFrameChanged() override;
  void HandleNativeFocus(HWND last_focused_window) override;
  void HandleNativeBlur(HWND focused_window) override;
  bool HandleMouseEvent(const ui::MouseEvent& event) override;
  bool HandleKeyEvent(const ui::KeyEvent& event) override;
  bool HandleUntranslatedKeyEvent(const ui::KeyEvent& event) override;
  void HandleTouchEvent(const ui::TouchEvent& event) override;
  bool HandleIMEMessage(UINT message,
                        WPARAM w_param,
                        LPARAM l_param,
                        LRESULT* result) override;
  void HandleInputLanguageChange(DWORD character_set,
                                 HKL input_language_id) override;
  bool HandlePaintAccelerated(const gfx::Rect& invalid_rect) override;
  void HandlePaint(gfx::Canvas* canvas) override;
  bool HandleTooltipNotify(int w_param,
                           NMHDR* l_param,
                           LRESULT* l_result) override;
  void HandleMenuLoop(bool in_menu_loop) override;
  bool PreHandleMSG(UINT message,
                    WPARAM w_param,
                    LPARAM l_param,
                    LRESULT* result) override;
  void PostHandleMSG(UINT message, WPARAM w_param, LPARAM l_param) override;
  bool HandleScrollEvent(const ui::ScrollEvent& event) override;
  void HandleWindowSizeChanging() override;

  Widget* GetWidget();
  const Widget* GetWidget() const;
  HWND GetHWND() const;

 private:
  void SetWindowTransparency();

  // Returns true if a modal window is active in the current root window chain.
  bool IsModalWindowActive() const;

  scoped_ptr<HWNDMessageHandler> message_handler_;
  scoped_ptr<aura::client::FocusClient> focus_client_;

  // TODO(beng): Consider providing an interface to DesktopNativeWidgetAura
  //             instead of providing this route back to Widget.
  internal::NativeWidgetDelegate* native_widget_delegate_;

  DesktopNativeWidgetAura* desktop_native_widget_aura_;

  aura::Window* content_window_;

  // Owned by DesktopNativeWidgetAura.
  DesktopDragDropClientWin* drag_drop_client_;

  // When certain windows are being shown, we augment the window size
  // temporarily for animation. The following two members contain the top left
  // and bottom right offsets which are used to enlarge the window.
  gfx::Vector2d window_expansion_top_left_delta_;
  gfx::Vector2d window_expansion_bottom_right_delta_;

  // Windows are enlarged to be at least 64x64 pixels, so keep track of the
  // extra added here.
  gfx::Vector2d window_enlargement_;

  // Whether the window close should be converted to a hide, and then actually
  // closed on the completion of the hide animation. This is cached because
  // the property is set on the contained window which has a shorter lifetime.
  bool should_animate_window_close_;

  // When Close()d and animations are being applied to this window, the close
  // of the window needs to be deferred to when the close animation is
  // completed. This variable indicates that a Close was converted to a Hide,
  // so that when the Hide is completed the host window should be closed.
  bool pending_close_;

  // True if the widget is going to have a non_client_view. We cache this value
  // rather than asking the Widget for the non_client_view so that we know at
  // Init time, before the Widget has created the NonClientView.
  bool has_non_client_view_;

  // Owned by TooltipController, but we need to forward events to it so we keep
  // a reference.
  corewm::TooltipWin* tooltip_;

  // Visibility of the cursor. On Windows we can have multiple root windows and
  // the implementation of ::ShowCursor() is based on a counter, so making this
  // member static ensures that ::ShowCursor() is always called exactly once
  // whenever the cursor visibility state changes.
  static bool is_cursor_visible_;

  scoped_ptr<aura::client::ScopedTooltipDisabler> tooltip_disabler_;

  // This flag is set to true in cases where we need to force a synchronous
  // paint via the compositor. Cases include restoring/resizing/maximizing the
  // window. Defaults to false.
  bool need_synchronous_paint_;

  // Set to true if we are about to enter a sizing loop.
  bool in_sizing_loop_;

  DISALLOW_COPY_AND_ASSIGN(DesktopWindowTreeHostWin);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_WIN_H_
