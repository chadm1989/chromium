// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_STUB_API_H_
#define UI_GL_GL_STUB_API_H_

#include "ui/gl/gl_export.h"
#include "ui/gl/gl_stub_api_base.h"

namespace gl {

class GL_EXPORT GLStubApi: public GLStubApiBase {
 public:
  GLenum glCheckFramebufferStatusEXTFn(GLenum target) override;
  GLuint glCreateProgramFn(void) override;
  GLuint glCreateShaderFn(GLenum type) override;
  GLsync glFenceSyncFn(GLenum condition, GLbitfield flags) override;
  void glGenBuffersARBFn(GLsizei n, GLuint* buffers) override;
  void glGenerateMipmapEXTFn(GLenum target) override;
  void glGenFencesAPPLEFn(GLsizei n, GLuint* fences) override;
  void glGenFencesNVFn(GLsizei n, GLuint* fences) override;
  void glGenFramebuffersEXTFn(GLsizei n, GLuint* framebuffers) override;
  GLuint glGenPathsNVFn(GLsizei range) override;
  void glGenQueriesFn(GLsizei n, GLuint* ids) override;
  void glGenRenderbuffersEXTFn(GLsizei n, GLuint* renderbuffers) override;
  void glGenSamplersFn(GLsizei n, GLuint* samplers) override;
  void glGenTexturesFn(GLsizei n, GLuint* textures) override;
  void glGenTransformFeedbacksFn(GLsizei n, GLuint* ids) override;
  void glGenVertexArraysOESFn(GLsizei n, GLuint* arrays) override;
  void glGetIntegervFn(GLenum pname, GLint* params) override;
  void glGetProgramivFn(GLuint program, GLenum pname, GLint* params) override;
  void glGetShaderivFn(GLuint shader, GLenum pname, GLint* params) override;
  const GLubyte* glGetStringFn(GLenum name) override;
  const GLubyte* glGetStringiFn(GLenum name, GLuint index) override;
  GLboolean glIsBufferFn(GLuint buffer) override;
  GLboolean glIsEnabledFn(GLenum cap) override;
  GLboolean glIsFenceAPPLEFn(GLuint fence) override;
  GLboolean glIsFenceNVFn(GLuint fence) override;
  GLboolean glIsFramebufferEXTFn(GLuint framebuffer) override;
  GLboolean glIsPathNVFn(GLuint path) override;
  GLboolean glIsProgramFn(GLuint program) override;
  GLboolean glIsQueryFn(GLuint query) override;
  GLboolean glIsRenderbufferEXTFn(GLuint renderbuffer) override;
  GLboolean glIsSamplerFn(GLuint sampler) override;
  GLboolean glIsShaderFn(GLuint shader) override;
  GLboolean glIsSyncFn(GLsync sync) override;
  GLboolean glIsTextureFn(GLuint texture) override;
  GLboolean glIsTransformFeedbackFn(GLuint id) override;
  GLboolean glIsVertexArrayOESFn(GLuint array) override;
  GLboolean glTestFenceAPPLEFn(GLuint fence) override;
  GLboolean glTestFenceNVFn(GLuint fence) override;
  GLboolean glUnmapBufferFn(GLenum target) override;
  GLenum glWaitSyncFn(GLsync sync,
                      GLbitfield flags,
                      GLuint64 timeout) override;
};

}  // namespace gl

#endif  // UI_GL_GL_STUB_API_H_
