/*
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reseved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef kjs_window_h
#define kjs_window_h

#include "PlatformString.h"
#include "kjs_binding.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>

namespace WebCore {
    class AtomicString;
    class DOMWindow;
    class Frame;
    class FrameView;
    class JSDOMWindow;
    class Node;
}

namespace KJS {

    class BarInfo;
    class History;
    class JSEventListener;
    class JSLazyEventListener;
    class JSUnprotectedEventListener;
    class Location;
    class PausedTimeout;
    class ScheduledAction;
    class Selection;
    class Window;
    class WindowFunc;

    class PausedTimeouts {
    public:
        PausedTimeouts(PausedTimeout *a, size_t length) : m_array(a), m_length(length) { }
        ~PausedTimeouts();

        size_t numTimeouts() const { return m_length; }
        PausedTimeout *takeTimeouts()
            { PausedTimeout *a = m_array; m_array = 0; return a; }

    private:
        PausedTimeout *m_array;
        size_t m_length;

        PausedTimeouts(const PausedTimeouts&);
        PausedTimeouts& operator=(const PausedTimeouts&);
    };

    class DOMWindowTimer;

  class WindowPrivate;

  class Window : public DOMObject {
    friend class Location;
    friend class WindowFunc;
    friend class ScheduledAction;
  protected:
    Window(WebCore::DOMWindow*);
  public:
    ~Window();
    WebCore::DOMWindow* impl() const;
    void disconnectFrame();
    /**
     * Returns and registers a window object. In case there's already a Window
     * for the specified frame p this will be returned in order to have unique
     * bindings.
     */
    static JSValue* retrieve(WebCore::Frame*);
    /**
     * Returns the Window object for a given HTML frame
     */
    static Window* retrieveWindow(WebCore::Frame*);
    /**
     * returns a pointer to the Window object this javascript interpreting instance
     * was called from.
     */
    static Window* retrieveActive(ExecState*);
    WebCore::Frame* frame() const { return m_frame; }
    virtual void mark();
    virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
    bool getOverridePropertySlot(ExecState*, const Identifier& propertyName, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
    virtual bool toBoolean(ExecState*) const;

    int installTimeout(const UString& handler, int t, bool singleShot);
    int installTimeout(JSValue* function, const List& args, int t, bool singleShot);
    void clearTimeout(int timerId, bool delAction = true);
    PausedTimeouts* pauseTimeouts();
    void resumeTimeouts(PausedTimeouts*);

    void timerFired(DOMWindowTimer*);
    
    KJS::ScriptInterpreter *interpreter() const;

    void scheduleClose();
        
    bool isSafeScript(ExecState*) const;
    static bool isSafeScript(const ScriptInterpreter *origin, const ScriptInterpreter *target);
    Location *location() const;
    Selection *selection() const;
    bool find(const WebCore::String&, bool, bool, bool, bool, bool, bool) const;

    // Finds a wrapper of a JS EventListener, returns 0 if no existing one.
    JSEventListener* findJSEventListener(JSValue*, bool html = false);

    // Finds or creates a wrapper of a JS EventListener. JS EventListener object is GC-protected.
    JSEventListener *findOrCreateJSEventListener(JSValue*, bool html = false);

    // Finds a wrapper of a GC-unprotected JS EventListener, returns 0 if no existing one.
    JSUnprotectedEventListener* findJSUnprotectedEventListener(JSValue*, bool html = false);

    // Finds or creates a wrapper of a JS EventListener. JS EventListener object is *NOT* GC-protected.
    JSUnprotectedEventListener *findOrCreateJSUnprotectedEventListener(JSValue*, bool html = false);

    void clear();

    // Set the current "event" object
    void setCurrentEvent(WebCore::Event*);

    // Set a place to put a dialog return value when the window is cleared.
    void setReturnValueSlot(JSValue **slot);

    typedef HashMap<JSObject*, JSEventListener*> ListenersMap;
    typedef HashMap<JSObject*, JSUnprotectedEventListener*> UnprotectedListenersMap;
    
    ListenersMap& jsEventListeners();
    ListenersMap& jsHTMLEventListeners();
    UnprotectedListenersMap& jsUnprotectedEventListeners();
    UnprotectedListenersMap& jsUnprotectedHTMLEventListeners();
    
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
    enum { AToB, BToA, Closed, Crypto, DefaultStatus, Status, DOMException, Frames, History_, Event_, InnerHeight,
           InnerWidth, Length, Location_, Name, Navigator_, ClientInformation,
           OffscreenBuffering, Opener, OuterHeight, OuterWidth, PageXOffset, PageYOffset,
           Parent, ScreenX, ScreenY, Scroll, ScrollBy,
           ScreenTop, ScreenLeft,
           ScrollTo, ScrollX, ScrollY, MoveBy, MoveTo, ResizeBy, ResizeTo, Self, Window_, Top,
           Image, Option, Alert, Confirm, Prompt, Open, Print, SetTimeout, ClearTimeout,
           Focus, GetSelection, Blur, Close, SetInterval, ClearInterval, CaptureEvents, 
           ReleaseEvents, AddEventListener, RemoveEventListener,
           XMLHttpRequest, XSLTProcessor_,
           Onabort, Onblur, Onchange, Onclick, Ondblclick, Onerror, 
           Onfocus, Onkeydown, Onkeypress, Onkeyup, Onload, Onmousedown, Onmousemove,
           Onmouseout, Onmouseover, Onmouseup, OnWindowMouseWheel, Onreset, Onresize, Onscroll, Onsearch,
           Onselect, Onsubmit, Onunload, Onbeforeunload,
           FrameElement, ShowModalDialog, Find, Stop };

  private:
    JSValue* getListener(ExecState*, const WebCore::AtomicString& eventType) const;
    void setListener(ExecState*, const WebCore::AtomicString& eventType, JSValue* func);

    static JSValue *childFrameGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *namedFrameGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *indexGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);
    static JSValue *namedItemGetter(ExecState *exec, JSObject *, const Identifier&, const PropertySlot& slot);

    void updateLayout() const;

    void clearHelperObjectProperties();
    void clearAllTimeouts();
    int installTimeout(ScheduledAction*, int interval, bool singleShot);

    WebCore::Frame* m_frame;
    OwnPtr<WindowPrivate> d;
  };

  /**
   * An action (either function or string) to be executed after a specified
   * time interval, either once or repeatedly. Used for window.setTimeout()
   * and window.setInterval()
   */
    class ScheduledAction {
    public:
        ScheduledAction(JSValue *func, const List& args)
            : m_func(func), m_args(args) { }
        ScheduledAction(const WebCore::String& code)
            : m_code(code) { }
        void execute(Window *);

    private:
        ProtectedPtr<JSValue> m_func;
        List m_args;
        WebCore::String m_code;
    };

  class Location : public DOMObject {
  public:
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value, int attr = None);
    enum { Hash, Href, Hostname, Host, Pathname, Port, Protocol, Search, 
           Replace, Reload, ToString, Assign };
    WebCore::Frame* frame() const { return m_frame; }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  private:
    friend class Window;
    Location(WebCore::Frame*);
    WebCore::Frame* m_frame;
  };

  class Selection : public DOMObject {
  public:
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    JSValue *getValueProperty(ExecState *exec, int token) const;
    enum { AnchorNode, AnchorOffset, FocusNode, FocusOffset, BaseNode, BaseOffset, ExtentNode, ExtentOffset, 
           IsCollapsed, _Type, RangeCount, Collapse, CollapseToEnd, CollapseToStart, Empty, ToString, 
           SetBaseAndExtent, SetPosition, Modify, GetRangeAt, RemoveAllRanges, AddRange };
    WebCore::Frame* frame() const { return m_frame; }
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;
  private:
    friend class Window;
    Selection(WebCore::Frame*);
    WebCore::Frame* m_frame;
  };

} // namespace

namespace WebCore {
    KJS::JSValue* toJS(KJS::ExecState*, DOMWindow*);
} // namespace WebCore

#endif
