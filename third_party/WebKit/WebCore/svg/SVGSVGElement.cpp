/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2006, 2007, 2008 Rob Buis <buis@kde.org>
                  2007 Apple Inc.  All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(SVG)
#include "SVGSVGElement.h"

#include "AffineTransform.h"
#include "CSSHelper.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "EventListener.h"
#include "EventNames.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "Frame.h"
#include "HTMLNames.h"
#include "MappedAttribute.h"
#include "RenderSVGRoot.h"
#include "RenderSVGViewportContainer.h"
#include "SMILTimeContainer.h"
#include "SVGAngle.h"
#include "SVGLength.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGTransform.h"
#include "SVGTransformList.h"
#include "SVGViewElement.h"
#include "SVGViewSpec.h"
#include "SVGZoomEvent.h"
#include "ScriptEventListener.h"
#include "SelectionController.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;
using namespace SVGNames;

SVGSVGElement::SVGSVGElement(const QualifiedName& tagName, Document* doc)
    : SVGStyledLocatableElement(tagName, doc)
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , SVGFitToViewBox()
    , SVGZoomAndPan()
    , m_x(LengthModeWidth)
    , m_y(LengthModeHeight)
    , m_width(LengthModeWidth, "100%")
    , m_height(LengthModeHeight, "100%") 
    , m_useCurrentView(false)
    , m_timeContainer(SMILTimeContainer::create(this))
    , m_scale(1)
    , m_viewSpec(0)
    , m_containerSize(300, 150)
    , m_hasSetContainerSize(false)
{
    doc->registerForDocumentActivationCallbacks(this);
}

SVGSVGElement::~SVGSVGElement()
{
    document()->unregisterForDocumentActivationCallbacks(this);
    // There are cases where removedFromDocument() is not called.
    // see ContainerNode::removeAllChildren, called by its destructor.
    document()->accessSVGExtensions()->removeTimeContainer(this);
}

const AtomicString& SVGSVGElement::contentScriptType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultValue, ("text/ecmascript"));
    const AtomicString& n = getAttribute(contentScriptTypeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGSVGElement::setContentScriptType(const AtomicString& type)
{
    setAttribute(SVGNames::contentScriptTypeAttr, type);
}

const AtomicString& SVGSVGElement::contentStyleType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, defaultValue, ("text/css"));
    const AtomicString& n = getAttribute(contentStyleTypeAttr);
    return n.isNull() ? defaultValue : n;
}

void SVGSVGElement::setContentStyleType(const AtomicString& type)
{
    setAttribute(SVGNames::contentStyleTypeAttr, type);
}

FloatRect SVGSVGElement::viewport() const
{
    double _x = 0.0;
    double _y = 0.0;
    if (!isOutermostSVG()) {
        _x = x().value(this);
        _y = y().value(this);
    }
    float w = width().value(this);
    float h = height().value(this);
    AffineTransform viewBox = viewBoxToViewTransform(w, h);
    double wDouble = w;
    double hDouble = h;
    viewBox.map(_x, _y, _x, _y);
    viewBox.map(w, h, wDouble, hDouble);
    return FloatRect::narrowPrecision(_x, _y, wDouble, hDouble);
}

int SVGSVGElement::relativeWidthValue() const
{
    SVGLength w = width();
    if (w.unitType() != LengthTypePercentage)
        return 0;

    return static_cast<int>(w.valueAsPercentage() * m_containerSize.width());
}

int SVGSVGElement::relativeHeightValue() const
{
    SVGLength h = height();
    if (h.unitType() != LengthTypePercentage)
        return 0;

    return static_cast<int>(h.valueAsPercentage() * m_containerSize.height());
}

float SVGSVGElement::pixelUnitToMillimeterX() const
{
    // 2.54 / cssPixelsPerInch gives CM.
    return (2.54f / cssPixelsPerInch) * 10.0f;
}

float SVGSVGElement::pixelUnitToMillimeterY() const
{
    // 2.54 / cssPixelsPerInch gives CM.
    return (2.54f / cssPixelsPerInch) * 10.0f;
}

float SVGSVGElement::screenPixelToMillimeterX() const
{
    return pixelUnitToMillimeterX();
}

float SVGSVGElement::screenPixelToMillimeterY() const
{
    return pixelUnitToMillimeterY();
}

