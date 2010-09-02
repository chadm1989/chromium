/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "RenderEmbeddedObject.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "CSSValueKeywords.h"
#include "Font.h"
#include "FontSelector.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "GraphicsContext.h"
#include "HTMLEmbedElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLParamElement.h"
#include "LocalizedStrings.h"
#include "MIMETypeRegistry.h"
#include "MouseEvent.h"
#include "Page.h"
#include "Path.h"
#include "PluginViewBase.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidgetProtector.h"
#include "Settings.h"
#include "Text.h"

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "HTMLVideoElement.h"
#endif

namespace WebCore {

using namespace HTMLNames;
    
static const float replacementTextRoundedRectHeight = 18;
static const float replacementTextRoundedRectLeftRightTextMargin = 6;
static const float replacementTextRoundedRectOpacity = 0.20f;
static const float replacementTextPressedRoundedRectOpacity = 0.65f;
static const float replacementTextRoundedRectRadius = 5;
static const float replacementTextTextOpacity = 0.55f;
static const float replacementTextPressedTextOpacity = 0.65f;

static const Color& replacementTextRoundedRectPressedColor()
{
    static const Color lightGray(205, 205, 205);
    return lightGray;
}
    
RenderEmbeddedObject::RenderEmbeddedObject(Element* element)
    : RenderPart(element)
    , m_hasFallbackContent(false)
    , m_showsMissingPluginIndicator(false)
    , m_missingPluginIndicatorIsPressed(false)
    , m_mouseDownWasInMissingPluginIndicator(false)
{
    view()->frameView()->setIsVisuallyNonEmpty();
}

RenderEmbeddedObject::~RenderEmbeddedObject()
{
    if (frameView())
        frameView()->removeWidgetToUpdate(this);
}

#if USE(ACCELERATED_COMPOSITING)
bool RenderEmbeddedObject::requiresLayer() const
{
    if (RenderPart::requiresLayer())
        return true;
    
    return allowsAcceleratedCompositing();
}

bool RenderEmbeddedObject::allowsAcceleratedCompositing() const
{
    return widget() && widget()->isPluginViewBase() && static_cast<PluginViewBase*>(widget())->platformLayer();
}
#endif

// FIXME: This belongs in HTMLPluginElement.cpp to be shared by HTMLObjectElement and HTMLEmbedElement.
static bool isURLAllowed(Document* doc, const String& url)
{
    if (doc->frame()->page()->frameCount() >= Page::maxNumberOfFrames)
        return false;

    // We allow one level of self-reference because some sites depend on that.
    // But we don't allow more than one.
    KURL completeURL = doc->completeURL(url);
    bool foundSelfReference = false;
    for (Frame* frame = doc->frame(); frame; frame = frame->tree()->parent()) {
        if (equalIgnoringFragmentIdentifier(frame->loader()->url(), completeURL)) {
            if (foundSelfReference)
                return false;
            foundSelfReference = true;
        }
    }
    return true;
}

typedef HashMap<String, String, CaseFoldingHash> ClassIdToTypeMap;

// FIXME: This belongs in HTMLObjectElement.cpp
static ClassIdToTypeMap* createClassIdToTypeMap()
{
    ClassIdToTypeMap* map = new ClassIdToTypeMap;
    map->add("clsid:D27CDB6E-AE6D-11CF-96B8-444553540000", "application/x-shockwave-flash");
    map->add("clsid:CFCDAA03-8BE4-11CF-B84B-0020AFBBCCFA", "audio/x-pn-realaudio-plugin");
    map->add("clsid:02BF25D5-8C17-4B23-BC80-D3488ABDDC6B", "video/quicktime");
    map->add("clsid:166B1BCA-3F9C-11CF-8075-444553540000", "application/x-director");
    map->add("clsid:6BF52A52-394A-11D3-B153-00C04F79FAA6", "application/x-mplayer2");
    map->add("clsid:22D6F312-B0F6-11D0-94AB-0080C74C7E95", "application/x-mplayer2");
    return map;
}

// FIXME: This belongs in HTMLObjectElement.cpp
static String serviceTypeForClassId(const String& classId)
{
    // Return early if classId is empty (since we won't do anything below).
    // Furthermore, if classId is null, calling get() below will crash.
    if (classId.isEmpty())
        return String();

    static ClassIdToTypeMap* map = createClassIdToTypeMap();
    return map->get(classId);
}

// FIXME: This belongs in HTMLObjectElement.cpp
static void mapDataParamToSrc(Vector<String>* paramNames, Vector<String>* paramValues)
{
    // Some plugins don't understand the "data" attribute of the OBJECT tag (i.e. Real and WMP
    // require "src" attribute).
    int srcIndex = -1, dataIndex = -1;
    for (unsigned int i = 0; i < paramNames->size(); ++i) {
        if (equalIgnoringCase((*paramNames)[i], "src"))
            srcIndex = i;
        else if (equalIgnoringCase((*paramNames)[i], "data"))
            dataIndex = i;
    }

    if (srcIndex == -1 && dataIndex != -1) {
        paramNames->append("src");
        paramValues->append((*paramValues)[dataIndex]);
    }
}

// FIXME: This belongs in some loader header?
static bool isNetscapePlugin(Frame* frame, const String& url, const String& serviceType)
{
    KURL completedURL;
    if (!url.isEmpty())
        completedURL = frame->loader()->completeURL(url);

    if (frame->loader()->client()->objectContentType(completedURL, serviceType) == ObjectContentNetscapePlugin)
        return true;
    return false;
}

// FIXME: This belongs on HTMLObjectElement.
static bool hasFallbackContent(HTMLObjectElement* objectElement)
{
    for (Node* child = objectElement->firstChild(); child; child = child->nextSibling()) {
        if ((!child->isTextNode() && !child->hasTagName(paramTag)) // Discount <param>
            || (child->isTextNode() && !static_cast<Text*>(child)->containsOnlyWhitespace()))
            return true;
    }
    return false;
}

// FIXME: This belongs on HTMLObjectElement.
static void parametersFromObject(HTMLObjectElement* objectElement, Vector<String>& paramNames, Vector<String>& paramValues, String& url, String& serviceType)
{
    HashSet<StringImpl*, CaseFoldingHash> uniqueParamNames;

    // Scan the PARAM children and store their name/value pairs.
    // Get the URL and type from the params if we don't already have them.
    Node* child = objectElement->firstChild();
    while (child) {
        if (child->hasTagName(paramTag)) {
            HTMLParamElement* p = static_cast<HTMLParamElement*>(child);
            String name = p->name();
            if (url.isEmpty() && (equalIgnoringCase(name, "src") || equalIgnoringCase(name, "movie") || equalIgnoringCase(name, "code") || equalIgnoringCase(name, "url")))
                url = deprecatedParseURL(p->value());
            // FIXME: serviceType calculation does not belong in this function.
            if (serviceType.isEmpty() && equalIgnoringCase(name, "type")) {
                serviceType = p->value();
                size_t pos = serviceType.find(";");
                if (pos != notFound)
                    serviceType = serviceType.left(pos);
            }
            if (!name.isEmpty()) {
                uniqueParamNames.add(name.impl());
                paramNames.append(p->name());
                paramValues.append(p->value());
            }
        }
        child = child->nextSibling();
    }

    // When OBJECT is used for an applet via Sun's Java plugin, the CODEBASE attribute in the tag
    // points to the Java plugin itself (an ActiveX component) while the actual applet CODEBASE is
    // in a PARAM tag. See <http://java.sun.com/products/plugin/1.2/docs/tags.html>. This means
    // we have to explicitly suppress the tag's CODEBASE attribute if there is none in a PARAM,
    // else our Java plugin will misinterpret it. [4004531]
    String codebase;
    if (MIMETypeRegistry::isJavaAppletMIMEType(serviceType)) {
        codebase = "codebase";
        uniqueParamNames.add(codebase.impl()); // pretend we found it in a PARAM already
    }

    // Turn the attributes of the <object> element into arrays, but don't override <param> values.
    NamedNodeMap* attributes = objectElement->attributes();
    if (attributes) {
        for (unsigned i = 0; i < attributes->length(); ++i) {
            Attribute* it = attributes->attributeItem(i);
            const AtomicString& name = it->name().localName();
            if (!uniqueParamNames.contains(name.impl())) {
                paramNames.append(name.string());
                paramValues.append(it->value().string());
            }
        }
    }

    mapDataParamToSrc(&paramNames, &paramValues);

    // If we still don't have a type, try to map from a specific CLASSID to a type.
    if (serviceType.isEmpty())
        serviceType = serviceTypeForClassId(objectElement->classId());
}

// FIXME: This belongs on HTMLObjectElement, HTMLPluginElement or HTMLFrameOwnerElement.
static void updateWidgetForObjectElement(HTMLObjectElement* objectElement, bool onlyCreateNonNetscapePlugins)
{
    objectElement->setNeedWidgetUpdate(false);
    if (!objectElement->isFinishedParsingChildren())
        return;

    Frame* frame = objectElement->document()->frame();
    String url = objectElement->url();
    String serviceType = objectElement->serviceType();

    Vector<String> paramNames;
    Vector<String> paramValues;
    parametersFromObject(objectElement, paramNames, paramValues, url, serviceType);

    if (!isURLAllowed(objectElement->document(), url))
        return;

    bool fallbackContent = hasFallbackContent(objectElement);
    objectElement->renderEmbeddedObject()->setHasFallbackContent(fallbackContent);

    if (onlyCreateNonNetscapePlugins && isNetscapePlugin(frame, url, serviceType))
        return;

    bool beforeLoadAllowedLoad = objectElement->dispatchBeforeLoadEvent(url);

    // beforeload events can modify the DOM, potentially causing
    // RenderWidget::destroy() to be called.  Ensure we haven't been
    // destroyed before continuing.
    // FIXME: Should this render fallback content?
    if (!objectElement->renderer())
        return;

    bool success = beforeLoadAllowedLoad && frame->loader()->subframeLoader()->requestObject(objectElement, url, objectElement->getAttribute(nameAttr), serviceType, paramNames, paramValues);

    if (!success && fallbackContent)
        objectElement->renderFallbackContent();
}

// FIXME: This belongs on HTMLEmbedElement.
static void parametersFromEmbed(HTMLEmbedElement* embedElement, Vector<String>& paramNames, Vector<String>& paramValues)
{
    NamedNodeMap* attributes = embedElement->attributes();
    if (attributes) {
        for (unsigned i = 0; i < attributes->length(); ++i) {
            Attribute* it = attributes->attributeItem(i);
            paramNames.append(it->name().localName().string());
            paramValues.append(it->value().string());
        }
    }
}

// FIXME: This belongs on HTMLEmbedElement, HTMLPluginElement or HTMLFrameOwnerElement.
static void updateWidgetForEmbedElement(HTMLEmbedElement* embedElement, bool onlyCreateNonNetscapePlugins)
{
    Frame* frame = embedElement->document()->frame();
    String url = embedElement->url();
    String serviceType = embedElement->serviceType();

    embedElement->setNeedWidgetUpdate(false);

    if (url.isEmpty() && serviceType.isEmpty())
        return;
    if (!isURLAllowed(embedElement->document(), url))
        return;

    Vector<String> paramNames;
    Vector<String> paramValues;
    parametersFromEmbed(embedElement, paramNames, paramValues);

    if (onlyCreateNonNetscapePlugins && isNetscapePlugin(frame, url, serviceType))
        return;

    if (embedElement->dispatchBeforeLoadEvent(url)) {
        // FIXME: beforeLoad could have detached the renderer!  Just like in the <object> case above.
        frame->loader()->subframeLoader()->requestObject(embedElement, url, embedElement->getAttribute(nameAttr), serviceType, paramNames, paramValues);
    }
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
// FIXME: This belongs on HTMLMediaElement.
static void updateWidgetForMediaElement(HTMLMediaElement* mediaElement, bool ignored)
{
    Vector<String> paramNames;
    Vector<String> paramValues;
    KURL kurl;

    mediaElement->getPluginProxyParams(kurl, paramNames, paramValues);
    mediaElement->setNeedWidgetUpdate(false);
    frame->loader()->subframeLoader()->loadMediaPlayerProxyPlugin(mediaElement, kurl, paramNames, paramValues);
}
#endif

void RenderEmbeddedObject::updateWidget(bool onlyCreateNonNetscapePlugins)
{
    if (!m_replacementText.isNull() || !node()) // Check the node in case destroy() has been called.
        return;

    // The calls to SubframeLoader::requestObject within this function can result in a plug-in being initialized.
    // This can run cause arbitrary JavaScript to run and may result in this RenderObject being detached from
    // the render tree and destroyed, causing a crash like <rdar://problem/6954546>.  By extending our lifetime
    // artifically to ensure that we remain alive for the duration of plug-in initialization.
    RenderWidgetProtector protector(this);

    if (node()->hasTagName(objectTag)) {
        HTMLObjectElement* objectElement = static_cast<HTMLObjectElement*>(node());
        updateWidgetForObjectElement(objectElement, onlyCreateNonNetscapePlugins);
    } else if (node()->hasTagName(embedTag)) {
        HTMLEmbedElement* embedElement = static_cast<HTMLEmbedElement*>(node());
        updateWidgetForEmbedElement(embedElement, onlyCreateNonNetscapePlugins);
    }
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)        
    else if (node()->hasTagName(videoTag) || node()->hasTagName(audioTag)) {
        HTMLMediaElement* mediaElement = static_cast<HTMLMediaElement*>(node());
        updateWidgetForMediaElement(mediaElement, onlyCreateNonNetscapePlugins);
    }
#endif
}

void RenderEmbeddedObject::setShowsMissingPluginIndicator()
{
    ASSERT(m_replacementText.isEmpty());
    m_replacementText = missingPluginText();
    m_showsMissingPluginIndicator = true;
}

void RenderEmbeddedObject::setShowsCrashedPluginIndicator()
{
    ASSERT(m_replacementText.isEmpty());
    m_replacementText = crashedPluginText();
}

void RenderEmbeddedObject::setMissingPluginIndicatorIsPressed(bool pressed)
{
    if (m_missingPluginIndicatorIsPressed == pressed)
        return;
    
    m_missingPluginIndicatorIsPressed = pressed;
    repaint();
}

void RenderEmbeddedObject::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (!m_replacementText.isNull()) {
        RenderReplaced::paint(paintInfo, tx, ty);
        return;
    }
    
