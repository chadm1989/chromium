/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebKit.h"
#include "WebKitDLL.h"
#include "WebScriptDebugServer.h"

#include "WebScriptCallFrame.h"
#include "WebScriptDebugger.h"
#include "WebView.h"
#pragma warning(push, 0)
#include <WebCore/Page.h>
#pragma warning(pop)
#include <kjs/ExecState.h>
#include <wtf/Assertions.h>
#include <wtf/Vector.h>

using namespace KJS;
using namespace WebCore;

typedef HashSet<COMPtr<IWebScriptDebugListener> > ListenerSet;

static ListenerSet s_Listeners;
static unsigned s_ListenerCount = 0;
static OwnPtr<WebScriptDebugServer> s_SharedWebScriptDebugServer;
static bool s_dying = false;

unsigned WebScriptDebugServer::listenerCount() { return s_ListenerCount; };

// WebScriptDebugServer ------------------------------------------------------------

WebScriptDebugServer::WebScriptDebugServer()
    : m_refCount(0)
    , m_paused(false)
    , m_step(false)
{
    gClassCount++;
}

WebScriptDebugServer::~WebScriptDebugServer()
{
    gClassCount--;
}

WebScriptDebugServer* WebScriptDebugServer::createInstance()
{
    WebScriptDebugServer* instance = new WebScriptDebugServer;
    instance->AddRef();
    return instance;
}

WebScriptDebugServer* WebScriptDebugServer::sharedWebScriptDebugServer()
{
    if (!s_SharedWebScriptDebugServer) {
        s_dying = false;
        s_SharedWebScriptDebugServer.set(WebScriptDebugServer::createInstance());
    }

    return s_SharedWebScriptDebugServer.get();
}

