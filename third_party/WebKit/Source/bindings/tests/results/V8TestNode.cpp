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
#include "V8TestNode.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestNode* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestNode::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestNode* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestNode::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestNode::domTemplate, V8TestNode::derefObject, 0, V8TestNode::toEventTarget, 0, V8TestNode::installPerContextEnabledMethods, &V8Node::wrapperTypeInfo, WrapperTypeObjectPrototype };

namespace TestNodeV8Internal {

template <typename T> void V8_USE(T) { }

static void hrefAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestNode* imp = V8TestNode::toNative(info.Holder());
    v8SetReturnValueString(info, imp->href(), info.GetIsolate());
}

static void hrefAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestNodeV8Internal::hrefAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void hrefAttributeSetter(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TestNode* imp = V8TestNode::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    imp->setHref(cppValue);
}

static void hrefAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestNodeV8Internal::hrefAttributeSetter(jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void hrefThrowsAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TestNode* imp = V8TestNode::toNative(info.Holder());
    v8SetReturnValueString(info, imp->hrefThrows(), info.GetIsolate());
}

static void hrefThrowsAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestNodeV8Internal::hrefThrowsAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void hrefThrowsAttributeSetter(v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    ExceptionState exceptionState(ExceptionState::SetterContext, "hrefThrows", "TestNode", info.Holder(), info.GetIsolate());
    TestNode* imp = V8TestNode::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, jsValue);
    imp->setHrefThrows(cppValue, exceptionState);
    exceptionState.throwIfNeeded();
}

static void hrefThrowsAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> jsValue, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestNodeV8Internal::hrefThrowsAttributeSetter(jsValue, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    RefPtr<TestNode> impl = TestNode::create();
    v8::Handle<v8::Object> wrapper = info.Holder();

    V8DOMWrapper::associateObjectWithWrapper<V8TestNode>(impl.release(), &V8TestNode::wrapperTypeInfo, wrapper, info.GetIsolate(), WrapperConfiguration::Dependent);
    v8SetReturnValue(info, wrapper);
}

} // namespace TestNodeV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestNodeAttributes[] = {
    {"href", TestNodeV8Internal::hrefAttributeGetterCallback, TestNodeV8Internal::hrefAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"hrefThrows", TestNodeV8Internal::hrefThrowsAttributeGetterCallback, TestNodeV8Internal::hrefThrowsAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

void V8TestNode::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::failedToConstruct("TestNode", "Please use the 'new' operator, this DOM object constructor cannot be called as a function."), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    TestNodeV8Internal::constructor(info);
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestNodeTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestNode", V8Node::domTemplate(isolate, currentWorldType), V8TestNode::internalFieldCount,
        V8TestNodeAttributes, WTF_ARRAY_LENGTH(V8TestNodeAttributes),
        0, 0,
        0, 0,
        isolate, currentWorldType);
    functionTemplate->SetCallHandler(V8TestNode::constructorCallback);
    functionTemplate->SetLength(0);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();

    // Custom toString template
    functionTemplate->Set(v8::String::NewFromUtf8(isolate, "toString", v8::String::kInternalizedString), V8PerIsolateData::current()->toStringTemplate());
    return functionTemplate;
}

v8::Handle<v8::FunctionTemplate> V8TestNode::domTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ =
        ConfigureV8TestNodeTemplate(data->rawDOMTemplate(&wrapperTypeInfo, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

bool V8TestNode::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, currentWorldType);
}

bool V8TestNode::hasInstanceInAnyWorld(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue, WorkerWorld);
}

EventTarget* V8TestNode::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}

v8::Handle<v8::Object> V8TestNode::createWrapper(PassRefPtr<TestNode> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestNode>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestNode>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Dependent);
    return wrapper;
}

void V8TestNode::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestNode* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