    RenderPart::paint(paintInfo, tx, ty);
}

void RenderEmbeddedObject::paintReplaced(PaintInfo& paintInfo, int tx, int ty)
{
    if (!m_replacementText)
        return;

    if (paintInfo.phase == PaintPhaseSelection)
        return;
    
    GraphicsContext* context = paintInfo.context;
    if (context->paintingDisabled())
        return;
    
    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    Font font;
    TextRun run("");
    float textWidth;
    if (!getReplacementTextGeometry(tx, ty, contentRect, path, replacementTextRect, font, run, textWidth))
        return;
    
    context->save();
    context->clip(contentRect);
    context->beginPath();
    context->addPath(path);  
    context->setAlpha(m_missingPluginIndicatorIsPressed ? replacementTextPressedRoundedRectOpacity : replacementTextRoundedRectOpacity);
    context->setFillColor(m_missingPluginIndicatorIsPressed ? replacementTextRoundedRectPressedColor() : Color::white, style()->colorSpace());
    context->fillPath();

    float labelX = roundf(replacementTextRect.location().x() + (replacementTextRect.size().width() - textWidth) / 2);
    float labelY = roundf(replacementTextRect.location().y() + (replacementTextRect.size().height() - font.height()) / 2 + font.ascent());
    context->setAlpha(m_missingPluginIndicatorIsPressed ? replacementTextPressedTextOpacity : replacementTextTextOpacity);
    context->setFillColor(Color::black, style()->colorSpace());
    context->drawBidiText(font, run, FloatPoint(labelX, labelY));
    context->restore();
}

