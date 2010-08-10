/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "WebDevToolsAgentImpl.h"

#include "BoundObject.h"
#include "DebuggerAgentImpl.h"
#include "DebuggerAgentManager.h"
#include "Document.h"
#include "EventListener.h"
#include "InjectedScriptHost.h"
#include "InspectorBackend.h"
#include "InspectorBackendDispatcher.h"
#include "InspectorController.h"
#include "InspectorFrontend.h"
#include "InspectorResource.h"
#include "Node.h"
#include "Page.h"
#include "PageGroup.h"
#include "PlatformString.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptDebugServer.h"
#include "ScriptState.h"
#include "ScriptValue.h"
#include "V8Binding.h"
#include "V8Proxy.h"
#include "V8Utilities.h"
#include "WebDataSource.h"
#include "WebDevToolsAgentClient.h"
#include "WebFrameImpl.h"
#include "WebRect.h"
#include "WebString.h"
#include "WebURL.h"
#include "WebURLError.h"
#include "WebURLRequest.h"
#include "WebURLResponse.h"
#include "WebViewClient.h"
#include "WebViewImpl.h"
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>

using WebCore::Document;
using WebCore::DocumentLoader;
using WebCore::FrameLoader;
using WebCore::InjectedScriptHost;
using WebCore::InspectorBackend;
using WebCore::InspectorController;
using WebCore::InspectorFrontend;
using WebCore::InspectorResource;
using WebCore::Node;
using WebCore::Page;
using WebCore::ResourceError;
using WebCore::ResourceRequest;
using WebCore::ResourceResponse;
using WebCore::SafeAllocation;
using WebCore::ScriptState;
using WebCore::ScriptValue;
using WTF::String;
using WebCore::V8DOMWrapper;
using WebCore::V8Proxy;

namespace WebKit {

namespace {

void InspectorBackendWeakReferenceCallback(v8::Persistent<v8::Value> object, void* parameter)
{
    InspectorBackend* backend = static_cast<InspectorBackend*>(parameter);
    backend->deref();
    object.Dispose();
}

static const char kFrontendConnectedFeatureName[] = "frontend-connected";
static const char kResourceTrackingFeatureName[] = "resource-tracking";
static const char kTimelineFeatureName[] = "timeline-profiler";
static const char kApuAgentFeatureName[] = "apu-agent";

class ClientMessageLoopAdapter : public WebCore::ScriptDebugServer::ClientMessageLoop {
public:
    static void ensureClientMessageLoopCreated(WebDevToolsAgentClient* client)
    {
        if (s_instance)
            return;
        s_instance = new ClientMessageLoopAdapter(client->createClientMessageLoop());
        WebCore::ScriptDebugServer::shared().setClientMessageLoop(s_instance);
    }

    static void inspectedViewClosed(WebViewImpl* view)
    {
        if (s_instance)
            s_instance->m_frozenViews.remove(view);
    }

    static void didNavigate()
    {
        // Release render thread if necessary.
        if (s_instance && s_instance->m_running)
            WebCore::ScriptDebugServer::shared().continueProgram();
    }

private:
    ClientMessageLoopAdapter(PassOwnPtr<WebKit::WebDevToolsAgentClient::WebKitClientMessageLoop> messageLoop)
        : m_running(false)
        , m_messageLoop(messageLoop) { }


    virtual void run(Page* page)
    {
        if (m_running)
            return;
        m_running = true;

        Vector<WebViewImpl*> views;

        // 1. Disable input events.
        HashSet<Page*>::const_iterator end =  page->group().pages().end();
        for (HashSet<Page*>::const_iterator it =  page->group().pages().begin(); it != end; ++it) {
            WebViewImpl* view = WebViewImpl::fromPage(*it);
            m_frozenViews.add(view);
            views.append(view);
            view->setIgnoreInputEvents(true);
        }

        // 2. Disable active objects
        WebView::willEnterModalLoop();

        // 3. Process messages until quitNow is called.
        m_messageLoop->run();

        // 4. Resume active objects
        WebView::didExitModalLoop();

        // 5. Resume input events.
        for (Vector<WebViewImpl*>::iterator it = views.begin(); it != views.end(); ++it) {
            if (m_frozenViews.contains(*it)) {
                // The view was not closed during the dispatch.
                (*it)->setIgnoreInputEvents(false);
            }
        }

        // 6. All views have been resumed, clear the set.
        m_frozenViews.clear();

        m_running = false;
    }

