/**
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "HTMLPlugInElement.h"

#include "Attribute.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameTree.h"
#include "HTMLNames.h"
#include "Page.h"
#include "RenderEmbeddedObject.h"
#include "RenderWidget.h"
#include "ScriptController.h"
#include "Settings.h"
#include "Widget.h"

#if ENABLE(NETSCAPE_PLUGIN_API)
#include "npruntime_impl.h"
#endif

namespace WebCore {

using namespace HTMLNames;

HTMLPlugInElement::HTMLPlugInElement(const QualifiedName& tagName, Document* doc)
    : HTMLFrameOwnerElement(tagName, doc)
#if ENABLE(NETSCAPE_PLUGIN_API)
    , m_NPObject(0)
    , m_isCapturingMouseEvents(false)
#endif
{
}

HTMLPlugInElement::~HTMLPlugInElement()
{
    ASSERT(!m_instance); // cleared in detach()

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_NPObject) {
        _NPN_ReleaseObject(m_NPObject);
        m_NPObject = 0;
    }
#endif
}

void HTMLPlugInElement::detach()
{
    m_instance.clear();

    if (m_isCapturingMouseEvents) {
        if (Frame* frame = document()->frame())
            frame->eventHandler()->setCapturingMouseEventsNode(0);
        m_isCapturingMouseEvents = false;
    }

    HTMLFrameOwnerElement::detach();
}

RenderEmbeddedObject* HTMLPlugInElement::renderEmbeddedObject() const
{
    // HTMLObjectElement and HTMLEmbedElement may return arbitrary renderers
    // when using fallback content.
    if (!renderer() || !renderer()->isEmbeddedObject())
        return 0;
    return toRenderEmbeddedObject(renderer());
}

PassScriptInstance HTMLPlugInElement::getInstance() const
{
    Frame* frame = document()->frame();
    if (!frame)
        return 0;

    // If the host dynamically turns off JavaScript (or Java) we will still return
    // the cached allocated Bindings::Instance.  Not supporting this edge-case is OK.
    if (m_instance)
        return m_instance;

    if (Widget* widget = pluginWidget())
        m_instance = frame->script()->createScriptInstanceForWidget(widget);

    return m_instance;
}

Widget* HTMLPlugInElement::pluginWidget() const
{
    RenderWidget* renderWidget = renderWidgetForJSBindings();
    if (!renderWidget)
        return 0;

    return renderWidget->widget();
}

bool HTMLPlugInElement::mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const
{
    if (attrName == widthAttr ||
        attrName == heightAttr ||
        attrName == vspaceAttr ||
        attrName == hspaceAttr) {
            result = eUniversal;
            return false;
    }
    
    if (attrName == alignAttr) {
        result = eReplaced; // Share with <img> since the alignment behavior is the same.
        return false;
    }
    
    return HTMLFrameOwnerElement::mapToEntry(attrName, result);
}

void HTMLPlugInElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == widthAttr)
        addCSSLength(attr, CSSPropertyWidth, attr->value());
    else if (attr->name() == heightAttr)
        addCSSLength(attr, CSSPropertyHeight, attr->value());
    else if (attr->name() == vspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginTop, attr->value());
        addCSSLength(attr, CSSPropertyMarginBottom, attr->value());
    } else if (attr->name() == hspaceAttr) {
        addCSSLength(attr, CSSPropertyMarginLeft, attr->value());
        addCSSLength(attr, CSSPropertyMarginRight, attr->value());
    } else if (attr->name() == alignAttr)
        addHTMLAlignment(attr);
    else
        HTMLFrameOwnerElement::parseMappedAttribute(attr);
}

void HTMLPlugInElement::defaultEventHandler(Event* event)
{
    // Firefox seems to use a fake event listener to dispatch events to plug-in (tested with mouse events only).
    // This is observable via different order of events - in Firefox, event listeners specified in HTML attributes fires first, then an event
    // gets dispatched to plug-in, and only then other event listeners fire. Hopefully, this difference does not matter in practice.

    // FIXME: Mouse down and scroll events are passed down to plug-in via custom code in EventHandler; these code paths should be united.

    RenderObject* r = renderer();
    if (r && r->isEmbeddedObject() && toRenderEmbeddedObject(r)->showsMissingPluginIndicator()) {
        toRenderEmbeddedObject(r)->handleMissingPluginIndicatorEvent(event);
        return;
    }

    if (!r || !r->isWidget())
        return;
    Widget* widget = toRenderWidget(r)->widget();
    if (!widget)
        return;
    widget->handleEvent(event);
}

#if ENABLE(NETSCAPE_PLUGIN_API)

NPObject* HTMLPlugInElement::getNPObject()
{
    ASSERT(document()->frame());
    if (!m_NPObject)
        m_NPObject = document()->frame()->script()->createScriptObjectForPluginElement(this);
    return m_NPObject;
}

#endif /* ENABLE(NETSCAPE_PLUGIN_API) */

void HTMLPlugInElement::updateWidgetCallback(Node* n)
{
    static_cast<HTMLPlugInElement*>(n)->updateWidget();
}

}