bool RenderEmbeddedObject::getReplacementTextGeometry(int tx, int ty, FloatRect& contentRect, Path& path, FloatRect& replacementTextRect, Font& font, TextRun& run, float& textWidth)
{
    contentRect = contentBoxRect();
    contentRect.move(tx, ty);
    
    FontDescription fontDescription;
    RenderTheme::defaultTheme()->systemFont(CSSValueWebkitSmallControl, fontDescription);
    fontDescription.setWeight(FontWeightBold);
    Settings* settings = document()->settings();
    ASSERT(settings);
    if (!settings)
        return false;
    fontDescription.setRenderingMode(settings->fontRenderingMode());
    fontDescription.setComputedSize(fontDescription.specifiedSize());
    font = Font(fontDescription, 0, 0);
    font.update(0);
    
    run = TextRun(m_replacementText.characters(), m_replacementText.length());
    run.disableRoundingHacks();
    textWidth = font.floatWidth(run);
    
    replacementTextRect.setSize(FloatSize(textWidth + replacementTextRoundedRectLeftRightTextMargin * 2, replacementTextRoundedRectHeight));
    float x = (contentRect.size().width() / 2 - replacementTextRect.size().width() / 2) + contentRect.location().x();
    float y = (contentRect.size().height() / 2 - replacementTextRect.size().height() / 2) + contentRect.location().y();
    replacementTextRect.setLocation(FloatPoint(x, y));
    
    path = Path::createRoundedRectangle(replacementTextRect, FloatSize(replacementTextRoundedRectRadius, replacementTextRoundedRectRadius));

    return true;
}

