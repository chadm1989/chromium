/*
 * Copyright (C) 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef DOMWindow_h
#define DOMWindow_h

#include "bindings/v8/ScriptWrappable.h"
#include "core/events/EventTarget.h"
#include "core/page/FrameDestructionObserver.h"
#include "core/platform/LifecycleContext.h"
#include "platform/Supplementable.h"

#include "wtf/Forward.h"

namespace WebCore {
    class ApplicationCache;
    class BarProp;
    class CSSRuleList;
    class CSSStyleDeclaration;
    class Console;
    class DOMPoint;
    class DOMSelection;
    class DOMURL;
    class DOMWindowProperty;
    class Database;
    class DatabaseCallback;
    class Document;
    class DOMWindowLifecycleNotifier;
    class Element;
    class EventListener;
    class ExceptionState;
    class FloatRect;
    class Frame;
    class History;
    class IDBFactory;
    class Location;
    class MediaQueryList;
    class MessageEvent;
    class Navigator;
    class Node;
    class Page;
    class PageConsole;
    class Performance;
    class PostMessageTimer;
    class RequestAnimationFrameCallback;
    class ScheduledAction;
    class Screen;
    class ScriptCallStack;
    class SecurityOrigin;
    class SerializedScriptValue;
    class Storage;
    class StyleMedia;
    class DOMWindowCSS;

    struct WindowFeatures;

    typedef Vector<RefPtr<MessagePort>, 1> MessagePortArray;

    enum SetLocationLocking { LockHistoryBasedOnGestureState, LockHistoryAndBackForwardList };

    class DOMWindow : public RefCounted<DOMWindow>, public ScriptWrappable, public EventTarget, public FrameDestructionObserver, public Supplementable<DOMWindow>, public LifecycleContext {
    public:
        static PassRefPtr<DOMWindow> create(Frame* frame) { return adoptRef(new DOMWindow(frame)); }
        virtual ~DOMWindow();

        // In some rare cases, we'll re-used a DOMWindow for a new Document. For example,
        // when a script calls window.open("..."), the browser gives JavaScript a window
        // synchronously but kicks off the load in the window asynchronously. Web sites
        // expect that modifications that they make to the window object synchronously
        // won't be blown away when the network load commits. To make that happen, we
        // "securely transition" the existing DOMWindow to the Document that results from
        // the network load. See also SecurityContext::isSecureTransitionTo.
        void setDocument(PassRefPtr<Document>);

        virtual const AtomicString& interfaceName() const;
        virtual ScriptExecutionContext* scriptExecutionContext() const;

        virtual DOMWindow* toDOMWindow();

        void registerProperty(DOMWindowProperty*);
        void unregisterProperty(DOMWindowProperty*);

        void reset();

        PassRefPtr<MediaQueryList> matchMedia(const String&);

        unsigned pendingUnloadEventListeners() const;

        static FloatRect adjustWindowRect(Page*, const FloatRect& pendingChanges);

        bool allowPopUp(); // Call on first window, not target window.
        static bool allowPopUp(Frame* firstFrame);
        static bool canShowModalDialog(const Frame*);
        static bool canShowModalDialogNow(const Frame*);

        // DOM Level 0

        Screen* screen() const;
        History* history() const;
        BarProp* locationbar() const;
        BarProp* menubar() const;
        BarProp* personalbar() const;
        BarProp* scrollbars() const;
        BarProp* statusbar() const;
        BarProp* toolbar() const;
        Navigator* navigator() const;
        Navigator* clientInformation() const { return navigator(); }

        Location* location() const;
        void setLocation(const String& location, DOMWindow* activeWindow, DOMWindow* firstWindow,
            SetLocationLocking = LockHistoryBasedOnGestureState);

        DOMSelection* getSelection();

        Element* frameElement() const;

        void focus(ScriptExecutionContext* = 0);
        void blur();
        void close(ScriptExecutionContext* = 0);
        void print();
        void stop();

        PassRefPtr<DOMWindow> open(const String& urlString, const AtomicString& frameName, const String& windowFeaturesString,
            DOMWindow* activeWindow, DOMWindow* firstWindow);

        typedef void (*PrepareDialogFunction)(DOMWindow*, void* context);
        void showModalDialog(const String& urlString, const String& dialogFeaturesString,
            DOMWindow* activeWindow, DOMWindow* firstWindow, PrepareDialogFunction, void* functionContext);

        void alert(const String& message);
        bool confirm(const String& message);
        String prompt(const String& message, const String& defaultValue);

        bool find(const String&, bool caseSensitive, bool backwards, bool wrap, bool wholeWord, bool searchInFrames, bool showDialog) const;

        bool offscreenBuffering() const;

        int outerHeight() const;
        int outerWidth() const;
        int innerHeight() const;
        int innerWidth() const;
        int screenX() const;
        int screenY() const;
        int screenLeft() const { return screenX(); }
        int screenTop() const { return screenY(); }
        int scrollX() const;
        int scrollY() const;
        int pageXOffset() const { return scrollX(); }
        int pageYOffset() const { return scrollY(); }

        bool closed() const;

        unsigned length() const;

        String name() const;
        void setName(const String&);

        String status() const;
        void setStatus(const String&);
        String defaultStatus() const;
        void setDefaultStatus(const String&);

        // This attribute is an alias of defaultStatus and is necessary for legacy uses.
        String defaultstatus() const { return defaultStatus(); }
        void setDefaultstatus(const String& status) { setDefaultStatus(status); }

        // Self-referential attributes

        DOMWindow* self() const;
        DOMWindow* window() const { return self(); }
        DOMWindow* frames() const { return self(); }

        DOMWindow* opener() const;
        DOMWindow* parent() const;
        DOMWindow* top() const;

        // DOM Level 2 AbstractView Interface

        Document* document() const;

        // CSSOM View Module

        PassRefPtr<StyleMedia> styleMedia() const;

        // DOM Level 2 Style Interface

        PassRefPtr<CSSStyleDeclaration> getComputedStyle(Element*, const String& pseudoElt) const;

        // WebKit extensions

        PassRefPtr<CSSRuleList> getMatchedCSSRules(Element*, const String& pseudoElt, bool authorOnly = true) const;
        double devicePixelRatio() const;

        PassRefPtr<DOMPoint> webkitConvertPointFromPageToNode(Node*, const DOMPoint*) const;
        PassRefPtr<DOMPoint> webkitConvertPointFromNodeToPage(Node*, const DOMPoint*) const;

        Console* console() const;
        PageConsole* pageConsole() const;

        void printErrorMessage(const String&);
        String crossDomainAccessErrorMessage(DOMWindow* activeWindow);
        String sanitizedCrossDomainAccessErrorMessage(DOMWindow* activeWindow);

        void postMessage(PassRefPtr<SerializedScriptValue> message, const MessagePortArray*, const String& targetOrigin, DOMWindow* source, ExceptionState&);
        void postMessageTimerFired(PassOwnPtr<PostMessageTimer>);
        void dispatchMessageEventWithOriginCheck(SecurityOrigin* intendedTargetOrigin, PassRefPtr<Event>, PassRefPtr<ScriptCallStack>);

        void scrollBy(int x, int y) const;
        void scrollTo(int x, int y) const;
        void scroll(int x, int y) const { scrollTo(x, y); }

        void moveBy(float x, float y) const;
        void moveTo(float x, float y) const;

        void resizeBy(float x, float y) const;
        void resizeTo(float width, float height) const;

        // WebKit animation extensions
        int requestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>);
        int webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>);
        void cancelAnimationFrame(int id);

        DOMWindowCSS* css();

        // Events
        // EventTarget API
        virtual bool addEventListener(const AtomicString& eventType, PassRefPtr<EventListener>, bool useCapture);
        virtual bool removeEventListener(const AtomicString& eventType, EventListener*, bool useCapture);
        virtual void removeAllEventListeners();

        using EventTarget::dispatchEvent;
        bool dispatchEvent(PassRefPtr<Event> prpEvent, PassRefPtr<EventTarget> prpTarget);

        void dispatchLoadEvent();

        DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(animationend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(animationiteration);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(animationstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(beforeunload);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(blur);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(canplay);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(canplaythrough);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(change);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(click);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(contextmenu);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dblclick);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(drag);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragenter);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragleave);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragover);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(dragstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(drop);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(durationchange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(emptied);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(ended);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(focus);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(hashchange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(input);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(invalid);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(keydown);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(keypress);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(keyup);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadeddata);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadedmetadata);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(loadstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mousedown);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseenter);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseleave);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mousemove);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseout);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseover);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseup);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(mousewheel);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(offline);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(online);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(pagehide);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(pageshow);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(pause);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(play);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(playing);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(popstate);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(progress);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(ratechange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(reset);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(resize);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(scroll);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(search);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(seeked);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(seeking);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(select);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(stalled);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(storage);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(submit);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(suspend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(timeupdate);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(transitionend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(unload);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(volumechange);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(waiting);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(wheel);

        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkitanimationstart, webkitAnimationStart);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkitanimationiteration, webkitAnimationIteration);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkitanimationend, webkitAnimationEnd);
        DEFINE_MAPPED_ATTRIBUTE_EVENT_LISTENER(webkittransitionend, webkitTransitionEnd);

        void captureEvents() { }
        void releaseEvents() { }

        void finishedLoading();

        using RefCounted<DOMWindow>::ref;
        using RefCounted<DOMWindow>::deref;

        DEFINE_ATTRIBUTE_EVENT_LISTENER(devicemotion);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(deviceorientation);

        // HTML 5 key/value storage
        Storage* sessionStorage(ExceptionState&) const;
        Storage* localStorage(ExceptionState&) const;
        Storage* optionalSessionStorage() const { return m_sessionStorage.get(); }
        Storage* optionalLocalStorage() const { return m_localStorage.get(); }

        ApplicationCache* applicationCache() const;
        ApplicationCache* optionalApplicationCache() const { return m_applicationCache.get(); }

#if ENABLE(ORIENTATION_EVENTS)
        // This is the interface orientation in degrees. Some examples are:
        //  0 is straight up; -90 is when the device is rotated 90 clockwise;
        //  90 is when rotated counter clockwise.
        int orientation() const;

        DEFINE_ATTRIBUTE_EVENT_LISTENER(orientationchange);
#endif

        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchstart);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchmove);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchend);
        DEFINE_ATTRIBUTE_EVENT_LISTENER(touchcancel);

        Performance* performance() const;

        // FIXME: When this DOMWindow is no longer the active DOMWindow (i.e.,
        // when its document is no longer the document that is displayed in its
        // frame), we would like to zero out m_frame to avoid being confused
        // by the document that is currently active in m_frame.
        bool isCurrentlyDisplayedInFrame() const;

        void willDetachDocumentFromFrame();
        DOMWindow* anonymousIndexedGetter(uint32_t);

        bool isInsecureScriptAccess(DOMWindow* activeWindow, const String& urlString);

    protected:
        DOMWindowLifecycleNotifier* lifecycleNotifier();

    private:
        explicit DOMWindow(Frame*);

        Page* page();

        virtual PassOwnPtr<LifecycleNotifier> createLifecycleNotifier() OVERRIDE;

        virtual void frameDestroyed() OVERRIDE;
        virtual void willDetachPage() OVERRIDE;

        virtual void refEventTarget() { ref(); }
        virtual void derefEventTarget() { deref(); }
        virtual EventTargetData* eventTargetData();
        virtual EventTargetData* ensureEventTargetData();

        void resetDOMWindowProperties();
        void willDestroyDocumentInFrame();

        RefPtr<Document> m_document;

        bool m_shouldPrintWhenFinishedLoading;

        HashSet<DOMWindowProperty*> m_properties;

        mutable RefPtr<Screen> m_screen;
        mutable RefPtr<History> m_history;
        mutable RefPtr<BarProp> m_locationbar;
        mutable RefPtr<BarProp> m_menubar;
        mutable RefPtr<BarProp> m_personalbar;
        mutable RefPtr<BarProp> m_scrollbars;
        mutable RefPtr<BarProp> m_statusbar;
        mutable RefPtr<BarProp> m_toolbar;
        mutable RefPtr<Console> m_console;
        mutable RefPtr<Navigator> m_navigator;
        mutable RefPtr<Location> m_location;
        mutable RefPtr<StyleMedia> m_media;

        EventTargetData m_eventTargetData;

        String m_status;
        String m_defaultStatus;

        mutable RefPtr<Storage> m_sessionStorage;
        mutable RefPtr<Storage> m_localStorage;
        mutable RefPtr<ApplicationCache> m_applicationCache;

        mutable RefPtr<Performance> m_performance;

        mutable RefPtr<DOMWindowCSS> m_css;
    };

    inline String DOMWindow::status() const
    {
        return m_status;
    }

    inline String DOMWindow::defaultStatus() const
    {
        return m_defaultStatus;
    }

} // namespace WebCore

#endif // DOMWindow_h
