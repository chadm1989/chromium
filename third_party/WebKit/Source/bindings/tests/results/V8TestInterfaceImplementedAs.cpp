/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// This file has been auto-generated by code_generator_v8.pm. DO NOT MODIFY!

#include "config.h"
#include "V8TestInterfaceImplementedAs.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(RealClass* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterfaceImplementedAs::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::RealClass* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterfaceImplementedAs::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceImplementedAs::domTemplate, V8TestInterfaceImplementedAs::derefObject, 0, 0, 0, V8TestInterfaceImplementedAs::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype };

namespace RealClassV8Internal {

template <typename T> void V8_USE(T) { }

static void aAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    v8SetReturnValueString(info, imp->a(), info.GetIsolate());
}

static void aAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    RealClassV8Internal::aAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void aAttributeSetter(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    imp->setA(cppValue);
}

static void aAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    RealClassV8Internal::aAttributeSetter(jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void bAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    v8SetReturnValueFast(info, imp->b(), imp);
}

static void bAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    RealClassV8Internal::bAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void bAttributeSetter(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    V8TRYCATCH_VOID(RealClass*, cppValue, V8TestInterfaceImplementedAs::hasInstance(jsValue, info.GetIsolate(), worldType(info.GetIsolate())) ? V8TestInterfaceImplementedAs::toNative(v8::Handle<v8::Object>::Cast(jsValue)) : 0);
    imp->setB(WTF::getPtr(cppValue));
}

static void bAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    RealClassV8Internal::bAttributeSetter(jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void func1Method(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("func1", "TestInterfaceImplementedAs", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, a, info[0]);
    v8SetReturnValueString(info, imp->func1(a), info.GetIsolate());
}

static void func1MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    RealClassV8Internal::func1Method(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void funcTestInterfaceImplementedAsParamMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("funcTestInterfaceImplementedAsParam", "TestInterfaceImplementedAs", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    V8TRYCATCH_VOID(RealClass*, orange, V8TestInterfaceImplementedAs::hasInstance(info[0], info.GetIsolate(), worldType(info.GetIsolate())) ? V8TestInterfaceImplementedAs::toNative(v8::Handle<v8::Object>::Cast(info[0])) : 0);
    v8SetReturnValueString(info, imp->funcTestInterfaceImplementedAsParam(orange), info.GetIsolate());
}

static void funcTestInterfaceImplementedAsParamMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    RealClassV8Internal::funcTestInterfaceImplementedAsParamMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

} // namespace RealClassV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestInterfaceImplementedAsAttributes[] = {
    {"a", RealClassV8Internal::aAttributeGetterCallback, RealClassV8Internal::aAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"b", RealClassV8Internal::bAttributeGetterCallback, RealClassV8Internal::bAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static const V8DOMConfiguration::MethodConfiguration V8TestInterfaceImplementedAsMethods[] = {
    {"func1", RealClassV8Internal::func1MethodCallback, 0, 1},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestInterfaceImplementedAsTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterfaceImplementedAs", v8::Local<v8::FunctionTemplate>(), V8TestInterfaceImplementedAs::internalFieldCount,
        V8TestInterfaceImplementedAsAttributes, WTF_ARRAY_LENGTH(V8TestInterfaceImplementedAsAttributes),
        0, 0,
        V8TestInterfaceImplementedAsMethods, WTF_ARRAY_LENGTH(V8TestInterfaceImplementedAsMethods),
        isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> prototypeTemplate = functionTemplate->PrototypeTemplate();
    UNUSED_PARAM(instanceTemplate);
    UNUSED_PARAM(prototypeTemplate);

    // Custom Signature 'funcTestInterfaceImplementedAsParam'
    const int funcTestInterfaceImplementedAsParamArgc = 1;
    v8::Handle<v8::FunctionTemplate> funcTestInterfaceImplementedAsParamArgv[funcTestInterfaceImplementedAsParamArgc] = { V8PerIsolateData::from(isolate)->rawDOMTemplate(&V8TestInterfaceImplementedAs::wrapperTypeInfo, currentWorldType) };
    v8::Handle<v8::Signature> funcTestInterfaceImplementedAsParamSignature = v8::Signature::New(isolate, functionTemplate, funcTestInterfaceImplementedAsParamArgc, funcTestInterfaceImplementedAsParamArgv);
    prototypeTemplate->Set(v8::String::NewFromUtf8(isolate, "funcTestInterfaceImplementedAsParam", v8::String::kInternalizedString), v8::FunctionTemplate::New(isolate, RealClassV8Internal::funcTestInterfaceImplementedAsParamMethodCallback, v8Undefined(), funcTestInterfaceImplementedAsParamSignature, 1));

    // Custom toString template
    functionTemplate->Set(v8::String::NewFromUtf8(isolate, "toString", v8::String::kInternalizedString), V8PerIsolateData::current()->toStringTemplate());
    return functionTemplate;
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceImplementedAs::domTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ =
        ConfigureV8TestInterfaceImplementedAsTemplate(data->rawDOMTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

bool V8TestInterfaceImplementedAs::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestInterfaceImplementedAs::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

v8::Handle<v8::Object> V8TestInterfaceImplementedAs::createWrapper(PassRefPtr<RealClass> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfaceImplementedAs>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::wrapperTypeInfo instead of an XXX::wrapperTypeInfo. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == wrapperTypeInfo.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &wrapperTypeInfo, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceImplementedAs>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestInterfaceImplementedAs::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(RealClass* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