void WebScriptDebugServer::pageCreated(Page* page)
{
    ASSERT_ARG(page, page);

    if (s_ListenerCount > 0)
        page->setDebugger(sharedWebScriptDebugServer());
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<WebScriptDebugServer*>(this);
    else if (IsEqualGUID(riid, IID_IWebScriptDebugServer))
        *ppvObject = static_cast<WebScriptDebugServer*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebScriptDebugServer::AddRef()
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebScriptDebugServer::Release()
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebScriptDebugServer -----------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::sharedWebScriptDebugServer( 
    /* [retval][out] */ IWebScriptDebugServer** server)
{
    if (!server)
        return E_POINTER;

    *server = WebScriptDebugServer::sharedWebScriptDebugServer();
    (*server)->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::addListener(
    /* [in] */ IWebScriptDebugListener* listener)
{
    if (s_dying)
        return E_FAIL;

    if (!listener)
        return E_POINTER;

    if (!s_ListenerCount)
        Page::setDebuggerForAllPages(sharedWebScriptDebugServer());

    ++s_ListenerCount;
    s_Listeners.add(listener);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::removeListener(
    /* [in] */ IWebScriptDebugListener* listener)
{
    if (s_dying)
        return S_OK;

    if (!listener)
        return E_POINTER;

    if (!s_Listeners.contains(listener))
        return S_OK;

    s_Listeners.remove(listener);

    ASSERT(s_ListenerCount >= 1);
    if (--s_ListenerCount == 0) {
        Page::setDebuggerForAllPages(0);
        resume();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::step()
{
    m_step = true;
    m_paused = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::pause()
{
    m_paused = true;
    m_step = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::resume()
{
    m_paused = false;
    m_step = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::isPaused(
    /* [out, retval] */ BOOL* isPaused)
{
    if (!isPaused)
        return E_POINTER;

    *isPaused = m_paused;
    return S_OK;
}

static HWND comMessageWindow()
{
    static bool initialized;
    static HWND window;

    if (initialized)
        return window;
    initialized = true;

    static LPCTSTR windowClass = TEXT("OleMainThreadWndClass");
    static LPCTSTR windowText = TEXT("OleMainThreadWndName");
    static const DWORD currentProcess = GetCurrentProcessId();

    window = 0;
    DWORD windowProcess = 0;
    do {
        window = FindWindowEx(HWND_MESSAGE, window, windowClass, windowText);
        GetWindowThreadProcessId(window, &windowProcess);
    } while (window && windowProcess != currentProcess);

    return window;
}

void WebScriptDebugServer::suspendProcessIfPaused()
{
    static bool alreadyHere = false;

    if (alreadyHere)
        return;

    alreadyHere = true;

    // We only deliver messages to COM's message window to pause the process while still allowing RPC to work.
    // FIXME: It would be nice if we could keep delivering WM_[NC]PAINT messages to all windows to keep them painting on XP.

    HWND messageWindow = comMessageWindow();

    MSG msg;
    while (m_paused && GetMessage(&msg, messageWindow, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (m_step) {
        m_step = false;
        m_paused = true;
    }

    alreadyHere = false;
}

// IWebScriptDebugListener
HRESULT STDMETHODCALLTYPE WebScriptDebugServer::didLoadMainResourceForDataSource(
    /* [in] */ IWebView* webView,
    /* [in] */ IWebDataSource* dataSource)
{
    if (!webView || !dataSource)
        return E_FAIL;

    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).didLoadMainResourceForDataSource(webView, dataSource);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::didParseSource(
    /* [in] */ IWebView* webView,
    /* [in] */ BSTR sourceCode,
    /* [in] */ UINT baseLineNumber,
    /* [in] */ BSTR url,
    /* [in] */ int sourceID,
    /* [in] */ IWebFrame* webFrame)
{
    if (!webView || !sourceCode || !url || !webFrame)
        return E_FAIL;

    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).didParseSource(webView, sourceCode, baseLineNumber, url, sourceID, webFrame);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::failedToParseSource(
    /* [in] */ IWebView* webView,
    /* [in] */ BSTR sourceCode,
    /* [in] */ UINT baseLineNumber,
    /* [in] */ BSTR url,
    /* [in] */ BSTR error,
    /* [in] */ IWebFrame* webFrame)
{
    if (!webView || !sourceCode || !url || !error || !webFrame)
        return E_FAIL;

    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).failedToParseSource(webView, sourceCode, baseLineNumber, url, error, webFrame);

    return S_OK;
}

bool WebScriptDebugServer::callEvent(ExecState* exec, int sourceID, int lineNumber, JSObject* /*function*/, const List &/*args*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec));
    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).didEnterCallFrame(webView(exec), callFrame.get(), sourceID, lineNumber, webFrame(exec));

    suspendProcessIfPaused();

    m_callingServer = false;

    return true;
}

bool WebScriptDebugServer::atStatement(ExecState* exec, int sourceID, int firstLine, int /*lastLine*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec));
    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).willExecuteStatement(webView(exec), callFrame.get(), sourceID, firstLine, webFrame(exec));

    suspendProcessIfPaused();

    m_callingServer = false;

    return true;
}

bool WebScriptDebugServer::returnEvent(ExecState* exec, int sourceID, int lineNumber, JSObject* /*function*/)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec->callingExecState()));
    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).willLeaveCallFrame(webView(exec), callFrame.get(), sourceID, lineNumber, webFrame(exec));

    suspendProcessIfPaused();

    m_callingServer = false;

    return true;
}

bool WebScriptDebugServer::exception(ExecState* exec, int sourceID, int lineNumber, JSValue* /*exception */)
{
    if (m_callingServer)
        return true;

    m_callingServer = true;

    COMPtr<WebScriptCallFrame> callFrame(AdoptCOM, WebScriptCallFrame::createInstance(exec));
    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it)
        (**it).exceptionWasRaised(webView(exec), callFrame.get(), sourceID, lineNumber, webFrame(exec));

    suspendProcessIfPaused();

    m_callingServer = false;

    return true;
}

HRESULT STDMETHODCALLTYPE WebScriptDebugServer::serverDidDie()
{
    s_dying = true;

    ListenerSet listenersCopy = s_Listeners;
    ListenerSet::iterator end = listenersCopy.end();
    for (ListenerSet::iterator it = listenersCopy.begin(); it != end; ++it) {
        (**it).serverDidDie();
        s_Listeners.remove((*it).get());
    }

    return S_OK;
}
