// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/v8_inspector/V8StackTraceImpl.h"

#include "platform/inspector_protocol/Platform.h"
#include "platform/inspector_protocol/String16.h"
#include "platform/v8_inspector/V8DebuggerImpl.h"
#include "platform/v8_inspector/V8StringUtil.h"

#include <v8-debug.h>
#include <v8-profiler.h>
#include <v8-version.h>

namespace blink {

namespace {

static const v8::StackTrace::StackTraceOptions stackTraceOptions = static_cast<v8::StackTrace::StackTraceOptions>(
    v8::StackTrace::kLineNumber |
    v8::StackTrace::kColumnOffset |
    v8::StackTrace::kScriptId |
    v8::StackTrace::kScriptNameOrSourceURL |
    v8::StackTrace::kFunctionName);

V8StackTraceImpl::Frame toFrame(v8::Local<v8::StackFrame> frame)
{
    String16 scriptId = String16::number(frame->GetScriptId());
    String16 sourceName;
    v8::Local<v8::String> sourceNameValue(frame->GetScriptNameOrSourceURL());
    if (!sourceNameValue.IsEmpty())
        sourceName = toProtocolString(sourceNameValue);

    String16 functionName;
    v8::Local<v8::String> functionNameValue(frame->GetFunctionName());
    if (!functionNameValue.IsEmpty())
        functionName = toProtocolString(functionNameValue);

    int sourceLineNumber = frame->GetLineNumber();
    int sourceColumn = frame->GetColumn();
    return V8StackTraceImpl::Frame(functionName, scriptId, sourceName, sourceLineNumber, sourceColumn);
}

void toFramesVector(v8::Local<v8::StackTrace> stackTrace, std::vector<V8StackTraceImpl::Frame>& frames, size_t maxStackSize, v8::Isolate* isolate)
{
    DCHECK(isolate->InContext());
    int frameCount = stackTrace->GetFrameCount();
    if (frameCount > static_cast<int>(maxStackSize))
        frameCount = maxStackSize;
    for (int i = 0; i < frameCount; i++) {
        v8::Local<v8::StackFrame> stackFrame = stackTrace->GetFrame(i);
        frames.push_back(toFrame(stackFrame));
    }
}

} //  namespace

V8StackTraceImpl::Frame::Frame()
    : m_functionName("undefined")
    , m_scriptId("")
    , m_scriptName("undefined")
    , m_lineNumber(0)
    , m_columnNumber(0)
{
}

V8StackTraceImpl::Frame::Frame(const String16& functionName, const String16& scriptId, const String16& scriptName, int lineNumber, int column)
    : m_functionName(functionName)
    , m_scriptId(scriptId)
    , m_scriptName(scriptName)
    , m_lineNumber(lineNumber)
    , m_columnNumber(column)
{
    DCHECK(m_lineNumber != v8::Message::kNoLineNumberInfo);
    DCHECK(m_columnNumber != v8::Message::kNoColumnInfo);
}

V8StackTraceImpl::Frame::~Frame()
{
}

// buildInspectorObject() and SourceLocation's toTracedValue() should set the same fields.
// If either of them is modified, the other should be also modified.
std::unique_ptr<protocol::Runtime::CallFrame> V8StackTraceImpl::Frame::buildInspectorObject() const
{
    return protocol::Runtime::CallFrame::create()
        .setFunctionName(m_functionName)
        .setScriptId(m_scriptId)
        .setUrl(m_scriptName)
        .setLineNumber(m_lineNumber - 1)
        .setColumnNumber(m_columnNumber - 1)
        .build();
}

V8StackTraceImpl::Frame V8StackTraceImpl::Frame::isolatedCopy() const
{
    return Frame(m_functionName.isolatedCopy(), m_scriptId.isolatedCopy(), m_scriptName.isolatedCopy(), m_lineNumber, m_columnNumber);
}

// static
void V8StackTraceImpl::setCaptureStackTraceForUncaughtExceptions(v8::Isolate* isolate, bool capture)
{
    isolate->SetCaptureStackTraceForUncaughtExceptions(capture, V8StackTraceImpl::maxCallStackSizeToCapture, stackTraceOptions);
}

// static
std::unique_ptr<V8StackTraceImpl> V8StackTraceImpl::create(V8DebuggerImpl* debugger, int contextGroupId, v8::Local<v8::StackTrace> stackTrace, size_t maxStackSize, const String16& description)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope scope(isolate);
    std::vector<V8StackTraceImpl::Frame> frames;
    if (!stackTrace.IsEmpty())
        toFramesVector(stackTrace, frames, maxStackSize, isolate);

    int maxAsyncCallChainDepth = 1;
    V8StackTraceImpl* asyncCallChain = nullptr;
    if (debugger && maxStackSize > 1) {
        asyncCallChain = debugger->currentAsyncCallChain();
        maxAsyncCallChainDepth = debugger->maxAsyncCallChainDepth();
    }
    // Do not accidentally append async call chain from another group. This should not
    // happen if we have proper instrumentation, but let's double-check to be safe.
    if (contextGroupId && asyncCallChain && asyncCallChain->m_contextGroupId && asyncCallChain->m_contextGroupId != contextGroupId) {
        asyncCallChain = nullptr;
        maxAsyncCallChainDepth = 1;
    }

    // Only the top stack in the chain may be empty, so ensure that second stack is non-empty (it's the top of appended chain).
    if (asyncCallChain && asyncCallChain->isEmpty())
        asyncCallChain = asyncCallChain->m_parent.get();

    if (stackTrace.IsEmpty() && !asyncCallChain)
        return nullptr;

