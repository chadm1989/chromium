// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/v8_inspector/V8ConsoleMessage.h"

#include "platform/v8_inspector/InspectedContext.h"
#include "platform/v8_inspector/V8ConsoleAgentImpl.h"
#include "platform/v8_inspector/V8DebuggerImpl.h"
#include "platform/v8_inspector/V8InspectorSessionImpl.h"
#include "platform/v8_inspector/V8RuntimeAgentImpl.h"
#include "platform/v8_inspector/V8StackTraceImpl.h"
#include "platform/v8_inspector/V8StringUtil.h"
#include "platform/v8_inspector/public/V8DebuggerClient.h"

namespace blink {

namespace {

String16 messageSourceValue(MessageSource source)
{
    switch (source) {
    case XMLMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Xml;
    case JSMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Javascript;
    case NetworkMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Network;
    case ConsoleAPIMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::ConsoleApi;
    case StorageMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Storage;
    case AppCacheMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Appcache;
    case RenderingMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Rendering;
    case SecurityMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Security;
    case OtherMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Other;
    case DeprecationMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Deprecation;
    case WorkerMessageSource: return protocol::Console::ConsoleMessage::SourceEnum::Worker;
    }
    return protocol::Console::ConsoleMessage::SourceEnum::Other;
}


String16 consoleAPITypeValue(ConsoleAPIType type)
{
    switch (type) {
    case ConsoleAPIType::kLog: return protocol::Console::ConsoleMessage::TypeEnum::Log;
    case ConsoleAPIType::kClear: return protocol::Console::ConsoleMessage::TypeEnum::Clear;
    case ConsoleAPIType::kDir: return protocol::Console::ConsoleMessage::TypeEnum::Dir;
    case ConsoleAPIType::kDirXML: return protocol::Console::ConsoleMessage::TypeEnum::Dirxml;
    case ConsoleAPIType::kTable: return protocol::Console::ConsoleMessage::TypeEnum::Table;
    case ConsoleAPIType::kTrace: return protocol::Console::ConsoleMessage::TypeEnum::Trace;
    case ConsoleAPIType::kStartGroup: return protocol::Console::ConsoleMessage::TypeEnum::StartGroup;
    case ConsoleAPIType::kStartGroupCollapsed: return protocol::Console::ConsoleMessage::TypeEnum::StartGroupCollapsed;
    case ConsoleAPIType::kEndGroup: return protocol::Console::ConsoleMessage::TypeEnum::EndGroup;
    case ConsoleAPIType::kAssert: return protocol::Console::ConsoleMessage::TypeEnum::Assert;
    case ConsoleAPIType::kTimeEnd: return protocol::Console::ConsoleMessage::TypeEnum::Log;
    case ConsoleAPIType::kCount: return protocol::Console::ConsoleMessage::TypeEnum::Log;
    }
    return protocol::Console::ConsoleMessage::TypeEnum::Log;
}

String16 messageLevelValue(MessageLevel level)
{
    switch (level) {
    case DebugMessageLevel: return protocol::Console::ConsoleMessage::LevelEnum::Debug;
    case LogMessageLevel: return protocol::Console::ConsoleMessage::LevelEnum::Log;
    case WarningMessageLevel: return protocol::Console::ConsoleMessage::LevelEnum::Warning;
    case ErrorMessageLevel: return protocol::Console::ConsoleMessage::LevelEnum::Error;
    case InfoMessageLevel: return protocol::Console::ConsoleMessage::LevelEnum::Info;
    }
    return protocol::Console::ConsoleMessage::LevelEnum::Log;
}

const unsigned maxConsoleMessageCount = 1000;
const unsigned maxArrayItemsLimit = 10000;
const unsigned maxStackDepthLimit = 32;

class V8ValueStringBuilder {
public:
    static String16 toString(v8::Local<v8::Value> value, v8::Isolate* isolate)
    {
        V8ValueStringBuilder builder(isolate);
        if (!builder.append(value))
            return String16();
        return builder.toString();
    }

private:
    enum {
        IgnoreNull = 1 << 0,
        IgnoreUndefined = 1 << 1,
    };

