/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

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
#include "V8TestEventTarget.h"

#include "V8Event.h"
#include "V8Node.h"
#include "bindings/v8/BindingState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8Collection.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8EventListenerList.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/ExceptionCode.h"
#include "core/page/Frame.h"
#include "core/page/RuntimeEnabledFeatures.h"
#include <wtf/UnusedParam.h>

#if ENABLE(BINDING_INTEGRITY)
#if defined(OS_WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestEventTarget@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore15TestEventTargetE[]; }
#endif
#endif // ENABLE(BINDING_INTEGRITY)

namespace WebCore {

#if ENABLE(BINDING_INTEGRITY)
// This checks if a DOM object that is about to be wrapped is valid.
// Specifically, it checks that a vtable of the DOM object is equal to
// a vtable of an expected class.
// Due to a dangling pointer, the DOM object you are wrapping might be
// already freed or realloced. If freed, the check will fail because
// a free list pointer should be stored at the head of the DOM object.
// If realloced, the check will fail because the vtable of the DOM object
// differs from the expected vtable (unless the same class of DOM object
// is realloced on the slot).
inline void checkTypeOrDieTrying(TestEventTarget* object)
{
    void* actualVTablePointer = *(reinterpret_cast<void**>(object));
#if defined(OS_WIN)
    void* expectedVTablePointer = reinterpret_cast<void*>(__identifier("??_7TestEventTarget@WebCore@@6B@"));
#else
    void* expectedVTablePointer = &_ZTVN7WebCore15TestEventTargetE[2];
#endif
    if (actualVTablePointer != expectedVTablePointer)
        CRASH();
}
#endif // ENABLE(BINDING_INTEGRITY)

#if defined(OS_WIN)
// In ScriptWrappable, the use of extern function prototypes inside templated static methods has an issue on windows.
// These prototypes do not pick up the surrounding namespace, so drop out of WebCore as a workaround.
} // namespace WebCore
using WebCore::ScriptWrappable;
using WebCore::V8TestEventTarget;
using WebCore::TestEventTarget;
#endif
void initializeScriptWrappableForInterface(TestEventTarget* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestEventTarget::info);
}
#if defined(OS_WIN)
namespace WebCore {
#endif
WrapperTypeInfo V8TestEventTarget::info = { V8TestEventTarget::GetTemplate, V8TestEventTarget::derefObject, 0, V8TestEventTarget::toEventTarget, 0, V8TestEventTarget::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace TestEventTargetV8Internal {

template <typename T> void V8_USE(T) { }

static v8::Handle<v8::Value> itemMethod(const v8::Arguments& args)
{
    if (args.Length() < 1)
        return throwNotEnoughArgumentsError(args.GetIsolate());
    TestEventTarget* imp = V8TestEventTarget::toNative(args.Holder());
    ExceptionCode ec = 0;
    {
    V8TRYCATCH(int, index, toUInt32(args[0]));
    if (UNLIKELY(index < 0)) {
        ec = INDEX_SIZE_ERR;
        goto fail;
    }
    return toV8(imp->item(index), args.Holder(), args.GetIsolate());
    }
    fail:
    return setDOMException(ec, args.GetIsolate());
}

static v8::Handle<v8::Value> itemMethodCallback(const v8::Arguments& args)
{
    return TestEventTargetV8Internal::itemMethod(args);
}

static v8::Handle<v8::Value> namedItemMethod(const v8::Arguments& args)
{
    if (args.Length() < 1)
        return throwNotEnoughArgumentsError(args.GetIsolate());
    TestEventTarget* imp = V8TestEventTarget::toNative(args.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE(V8StringResource<>, name, args[0]);
    return toV8(imp->namedItem(name), args.Holder(), args.GetIsolate());
}

static v8::Handle<v8::Value> namedItemMethodCallback(const v8::Arguments& args)
{
    return TestEventTargetV8Internal::namedItemMethod(args);
}

static v8::Handle<v8::Value> addEventListenerMethod(const v8::Arguments& args)
{
    RefPtr<EventListener> listener = V8EventListenerList::getEventListener(args[1], false, ListenerFindOrCreate);
    if (listener) {
        V8TRYCATCH_FOR_V8STRINGRESOURCE(V8StringResource<WithNullCheck>, stringResource, args[0]);
        V8TestEventTarget::toNative(args.Holder())->addEventListener(stringResource, listener, args[2]->BooleanValue());
        createHiddenDependency(args.Holder(), args[1], V8TestEventTarget::eventListenerCacheIndex, args.GetIsolate());
    }
    return v8Undefined();
}

static v8::Handle<v8::Value> addEventListenerMethodCallback(const v8::Arguments& args)
{
    return TestEventTargetV8Internal::addEventListenerMethod(args);
}

static v8::Handle<v8::Value> removeEventListenerMethod(const v8::Arguments& args)
{
    RefPtr<EventListener> listener = V8EventListenerList::getEventListener(args[1], false, ListenerFindOnly);
    if (listener) {
        V8TRYCATCH_FOR_V8STRINGRESOURCE(V8StringResource<WithNullCheck>, stringResource, args[0]);
        V8TestEventTarget::toNative(args.Holder())->removeEventListener(stringResource, listener.get(), args[2]->BooleanValue());
        removeHiddenDependency(args.Holder(), args[1], V8TestEventTarget::eventListenerCacheIndex, args.GetIsolate());
    }
    return v8Undefined();
}

static v8::Handle<v8::Value> removeEventListenerMethodCallback(const v8::Arguments& args)
{
    return TestEventTargetV8Internal::removeEventListenerMethod(args);
}

static v8::Handle<v8::Value> dispatchEventMethod(const v8::Arguments& args)
{
    if (args.Length() < 1)
        return throwNotEnoughArgumentsError(args.GetIsolate());
    TestEventTarget* imp = V8TestEventTarget::toNative(args.Holder());
    ExceptionCode ec = 0;
    {
    V8TRYCATCH(Event*, evt, V8Event::HasInstance(args[0], args.GetIsolate(), worldType(args.GetIsolate())) ? V8Event::toNative(v8::Handle<v8::Object>::Cast(args[0])) : 0);
    bool result = imp->dispatchEvent(evt, ec);
    if (UNLIKELY(ec))
        goto fail;
    return v8Boolean(result, args.GetIsolate());
    }
    fail:
    return setDOMException(ec, args.GetIsolate());
}

static v8::Handle<v8::Value> dispatchEventMethodCallback(const v8::Arguments& args)
{
    return TestEventTargetV8Internal::dispatchEventMethod(args);
}

} // namespace TestEventTargetV8Internal

static const V8DOMConfiguration::BatchedMethod V8TestEventTargetMethods[] = {
    {"item", TestEventTargetV8Internal::itemMethodCallback, 0, 1},
    {"namedItem", TestEventTargetV8Internal::namedItemMethodCallback, 0, 1},
    {"addEventListener", TestEventTargetV8Internal::addEventListenerMethodCallback, 0, 2},
    {"removeEventListener", TestEventTargetV8Internal::removeEventListenerMethodCallback, 0, 2},
};

v8::Handle<v8::Value> V8TestEventTarget::indexedPropertyGetter(uint32_t index, const v8::AccessorInfo& info)
{
    ASSERT(V8DOMWrapper::maybeDOMWrapper(info.Holder()));
    TestEventTarget* collection = toNative(info.Holder());
    RefPtr<Node> element = collection->item(index);
    if (!element)
        return v8Undefined();
    return toV8(element.release(), info.Holder(), info.GetIsolate());
}
v8::Handle<v8::Value> V8TestEventTarget::namedPropertyGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return v8Undefined();
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return v8Undefined();

    v8::Local<v8::Object> object = info.Holder();
    ASSERT(V8DOMWrapper::maybeDOMWrapper(object));
    ASSERT(toWrapperTypeInfo(object) != &V8Node::info);
    TestEventTarget* collection = toNative(object);

    AtomicString propertyName = toWebCoreAtomicStringWithNullCheck(name);
    RefPtr<Node> element = collection->namedItem(propertyName);

    if (!element)
        return v8Undefined();

    return toV8Fast(element.release(), info, collection);
}

static v8::Persistent<v8::FunctionTemplate> ConfigureV8TestEventTargetTemplate(v8::Persistent<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::configureTemplate(desc, "TestEventTarget", v8::Persistent<v8::FunctionTemplate>(), V8TestEventTarget::internalFieldCount,
        0, 0,
        V8TestEventTargetMethods, WTF_ARRAY_LENGTH(V8TestEventTargetMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature); // In some cases, it will not be used.
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance); // In some cases, it will not be used.
    UNUSED_PARAM(proto); // In some cases, it will not be used.
    desc->InstanceTemplate()->SetIndexedPropertyHandler(V8TestEventTarget::indexedPropertyGetter, 0, 0, 0, nodeCollectionIndexedPropertyEnumerator<TestEventTarget>);
    desc->InstanceTemplate()->SetNamedPropertyHandler(V8TestEventTarget::namedPropertyGetter, 0, 0, 0, 0);
    desc->InstanceTemplate()->MarkAsUndetectable();

    // Custom Signature 'dispatchEvent'
    const int dispatchEventArgc = 1;
    v8::Handle<v8::FunctionTemplate> dispatchEventArgv[dispatchEventArgc] = { V8PerIsolateData::from(isolate)->rawTemplate(&V8Event::info, currentWorldType) };
    v8::Handle<v8::Signature> dispatchEventSignature = v8::Signature::New(desc, dispatchEventArgc, dispatchEventArgv);
    proto->Set(v8::String::NewSymbol("dispatchEvent"), v8::FunctionTemplate::New(TestEventTargetV8Internal::dispatchEventMethodCallback, v8Undefined(), dispatchEventSignature, 1));

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Persistent<v8::FunctionTemplate> V8TestEventTarget::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value;

    v8::HandleScope handleScope;
    v8::Persistent<v8::FunctionTemplate> templ =
        ConfigureV8TestEventTargetTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, templ);
    return templ;
}

bool V8TestEventTarget::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestEventTarget::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

EventTarget* V8TestEventTarget::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}


v8::Handle<v8::Object> V8TestEventTarget::createWrapper(PassRefPtr<TestEventTarget> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(DOMDataStore::getWrapper(impl.get(), isolate).IsEmpty());

#if ENABLE(BINDING_INTEGRITY)
    checkTypeOrDieTrying(impl.get());
#endif

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, impl.get(), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper(impl, &info, wrapper, isolate, hasDependentLifetime ? WrapperConfiguration::Dependent : WrapperConfiguration::Independent);
    return wrapper;
}
void V8TestEventTarget::derefObject(void* object)
{
    static_cast<TestEventTarget*>(object)->deref();
}

} // namespace WebCore