bool SVGSVGElement::useCurrentView() const
{
    return m_useCurrentView;
}

void SVGSVGElement::setUseCurrentView(bool currentView)
{
    m_useCurrentView = currentView;
}

SVGViewSpec* SVGSVGElement::currentView() const
{
    if (!m_viewSpec)
        m_viewSpec.set(new SVGViewSpec(this));

    return m_viewSpec.get();
}

float SVGSVGElement::currentScale() const
{
    // Only the page zoom factor is relevant for SVG
    if (Frame* frame = document()->frame())
        return frame->pageZoomFactor();
    return m_scale;
}

void SVGSVGElement::setCurrentScale(float scale)
{
    if (Frame* frame = document()->frame()) {
        // Calling setCurrentScale() on the outermost <svg> element in a standalone SVG document
        // is allowed to change the page zoom factor, influencing the document size, scrollbars etc.
        if (parentNode() == document())
            frame->setZoomFactor(scale, ZoomPage);
        return;
    }

    m_scale = scale;
    if (renderer())
        renderer()->setNeedsLayout(true);
}

FloatPoint SVGSVGElement::currentTranslate() const
{
    return m_translation;
}

void SVGSVGElement::setCurrentTranslate(const FloatPoint &translation)
{
    m_translation = translation;
    if (parentNode() == document() && document()->renderer())
        document()->renderer()->repaint();
}

void SVGSVGElement::parseMappedAttribute(MappedAttribute* attr)
{
    if (!nearestViewportElement()) {
        bool setListener = true;

        // Only handle events if we're the outermost <svg> element
        if (attr->name() == onunloadAttr)
            document()->setWindowAttributeEventListener(eventNames().unloadEvent, createAttributeEventListener(document()->frame(), attr));
        else if (attr->name() == onresizeAttr)
            document()->setWindowAttributeEventListener(eventNames().resizeEvent, createAttributeEventListener(document()->frame(), attr));
        else if (attr->name() == onscrollAttr)
            document()->setWindowAttributeEventListener(eventNames().scrollEvent, createAttributeEventListener(document()->frame(), attr));
        else if (attr->name() == SVGNames::onzoomAttr)
            document()->setWindowAttributeEventListener(eventNames().zoomEvent, createAttributeEventListener(document()->frame(), attr));
        else
            setListener = false;
 
        if (setListener)
            return;
    }

    if (attr->name() == onabortAttr)
        document()->setWindowAttributeEventListener(eventNames().abortEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == onerrorAttr)
        document()->setWindowAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(document()->frame(), attr));
    else if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(LengthModeWidth, attr->value()));
        addCSSProperty(attr, CSSPropertyWidth, attr->value());
        if (widthBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for svg attribute <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(LengthModeHeight, attr->value()));
        addCSSProperty(attr, CSSPropertyHeight, attr->value());
        if (heightBaseValue().value(this) < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for svg attribute <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        if (SVGFitToViewBox::parseMappedAttribute(document(), attr))
            return;
        if (SVGZoomAndPan::parseMappedAttribute(attr))
            return;

        SVGStyledLocatableElement::parseMappedAttribute(attr);
    }
}

// This hack will not handle the case where we're setting a width/height
// on a root <svg> via svg.width.baseValue = when it has none.
static void updateCSSForAttribute(SVGSVGElement* element, const QualifiedName& attrName, CSSPropertyID property, const SVGLength& value)
{
    Attribute* attribute = element->attributes(false)->getAttributeItem(attrName);
    if (!attribute || !attribute->isMappedAttribute())
        return;
    element->addCSSProperty(static_cast<MappedAttribute*>(attribute), property, value.valueAsString());
}

void SVGSVGElement::svgAttributeChanged(const QualifiedName& attrName)
{
    SVGStyledElement::svgAttributeChanged(attrName);

    // FIXME: Ugly, ugly hack to around that parseMappedAttribute is not called
    // when svg.width.baseValue = 100 is evaluated.
    // Thus the CSS length value for width is not updated, and width() calcWidth()
    // calculations on RenderSVGRoot will be wrong.
    // https://bugs.webkit.org/show_bug.cgi?id=25387
    if (attrName == SVGNames::widthAttr)
        updateCSSForAttribute(this, attrName, CSSPropertyWidth, widthBaseValue());
    else if (attrName == SVGNames::heightAttr)
        updateCSSForAttribute(this, attrName, CSSPropertyHeight, heightBaseValue());

    if (!renderer())
        return;

    if (attrName == SVGNames::xAttr || attrName == SVGNames::yAttr ||
        SVGTests::isKnownAttribute(attrName) ||
        SVGLangSpace::isKnownAttribute(attrName) ||
        SVGExternalResourcesRequired::isKnownAttribute(attrName) ||
        SVGFitToViewBox::isKnownAttribute(attrName) ||
        SVGZoomAndPan::isKnownAttribute(attrName) ||
        SVGStyledLocatableElement::isKnownAttribute(attrName))
        renderer()->setNeedsLayout(true);
}

