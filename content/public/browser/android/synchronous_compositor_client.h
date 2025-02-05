// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_CLIENT_H_
#define CONTENT_PUBLIC_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_CLIENT_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace content {

class SynchronousCompositor;

class SynchronousCompositorClient {
 public:
  // Indication to the client that |compositor| is now initialized on the
  // compositor thread, and open for business. |process_id| and |routing_id|
  // belong to the RVH that owns the compositor.
  virtual void DidInitializeCompositor(SynchronousCompositor* compositor,
                                       int process_id,
                                       int routing_id) = 0;

  // Indication to the client that |compositor| is going out of scope, and
  // must not be accessed within or after this call.
  // NOTE if the client goes away before the compositor it must call
  // SynchronousCompositor::SetClient(nullptr) to release the back pointer.
  virtual void DidDestroyCompositor(SynchronousCompositor* compositor,
                                    int process_id,
                                    int routing_id) = 0;

  virtual void UpdateRootLayerState(SynchronousCompositor* compositor,
                                    const gfx::Vector2dF& total_scroll_offset,
                                    const gfx::Vector2dF& max_scroll_offset,
                                    const gfx::SizeF& scrollable_size,
                                    float page_scale_factor,
                                    float min_page_scale_factor,
                                    float max_page_scale_factor) = 0;

  virtual void DidOverscroll(SynchronousCompositor* compositor,
                             const gfx::Vector2dF& accumulated_overscroll,
                             const gfx::Vector2dF& latest_overscroll_delta,
                             const gfx::Vector2dF& current_fling_velocity) = 0;

  virtual void PostInvalidate(SynchronousCompositor* compositor) = 0;

  virtual void DidUpdateContent(SynchronousCompositor* compositor) = 0;

 protected:
  SynchronousCompositorClient() {}
  virtual ~SynchronousCompositorClient() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SynchronousCompositorClient);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_ANDROID_SYNCHRONOUS_COMPOSITOR_CLIENT_H_
