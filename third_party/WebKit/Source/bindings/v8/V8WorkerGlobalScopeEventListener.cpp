/*
 * Copyright (C) 2006, 2007, 2008, 2009 Google Inc. All rights reserved.
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

#include "config.h"

#include "bindings/v8/V8WorkerGlobalScopeEventListener.h"

#include "V8Event.h"
#include "V8EventTarget.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/V8GCController.h"
#include "bindings/v8/V8ScriptRunner.h"
#include "bindings/v8/WorkerScriptController.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/inspector/InspectorTraceEvents.h"
#include "core/workers/WorkerGlobalScope.h"

namespace WebCore {

V8WorkerGlobalScopeEventListener::V8WorkerGlobalScopeEventListener(v8::Local<v8::Object> listener, bool isInline, v8::Isolate* isolate)
    : V8EventListener(listener, isInline, isolate)
{
}

void V8WorkerGlobalScopeEventListener::handleEvent(ExecutionContext* context, Event* event)
{
    if (!context)
        return;

    // The callback function on XMLHttpRequest can clear the event listener and destroys 'this' object. Keep a local reference to it.
    // See issue 889829.
    RefPtr<V8AbstractEventListener> protect(this);

    v8::Isolate* isolate = toIsolate(context);
    v8::HandleScope handleScope(isolate);

    WorkerScriptController* script = toWorkerGlobalScope(context)->script();
    if (!script)
        return;

    v8::Handle<v8::Context> v8Context = script->context();
    if (v8Context.IsEmpty())
        return;

    // Enter the V8 context in which to perform the event handling.
    v8::Context::Scope scope(v8Context);

    // Get the V8 wrapper for the event object.
    v8::Handle<v8::Value> jsEvent = toV8(event, v8::Handle<v8::Object>(), isolate);

    invokeEventHandler(context, event, v8::Local<v8::Value>::New(isolate, jsEvent));
}

static void devtoolsFunctionInfo(v8::Handle<v8::Function> handlerFunction, v8::Isolate* isolate, int& scriptId, String& resourceName, int& lineNumber)
{
    lineNumber = 1;
    v8::Handle<v8::Function> originalFunction = getBoundFunction(handlerFunction);
    v8::ScriptOrigin origin = originalFunction->GetScriptOrigin();
    if (!origin.ResourceName().IsEmpty()) {
        resourceName = NativeValueTraits<String>::nativeValue(origin.ResourceName(), isolate);
        lineNumber = originalFunction->GetScriptLineNumber() + 1;
    }
    if (resourceName.isEmpty())
        resourceName = "undefined";
}

static PassRefPtr<TraceEvent::ConvertableToTraceFormat> devtoolsTraceEventData(ExecutionContext* context, v8::Handle<v8::Function> handlerFunction, v8::Isolate* isolate)
{
    int scriptId = 0;
    String resourceName;
    int lineNumber = 1;
    devtoolsFunctionInfo(handlerFunction, isolate, scriptId, resourceName, lineNumber);
    return InspectorFunctionCallEvent::data(context, scriptId, resourceName, lineNumber);
}

v8::Local<v8::Value> V8WorkerGlobalScopeEventListener::callListenerFunction(ExecutionContext* context, v8::Handle<v8::Value> jsEvent, Event* event)
{
    v8::Local<v8::Function> handlerFunction = getListenerFunction(context);
    v8::Local<v8::Object> receiver = getReceiverObject(context, event);
    if (handlerFunction.IsEmpty() || receiver.IsEmpty())
        return v8::Local<v8::Value>();

    v8::Isolate* isolate = toIsolate(context);
    TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "FunctionCall", "data", devtoolsTraceEventData(context, handlerFunction, isolate));
    // FIXME(361045): remove InspectorInstrumentation calls once DevTools Timeline migrates to tracing.
    InspectorInstrumentationCookie cookie;
    if (InspectorInstrumentation::timelineAgentEnabled(context)) {
        int scriptId = 0;
        String resourceName;
        int lineNumber = 1;
        devtoolsFunctionInfo(handlerFunction, isolate, scriptId, resourceName, lineNumber);
        cookie = InspectorInstrumentation::willCallFunction(context, scriptId, resourceName, lineNumber);
    }

    v8::Handle<v8::Value> parameters[1] = { jsEvent };
    v8::Local<v8::Value> result = V8ScriptRunner::callFunction(handlerFunction, context, receiver, WTF_ARRAY_LENGTH(parameters), parameters, isolate);

    InspectorInstrumentation::didCallFunction(cookie);
    TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"), "UpdateCounters", "data", InspectorUpdateCountersEvent::data());

    return result;
}

v8::Local<v8::Object> V8WorkerGlobalScopeEventListener::getReceiverObject(ExecutionContext* context, Event* event)
{
    v8::Local<v8::Object> listener = getListenerObject(context);

    if (!listener.IsEmpty() && !listener->IsFunction())
        return listener;

    EventTarget* target = event->currentTarget();
    v8::Isolate* isolate = toIsolate(context);
    v8::Handle<v8::Value> value = toV8(target, v8::Handle<v8::Object>(), isolate);
    if (value.IsEmpty())
        return v8::Local<v8::Object>();
    return v8::Local<v8::Object>::New(isolate, v8::Handle<v8::Object>::Cast(value));
}

} // namespace WebCore