void SVGSVGElement::synchronizeProperty(const QualifiedName& attrName)
{
    SVGStyledElement::synchronizeProperty(attrName);

    if (attrName == anyQName()) {
        synchronizeX();
        synchronizeY();
        synchronizeWidth();
        synchronizeHeight();
        synchronizeExternalResourcesRequired();
        synchronizeViewBox();
        synchronizePreserveAspectRatio();
        return;
    }

    if (attrName == SVGNames::xAttr)
        synchronizeX();
    else if (attrName == SVGNames::yAttr)
        synchronizeY();
    else if (attrName == SVGNames::widthAttr)
        synchronizeWidth();
    else if (attrName == SVGNames::heightAttr)
        synchronizeHeight();
    else if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        synchronizeExternalResourcesRequired();
    else if (SVGFitToViewBox::isKnownAttribute(attrName)) {
        synchronizeViewBox();
        synchronizePreserveAspectRatio();
    }
}

unsigned SVGSVGElement::suspendRedraw(unsigned /* maxWaitMilliseconds */)
{
    // FIXME: Implement me (see bug 11275)
    return 0;
}

void SVGSVGElement::unsuspendRedraw(unsigned /* suspendHandleId */)
{
    // FIXME: Implement me (see bug 11275)
}

void SVGSVGElement::unsuspendRedrawAll()
{
    // FIXME: Implement me (see bug 11275)
}

void SVGSVGElement::forceRedraw()
{
    // FIXME: Implement me (see bug 11275)
}

NodeList* SVGSVGElement::getIntersectionList(const FloatRect&, SVGElement*)
{
    // FIXME: Implement me (see bug 11274)
    return 0;
}

NodeList* SVGSVGElement::getEnclosureList(const FloatRect&, SVGElement*)
{
    // FIXME: Implement me (see bug 11274)
    return 0;
}

bool SVGSVGElement::checkIntersection(SVGElement*, const FloatRect& rect)
{
    // TODO : take into account pointer-events?
    // FIXME: Why is element ignored??
    // FIXME: Implement me (see bug 11274)
    return rect.intersects(getBBox());
}

bool SVGSVGElement::checkEnclosure(SVGElement*, const FloatRect& rect)
{
    // TODO : take into account pointer-events?
    // FIXME: Why is element ignored??
    // FIXME: Implement me (see bug 11274)
    return rect.contains(getBBox());
}

void SVGSVGElement::deselectAll()
{
    document()->frame()->selection()->clear();
}

float SVGSVGElement::createSVGNumber()
{
    return 0.0f;
}

SVGLength SVGSVGElement::createSVGLength()
{
    return SVGLength();
}

SVGAngle SVGSVGElement::createSVGAngle()
{
    return SVGAngle();
}

FloatPoint SVGSVGElement::createSVGPoint()
{
    return FloatPoint();
}

AffineTransform SVGSVGElement::createSVGMatrix()
{
    return AffineTransform();
}

FloatRect SVGSVGElement::createSVGRect()
{
    return FloatRect();
}

SVGTransform SVGSVGElement::createSVGTransform()
{
    return SVGTransform();
}

SVGTransform SVGSVGElement::createSVGTransformFromMatrix(const AffineTransform& matrix)
{
    return SVGTransform(matrix);
}

AffineTransform SVGSVGElement::getCTM() const
{
    AffineTransform mat;
    if (!isOutermostSVG())
        mat.translate(x().value(this), y().value(this));

    if (attributes()->getAttributeItem(SVGNames::viewBoxAttr)) {
        AffineTransform viewBox = viewBoxToViewTransform(width().value(this), height().value(this));
        mat = viewBox * mat;
    }

    return mat;
}

