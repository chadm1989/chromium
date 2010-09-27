/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebGLObject_h
#define WebGLObject_h

#include "GraphicsContext3D.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class WebGLRenderingContext;

class WebGLObject : public RefCounted<WebGLObject> {
public:
    virtual ~WebGLObject();

    Platform3DObject object() const { return m_object; }
    void deleteObject();

    void detachContext()
    {
        deleteObject();
        m_context = 0;
    }

    WebGLRenderingContext* context() const { return m_context; }

    virtual bool isBuffer() const { return false; }
    virtual bool isFramebuffer() const { return false; }
    virtual bool isProgram() const { return false; }
    virtual bool isRenderbuffer() const { return false; }
    virtual bool isShader() const { return false; }
    virtual bool isTexture() const { return false; }

    void onAttached() { ++m_attachmentCount; }
    void onDetached()
    {
        if (m_attachmentCount)
            --m_attachmentCount;
        if (!m_attachmentCount && m_deleted)
            deleteObject();
    }
    unsigned getAttachmentCount() { return m_attachmentCount; }

protected:
    WebGLObject(WebGLRenderingContext*);

    // setObject should be only called once right after creating a WebGLObject.
    void setObject(Platform3DObject);

    // deleteObjectImpl() may be called multiple times for the same object;
    // isDeleted() needs to be tested in implementations when deciding whether
    // to delete the OpenGL resource.
    virtual void deleteObjectImpl(Platform3DObject) = 0;
    bool isDeleted() { return m_deleted; }
    void setDeteled() { m_deleted = true; }

private:
    Platform3DObject m_object;
    WebGLRenderingContext* m_context;
    unsigned m_attachmentCount;
    bool m_deleted;
};

} // namespace WebCore

#endif // WebGLObject_h
