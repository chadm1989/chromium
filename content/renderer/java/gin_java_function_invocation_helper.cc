// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/java/gin_java_function_invocation_helper.h"

#include "content/common/android/gin_java_bridge_errors.h"
#include "content/common/android/gin_java_bridge_value.h"
#include "content/public/child/v8_value_converter.h"
#include "content/renderer/java/gin_java_bridge_object.h"
#include "content/renderer/java/gin_java_bridge_value_converter.h"

namespace content {

namespace {

const char kMethodInvocationAsConstructorDisallowed[] =
    "Java bridge method can't be invoked as a constructor";
const char kMethodInvocationOnNonInjectedObjectDisallowed[] =
    "Java bridge method can't be invoked on a non-injected object";
const char kMethodInvocationErrorMessage[] =
    "Java bridge method invocation error";

}  // namespace

GinJavaFunctionInvocationHelper::GinJavaFunctionInvocationHelper(
    const std::string& method_name,
    const base::WeakPtr<GinJavaBridgeDispatcher>& dispatcher)
    : method_name_(method_name),
      dispatcher_(dispatcher),
      converter_(new GinJavaBridgeValueConverter()) {
}

GinJavaFunctionInvocationHelper::~GinJavaFunctionInvocationHelper() {
}

v8::Local<v8::Value> GinJavaFunctionInvocationHelper::Invoke(
    gin::Arguments* args) {
  if (!dispatcher_) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), kMethodInvocationErrorMessage)));
    return v8::Undefined(args->isolate());
  }

  if (args->IsConstructCall()) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), kMethodInvocationAsConstructorDisallowed)));
    return v8::Undefined(args->isolate());
  }

  content::GinJavaBridgeObject* object = NULL;
  if (!args->GetHolder(&object) || !object) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), kMethodInvocationOnNonInjectedObjectDisallowed)));
    return v8::Undefined(args->isolate());
  }

  base::ListValue arguments;
  {
    v8::HandleScope handle_scope(args->isolate());
    v8::Local<v8::Context> context = args->isolate()->GetCurrentContext();
    v8::Local<v8::Value> val;
    while (args->GetNext(&val)) {
      scoped_ptr<base::Value> arg(converter_->FromV8Value(val, context));
      if (arg.get()) {
        arguments.Append(arg.release());
      } else {
        arguments.Append(base::Value::CreateNullValue());
      }
    }
  }

  GinJavaBridgeError error;
  scoped_ptr<base::Value> result = dispatcher_->InvokeJavaMethod(
      object->object_id(), method_name_, arguments, &error);
  if (!result.get()) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), GinJavaBridgeErrorToString(error))));
    return v8::Undefined(args->isolate());
  }
  if (!result->IsType(base::Value::TYPE_BINARY)) {
    return converter_->ToV8Value(result.get(),
                                 args->isolate()->GetCurrentContext());
  }

  scoped_ptr<const GinJavaBridgeValue> gin_value =
      GinJavaBridgeValue::FromValue(result.get());
  if (gin_value->IsType(GinJavaBridgeValue::TYPE_OBJECT_ID)) {
    GinJavaBridgeObject* result = NULL;
    GinJavaBridgeDispatcher::ObjectID object_id;
    if (gin_value->GetAsObjectID(&object_id)) {
      result = dispatcher_->GetObject(object_id);
    }
    if (result) {
      gin::Handle<GinJavaBridgeObject> controller =
          gin::CreateHandle(args->isolate(), result);
      if (controller.IsEmpty())
        return v8::Undefined(args->isolate());
      return controller.ToV8();
    }
  } else if (gin_value->IsType(GinJavaBridgeValue::TYPE_NONFINITE)) {
    float float_value;
    gin_value->GetAsNonFinite(&float_value);
    return v8::Number::New(args->isolate(), float_value);
  }
  return v8::Undefined(args->isolate());
}

}  // namespace content