    V8ValueStringBuilder(v8::Isolate* isolate)
        : m_arrayLimit(maxArrayItemsLimit)
        , m_isolate(isolate)
        , m_tryCatch(isolate)
    {
    }

    bool append(v8::Local<v8::Value> value, unsigned ignoreOptions = 0)
    {
        if (value.IsEmpty())
            return true;
        if ((ignoreOptions & IgnoreNull) && value->IsNull())
            return true;
        if ((ignoreOptions & IgnoreUndefined) && value->IsUndefined())
            return true;
        if (value->IsString())
            return append(v8::Local<v8::String>::Cast(value));
        if (value->IsStringObject())
            return append(v8::Local<v8::StringObject>::Cast(value)->ValueOf());
        if (value->IsSymbol())
            return append(v8::Local<v8::Symbol>::Cast(value));
        if (value->IsSymbolObject())
            return append(v8::Local<v8::SymbolObject>::Cast(value)->ValueOf());
        if (value->IsNumberObject()) {
            m_builder.appendNumber(v8::Local<v8::NumberObject>::Cast(value)->ValueOf());
            return true;
        }
        if (value->IsBooleanObject()) {
            m_builder.append(v8::Local<v8::BooleanObject>::Cast(value)->ValueOf() ? "true" : "false");
            return true;
        }
        if (value->IsArray())
            return append(v8::Local<v8::Array>::Cast(value));
        if (value->IsProxy()) {
            m_builder.append("[object Proxy]");
            return true;
        }
        if (value->IsObject()
            && !value->IsDate()
            && !value->IsFunction()
            && !value->IsNativeError()
            && !value->IsRegExp()) {
            v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(value);
            v8::Local<v8::String> stringValue;
            if (object->ObjectProtoToString(m_isolate->GetCurrentContext()).ToLocal(&stringValue))
                return append(stringValue);
        }
        v8::Local<v8::String> stringValue;
        if (!value->ToString(m_isolate->GetCurrentContext()).ToLocal(&stringValue))
            return false;
        return append(stringValue);
    }

    bool append(v8::Local<v8::Array> array)
    {
        for (const auto& it : m_visitedArrays) {
            if (it == array)
                return true;
        }
        uint32_t length = array->Length();
        if (length > m_arrayLimit)
            return false;
        if (m_visitedArrays.size() > maxStackDepthLimit)
            return false;

        bool result = true;
        m_arrayLimit -= length;
        m_visitedArrays.push_back(array);
        for (uint32_t i = 0; i < length; ++i) {
            if (i)
                m_builder.append(',');
            if (!append(array->Get(i), IgnoreNull | IgnoreUndefined)) {
                result = false;
                break;
            }
        }
        m_visitedArrays.pop_back();
        return result;
    }

    bool append(v8::Local<v8::Symbol> symbol)
    {
        m_builder.append("Symbol(");
        bool result = append(symbol->Name(), IgnoreUndefined);
        m_builder.append(')');
        return result;
    }

    bool append(v8::Local<v8::String> string)
    {
        if (m_tryCatch.HasCaught())
            return false;
        if (!string.IsEmpty())
            m_builder.append(toProtocolString(string));
        return true;
    }

    String16 toString()
    {
        if (m_tryCatch.HasCaught())
            return String16();
        return m_builder.toString();
    }

