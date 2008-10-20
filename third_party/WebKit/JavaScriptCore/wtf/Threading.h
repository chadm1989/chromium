/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * Note: The implementations of InterlockedIncrement and InterlockedDecrement are based
 * on atomic_increment and atomic_exchange_and_add from the Boost C++ Library. The license
 * is virtually identical to the Apple license above but is included here for completeness.
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef Threading_h
#define Threading_h

#include <wtf/Assertions.h>
#include <wtf/Locker.h>
#include <wtf/Noncopyable.h>

#if PLATFORM(WIN_OS)
#include <windows.h>
#elif PLATFORM(DARWIN)
#include <libkern/OSAtomic.h>
#elif COMPILER(GCC)
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 2))
#include <ext/atomicity.h>
#else
#include <bits/atomicity.h>
#endif
#endif

#if USE(PTHREADS)
#include <pthread.h>
#elif PLATFORM(GTK)
#include <wtf/GOwnPtr.h>
typedef struct _GMutex GMutex;
typedef struct _GCond GCond;
#endif

#if PLATFORM(QT)
#include <qglobal.h>
QT_BEGIN_NAMESPACE
class QMutex;
class QWaitCondition;
QT_END_NAMESPACE
#endif

#include <stdint.h>

// For portability, we do not use thread-safe statics natively supported by some compilers (e.g. gcc).
#define AtomicallyInitializedStatic(T, name) \
    WTF::atomicallyInitializedStaticMutex->lock(); \
    static T name; \
    WTF::atomicallyInitializedStaticMutex->unlock();

namespace WTF {

typedef uint32_t ThreadIdentifier;
typedef void* (*ThreadFunction)(void* argument);

// Returns 0 if thread creation failed
ThreadIdentifier createThread(ThreadFunction, void*, const char* threadName);

ThreadIdentifier currentThread();
bool isMainThread();
int waitForThreadCompletion(ThreadIdentifier, void**);
void detachThread(ThreadIdentifier);

#if USE(PTHREADS)
typedef pthread_mutex_t PlatformMutex;
typedef pthread_cond_t PlatformCondition;
#elif PLATFORM(GTK)
typedef GOwnPtr<GMutex> PlatformMutex;
typedef GOwnPtr<GCond> PlatformCondition;
#elif PLATFORM(QT)
typedef QT_PREPEND_NAMESPACE(QMutex)* PlatformMutex;
typedef QT_PREPEND_NAMESPACE(QWaitCondition)* PlatformCondition;
#elif PLATFORM(WIN_OS)
struct PlatformMutex {
    CRITICAL_SECTION m_internalMutex;
    size_t m_recursionCount;
};
struct PlatformCondition {
    size_t m_timedOut;
    size_t m_blocked;
    size_t m_waitingForRemoval;
    HANDLE m_gate;
    HANDLE m_queue;
    HANDLE m_mutex;
};
#else
typedef void* PlatformMutex;
typedef void* PlatformCondition;
#endif
    
class Mutex : Noncopyable {
public:
    Mutex();
    ~Mutex();

    void lock();
    bool tryLock();
    void unlock();

public:
    PlatformMutex& impl() { return m_mutex; }
private:
    PlatformMutex m_mutex;
};

typedef Locker<Mutex> MutexLocker;

class ThreadCondition : Noncopyable {
public:
    ThreadCondition();
    ~ThreadCondition();
    
    void wait(Mutex& mutex);
    // Returns true if the condition was signaled before the timeout, false if the timeout was reached
    bool timedWait(Mutex&, double interval);
    void signal();
    void broadcast();
    
private:
    PlatformCondition m_condition;
};

#if PLATFORM(WIN_OS)
#define WTF_USE_LOCKFREE_THREADSAFESHARED 1

#if COMPILER(MINGW) || COMPILER(MSVC7)
inline void atomicIncrement(int* addend) { InterlockedIncrement(reinterpret_cast<long*>(addend)); }
inline int atomicDecrement(int* addend) { return InterlockedDecrement(reinterpret_cast<long*>(addend)); }
#else
inline void atomicIncrement(int volatile* addend) { InterlockedIncrement(reinterpret_cast<long volatile*>(addend)); }
inline int atomicDecrement(int volatile* addend) { return InterlockedDecrement(reinterpret_cast<long volatile*>(addend)); }
#endif

#elif PLATFORM(DARWIN)
#define WTF_USE_LOCKFREE_THREADSAFESHARED 1

inline void atomicIncrement(int volatile* addend) { OSAtomicIncrement32Barrier(const_cast<int*>(addend)); }
inline int atomicDecrement(int volatile* addend) { return OSAtomicDecrement32Barrier(const_cast<int*>(addend)); }

#elif COMPILER(GCC)
#define WTF_USE_LOCKFREE_THREADSAFESHARED 1

inline void atomicIncrement(int volatile* addend) { __gnu_cxx::__atomic_add(addend, 1); }
inline int atomicDecrement(int volatile* addend) { return __gnu_cxx::__exchange_and_add(addend, -1) - 1; }

#endif

template<class T> class ThreadSafeShared : Noncopyable {
public:
    ThreadSafeShared(int initialRefCount = 1)
        : m_refCount(initialRefCount)
    {
    }

    void ref()
    {
#if USE(LOCKFREE_THREADSAFESHARED)
        atomicIncrement(&m_refCount);
#else
        MutexLocker locker(m_mutex);
        ++m_refCount;
#endif
    }

    void deref()
    {
#if USE(LOCKFREE_THREADSAFESHARED)
        if (atomicDecrement(&m_refCount) <= 0)
#else
        {
            MutexLocker locker(m_mutex);
            --m_refCount;
        }
        if (m_refCount <= 0)
#endif
            delete static_cast<T*>(this);
    }

    bool hasOneRef()
    {
        return refCount() == 1;
    }

    int refCount() const
    {
#if !USE(LOCKFREE_THREADSAFESHARED)
        MutexLocker locker(m_mutex);
#endif
        return static_cast<int const volatile &>(m_refCount);
    }

private:
    int m_refCount;
#if !USE(LOCKFREE_THREADSAFESHARED)
    mutable Mutex m_mutex;
#endif
};

// This function must be called from the main thread. It is safe to call it repeatedly.
// Darwin is an exception to this rule: it is OK to call it from any thread, the only requirement is that the calls are not reentrant.
void initializeThreading();

extern Mutex* atomicallyInitializedStaticMutex;

} // namespace WTF

using WTF::Mutex;
using WTF::MutexLocker;
using WTF::ThreadCondition;
using WTF::ThreadIdentifier;
using WTF::ThreadSafeShared;

using WTF::createThread;
using WTF::currentThread;
using WTF::isMainThread;
using WTF::detachThread;
using WTF::waitForThreadCompletion;

#endif // Threading_h
