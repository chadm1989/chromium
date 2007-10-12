/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>

    This file is part of the KDE project

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
#include "SVGRectElement.h"

#include "SVGLength.h"
#include "SVGNames.h"

namespace WebCore {

SVGRectElement::SVGRectElement(const QualifiedName& tagName, Document *doc)
    : SVGStyledTransformableElement(tagName, doc)
    , SVGTests()
    , SVGLangSpace()
    , SVGExternalResourcesRequired()
    , m_x(this, LengthModeWidth)
    , m_y(this, LengthModeHeight)
    , m_width(this, LengthModeWidth)
    , m_height(this, LengthModeHeight)
    , m_rx(this, LengthModeWidth)
    , m_ry(this, LengthModeHeight)
{
}

SVGRectElement::~SVGRectElement()
{
}

ANIMATED_PROPERTY_DEFINITIONS(SVGRectElement, SVGLength, Length, length, X, x, SVGNames::xAttr.localName(), m_x)
ANIMATED_PROPERTY_DEFINITIONS(SVGRectElement, SVGLength, Length, length, Y, y, SVGNames::yAttr.localName(), m_y)
ANIMATED_PROPERTY_DEFINITIONS(SVGRectElement, SVGLength, Length, length, Width, width, SVGNames::widthAttr.localName(), m_width)
ANIMATED_PROPERTY_DEFINITIONS(SVGRectElement, SVGLength, Length, length, Height, height, SVGNames::heightAttr.localName(), m_height)
ANIMATED_PROPERTY_DEFINITIONS(SVGRectElement, SVGLength, Length, length, Rx, rx, SVGNames::rxAttr.localName(), m_rx)
ANIMATED_PROPERTY_DEFINITIONS(SVGRectElement, SVGLength, Length, length, Ry, ry, SVGNames::ryAttr.localName(), m_ry)

void SVGRectElement::parseMappedAttribute(MappedAttribute* attr)
{
    if (attr->name() == SVGNames::xAttr)
        setXBaseValue(SVGLength(this, LengthModeWidth, attr->value()));
    else if (attr->name() == SVGNames::yAttr)
        setYBaseValue(SVGLength(this, LengthModeHeight, attr->value()));
    else if (attr->name() == SVGNames::rxAttr) {
        setRxBaseValue(SVGLength(this, LengthModeWidth, attr->value()));
        if (rx().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <rx> is not allowed");
    } else if (attr->name() == SVGNames::ryAttr) {
        setRyBaseValue(SVGLength(this, LengthModeHeight, attr->value()));
        if (ry().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <ry> is not allowed");
    } else if (attr->name() == SVGNames::widthAttr) {
        setWidthBaseValue(SVGLength(this, LengthModeWidth, attr->value()));
        if (width().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <width> is not allowed");
    } else if (attr->name() == SVGNames::heightAttr) {
        setHeightBaseValue(SVGLength(this, LengthModeHeight, attr->value()));
        if (height().value() < 0.0)
            document()->accessSVGExtensions()->reportError("A negative value for rect <height> is not allowed");
    } else {
        if (SVGTests::parseMappedAttribute(attr))
            return;
        if (SVGLangSpace::parseMappedAttribute(attr))
            return;
        if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
            return;
        SVGStyledTransformableElement::parseMappedAttribute(attr);
    }
}

void SVGRectElement::notifyAttributeChange() const
{
    if (!document()->parsing())
        rebuildRenderer();

    SVGStyledTransformableElement::notifyAttributeChange();
}

Path SVGRectElement::toPathData() const
{
    FloatRect rect(x().value(), y().value(), width().value(), height().value());

    bool hasRx = hasAttribute(SVGNames::rxAttr);
    bool hasRy = hasAttribute(SVGNames::ryAttr);
    if (hasRx || hasRy) {
        float _rx = hasRx ? rx().value() : ry().value();
        float _ry = hasRy ? ry().value() : rx().value();
        return Path::createRoundedRectangle(rect, FloatSize(_rx, _ry));
    }

    return Path::createRectangle(rect);
}

bool SVGRectElement::hasRelativeValues() const
{
    return (x().isRelative() || width().isRelative() ||
            y().isRelative() || height().isRelative());
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
