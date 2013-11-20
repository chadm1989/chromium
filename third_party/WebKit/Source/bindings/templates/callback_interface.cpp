{# http://www.chromium.org/blink/coding-style#TOC-License #}
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

{# FIXME: Rename to Python when switch #}
// This file has been auto-generated by code_generator_v8.pm. DO NOT MODIFY!

#include "config.h"
{% filter conditional(conditional_string) %}
#include "{{v8_class_name}}.h"

{% for filename in cpp_includes %}
#include "{{filename}}"
{% endfor %}
namespace WebCore {

{{v8_class_name}}::{{v8_class_name}}(v8::Handle<v8::Object> callback, ExecutionContext* context)
    : ActiveDOMCallback(context)
    , m_callback(toIsolate(context), callback)
    , m_world(DOMWrapperWorld::current())
{
}

{{v8_class_name}}::~{{v8_class_name}}()
{
}

{% for method in methods if not method.custom %}
{{method.return_cpp_type}} {{v8_class_name}}::{{method.name}}({{method.argument_declarations | join(', ')}})
{
    if (!canInvokeCallback())
        return true;

    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);

    v8::Handle<v8::Context> v8Context = toV8Context(executionContext(), m_world.get());
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);
    {% if method.call_with_this_handle %}
    v8::Handle<v8::Value> thisHandle = thisValue.v8Value();
    if (thisHandle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }
    ASSERT(thisHandle->IsObject());
    {% endif %}
    {% for argument in method.arguments %}
    {{argument.cpp_to_v8_conversion | indent}}
    if ({{argument.name}}Handle.IsEmpty()) {
        if (!isScriptControllerTerminating())
            CRASH();
        return true;
    }
    {% endfor %}
    {% if method.arguments %}
    v8::Handle<v8::Value> argv[] = { {{method.handles | join(', ')}} };
    {% else %}
    v8::Handle<v8::Value> *argv = 0;
    {% endif %}

    bool callbackReturnValue = false;
    {% set this_handle_parameter = 'v8::Handle<v8::Object>::Cast(thisHandle), ' if method.call_with_this_handle else '' %}
    return !invokeCallback(m_callback.newLocal(isolate), {{this_handle_parameter}}{{method.arguments | length}}, argv, callbackReturnValue, executionContext(), isolate);
}

{% endfor %}
} // namespace WebCore
{% endfilter %}
