/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#if ENABLE(WORKERS)

#include "ScriptExecutionContext.h"
#include "SharedTimer.h"
#include "ThreadGlobalData.h"
#include "ThreadTimers.h"
#include "WorkerRunLoop.h"
#include "WorkerContext.h"
#include "WorkerThread.h"

namespace WebCore {

class WorkerSharedTimer : public SharedTimer {
public:
    WorkerSharedTimer()
        : m_sharedTimerFunction(0)
        , m_nextFireTime(0)
    {
    }

    // SharedTimer interface.
    virtual void setFiredFunction(void (*function)()) { m_sharedTimerFunction = function; }
    virtual void setFireTime(double fireTime) { m_nextFireTime = fireTime; }
    virtual void stop() { m_nextFireTime = 0; }

    bool isActive() { return m_sharedTimerFunction && m_nextFireTime; }
    double fireTime() { return m_nextFireTime; }
    void fire() { m_sharedTimerFunction(); }

private:
    void (*m_sharedTimerFunction)();
    double m_nextFireTime;
};

class WorkerRunLoop::Task : public RefCounted<Task> {
public:
    static PassRefPtr<Task> create(PassRefPtr<ScriptExecutionContext::Task> task, const String& mode)
    {
        return adoptRef(new Task(task, mode));
    }

    const String& mode() const { return m_mode; }
    void performTask(ScriptExecutionContext* context) { m_task->performTask(context); }

private:
    Task(PassRefPtr<ScriptExecutionContext::Task> task, const String& mode)
        : m_task(task)
        , m_mode(mode.copy())
    {
    }

    RefPtr<ScriptExecutionContext::Task> m_task;
    String m_mode;
};

class ModePredicate {
public:
    ModePredicate(const String& mode)
        : m_mode(mode)
        , m_defaultMode(mode == WorkerRunLoop::defaultMode())
    {
    }

    bool isDefaultMode() const
    {
        return m_defaultMode;
    }

    bool operator()(PassRefPtr<WorkerRunLoop::Task> task) const
    {
        return m_defaultMode || m_mode == task->mode();
    }

private:
    String m_mode;
    bool m_defaultMode;
};

WorkerRunLoop::WorkerRunLoop()
    : m_sharedTimer(new WorkerSharedTimer)
    , m_nestedCount(0)
    , m_uniqueId(0)
{
}

WorkerRunLoop::~WorkerRunLoop()
{
    ASSERT(!m_nestedCount);
}

String WorkerRunLoop::defaultMode()
{
    return String();
}

class RunLoopSetup : public Noncopyable {
public:
    RunLoopSetup(WorkerRunLoop& runLoop)
        : m_runLoop(runLoop)
    {
        if (!m_runLoop.m_nestedCount)
            threadGlobalData().threadTimers().setSharedTimer(m_runLoop.m_sharedTimer.get());
        m_runLoop.m_nestedCount++;
    }

    ~RunLoopSetup()
    {
        m_runLoop.m_nestedCount--;
        if (!m_runLoop.m_nestedCount)
            threadGlobalData().threadTimers().setSharedTimer(0);
    }
private:
    WorkerRunLoop& m_runLoop;
};

void WorkerRunLoop::run(WorkerContext* context)
{
    RunLoopSetup setup(*this);
    ModePredicate modePredicate(defaultMode());
    MessageQueueWaitResult result;
    do {
        result = runInMode(context, modePredicate);
    } while (result != MessageQueueTerminated);
}

MessageQueueWaitResult WorkerRunLoop::runInMode(WorkerContext* context, const String& mode)
{
    RunLoopSetup setup(*this);
    ModePredicate modePredicate(mode);
    MessageQueueWaitResult result = runInMode(context, modePredicate);
    return result;
}

MessageQueueWaitResult WorkerRunLoop::runInMode(WorkerContext* context, const ModePredicate& predicate)
{
    ASSERT(context);
    ASSERT(context->thread());
    ASSERT(context->thread()->threadID() == currentThread());

    double absoluteTime = (predicate.isDefaultMode() && m_sharedTimer->isActive()) ? m_sharedTimer->fireTime() : MessageQueue<RefPtr<Task> >::infiniteTime();
    RefPtr<Task> task;
    MessageQueueWaitResult result = m_messageQueue.waitForMessageFilteredWithTimeout(task, predicate, absoluteTime);

    switch (result) {
    case MessageQueueTerminated:
        break;

    case MessageQueueMessageReceived:
        task->performTask(context);
        break;

    case MessageQueueTimeout:
        m_sharedTimer->fire();
        break;
    }

    return result;
}

void WorkerRunLoop::terminate()
{
    m_messageQueue.kill();
}

void WorkerRunLoop::postTask(PassRefPtr<ScriptExecutionContext::Task> task)
{
    postTaskForMode(task, defaultMode());
}

void WorkerRunLoop::postTaskForMode(PassRefPtr<ScriptExecutionContext::Task> task, const String& mode)
{
    m_messageQueue.append(Task::create(task, mode.copy()));
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