    std::unique_ptr<V8StackTraceImpl> result(new V8StackTraceImpl(contextGroupId, description, frames, asyncCallChain ? asyncCallChain->cloneImpl() : nullptr));

    // Crop to not exceed maxAsyncCallChainDepth.
    V8StackTraceImpl* deepest = result.get();
    while (deepest && maxAsyncCallChainDepth) {
        deepest = deepest->m_parent.get();
        maxAsyncCallChainDepth--;
    }
    if (deepest)
        deepest->m_parent.reset();

    return result;
}

std::unique_ptr<V8StackTraceImpl> V8StackTraceImpl::capture(V8DebuggerImpl* debugger, int contextGroupId, size_t maxStackSize, const String16& description)
{
    v8::Isolate* isolate = v8::Isolate::GetCurrent();
    v8::HandleScope handleScope(isolate);
    v8::Local<v8::StackTrace> stackTrace;
    if (isolate->InContext()) {
#if V8_MAJOR_VERSION >= 5
        isolate->GetCpuProfiler()->CollectSample();
#endif
        stackTrace = v8::StackTrace::CurrentStackTrace(isolate, maxStackSize, stackTraceOptions);
    }
    return V8StackTraceImpl::create(debugger, contextGroupId, stackTrace, maxStackSize, description);
}

std::unique_ptr<V8StackTrace> V8StackTraceImpl::clone()
{
    return cloneImpl();
}

std::unique_ptr<V8StackTraceImpl> V8StackTraceImpl::cloneImpl()
{
    std::vector<Frame> framesCopy(m_frames);
    return wrapUnique(new V8StackTraceImpl(m_contextGroupId, m_description, framesCopy, m_parent ? m_parent->cloneImpl() : nullptr));
}

std::unique_ptr<V8StackTrace> V8StackTraceImpl::isolatedCopy()
{
    return isolatedCopyImpl();
}

std::unique_ptr<V8StackTraceImpl> V8StackTraceImpl::isolatedCopyImpl()
{
    std::vector<Frame> frames;
    for (size_t i = 0; i < m_frames.size(); i++)
        frames.push_back(m_frames.at(i).isolatedCopy());
    return wrapUnique(new V8StackTraceImpl(m_contextGroupId, m_description.isolatedCopy(), frames, m_parent ? m_parent->isolatedCopyImpl() : nullptr));
}

V8StackTraceImpl::V8StackTraceImpl(int contextGroupId, const String16& description, std::vector<Frame>& frames, std::unique_ptr<V8StackTraceImpl> parent)
    : m_contextGroupId(contextGroupId)
    , m_description(description)
    , m_parent(std::move(parent))
{
    m_frames.swap(frames);
}

V8StackTraceImpl::~V8StackTraceImpl()
{
}

String16 V8StackTraceImpl::topSourceURL() const
{
    DCHECK(m_frames.size());
    return m_frames[0].m_scriptName;
}

int V8StackTraceImpl::topLineNumber() const
{
    DCHECK(m_frames.size());
    return m_frames[0].m_lineNumber;
}

int V8StackTraceImpl::topColumnNumber() const
{
    DCHECK(m_frames.size());
    return m_frames[0].m_columnNumber;
}

String16 V8StackTraceImpl::topFunctionName() const
{
    DCHECK(m_frames.size());
    return m_frames[0].m_functionName;
}

String16 V8StackTraceImpl::topScriptId() const
{
    DCHECK(m_frames.size());
    return m_frames[0].m_scriptId;
}

std::unique_ptr<protocol::Runtime::StackTrace> V8StackTraceImpl::buildInspectorObject() const
{
    std::unique_ptr<protocol::Array<protocol::Runtime::CallFrame>> frames = protocol::Array<protocol::Runtime::CallFrame>::create();
    for (size_t i = 0; i < m_frames.size(); i++)
        frames->addItem(m_frames.at(i).buildInspectorObject());

    std::unique_ptr<protocol::Runtime::StackTrace> stackTrace = protocol::Runtime::StackTrace::create()
        .setCallFrames(std::move(frames)).build();
    if (!m_description.isEmpty())
        stackTrace->setDescription(m_description);
    if (m_parent)
        stackTrace->setParent(m_parent->buildInspectorObject());
    return stackTrace;
}

std::unique_ptr<protocol::Runtime::StackTrace> V8StackTraceImpl::buildInspectorObjectForTail(V8DebuggerImpl* debugger) const
{
    v8::HandleScope handleScope(v8::Isolate::GetCurrent());
    // Next call collapses possible empty stack and ensures maxAsyncCallChainDepth.
    std::unique_ptr<V8StackTraceImpl> fullChain = V8StackTraceImpl::create(debugger, m_contextGroupId, v8::Local<v8::StackTrace>(), V8StackTraceImpl::maxCallStackSizeToCapture);
    if (!fullChain || !fullChain->m_parent)
        return nullptr;
    return fullChain->m_parent->buildInspectorObject();
}

String16 V8StackTraceImpl::toString() const
{
    String16Builder stackTrace;
    for (size_t i = 0; i < m_frames.size(); ++i) {
        const Frame& frame = m_frames[i];
        stackTrace.append("\n    at " + (frame.functionName().length() ? frame.functionName() : "(anonymous function)"));
        stackTrace.append(" (");
        stackTrace.append(frame.sourceURL());
        stackTrace.append(':');
        stackTrace.appendNumber(frame.lineNumber());
        stackTrace.append(':');
        stackTrace.appendNumber(frame.columnNumber());
        stackTrace.append(')');
    }
    return stackTrace.toString();
}

} // namespace blink
