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
#if ENABLE(CONDITION)
#include "V8TestInterfacePython.h"

#include "RuntimeEnabledFeatures.h"
#include "V8ReferencedType.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestInterfacePythonImplementation* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterfacePython::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestInterfacePythonImplementation* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterfacePython::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfacePython::domTemplate, V8TestInterfacePython::derefObject, V8TestInterfacePython::toActiveDOMObject, 0, V8TestInterfacePython::visitDOMWrapper, V8TestInterfacePython::installPerContextEnabledMethods, &V8TestInterfaceEmpty::wrapperTypeInfo, WrapperTypeObjectPrototype };

namespace TestInterfacePythonImplementationV8Internal {

template <typename T> void V8_USE(T) { }

static void perWorldBindingsStringAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestInterfacePythonImplementation* imp = V8TestInterfacePython::toNative(info.Holder());
    v8SetReturnValueString(info, imp->perWorldBindingsStringAttribute(), info.GetIsolate());
}

static void perWorldBindingsStringAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsStringAttributeAttributeSetter(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TestInterfacePythonImplementation* imp = V8TestInterfacePython::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    imp->setPerWorldBindingsStringAttribute(cppValue);
}

static void perWorldBindingsStringAttributeAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeSetter(jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsStringAttributeAttributeGetterForMainWorld(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestInterfacePythonImplementation* imp = V8TestInterfacePython::toNative(info.Holder());
    v8SetReturnValueString(info, imp->perWorldBindingsStringAttribute(), info.GetIsolate());
}

static void perWorldBindingsStringAttributeAttributeGetterCallbackForMainWorld(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeGetterForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void perWorldBindingsStringAttributeAttributeSetterForMainWorld(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TestInterfacePythonImplementation* imp = V8TestInterfacePython::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    imp->setPerWorldBindingsStringAttribute(cppValue);
}

static void perWorldBindingsStringAttributeAttributeSetterCallbackForMainWorld(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeSetterForMainWorld(jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void voidMethodMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfacePythonImplementation* imp = V8TestInterfacePython::toNative(info.Holder());
    imp->voidMethod();
}

static void voidMethodMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfacePythonImplementationV8Internal::voidMethodMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void voidMethodMethodForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfacePythonImplementation* imp = V8TestInterfacePython::toNative(info.Holder());
    imp->voidMethod();
}

static void voidMethodMethodCallbackForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfacePythonImplementationV8Internal::voidMethodMethodForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

} // namespace TestInterfacePythonImplementationV8Internal

void V8TestInterfacePython::visitDOMWrapper(void* object, const v8::Persistent<v8::Object>& wrapper, v8::Isolate* isolate)
{
    TestInterfacePythonImplementation* impl = fromInternalPointer(object);
    v8::Local<v8::Object> creationContext = v8::Local<v8::Object>::New(isolate, wrapper);
    V8WrapperInstantiationScope scope(creationContext, isolate);
    ReferencedType* referencedName = impl->referencedName();
    if (referencedName) {
        if (!DOMDataStore::containsWrapper<V8ReferencedType>(referencedName, isolate))
            wrap(referencedName, creationContext, isolate);
        DOMDataStore::setWrapperReference<V8ReferencedType>(wrapper, referencedName, isolate);
    }
    setObjectGroup(object, wrapper, isolate);
}

static const V8DOMConfiguration::AttributeConfiguration V8TestInterfacePythonAttributes[] = {
    {"perWorldBindingsStringAttribute", TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeGetterCallback, TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeSetterCallback, TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeGetterCallbackForMainWorld, TestInterfacePythonImplementationV8Internal::perWorldBindingsStringAttributeAttributeSetterCallbackForMainWorld, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static const V8DOMConfiguration::MethodConfiguration V8TestInterfacePythonMethods[] = {
    {"voidMethod", TestInterfacePythonImplementationV8Internal::voidMethodMethodCallback, TestInterfacePythonImplementationV8Internal::voidMethodMethodCallbackForMainWorld, 0},
};

static void configureV8TestInterfacePythonTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    if (!RuntimeEnabledFeatures::featureNameEnabled())
        defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "", V8TestInterfaceEmpty::domTemplate(isolate, currentWorldType), V8TestInterfacePython::internalFieldCount, 0, 0, 0, 0, 0, 0, isolate, currentWorldType);
    else
        defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterfacePython", V8TestInterfaceEmpty::domTemplate(isolate, currentWorldType), V8TestInterfacePython::internalFieldCount,
            V8TestInterfacePythonAttributes, WTF_ARRAY_LENGTH(V8TestInterfacePythonAttributes),
            0, 0,
            V8TestInterfacePythonMethods, WTF_ARRAY_LENGTH(V8TestInterfacePythonMethods),
            isolate, currentWorldType);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();
    functionTemplate->InstanceTemplate()->SetCallAsFunctionHandler(V8TestInterfacePython::legacyCallCustom);

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::current()->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestInterfacePython::domTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestInterfacePythonTemplate(templ, isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

bool V8TestInterfacePython::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestInterfacePython::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

ActiveDOMObject* V8TestInterfacePython::toActiveDOMObject(v8::Handle<v8::Object> wrapper)
{
    return toNative(wrapper);
}

void V8TestInterfacePython::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestInterfacePythonImplementation* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
#endif // ENABLE(CONDITION)