    virtual void quitNow()
    {
        m_messageLoop->quitNow();
    }

    bool m_running;
    OwnPtr<WebKit::WebDevToolsAgentClient::WebKitClientMessageLoop> m_messageLoop;
    typedef HashSet<WebViewImpl*> FrozenViewsSet;
    FrozenViewsSet m_frozenViews;
    static ClientMessageLoopAdapter* s_instance;

};

ClientMessageLoopAdapter* ClientMessageLoopAdapter::s_instance = 0;

} //  namespace

WebDevToolsAgentImpl::WebDevToolsAgentImpl(
    WebViewImpl* webViewImpl,
    WebDevToolsAgentClient* client)
    : m_hostId(client->hostIdentifier())
    , m_client(client)
    , m_webViewImpl(webViewImpl)
    , m_apuAgentEnabled(false)
    , m_resourceTrackingWasEnabled(false)
    , m_attached(false)
{
    DebuggerAgentManager::setExposeV8DebuggerProtocol(
        client->exposeV8DebuggerProtocol());
}

WebDevToolsAgentImpl::~WebDevToolsAgentImpl()
{
    DebuggerAgentManager::onWebViewClosed(m_webViewImpl);
    ClientMessageLoopAdapter::inspectedViewClosed(m_webViewImpl);
    disposeUtilityContext();
}

void WebDevToolsAgentImpl::disposeUtilityContext()
{
    if (!m_utilityContext.IsEmpty()) {
        m_utilityContext.Dispose();
        m_utilityContext.Clear();
    }
}

void WebDevToolsAgentImpl::attach()
{
    if (m_attached)
        return;

    if (!m_client->exposeV8DebuggerProtocol())
        ClientMessageLoopAdapter::ensureClientMessageLoopCreated(m_client);

    m_debuggerAgentImpl.set(
        new DebuggerAgentImpl(m_webViewImpl, this, m_client));
    createInspectorFrontendProxy();

    // Allow controller to send messages to the frontend.
    InspectorController* ic = inspectorController();

    { // TODO(yurys): the source should have already been pushed by the frontend.
        v8::HandleScope scope;
        v8::Context::Scope contextScope(m_utilityContext);
        v8::Handle<v8::Value> constructorValue = m_utilityContext->Global()->Get(
            v8::String::New("injectedScriptConstructor"));
        if (constructorValue->IsFunction()) {
            String source = WebCore::toWebCoreString(constructorValue);
            ic->injectedScriptHost()->setInjectedScriptSource("(" + source + ")");
        }
    }
    m_attached = true;
}

void WebDevToolsAgentImpl::detach()
{
    // Prevent controller from sending messages to the frontend.
    InspectorController* ic = m_webViewImpl->page()->inspectorController();
    ic->disconnectFrontend();
    ic->hideHighlight();
    ic->close();
    disposeUtilityContext();
    m_debuggerAgentImpl.set(0);
    m_attached = false;
    m_apuAgentEnabled = false;
}

void WebDevToolsAgentImpl::frontendLoaded()
{
    InspectorController* ic = inspectorController();
    ic->connectFrontend();
    m_client->runtimeFeatureStateChanged(kFrontendConnectedFeatureName, true);
}

void WebDevToolsAgentImpl::didNavigate()
{
    ClientMessageLoopAdapter::didNavigate();
    DebuggerAgentManager::onNavigate();
}

void WebDevToolsAgentImpl::didClearWindowObject(WebFrameImpl* webframe)
{
    DebuggerAgentManager::setHostId(webframe, m_hostId);
}

void WebDevToolsAgentImpl::forceRepaint()
{
    m_client->forceRepaint();
}

void WebDevToolsAgentImpl::dispatchOnInspectorBackend(const WebString& message)
{
    inspectorController()->inspectorBackendDispatcher()->dispatch(message);
}

void WebDevToolsAgentImpl::inspectElementAt(const WebPoint& point)
{
    m_webViewImpl->inspectElementAt(point);
}

void WebDevToolsAgentImpl::setRuntimeFeatureEnabled(const WebString& feature, bool enabled)
{
    if (feature == kApuAgentFeatureName)
        setApuAgentEnabled(enabled);
    else if (feature == kTimelineFeatureName)
        setTimelineProfilingEnabled(enabled);
    else if (feature == kResourceTrackingFeatureName) {
        InspectorController* ic = m_webViewImpl->page()->inspectorController();
        if (enabled)
          ic->enableResourceTracking(false /* not sticky */, false /* no reload */);
        else
          ic->disableResourceTracking(false /* not sticky */);
    } else if (feature == kFrontendConnectedFeatureName && enabled)
        frontendLoaded();
}

void WebDevToolsAgentImpl::compileUtilityScripts()
{
    v8::HandleScope handleScope;
    v8::Context::Scope contextScope(m_utilityContext);
    // Inject javascript into the context.
    WebCString injectedScriptJs = m_client->injectedScriptSource();
    v8::Script::Compile(v8::String::New(
        injectedScriptJs.data(),
        injectedScriptJs.length()))->Run();
    WebCString injectDispatchJs = m_client->injectedScriptDispatcherSource();
    v8::Script::Compile(v8::String::New(
        injectDispatchJs.data(),
        injectDispatchJs.length()))->Run();
}

void WebDevToolsAgentImpl::initDevToolsAgentHost()
{
    BoundObject devtoolsAgentHost(m_utilityContext, this, "DevToolsAgentHost");
    devtoolsAgentHost.addProtoFunction(
        "dispatch",
        WebDevToolsAgentImpl::jsDispatchOnClient);
    devtoolsAgentHost.build();
}

void WebDevToolsAgentImpl::createInspectorFrontendProxy()
{
    disposeUtilityContext();
    m_utilityContext = v8::Context::New();
    compileUtilityScripts();
    initDevToolsAgentHost();
    WebCString debuggerScriptJs = m_client->debuggerScriptSource();
    WebCore::ScriptDebugServer::shared().setDebuggerScriptSource(
        WTF::String(debuggerScriptJs.data(), debuggerScriptJs.length()));
}

void WebDevToolsAgentImpl::setApuAgentEnabled(bool enabled)
{
    m_apuAgentEnabled = enabled;
    InspectorController* ic = m_webViewImpl->page()->inspectorController();
    if (enabled) {
        m_resourceTrackingWasEnabled = ic->resourceTrackingEnabled();
        ic->startTimelineProfiler();
        if (!m_resourceTrackingWasEnabled) {
            // TODO(knorton): Introduce some kind of agents dependency here so that
            // user could turn off resource tracking while apu agent is on.
            ic->enableResourceTracking(false, false);
        }
        m_debuggerAgentImpl->setAutoContinueOnException(true);
    } else {
      ic->stopTimelineProfiler();
      if (!m_resourceTrackingWasEnabled)
          ic->disableResourceTracking(false);
      m_resourceTrackingWasEnabled = false;
    }
    m_client->runtimeFeatureStateChanged(
        kApuAgentFeatureName,
        enabled);
}

// static
v8::Handle<v8::Value> WebDevToolsAgentImpl::jsDispatchOnClient(const v8::Arguments& args)
{
    v8::TryCatch exceptionCatcher;
    String message = WebCore::toWebCoreStringWithNullCheck(args[0]);
    if (message.isEmpty() || exceptionCatcher.HasCaught())
        return v8::Undefined();

    WebDevToolsAgentImpl* agent = static_cast<WebDevToolsAgentImpl*>(v8::External::Cast(*args.Data())->Value());

    if (!agent->m_apuAgentEnabled) {
        agent->m_client->sendMessageToInspectorFrontend(message);
        return v8::Undefined();
    }

    String method = WebCore::toWebCoreStringWithNullCheck(args[1]);
    if (method.isEmpty() || exceptionCatcher.HasCaught())
        return v8::Undefined();

    if (method != "updateResource" && method != "addResource")
        return v8::Undefined();

    agent->m_client->sendDispatchToAPU(message);
    return v8::Undefined();
}

WebCore::InspectorController* WebDevToolsAgentImpl::inspectorController()
{
    if (Page* page = m_webViewImpl->page())
        return page->inspectorController();
    return 0;
}


//------- plugin resource load notifications ---------------
void WebDevToolsAgentImpl::identifierForInitialRequest(
    unsigned long resourceId,
    WebFrame* frame,
    const WebURLRequest& request)
{
    if (InspectorController* ic = inspectorController()) {
        WebFrameImpl* webFrameImpl = static_cast<WebFrameImpl*>(frame);
        FrameLoader* frameLoader = webFrameImpl->frame()->loader();
        DocumentLoader* loader = frameLoader->activeDocumentLoader();
        ic->identifierForInitialRequest(resourceId, loader, request.toResourceRequest());
    }
}

void WebDevToolsAgentImpl::willSendRequest(unsigned long resourceId, WebURLRequest& request)
{
    if (InspectorController* ic = inspectorController())
        ic->willSendRequest(resourceId, request.toMutableResourceRequest(), ResourceResponse());
}

void WebDevToolsAgentImpl::didReceiveData(unsigned long resourceId, int length)
{
    if (InspectorController* ic = inspectorController())
        ic->didReceiveContentLength(resourceId, length);
}

void WebDevToolsAgentImpl::didReceiveResponse(unsigned long resourceId, const WebURLResponse& response)
{
    if (InspectorController* ic = inspectorController())
        ic->didReceiveResponse(resourceId, response.toResourceResponse());
}

void WebDevToolsAgentImpl::didFinishLoading(unsigned long resourceId)
{
    if (InspectorController* ic = inspectorController())
        ic->didFinishLoading(resourceId);
}

void WebDevToolsAgentImpl::didFailLoading(unsigned long resourceId, const WebURLError& error)
{
    ResourceError resourceError;
    if (InspectorController* ic = inspectorController())
        ic->didFailLoading(resourceId, resourceError);
}

void WebDevToolsAgentImpl::inspectorDestroyed()
{
    // Our lifetime is bound to the WebViewImpl.
}

void WebDevToolsAgentImpl::openInspectorFrontend(InspectorController*)
{
}

void WebDevToolsAgentImpl::highlight(Node* node)
{
    // InspectorController does the actuall tracking of the highlighted node
    // and the drawing of the highlight. Here we just make sure to invalidate
    // the rects of the old and new nodes.
    hideHighlight();
}

void WebDevToolsAgentImpl::hideHighlight()
{
    // FIXME: able to invalidate a smaller rect.
    // FIXME: Is it important to just invalidate the rect of the node region
    // given that this is not on a critical codepath?  In order to do so, we'd
    // have to take scrolling into account.
    const WebSize& size = m_webViewImpl->size();
    WebRect damagedRect(0, 0, size.width, size.height);
    if (m_webViewImpl->client())
        m_webViewImpl->client()->didInvalidateRect(damagedRect);
}

void WebDevToolsAgentImpl::populateSetting(const String& key, String* value)
{
    WebString string;
    m_webViewImpl->inspectorSetting(key, &string);
    *value = string;
}

void WebDevToolsAgentImpl::storeSetting(const String& key, const String& value)
{
    m_webViewImpl->setInspectorSetting(key, value);
}

bool WebDevToolsAgentImpl::sendMessageToFrontend(const WTF::String& message)
{
    WebDevToolsAgentImpl* devToolsAgent = static_cast<WebDevToolsAgentImpl*>(m_webViewImpl->devToolsAgent());
    if (!devToolsAgent)
        return false;

    if (devToolsAgent->m_apuAgentEnabled) {
        m_client->sendDispatchToAPU(message);
        return true;
    }

    m_client->sendMessageToInspectorFrontend(message);
    return true;
}

void WebDevToolsAgentImpl::resourceTrackingWasEnabled()
{
    m_client->runtimeFeatureStateChanged(kResourceTrackingFeatureName, true);
}

void WebDevToolsAgentImpl::resourceTrackingWasDisabled()
{
    m_client->runtimeFeatureStateChanged(kResourceTrackingFeatureName, false);
}

void WebDevToolsAgentImpl::timelineProfilerWasStarted()
{
    m_client->runtimeFeatureStateChanged(kTimelineFeatureName, true);
}

void WebDevToolsAgentImpl::timelineProfilerWasStopped()
{
    m_client->runtimeFeatureStateChanged(kTimelineFeatureName, false);
}

void WebDevToolsAgentImpl::evaluateInWebInspector(long callId, const WebString& script)
{
    InspectorController* ic = inspectorController();
    ic->evaluateForTestInFrontend(callId, script);
}

void WebDevToolsAgentImpl::setTimelineProfilingEnabled(bool enabled)
{
    InspectorController* ic = inspectorController();
    if (enabled)
        ic->startTimelineProfiler();
    else
        ic->stopTimelineProfiler();
}

void WebDevToolsAgent::executeDebuggerCommand(const WebString& command, int callerId)
{
    DebuggerAgentManager::executeDebuggerCommand(command, callerId);
}

void WebDevToolsAgent::debuggerPauseScript()
{
    DebuggerAgentManager::pauseScript();
}

void WebDevToolsAgent::setMessageLoopDispatchHandler(MessageLoopDispatchHandler handler)
{
    DebuggerAgentManager::setMessageLoopDispatchHandler(handler);
}

} // namespace WebKit
