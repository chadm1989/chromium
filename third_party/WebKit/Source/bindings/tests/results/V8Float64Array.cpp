/*
    This file is part of the Blink open source project.
    This file has been auto-generated by CodeGeneratorV8.pm. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#include "V8Float64Array.h"

#include "RuntimeEnabledFeatures.h"
#include "V8ArrayBufferView.h"
#include "bindings/v8/ExceptionMessages.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "bindings/v8/custom/V8ArrayBufferCustom.h"
#include "bindings/v8/custom/V8ArrayBufferViewCustom.h"
#include "bindings/v8/custom/V8Float32ArrayCustom.h"
#include "bindings/v8/custom/V8Float64ArrayCustom.h"
#include "bindings/v8/custom/V8Int32ArrayCustom.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/platform/chromium/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(Float64Array* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8Float64Array::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(Float64Array* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8Float64Array::info = { V8Float64Array::GetTemplate, V8Float64Array::derefObject, 0, 0, 0, V8Float64Array::installPerContextPrototypeProperties, &V8ArrayBufferView::info, WrapperTypeObjectPrototype };

namespace Float64ArrayV8Internal {

template <typename T> void V8_USE(T) { }

static void fooMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (UNLIKELY(args.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToExecute("foo", "Float64Array", ExceptionMessages::notEnoughArguments(1, args.Length())), args.GetIsolate());
        return;
    }
    Float64Array* imp = V8Float64Array::toNative(args.Holder());
    V8TRYCATCH_VOID(Float32Array*, array, args[0]->IsFloat32Array() ? V8Float32Array::toNative(v8::Handle<v8::Float32Array>::Cast(args[0])) : 0);
    v8SetReturnValue(args, imp->foo(array), args.Holder());
    return;
}

static void fooMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    Float64ArrayV8Internal::fooMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void setMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    setWebGLArrayHelper<Float64Array, V8Float64Array>(args);
}

static void setMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    Float64ArrayV8Internal::setMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    V8Float64Array::indexedPropertyGetterCustom(index, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void indexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMIndexedProperty");
    V8Float64Array::indexedPropertySetterCustom(index, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

} // namespace Float64ArrayV8Internal

v8::Handle<v8::Object> wrap(Float64Array* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    v8::Handle<v8::Object> wrapper = V8Float64Array::createWrapper(impl, creationContext, isolate);
    if (!wrapper.IsEmpty())
        wrapper->SetIndexedPropertiesToExternalArrayData(impl->baseAddress(), v8::kExternalDoubleArray, impl->length());
    return wrapper;
}

static const V8DOMConfiguration::MethodConfiguration V8Float64ArrayMethods[] = {
    {"set", Float64ArrayV8Internal::setMethodCallback, 0, 0},
};

void V8Float64Array::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!args.IsConstructCall()) {
        throwTypeError(ExceptionMessages::failedToConstruct("Float64Array", "Please use the 'new' operator, this DOM object constructor cannot be called as a function."), args.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        args.GetReturnValue().Set(args.Holder());
        return;
    }

    Float64ArrayV8Internal::constructor(args);
}

static v8::Handle<v8::FunctionTemplate> ConfigureV8Float64ArrayTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(desc, "Float64Array", V8ArrayBufferView::GetTemplate(isolate, currentWorldType), V8Float64Array::internalFieldCount,
        0, 0,
        V8Float64ArrayMethods, WTF_ARRAY_LENGTH(V8Float64ArrayMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    desc->SetCallHandler(V8Float64Array::constructorCallback);
    desc->SetLength(1);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance);
    UNUSED_PARAM(proto);
    desc->InstanceTemplate()->SetIndexedPropertyHandler(Float64ArrayV8Internal::indexedPropertyGetterCallback, Float64ArrayV8Internal::indexedPropertySetterCallback, 0, 0, indexedPropertyEnumerator<Float64Array>);

    // Custom Signature 'foo'
    const int fooArgc = 1;
    v8::Handle<v8::FunctionTemplate> fooArgv[fooArgc] = { v8::Handle<v8::FunctionTemplate>() };
    v8::Handle<v8::Signature> fooSignature = v8::Signature::New(desc, fooArgc, fooArgv);
    proto->Set(v8::String::NewSymbol("foo"), v8::FunctionTemplate::New(Float64ArrayV8Internal::fooMethodCallback, v8Undefined(), fooSignature, 1));

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8Float64Array::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8Float64ArrayTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8Float64Array::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8Float64Array::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

v8::Handle<v8::Object> V8Float64Array::createWrapper(PassRefPtr<Float64Array> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(!DOMDataStore::containsWrapper<V8Float64Array>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::info instead of an XXX::info. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == info.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    impl->buffer()->setDeallocationObserver(V8ArrayBufferDeallocationObserver::instance());
    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8Float64Array>(impl, &info, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8Float64Array::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