AffineTransform SVGSVGElement::getScreenCTM() const
{
    document()->updateLayoutIgnorePendingStylesheets();
    FloatPoint rootLocation;

    if (RenderObject* renderer = this->renderer()) {
        if (isOutermostSVG()) {
            // FIXME: This doesn't work correctly with CSS transforms.
            FloatPoint point;
            if (renderer->parent())
                point = renderer->localToAbsolute(point, false, true);
            rootLocation.move(point.x(), point.y());
        } else
            rootLocation.move(x().value(this), y().value(this));
    }
    
    AffineTransform mat = SVGStyledLocatableElement::getScreenCTM();
    mat.translate(rootLocation.x(), rootLocation.y());

    if (attributes()->getAttributeItem(SVGNames::viewBoxAttr)) {
        AffineTransform viewBox = viewBoxToViewTransform(width().value(this), height().value(this));
        mat = viewBox * mat;
    }

    return mat;
}

RenderObject* SVGSVGElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    if (isOutermostSVG())
        return new (arena) RenderSVGRoot(this);
    else
        return new (arena) RenderSVGViewportContainer(this);
}

void SVGSVGElement::insertedIntoDocument()
{
    document()->accessSVGExtensions()->addTimeContainer(this);
    SVGStyledLocatableElement::insertedIntoDocument();
}

void SVGSVGElement::removedFromDocument()
{
    document()->accessSVGExtensions()->removeTimeContainer(this);
    SVGStyledLocatableElement::removedFromDocument();
}

void SVGSVGElement::pauseAnimations()
{
    if (!m_timeContainer->isPaused())
        m_timeContainer->pause();
}

void SVGSVGElement::unpauseAnimations()
{
    if (m_timeContainer->isPaused())
        m_timeContainer->resume();
}

bool SVGSVGElement::animationsPaused() const
{
    return m_timeContainer->isPaused();
}

float SVGSVGElement::getCurrentTime() const
{
    return narrowPrecisionToFloat(m_timeContainer->elapsed().value());
}

void SVGSVGElement::setCurrentTime(float /* seconds */)
{
    // FIXME: Implement me, bug 12073
}

bool SVGSVGElement::hasRelativeValues() const
{
    return (x().isRelative() || width().isRelative() ||
            y().isRelative() || height().isRelative());
}

bool SVGSVGElement::isOutermostSVG() const
{
    // Element may not be in the document, pretend we're outermost for viewport(), getCTM(), etc.
    if (!parentNode())
        return true;

    // This is true whenever this is the outermost SVG, even if there are HTML elements outside it
    return !parentNode()->isSVGElement();
}

AffineTransform SVGSVGElement::viewBoxToViewTransform(float viewWidth, float viewHeight) const
{
    FloatRect viewBoxRect;
    if (useCurrentView()) {
        if (currentView()) // what if we should use it but it is not set?
            viewBoxRect = currentView()->viewBox();
    } else
        viewBoxRect = viewBox();

    AffineTransform ctm = SVGFitToViewBox::viewBoxToViewTransform(viewBoxRect, preserveAspectRatio(), viewWidth, viewHeight);
    if (useCurrentView() && currentView())
        return currentView()->transform()->concatenate().matrix() * ctm;

    return ctm;
}

void SVGSVGElement::inheritViewAttributes(SVGViewElement* viewElement)
{
    setUseCurrentView(true);
    if (viewElement->hasAttribute(SVGNames::viewBoxAttr))
        currentView()->setViewBox(viewElement->viewBox());
    else
        currentView()->setViewBox(viewBox());

    SVGPreserveAspectRatio aspectRatio;
    if (viewElement->hasAttribute(SVGNames::preserveAspectRatioAttr))
        aspectRatio = viewElement->preserveAspectRatioBaseValue();
    else
        aspectRatio = preserveAspectRatioBaseValue();
    currentView()->setPreserveAspectRatioBaseValue(aspectRatio);

    if (viewElement->hasAttribute(SVGNames::zoomAndPanAttr))
        currentView()->setZoomAndPan(viewElement->zoomAndPan());
    renderer()->setNeedsLayout(true);
}
    
void SVGSVGElement::documentWillBecomeInactive()
{
    pauseAnimations();
}

void SVGSVGElement::documentDidBecomeActive()
{
    unpauseAnimations();
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
