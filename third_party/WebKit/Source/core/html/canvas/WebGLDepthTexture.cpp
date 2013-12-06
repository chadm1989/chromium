/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "core/html/canvas/WebGLDepthTexture.h"

#include "platform/graphics/Extensions3D.h"

namespace WebCore {

WebGLDepthTexture::WebGLDepthTexture(WebGLRenderingContext* context)
    : WebGLExtension(context)
{
    ScriptWrappable::init(this);
    context->graphicsContext3D()->extensions()->ensureEnabled("GL_CHROMIUM_depth_texture");
}

WebGLDepthTexture::~WebGLDepthTexture()
{
}

WebGLExtension::ExtensionName WebGLDepthTexture::name() const
{
    return WebGLDepthTextureName;
}

PassRefPtr<WebGLDepthTexture> WebGLDepthTexture::create(WebGLRenderingContext* context)
{
    return adoptRef(new WebGLDepthTexture(context));
}

bool WebGLDepthTexture::supported(WebGLRenderingContext* context)
{
    Extensions3D* extensions = context->graphicsContext3D()->extensions();
    // Emulating the UNSIGNED_INT_24_8_WEBGL texture internal format in terms
    // of two separate texture objects is too difficult, so disable depth
    // textures unless a packed depth/stencil format is available.
    if (!extensions->supports("GL_OES_packed_depth_stencil"))
        return false;
    return extensions->supports("GL_CHROMIUM_depth_texture")
        || extensions->supports("GL_OES_depth_texture")
        || extensions->supports("GL_ARB_depth_texture");
}

const char* WebGLDepthTexture::extensionName()
{
    return "WEBGL_depth_texture";
}

} // namespace WebCore
