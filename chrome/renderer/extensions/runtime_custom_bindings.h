// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_EXTENSIONS_RUNTIME_CUSTOM_BINDINGS_H_
#define CHROME_RENDERER_EXTENSIONS_RUNTIME_CUSTOM_BINDINGS_H_

#include "base/compiler_specific.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace extensions {

// The native component of custom bindings for the chrome.runtime API.
class RuntimeCustomBindings : public ObjectBackedNativeHandler {
 public:
  explicit RuntimeCustomBindings(ScriptContext* context);

  virtual ~RuntimeCustomBindings();

  // Creates a new messaging channel to the given extension.
  void OpenChannelToExtension(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Creates a new messaging channels for the specified native application.
  void OpenChannelToNativeApp(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  void GetManifest(const v8::FunctionCallbackInfo<v8::Value>& args);
  void GetExtensionViews(const v8::FunctionCallbackInfo<v8::Value>& args);
};

}  // namespace extensions

#endif  // CHROME_RENDERER_EXTENSIONS_RUNTIME_CUSTOM_BINDINGS_H_
