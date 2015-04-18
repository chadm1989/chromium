// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/drm_console_buffer.h"

#include <sys/mman.h>
#include <xf86drmMode.h>

#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/ozone/platform/drm/gpu/drm_device.h"
#include "ui/ozone/platform/drm/gpu/drm_util.h"
#include "ui/ozone/platform/drm/gpu/scoped_drm_types.h"

namespace ui {

DrmConsoleBuffer::DrmConsoleBuffer(const scoped_refptr<DrmDevice>& drm,
                                   uint32_t framebuffer)
    : drm_(drm),
      handle_(0),
      framebuffer_(framebuffer),
      mmap_base_(NULL),
      mmap_size_(0) {
}

DrmConsoleBuffer::~DrmConsoleBuffer() {
  if (mmap_base_)
    if (munmap(mmap_base_, mmap_size_))
      PLOG(ERROR) << "munmap";

  if (handle_ && !drm_->CloseBufferHandle(handle_))
    PLOG(ERROR) << "DrmConsoleBuffer: CloseBufferHandle: handle " << handle_;
}

bool DrmConsoleBuffer::Initialize() {
  ScopedDrmFramebufferPtr fb(drm_->GetFramebuffer(framebuffer_));

  if (!fb)
    return false;

  handle_ = fb->handle;
  stride_ = fb->pitch;
  SkImageInfo info = SkImageInfo::MakeN32Premul(fb->width, fb->height);

  mmap_size_ = info.getSafeSize(stride_);

  if (!MapDumbBuffer(drm_->get_fd(), fb->handle, mmap_size_, &mmap_base_)) {
    mmap_base_ = NULL;
    return false;
  }

  surface_ =
      skia::AdoptRef(SkSurface::NewRasterDirect(info, mmap_base_, stride_));
  if (!surface_)
    return false;

  return true;
}

}  // namespace ui
