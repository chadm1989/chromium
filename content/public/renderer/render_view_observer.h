// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_RENDER_VIEW_OBSERVER_H_
#define CONTENT_PUBLIC_RENDERER_RENDER_VIEW_OBSERVER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_sender.h"

class GURL;

namespace blink {
class WebFrame;
class WebGestureEvent;
class WebLocalFrame;
class WebNode;
struct WebURLError;
}

namespace content {

class RenderView;
class RenderViewImpl;

// Base class for objects that want to filter incoming IPCs, and also get
// notified of changes to the frame.
class CONTENT_EXPORT RenderViewObserver : public IPC::Listener,
                                          public IPC::Sender {
 public:
  // By default, observers will be deleted when the RenderView goes away.  If
  // they want to outlive it, they can override this function.
  virtual void OnDestruct();

  // These match the WebKit API notifications
  virtual void DidStartLoading() {}
  virtual void DidStopLoading() {}
  virtual void DidFinishDocumentLoad(blink::WebLocalFrame* frame) {}
  virtual void DidFailLoad(blink::WebLocalFrame* frame,
                           const blink::WebURLError& error) {}
  virtual void DidFinishLoad(blink::WebLocalFrame* frame) {}
  virtual void DidStartProvisionalLoad(blink::WebLocalFrame* frame) {}
  virtual void DidFailProvisionalLoad(blink::WebLocalFrame* frame,
                                      const blink::WebURLError& error) {}
  virtual void DidCommitProvisionalLoad(blink::WebLocalFrame* frame,
                                        bool is_new_navigation) {}
  virtual void DidCreateNewDocument(blink::WebLocalFrame* frame) {}
  virtual void DidClearWindowObject(blink::WebLocalFrame* frame) {}
  virtual void DidCreateDocumentElement(blink::WebLocalFrame* frame) {}
  virtual void FrameCreated(blink::WebLocalFrame* parent,
                            blink::WebFrame* frame) {}
  virtual void FrameDetached(blink::WebFrame* frame) {}
  virtual void FrameWillClose(blink::WebFrame* frame) {}
  virtual void PrintPage(blink::WebLocalFrame* frame, bool user_initiated) {}
  virtual void FocusedNodeChanged(const blink::WebNode& node) {}
  virtual void DraggableRegionsChanged(blink::WebFrame* frame) {}
  virtual void DidCommitCompositorFrame() {}
  virtual void DidUpdateLayout() {}

  // These match the RenderView methods.
  virtual void DidHandleGestureEvent(const blink::WebGestureEvent& event) {}

  virtual void OnMouseDown(const blink::WebNode& mouse_down_node) {}

  // These match incoming IPCs.
  virtual void Navigate(const GURL& url) {}
  virtual void ClosePage() {}

  // This indicates that animations to scroll the focused element into view (if
  // any) have completed. May be called more than once for a single focus. Can
  // be called from browser, renderer, or compositor.
  virtual void FocusChangeComplete() {}

  virtual void OnStop() {}
  virtual void OnZoomLevelChanged() {}

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

  // IPC::Sender implementation.
  bool Send(IPC::Message* message) override;

  RenderView* render_view() const;
  int routing_id() const { return routing_id_; }

 protected:
  explicit RenderViewObserver(RenderView* render_view);
  ~RenderViewObserver() override;

  // Sets |render_view_| to track.
  // Removes itself of previous (if any) |render_view_| observer list and adds
  // to the new |render_view|. Since it assumes that observer outlives
  // render_view, OnDestruct should be overridden.
  void Observe(RenderView* render_view);

 private:
  friend class RenderViewImpl;

  // This is called by the RenderView when it's going away so that this object
  // can null out its pointer.
  void RenderViewGone();

  RenderView* render_view_;
  // The routing ID of the associated RenderView.
  int routing_id_;

  DISALLOW_COPY_AND_ASSIGN(RenderViewObserver);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_RENDER_VIEW_OBSERVER_H_