void RenderEmbeddedObject::layout()
{
    ASSERT(needsLayout());

    calcWidth();
    calcHeight();

    RenderPart::layout();

    m_overflow.clear();
    addShadowOverflow();

    if (!widget() && frameView())
        frameView()->addWidgetToUpdate(this);

    setNeedsLayout(false);
}

void RenderEmbeddedObject::viewCleared()
{
    // This is required for <object> elements whose contents are rendered by WebCore (e.g. src="foo.html").
    if (node() && widget() && widget()->isFrameView()) {
        FrameView* view = static_cast<FrameView*>(widget());
        int marginw = -1;
        int marginh = -1;
        if (node()->hasTagName(iframeTag)) {
            HTMLIFrameElement* frame = static_cast<HTMLIFrameElement*>(node());
            marginw = frame->getMarginWidth();
            marginh = frame->getMarginHeight();
        }
        if (marginw != -1)
            view->setMarginWidth(marginw);
        if (marginh != -1)
            view->setMarginHeight(marginh);
    }
}
 
bool RenderEmbeddedObject::isInMissingPluginIndicator(MouseEvent* event)
{
    FloatRect contentRect;
    Path path;
    FloatRect replacementTextRect;
    Font font;
    TextRun run("");
    float textWidth;
    if (!getReplacementTextGeometry(0, 0, contentRect, path, replacementTextRect, font, run, textWidth))
        return false;
    
    return path.contains(absoluteToLocal(event->absoluteLocation(), false, true));
}

