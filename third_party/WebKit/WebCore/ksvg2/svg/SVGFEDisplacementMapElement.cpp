/*
 Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 
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
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.
 */


#include "config.h"
#ifdef SVG_SUPPORT
#include "DeprecatedStringList.h"

#include "Attr.h"

#include <kcanvas/KCanvasFilters.h>
#include <kcanvas/device/KRenderingDevice.h>

#include "ksvg.h"
#include "SVGHelper.h"
#include "SVGRenderStyle.h"
#include "SVGFEDisplacementMapElement.h"

using namespace WebCore;

SVGFEDisplacementMapElement::SVGFEDisplacementMapElement(const QualifiedName& tagName, Document* doc)
    : SVGFilterPrimitiveStandardAttributes(tagName, doc)
    , m_xChannelSelector(0)
    , m_yChannelSelector(0)
    , m_scale(0.0)
{
    m_filterEffect = 0;
}

SVGFEDisplacementMapElement::~SVGFEDisplacementMapElement()
{
    delete m_filterEffect;
}

ANIMATED_PROPERTY_DEFINITIONS(SVGFEDisplacementMapElement, String, String, string, In, in, SVGNames::inAttr.localName(), m_in)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDisplacementMapElement, String, String, string, In2, in2, SVGNames::in2Attr.localName(), m_in2)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDisplacementMapElement, int, Enumeration, enumeration, XChannelSelector, xChannelSelector, SVGNames::xChannelSelectorAttr.localName(), m_xChannelSelector)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDisplacementMapElement, int, Enumeration, enumeration, YChannelSelector, yChannelSelector, SVGNames::yChannelSelectorAttr.localName(), m_yChannelSelector)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDisplacementMapElement, double, Number, number, Scale, scale, SVGNames::scaleAttr.localName(), m_scale)

KCChannelSelectorType SVGFEDisplacementMapElement::stringToChannel(const String& key)
{
    if(key == "R")
        return CS_RED;
    else if(key == "G")
        return CS_GREEN;
    else if(key == "B")
        return CS_BLUE;
    else if(key == "A")
        return CS_ALPHA;
    //error
    return (KCChannelSelectorType)-1;
}

void SVGFEDisplacementMapElement::parseMappedAttribute(MappedAttribute* attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::xChannelSelectorAttr)
        setXChannelSelectorBaseValue(stringToChannel(value));
    else if (attr->name() == SVGNames::yChannelSelectorAttr)
        setYChannelSelectorBaseValue(stringToChannel(value));
    else if (attr->name() == SVGNames::inAttr)
        setInBaseValue(value);
    else if (attr->name() == SVGNames::in2Attr)
        setIn2BaseValue(value);
    else if (attr->name() == SVGNames::scaleAttr)
        setScaleBaseValue(value.deprecatedString().toDouble());
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

KCanvasFEDisplacementMap* SVGFEDisplacementMapElement::filterEffect() const
{
    if (!m_filterEffect)
        m_filterEffect = static_cast<KCanvasFEDisplacementMap *>(renderingDevice()->createFilterEffect(FE_DISPLACEMENT_MAP));
    if (!m_filterEffect)
        return 0;
    m_filterEffect->setXChannelSelector((KCChannelSelectorType)(xChannelSelector()));
    m_filterEffect->setYChannelSelector((KCChannelSelectorType)(yChannelSelector()));
    m_filterEffect->setIn(in().deprecatedString());
    m_filterEffect->setIn2(String(in2()).deprecatedString());
    m_filterEffect->setScale(scale());
    setStandardAttributes(m_filterEffect);
    return m_filterEffect;
}
#endif // SVG_SUPPORT
