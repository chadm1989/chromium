// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/cast/overlay_manager_cast.h"

#include "chromecast/public/cast_media_shlib.h"
#include "chromecast/public/graphics_types.h"
#include "chromecast/public/video_plane.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/ozone/public/overlay_candidates_ozone.h"

namespace ui {
namespace {

// Translates a gfx::OverlayTransform into a VideoPlane::Transform.
// Could be just a lookup table once we have unit tests for this code
// to ensure it stays in sync with OverlayTransform.
chromecast::media::VideoPlane::Transform ConvertTransform(
    gfx::OverlayTransform transform) {
  switch (transform) {
    case gfx::OVERLAY_TRANSFORM_NONE:
      return chromecast::media::VideoPlane::TRANSFORM_NONE;
    case gfx::OVERLAY_TRANSFORM_FLIP_HORIZONTAL:
      return chromecast::media::VideoPlane::FLIP_HORIZONTAL;
    case gfx::OVERLAY_TRANSFORM_FLIP_VERTICAL:
      return chromecast::media::VideoPlane::FLIP_VERTICAL;
    case gfx::OVERLAY_TRANSFORM_ROTATE_90:
      return chromecast::media::VideoPlane::ROTATE_90;
    case gfx::OVERLAY_TRANSFORM_ROTATE_180:
      return chromecast::media::VideoPlane::ROTATE_180;
    case gfx::OVERLAY_TRANSFORM_ROTATE_270:
      return chromecast::media::VideoPlane::ROTATE_270;
    default:
      NOTREACHED();
      return chromecast::media::VideoPlane::TRANSFORM_NONE;
  }
}

class OverlayCandidatesCast : public OverlayCandidatesOzone {
 public:
  void CheckOverlaySupport(OverlaySurfaceCandidateList* surfaces) override {
    for (auto& candidate : *surfaces) {
      if (candidate.plane_z_order == -1) {
        candidate.overlay_handled = true;

        // Compositor requires all overlay rectangles to have integer coords
        candidate.display_rect = gfx::ToEnclosedRect(candidate.display_rect);

        chromecast::media::VideoPlane* video_plane =
            chromecast::media::CastMediaShlib::GetVideoPlane();

        chromecast::RectF display_rect(
            candidate.display_rect.x(), candidate.display_rect.y(),
            candidate.display_rect.width(), candidate.display_rect.height());
        video_plane->SetGeometry(
            display_rect,
            chromecast::media::VideoPlane::COORDINATE_TYPE_GRAPHICS_PLANE,
            ConvertTransform(candidate.transform));
        return;
      }
    }
  }
};

}  // namespace

OverlayManagerCast::OverlayManagerCast()
    : candidates_(new OverlayCandidatesCast()) {
}

OverlayManagerCast::~OverlayManagerCast() {
}

OverlayCandidatesOzone* OverlayManagerCast::GetOverlayCandidates(
    gfx::AcceleratedWidget w) {
  return candidates_.get();
}

bool OverlayManagerCast::CanShowPrimaryPlaneAsOverlay() {
  return false;
}

}  // namespace ui