void RenderEmbeddedObject::handleMissingPluginIndicatorEvent(Event* event)
{
    if (Page* page = document()->page()) {
        if (!page->chrome()->client()->shouldMissingPluginMessageBeButton())
            return;
    }
    
    if (!event->isMouseEvent())
        return;
    
    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    HTMLPlugInElement* element = static_cast<HTMLPlugInElement*>(node());
    if (event->type() == eventNames().mousedownEvent && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        m_mouseDownWasInMissingPluginIndicator = isInMissingPluginIndicator(mouseEvent);
        if (m_mouseDownWasInMissingPluginIndicator) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(element);
                element->setIsCapturingMouseEvents(true);
            }
            setMissingPluginIndicatorIsPressed(true);
        }
        event->setDefaultHandled();
    }        
    if (event->type() == eventNames().mouseupEvent && static_cast<MouseEvent*>(event)->button() == LeftButton) {
        if (m_missingPluginIndicatorIsPressed) {
            if (Frame* frame = document()->frame()) {
                frame->eventHandler()->setCapturingMouseEventsNode(0);
                element->setIsCapturingMouseEvents(false);
            }
            setMissingPluginIndicatorIsPressed(false);
        }
        if (m_mouseDownWasInMissingPluginIndicator && isInMissingPluginIndicator(mouseEvent)) {
            if (Page* page = document()->page())
                page->chrome()->client()->missingPluginButtonClicked(element);            
        }
        m_mouseDownWasInMissingPluginIndicator = false;
        event->setDefaultHandled();
    }
    if (event->type() == eventNames().mousemoveEvent) {
        setMissingPluginIndicatorIsPressed(m_mouseDownWasInMissingPluginIndicator && isInMissingPluginIndicator(mouseEvent));
        event->setDefaultHandled();
    }
}

}