    uint32_t m_arrayLimit;
    v8::Isolate* m_isolate;
    String16Builder m_builder;
    std::vector<v8::Local<v8::Array>> m_visitedArrays;
    v8::TryCatch m_tryCatch;
};

} // namespace

V8ConsoleMessage::V8ConsoleMessage(V8MessageOrigin origin, double timestamp, MessageSource source, MessageLevel level, const String16& message)
    : m_origin(origin)
    , m_timestamp(timestamp)
    , m_source(source)
    , m_level(level)
    , m_message(message)
    , m_lineNumber(0)
    , m_columnNumber(0)
    , m_scriptId(0)
    , m_contextId(0)
    , m_type(ConsoleAPIType::kLog)
    , m_exceptionId(0)
    , m_revokedExceptionId(0)
{
}

V8ConsoleMessage::~V8ConsoleMessage()
{
}

void V8ConsoleMessage::setLocation(const String16& url, unsigned lineNumber, unsigned columnNumber, std::unique_ptr<V8StackTrace> stackTrace, int scriptId)
{
    m_url = url;
    m_lineNumber = lineNumber;
    m_columnNumber = columnNumber;
    m_stackTrace = std::move(stackTrace);
    m_scriptId = scriptId;
}

void V8ConsoleMessage::reportToFrontend(protocol::Console::Frontend* frontend, V8InspectorSessionImpl* session, bool generatePreview) const
{
    DCHECK_EQ(V8MessageOrigin::kConsole, m_origin);
    std::unique_ptr<protocol::Console::ConsoleMessage> result =
        protocol::Console::ConsoleMessage::create()
        .setSource(messageSourceValue(m_source))
        .setLevel(messageLevelValue(m_level))
        .setText(m_message)
        .setTimestamp(m_timestamp / 1000) // TODO(dgozman): migrate this to milliseconds.
        .build();
    result->setType(consoleAPITypeValue(m_type));
    result->setLine(static_cast<int>(m_lineNumber));
    result->setColumn(static_cast<int>(m_columnNumber));
    if (m_scriptId)
        result->setScriptId(String16::number(m_scriptId));
    result->setUrl(m_url);
    if (m_source == NetworkMessageSource && !m_requestIdentifier.isEmpty())
        result->setNetworkRequestId(m_requestIdentifier);
    if (m_contextId)
        result->setExecutionContextId(m_contextId);
    std::unique_ptr<protocol::Array<protocol::Runtime::RemoteObject>> args = wrapArguments(session, generatePreview);
    if (args)
        result->setParameters(std::move(args));
    if (m_stackTrace)
        result->setStack(m_stackTrace->buildInspectorObject());
    if (m_source == WorkerMessageSource && !m_workerId.isEmpty())
        result->setWorkerId(m_workerId);
    frontend->messageAdded(std::move(result));
}

std::unique_ptr<protocol::Array<protocol::Runtime::RemoteObject>> V8ConsoleMessage::wrapArguments(V8InspectorSessionImpl* session, bool generatePreview) const
{
    if (!m_arguments.size() || !m_contextId)
        return nullptr;
    InspectedContext* inspectedContext = session->debugger()->getContext(session->contextGroupId(), m_contextId);
    if (!inspectedContext)
        return nullptr;

    v8::Isolate* isolate = inspectedContext->isolate();
    v8::HandleScope handles(isolate);
    v8::Local<v8::Context> context = inspectedContext->context();

    std::unique_ptr<protocol::Array<protocol::Runtime::RemoteObject>> args = protocol::Array<protocol::Runtime::RemoteObject>::create();
    if (m_type == ConsoleAPIType::kTable && generatePreview) {
        v8::Local<v8::Value> table = m_arguments[0]->Get(isolate);
        v8::Local<v8::Value> columns = m_arguments.size() > 1 ? m_arguments[1]->Get(isolate) : v8::Local<v8::Value>();
        std::unique_ptr<protocol::Runtime::RemoteObject> wrapped = session->wrapTable(context, table, columns);
        if (wrapped)
            args->addItem(std::move(wrapped));
        else
            args = nullptr;
    } else {
        for (size_t i = 0; i < m_arguments.size(); ++i) {
            std::unique_ptr<protocol::Runtime::RemoteObject> wrapped = session->wrapObject(context, m_arguments[i]->Get(isolate), "console", generatePreview);
            if (!wrapped) {
                args = nullptr;
                break;
            }
            args->addItem(std::move(wrapped));
        }
    }
    return args;
}

void V8ConsoleMessage::reportToFrontend(protocol::Runtime::Frontend* frontend, V8InspectorSessionImpl* session, bool generatePreview) const
{
    if (m_origin == V8MessageOrigin::kException) {
        // TODO(dgozman): unify with InjectedScript::createExceptionDetails.
        std::unique_ptr<protocol::Runtime::ExceptionDetails> details = protocol::Runtime::ExceptionDetails::create().setText(m_message).build();
        details->setUrl(m_url);
        if (m_lineNumber)
            details->setLineNumber(static_cast<int>(m_lineNumber) - 1);
        if (m_columnNumber)
            details->setColumnNumber(static_cast<int>(m_columnNumber) - 1);
        if (m_scriptId)
            details->setScriptId(String16::number(m_scriptId));
        if (m_stackTrace)
            details->setStack(m_stackTrace->buildInspectorObject());

        std::unique_ptr<protocol::Runtime::RemoteObject> exception = wrapException(session, generatePreview);

        if (exception)
            frontend->exceptionThrown(m_exceptionId, m_timestamp, std::move(details), std::move(exception), m_contextId);
        else
            frontend->exceptionThrown(m_exceptionId, m_timestamp, std::move(details));
        return;
    }
    if (m_origin == V8MessageOrigin::kRevokedException) {
        frontend->exceptionRevoked(m_timestamp, m_message, m_revokedExceptionId);
        return;
    }
    NOTREACHED();
}

std::unique_ptr<protocol::Runtime::RemoteObject> V8ConsoleMessage::wrapException(V8InspectorSessionImpl* session, bool generatePreview) const
{
    if (!m_arguments.size() || !m_contextId)
        return nullptr;
    DCHECK_EQ(1u, m_arguments.size());
    InspectedContext* inspectedContext = session->debugger()->getContext(session->contextGroupId(), m_contextId);
    if (!inspectedContext)
        return nullptr;

    v8::Isolate* isolate = inspectedContext->isolate();
    v8::HandleScope handles(isolate);
    // TODO(dgozman): should we use different object group?
    return session->wrapObject(inspectedContext->context(), m_arguments[0]->Get(isolate), "console", generatePreview);
}

V8MessageOrigin V8ConsoleMessage::origin() const
{
    return m_origin;
}

unsigned V8ConsoleMessage::argumentCount() const
{
    return m_arguments.size();
}

ConsoleAPIType V8ConsoleMessage::type() const
{
    return m_type;
}

// static
std::unique_ptr<V8ConsoleMessage> V8ConsoleMessage::createForConsoleAPI(double timestamp, ConsoleAPIType type, MessageLevel level, const String16& messageText, std::vector<v8::Local<v8::Value>>* arguments, std::unique_ptr<V8StackTrace> stackTrace, InspectedContext* context)
{
    String16 url;
    unsigned lineNumber = 0;
    unsigned columnNumber = 0;
    if (stackTrace && !stackTrace->isEmpty()) {
        url = stackTrace->topSourceURL();
        lineNumber = stackTrace->topLineNumber();
        columnNumber = stackTrace->topColumnNumber();
    }

    String16 actualMessage = messageText;

    Arguments messageArguments;
    if (arguments && arguments->size()) {
        for (size_t i = 0; i < arguments->size(); ++i)
            messageArguments.push_back(wrapUnique(new v8::Global<v8::Value>(context->isolate(), arguments->at(i))));
        if (actualMessage.isEmpty())
            actualMessage = V8ValueStringBuilder::toString(messageArguments.at(0)->Get(context->isolate()), context->isolate());
    }

    std::unique_ptr<V8ConsoleMessage> message = wrapUnique(new V8ConsoleMessage(V8MessageOrigin::kConsole, timestamp, ConsoleAPIMessageSource, level, actualMessage));
    message->setLocation(url, lineNumber, columnNumber, std::move(stackTrace), 0);
    message->m_type = type;
    if (messageArguments.size()) {
        message->m_contextId = context->contextId();
        message->m_arguments.swap(messageArguments);
    }

    context->debugger()->client()->messageAddedToConsole(context->contextGroupId(), message->m_source, message->m_level, message->m_message, message->m_url, message->m_lineNumber, message->m_columnNumber, message->m_stackTrace.get());
    return message;
}

// static
std::unique_ptr<V8ConsoleMessage> V8ConsoleMessage::createForException(double timestamp, const String16& messageText, const String16& url, unsigned lineNumber, unsigned columnNumber, std::unique_ptr<V8StackTrace> stackTrace, int scriptId, v8::Isolate* isolate, int contextId, v8::Local<v8::Value> exception, unsigned exceptionId)
{
    std::unique_ptr<V8ConsoleMessage> message = wrapUnique(new V8ConsoleMessage(V8MessageOrigin::kException, timestamp, JSMessageSource, ErrorMessageLevel, messageText));
    message->setLocation(url, lineNumber, columnNumber, std::move(stackTrace), scriptId);
    message->m_exceptionId = exceptionId;
    if (contextId && !exception.IsEmpty()) {
        message->m_contextId = contextId;
        message->m_arguments.push_back(wrapUnique(new v8::Global<v8::Value>(isolate, exception)));
    }
    return message;
}

// static
std::unique_ptr<V8ConsoleMessage> V8ConsoleMessage::createForRevokedException(double timestamp, const String16& messageText, unsigned revokedExceptionId)
{
    std::unique_ptr<V8ConsoleMessage> message = wrapUnique(new V8ConsoleMessage(V8MessageOrigin::kRevokedException, timestamp, JSMessageSource, ErrorMessageLevel, messageText));
    message->m_revokedExceptionId = revokedExceptionId;
    return message;
}

// static
std::unique_ptr<V8ConsoleMessage> V8ConsoleMessage::createExternal(double timestamp, MessageSource source, MessageLevel level, const String16& messageText, const String16& url, unsigned lineNumber, unsigned columnNumber, std::unique_ptr<V8StackTrace> stackTrace, int scriptId, const String16& requestIdentifier, const String16& workerId)
{
    std::unique_ptr<V8ConsoleMessage> message = wrapUnique(new V8ConsoleMessage(V8MessageOrigin::kConsole, timestamp, source, level, messageText));
    message->setLocation(url, lineNumber, columnNumber, std::move(stackTrace), scriptId);
    message->m_requestIdentifier = requestIdentifier;
    message->m_workerId = workerId;
    return message;
}

void V8ConsoleMessage::contextDestroyed(int contextId)
{
    if (contextId != m_contextId)
        return;
    m_contextId = 0;
    if (m_message.isEmpty())
        m_message = "<message collected>";
    Arguments empty;
    m_arguments.swap(empty);
}

// ------------------------ V8ConsoleMessageStorage ----------------------------

V8ConsoleMessageStorage::V8ConsoleMessageStorage(V8DebuggerImpl* debugger, int contextGroupId)
    : m_debugger(debugger)
    , m_contextGroupId(contextGroupId)
    , m_expiredCount(0)
{
}

V8ConsoleMessageStorage::~V8ConsoleMessageStorage()
{
    clear();
}

void V8ConsoleMessageStorage::addMessage(std::unique_ptr<V8ConsoleMessage> message)
{
    if (message->type() == ConsoleAPIType::kClear)
        clear();

    V8InspectorSessionImpl* session = m_debugger->sessionForContextGroup(m_contextGroupId);
    if (session) {
        if (message->origin() == V8MessageOrigin::kConsole)
            session->consoleAgent()->messageAdded(message.get());
        else
            session->runtimeAgent()->exceptionMessageAdded(message.get());
    }

    DCHECK(m_messages.size() <= maxConsoleMessageCount);
    if (m_messages.size() == maxConsoleMessageCount) {
        ++m_expiredCount;
        m_messages.pop_front();
    }
    m_messages.push_back(std::move(message));
}

void V8ConsoleMessageStorage::clear()
{
    m_messages.clear();
    m_expiredCount = 0;
    V8InspectorSessionImpl* session = m_debugger->sessionForContextGroup(m_contextGroupId);
    if (session) {
        session->consoleAgent()->reset();
        session->releaseObjectGroup("console");
        session->client()->consoleCleared();
    }
}

void V8ConsoleMessageStorage::contextDestroyed(int contextId)
{
    for (size_t i = 0; i < m_messages.size(); ++i)
        m_messages[i]->contextDestroyed(contextId);
}

} // namespace blink
