// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_VIEW_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_VIEW_HOST_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/process/kill.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/common/drag_event_source_info.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/window_container_type.h"
#include "net/base/load_states.h"
#include "third_party/WebKit/public/web/WebAXEnums.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"
#include "third_party/WebKit/public/web/WebPopupType.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/window_open_disposition.h"

class SkBitmap;
class FrameMsg_Navigate;
struct FrameMsg_Navigate_Params;
struct MediaPlayerAction;
struct ViewHostMsg_CreateWindow_Params;
struct ViewMsg_PostMessage_Params;

namespace base {
class ListValue;
}

namespace gfx {
class Range;
}

namespace ui {
class AXTree;
}

namespace content {

class MediaWebContentsObserver;
class ChildProcessSecurityPolicyImpl;
class PageState;
class RenderWidgetHostDelegate;
class SessionStorageNamespace;
class SessionStorageNamespaceImpl;
class TestRenderViewHost;
struct FileChooserFileInfo;
struct FileChooserParams;

#if defined(COMPILER_MSVC)
// RenderViewHostImpl is the bottom of a diamond-shaped hierarchy,
// with RenderWidgetHost at the root. VS warns when methods from the
// root are overridden in only one of the base classes and not both
// (in this case, RenderWidgetHostImpl provides implementations of
// many of the methods).  This is a silly warning when dealing with
// pure virtual methods that only have a single implementation in the
// hierarchy above this class, and is safe to ignore in this case.
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

// This implements the RenderViewHost interface that is exposed to
// embedders of content, and adds things only visible to content.
//
// The exact API of this object needs to be more thoroughly designed. Right
// now it mimics what WebContentsImpl exposed, which is a fairly large API and
// may contain things that are not relevant to a common subset of views. See
// also the comment in render_view_host_delegate.h about the size and scope of
// the delegate API.
//
// Right now, the concept of page navigation (both top level and frame) exists
// in the WebContentsImpl still, so if you instantiate one of these elsewhere,
// you will not be able to traverse pages back and forward. We need to determine
// if we want to bring that and other functionality down into this object so it
// can be shared by others.
class CONTENT_EXPORT RenderViewHostImpl
    : public RenderViewHost,
      public RenderWidgetHostImpl {
 public:
  // Convenience function, just like RenderViewHost::FromID.
  static RenderViewHostImpl* FromID(int render_process_id, int render_view_id);

  // |routing_id| could be a valid route id, or it could be MSG_ROUTING_NONE, in
  // which case RenderWidgetHost will create a new one.  |swapped_out| indicates
  // whether the view should initially be swapped out (e.g., for an opener
  // frame being rendered by another process). |hidden| indicates whether the
  // view is initially hidden or visible.
  //
  // The |session_storage_namespace| parameter allows multiple render views and
  // WebContentses to share the same session storage (part of the WebStorage
  // spec) space. This is useful when restoring contentses, but most callers
  // should pass in NULL which will cause a new SessionStorageNamespace to be
  // created.
  RenderViewHostImpl(
      SiteInstance* instance,
      RenderViewHostDelegate* delegate,
      RenderWidgetHostDelegate* widget_delegate,
      int routing_id,
      int main_frame_routing_id,
      bool swapped_out,
      bool hidden);
  virtual ~RenderViewHostImpl();

  // RenderViewHost implementation.
  virtual RenderFrameHost* GetMainFrame() override;
  virtual void AllowBindings(int binding_flags) override;
  virtual void ClearFocusedElement() override;
  virtual bool IsFocusedElementEditable() override;
  virtual void ClosePage() override;
  virtual void CopyImageAt(int x, int y) override;
  virtual void SaveImageAt(int x, int y) override;
  virtual void DirectoryEnumerationFinished(
      int request_id,
      const std::vector<base::FilePath>& files) override;
  virtual void DisableScrollbarsForThreshold(const gfx::Size& size) override;
  virtual void DragSourceEndedAt(
      int client_x, int client_y, int screen_x, int screen_y,
      blink::WebDragOperation operation) override;
  virtual void DragSourceSystemDragEnded() override;
  virtual void DragTargetDragEnter(
      const DropData& drop_data,
      const gfx::Point& client_pt,
      const gfx::Point& screen_pt,
      blink::WebDragOperationsMask operations_allowed,
      int key_modifiers) override;
  virtual void DragTargetDragOver(
      const gfx::Point& client_pt,
      const gfx::Point& screen_pt,
      blink::WebDragOperationsMask operations_allowed,
      int key_modifiers) override;
  virtual void DragTargetDragLeave() override;
  virtual void DragTargetDrop(const gfx::Point& client_pt,
                              const gfx::Point& screen_pt,
                              int key_modifiers) override;
  virtual void EnableAutoResize(const gfx::Size& min_size,
                                const gfx::Size& max_size) override;
  virtual void DisableAutoResize(const gfx::Size& new_size) override;
  virtual void EnablePreferredSizeMode() override;
  virtual void ExecuteMediaPlayerActionAtLocation(
      const gfx::Point& location,
      const blink::WebMediaPlayerAction& action) override;
  virtual void ExecutePluginActionAtLocation(
      const gfx::Point& location,
      const blink::WebPluginAction& action) override;
  virtual void ExitFullscreen() override;
  virtual void FilesSelectedInChooser(
      const std::vector<content::FileChooserFileInfo>& files,
      FileChooserParams::Mode permissions) override;
  virtual RenderViewHostDelegate* GetDelegate() const override;
  virtual int GetEnabledBindings() const override;
  virtual SiteInstanceImpl* GetSiteInstance() const override;
  virtual bool IsRenderViewLive() const override;
  virtual void NotifyMoveOrResizeStarted() override;
  virtual void SetWebUIProperty(const std::string& name,
                                const std::string& value) override;
  virtual void Zoom(PageZoom zoom) override;
  virtual void SyncRendererPrefs() override;
  virtual WebPreferences GetWebkitPreferences() override;
  virtual void UpdateWebkitPreferences(
      const WebPreferences& prefs) override;
  virtual void OnWebkitPreferencesChanged() override;
  virtual void GetAudioOutputControllers(
      const GetAudioOutputControllersCallback& callback) const override;
  virtual void SelectWordAroundCaret() override;

#if defined(OS_ANDROID)
  virtual void ActivateNearestFindResult(int request_id,
                                         float x,
                                         float y) override;
  virtual void RequestFindMatchRects(int current_version) override;
#endif

  void set_delegate(RenderViewHostDelegate* d) {
    CHECK(d);  // http://crbug.com/82827
    delegate_ = d;
  }

  // Set up the RenderView child process. Virtual because it is overridden by
  // TestRenderViewHost. If the |frame_name| parameter is non-empty, it is used
  // as the name of the new top-level frame.
  // The |opener_route_id| parameter indicates which RenderView created this
  // (MSG_ROUTING_NONE if none). If |max_page_id| is larger than -1, the
  // RenderView is told to start issuing page IDs at |max_page_id| + 1.
  // |window_was_created_with_opener| is true if this top-level frame was
  // created with an opener. (The opener may have been closed since.)
  // The |proxy_route_id| is only used when creating a RenderView in swapped out
  // state.
  virtual bool CreateRenderView(const base::string16& frame_name,
                                int opener_route_id,
                                int proxy_route_id,
                                int32 max_page_id,
                                bool window_was_created_with_opener);

  base::TerminationStatus render_view_termination_status() const {
    return render_view_termination_status_;
  }

  // Returns the content specific prefs for this RenderViewHost.
  WebPreferences ComputeWebkitPrefs(const GURL& url);

  // Tracks whether this RenderViewHost is in an active state (rather than
  // pending swap out, pending deletion, or swapped out), according to its main
  // frame RenderFrameHost.
  bool is_active() const { return is_active_; }
  void set_is_active(bool is_active) { is_active_ = is_active; }

  // Tracks whether this RenderViewHost is swapped out, according to its main
  // frame RenderFrameHost.
  void set_is_swapped_out(bool is_swapped_out) {
    is_swapped_out_ = is_swapped_out;
  }

  // TODO(creis): Remove as part of http://crbug.com/418265.
  bool is_waiting_for_close_ack() const { return is_waiting_for_close_ack_; }

  // Tells the renderer that this RenderView will soon be swapped out, and thus
  // not to create any new modal dialogs until it happens.  This must be done
  // separately so that the PageGroupLoadDeferrers of any current dialogs are no
  // longer on the stack when we attempt to swap it out.
  void SuppressDialogsUntilSwapOut();

  // Close the page ignoring whether it has unload events registers.
  // This is called after the beforeunload and unload events have fired
  // and the user has agreed to continue with closing the page.
  void ClosePageIgnoringUnloadEvents();

  // Tells the renderer view to focus the first (last if reverse is true) node.
  void SetInitialFocus(bool reverse);

  // Get html data by serializing all frames of current page with lists
  // which contain all resource links that have local copy.
  // The parameter links contain original URLs of all saved links.
  // The parameter local_paths contain corresponding local file paths of
  // all saved links, which matched with vector:links one by one.
  // The parameter local_directory_name is relative path of directory which
  // contain all saved auxiliary files included all sub frames and resouces.
  void GetSerializedHtmlDataForCurrentPageWithLocalLinks(
      const std::vector<GURL>& links,
      const std::vector<base::FilePath>& local_paths,
      const base::FilePath& local_directory_name);

  // Notifies the RenderViewHost that its load state changed.
  void LoadStateChanged(const GURL& url,
                        const net::LoadStateWithParam& load_state,
                        uint64 upload_position,
                        uint64 upload_size);

  bool SuddenTerminationAllowed() const;
  void set_sudden_termination_allowed(bool enabled) {
    sudden_termination_allowed_ = enabled;
  }

  // RenderWidgetHost public overrides.
  virtual void Init() override;
  virtual void Shutdown() override;
  virtual void WasHidden() override;
  virtual void WasShown(const ui::LatencyInfo& latency_info) override;
  virtual bool IsRenderView() const override;
  virtual bool OnMessageReceived(const IPC::Message& msg) override;
  virtual void GotFocus() override;
  virtual void LostCapture() override;
  virtual void LostMouseLock() override;
  virtual void SetIsLoading(bool is_loading) override;
  virtual void ForwardMouseEvent(
      const blink::WebMouseEvent& mouse_event) override;
  virtual void OnPointerEventActivate() override;
  virtual void ForwardKeyboardEvent(
      const NativeWebKeyboardEvent& key_event) override;
  virtual gfx::Rect GetRootWindowResizerRect() const override;

  // Creates a new RenderView with the given route id.
  void CreateNewWindow(
      int route_id,
      int main_frame_route_id,
      const ViewHostMsg_CreateWindow_Params& params,
      SessionStorageNamespace* session_storage_namespace);

  // Creates a new RenderWidget with the given route id.  |popup_type| indicates
  // if this widget is a popup and what kind of popup it is (select, autofill).
  void CreateNewWidget(int route_id, blink::WebPopupType popup_type);

  // Creates a full screen RenderWidget.
  void CreateNewFullscreenWidget(int route_id);

#if defined(ENABLE_BROWSER_CDMS)
  MediaWebContentsObserver* media_web_contents_observer() {
    return media_web_contents_observer_.get();
  }
#endif

  int main_frame_routing_id() const {
    return main_frame_routing_id_;
  }

  void OnTextSurroundingSelectionResponse(const base::string16& content,
                                          size_t start_offset,
                                          size_t end_offset);

  // Update the FrameTree to use this RenderViewHost's main frame
  // RenderFrameHost. Called when the RenderViewHost is committed.
  //
  // TODO(ajwong): Remove once RenderViewHost no longer owns the main frame
  // RenderFrameHost.
  void AttachToFrameTree();

  // Increases the refcounting on this RVH. This is done by the FrameTree on
  // creation of a RenderFrameHost.
  void increment_ref_count() { ++frames_ref_count_; }

  // Decreases the refcounting on this RVH. This is done by the FrameTree on
  // destruction of a RenderFrameHost.
  void decrement_ref_count() { --frames_ref_count_; }

  // Returns the refcount on this RVH, that is the number of RenderFrameHosts
  // currently using it.
  int ref_count() { return frames_ref_count_; }

  // NOTE: Do not add functions that just send an IPC message that are called in
  // one or two places. Have the caller send the IPC message directly (unless
  // the caller places are in different platforms, in which case it's better
  // to keep them consistent).

 protected:
  // RenderWidgetHost protected overrides.
  virtual void OnUserGesture() override;
  virtual void NotifyRendererUnresponsive() override;
  virtual void NotifyRendererResponsive() override;
  virtual void OnRenderAutoResized(const gfx::Size& size) override;
  virtual void RequestToLockMouse(bool user_gesture,
                                  bool last_unlocked_by_target) override;
  virtual bool IsFullscreen() const override;
  virtual void OnFocus() override;
  virtual void OnBlur() override;

  // IPC message handlers.
  void OnShowView(int route_id,
                  WindowOpenDisposition disposition,
                  const gfx::Rect& initial_pos,
                  bool user_gesture);
  void OnShowWidget(int route_id, const gfx::Rect& initial_pos);
  void OnShowFullscreenWidget(int route_id);
  void OnRunModal(int opener_id, IPC::Message* reply_msg);
  void OnRenderViewReady();
  void OnRenderProcessGone(int status, int error_code);
  void OnUpdateState(int32 page_id, const PageState& state);
  void OnUpdateTargetURL(const GURL& url);
  void OnClose();
  void OnRequestMove(const gfx::Rect& pos);
  void OnDocumentAvailableInMainFrame(bool uses_temporary_zoom_level);
  void OnToggleFullscreen(bool enter_fullscreen);
  void OnDidContentsPreferredSizeChange(const gfx::Size& new_size);
  void OnPasteFromSelectionClipboard();
  void OnRouteCloseEvent();
  void OnRouteMessageEvent(const ViewMsg_PostMessage_Params& params);
  void OnStartDragging(const DropData& drop_data,
                       blink::WebDragOperationsMask operations_allowed,
                       const SkBitmap& bitmap,
                       const gfx::Vector2d& bitmap_offset_in_dip,
                       const DragEventSourceInfo& event_info);
  void OnUpdateDragCursor(blink::WebDragOperation drag_operation);
  void OnTargetDropACK();
  void OnTakeFocus(bool reverse);
  void OnFocusedNodeChanged(bool is_editable_node);
  void OnClosePageACK();
  void OnDidZoomURL(double zoom_level, const GURL& url);
  void OnRunFileChooser(const FileChooserParams& params);
  void OnFocusedNodeTouched(bool editable);

 private:
  // TODO(nasko): Temporarily friend RenderFrameHostImpl, so we don't duplicate
  // utility functions and state needed in both classes, while we move frame
  // specific code away from this class.
  friend class RenderFrameHostImpl;
  friend class TestRenderViewHost;
  FRIEND_TEST_ALL_PREFIXES(RenderViewHostTest, BasicRenderFrameHost);
  FRIEND_TEST_ALL_PREFIXES(RenderViewHostTest, RoutingIdSane);

  // TODO(creis): Move to a private namespace on RenderFrameHostImpl.
  // Delay to wait on closing the WebContents for a beforeunload/unload handler
  // to fire.
  static const int kUnloadTimeoutMS;

  bool CanAccessFilesOfPageState(const PageState& state) const;

  // The number of RenderFrameHosts which have a reference to this RVH.
  int frames_ref_count_;

  // Our delegate, which wants to know about changes in the RenderView.
  RenderViewHostDelegate* delegate_;

  // The SiteInstance associated with this RenderViewHost.  All pages drawn
  // in this RenderViewHost are part of this SiteInstance.  Cannot change
  // over time.
  scoped_refptr<SiteInstanceImpl> instance_;

  // true if we are currently waiting for a response for drag context
  // information.
  bool waiting_for_drag_context_response_;

  // A bitwise OR of bindings types that have been enabled for this RenderView.
  // See BindingsPolicy for details.
  int enabled_bindings_;

  // The most recent page ID we've heard from the renderer process.  This is
  // used as context when other session history related IPCs arrive.
  // TODO(creis): Allocate this in WebContents/NavigationController instead.
  int32 page_id_;

  // Tracks whether this RenderViewHost is in an active state.  False if the
  // main frame is pending swap out, pending deletion, or swapped out, because
  // it is not visible to the user in any of these cases.
  bool is_active_;

  // Tracks whether the main frame RenderFrameHost is swapped out.  Unlike
  // is_active_, this is false when the frame is pending swap out or deletion.
  // TODO(creis): Remove this when we no longer use swappedout://.
  // See http://crbug.com/357747.
  bool is_swapped_out_;

  // Routing ID for the main frame's RenderFrameHost.
  int main_frame_routing_id_;

  // If we were asked to RunModal, then this will hold the reply_msg that we
  // must return to the renderer to unblock it.
  IPC::Message* run_modal_reply_msg_;
  // This will hold the routing id of the RenderView that opened us.
  int run_modal_opener_id_;

  // Set to true when waiting for a ViewHostMsg_ClosePageACK.
  // TODO(creis): Move to RenderFrameHost and RenderWidgetHost.
  // See http://crbug.com/418265.
  bool is_waiting_for_close_ack_;

  // True if the render view can be shut down suddenly.
  bool sudden_termination_allowed_;

  // The termination status of the last render view that terminated.
  base::TerminationStatus render_view_termination_status_;

  // Set to true if we requested the on screen keyboard to be displayed.
  bool virtual_keyboard_requested_;

#if defined(ENABLE_BROWSER_CDMS)
  // Manages all the media player and CDM managers and forwards IPCs to them.
  scoped_ptr<MediaWebContentsObserver> media_web_contents_observer_;
#endif

  // True if the current focused element is editable.
  bool is_focused_element_editable_;

  // This is updated every time UpdateWebkitPreferences is called. That method
  // is in turn called when any of the settings change that the WebPreferences
  // values depend on.
  scoped_ptr<WebPreferences> web_preferences_;

  bool updating_web_preferences_;

  base::WeakPtrFactory<RenderViewHostImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewHostImpl);
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)
#endif

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_VIEW_HOST_IMPL_H_
