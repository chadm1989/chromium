// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_PAINT_CONTEXT_H_
#define UI_COMPOSITOR_PAINT_CONTEXT_H_

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "ui/compositor/compositor_export.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {
class DisplayItemList;
}

namespace gfx {
class Canvas;
}

class SkPictureRecorder;

namespace ui {
class ClipTransformRecorder;
class CompositingRecorder;
class PaintRecorder;

class COMPOSITOR_EXPORT PaintContext {
 public:
  // Construct a PaintContext that may only re-paint the area in the
  // |invalidation|.
  PaintContext(cc::DisplayItemList* list,
               float device_scale_factor,
               const gfx::Rect& bounds,
               const gfx::Rect& invalidation);

  // Clone a PaintContext with an additional |offset|.
  PaintContext(const PaintContext& other, const gfx::Vector2d& offset);

  // Clone a PaintContext that has no consideration for invalidation.
  enum CloneWithoutInvalidation {
    CLONE_WITHOUT_INVALIDATION,
  };
  PaintContext(const PaintContext& other, CloneWithoutInvalidation c);

  ~PaintContext();

  // When true, IsRectInvalid() can be called, otherwise its result would be
  // invalid.
  bool CanCheckInvalid() const { return !invalidation_.IsEmpty(); }

  // When true, the |bounds| touches an invalidated area, so should be
  // re-painted. When false, re-painting can be skipped. Bounds should be in
  // the local space with offsets up to the painting root in the PaintContext.
  bool IsRectInvalid(const gfx::Rect& bounds) const {
    DCHECK(CanCheckInvalid());
    return invalidation_.Intersects(bounds + offset_);
  }

#if DCHECK_IS_ON()
  void Visited(void* visited) const {
    if (!root_visited_)
      root_visited_ = visited;
  }
  void* RootVisited() const { return root_visited_; }
  const gfx::Vector2d& PaintOffset() const { return offset_; }
#endif

  const gfx::Rect& InvalidationForTesting() const { return invalidation_; }

 private:
  // The Recorder classes need access to the internal canvas and friends, but we
  // don't want to expose them on this class so that people must go through the
  // recorders to access them.
  friend class ClipTransformRecorder;
  friend class CompositingRecorder;
  friend class PaintRecorder;
  // The Cache class also needs to access the DisplayItemList to append its
  // cache contents.
  friend class PaintCache;

  cc::DisplayItemList* list_;
  scoped_ptr<SkPictureRecorder> owned_recorder_;
  // A pointer to the |owned_recorder_| in this PaintContext, or in another one
  // which this was copied from. We expect a copied-from PaintContext to outlive
  // copies made from it.
  SkPictureRecorder* recorder_;
  // The device scale of the frame being painted. Used to determine which bitmap
  // resources to use in the frame.
  float device_scale_factor_;
  // The bounds of the area being painted. Not all of it may be invalidated from
  // the previous frame.
  gfx::Rect bounds_;
  // Invalidation in the space of the paint root (ie the space of the layer
  // backing the paint taking place).
  gfx::Rect invalidation_;
  // Offset from the PaintContext to the space of the paint root and the
  // |invalidation_|.
  gfx::Vector2d offset_;

#if DCHECK_IS_ON()
  // Used to verify that the |invalidation_| is only used to compare against
  // rects in the same space.
  mutable void* root_visited_;
  // Used to verify that paint recorders are not nested. True while a paint
  // recorder is active.
  mutable bool inside_paint_recorder_;
#endif

  DISALLOW_COPY_AND_ASSIGN(PaintContext);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_PAINT_CONTEXT_H_
