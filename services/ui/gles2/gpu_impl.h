// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_GLES2_GPU_IMPL_H_
#define SERVICES_UI_GLES2_GPU_IMPL_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/ui/gles2/gpu_state.h"
#include "services/ui/public/interfaces/command_buffer.mojom.h"
#include "services/ui/public/interfaces/gpu.mojom.h"
#include "ui/gfx/geometry/mojo/geometry.mojom.h"

namespace ui {

class GpuImpl : public mojom::Gpu {
 public:
  GpuImpl(mojo::InterfaceRequest<mojom::Gpu> request,
          const scoped_refptr<GpuState>& state);
  ~GpuImpl() override;

 private:
  void CreateOffscreenGLES2Context(mojo::InterfaceRequest<mojom::CommandBuffer>
                                       command_buffer_request) override;
  void GetGpuInfo(const GetGpuInfoCallback& callback) override;

  mojo::StrongBinding<Gpu> binding_;
  scoped_refptr<GpuState> state_;

  DISALLOW_COPY_AND_ASSIGN(GpuImpl);
};

}  // namespace ui

#endif  // SERVICES_UI_GLES2_GPU_IMPL_H_
