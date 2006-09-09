/*
     Copyright (C) 2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>
     
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
#include "SVGNames.h"
#include "SVGHelper.h"
#include "SVGRenderStyle.h"
#include "SVGColor.h"
#include "SVGFELightElement.h"
#include "SVGFEDiffuseLightingElement.h"

namespace WebCore {

SVGFEDiffuseLightingElement::SVGFEDiffuseLightingElement(const QualifiedName& tagName, Document *doc)
    : SVGFilterPrimitiveStandardAttributes(tagName, doc)
    , m_diffuseConstant(0.0)
    , m_surfaceScale(0.0)
    , m_lightingColor(new SVGColor())
    , m_kernelUnitLengthX(0.0)
    , m_kernelUnitLengthY(0.0)
{
    m_filterEffect = 0;
}

SVGFEDiffuseLightingElement::~SVGFEDiffuseLightingElement()
{
    delete m_filterEffect;
}

ANIMATED_PROPERTY_DEFINITIONS(SVGFEDiffuseLightingElement, String, String, string, In, in, SVGNames::inAttr.localName(), m_in)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDiffuseLightingElement, double, Number, number, DiffuseConstant, diffuseConstant, SVGNames::diffuseConstantAttr.localName(), m_diffuseConstant)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDiffuseLightingElement, double, Number, number, SurfaceScale, surfaceScale, SVGNames::surfaceScaleAttr.localName(), m_surfaceScale)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDiffuseLightingElement, double, Number, number, KernelUnitLengthX, kernelUnitLengthX, "kernelUnitLengthX", m_kernelUnitLengthX)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDiffuseLightingElement, double, Number, number, KernelUnitLengthY, kernelUnitLengthY, "kernelUnitLengthY", m_kernelUnitLengthY)
ANIMATED_PROPERTY_DEFINITIONS(SVGFEDiffuseLightingElement, SVGColor*, Color, color, LightingColor, lightingColor, SVGNames::lighting_colorAttr.localName(), m_lightingColor.get())

void SVGFEDiffuseLightingElement::parseMappedAttribute(MappedAttribute *attr)
{
    const String& value = attr->value();
    if (attr->name() == SVGNames::inAttr)
        setInBaseValue(value);
    else if (attr->name() == SVGNames::surfaceScaleAttr)
        setSurfaceScaleBaseValue(value.deprecatedString().toDouble());
    else if (attr->name() == SVGNames::diffuseConstantAttr)
        setDiffuseConstantBaseValue(value.toInt());
    else if (attr->name() == SVGNames::kernelUnitLengthAttr) {
        DeprecatedStringList numbers = DeprecatedStringList::split(' ', value.deprecatedString());
        setKernelUnitLengthXBaseValue(numbers[0].toDouble());
        if (numbers.count() == 1)
            setKernelUnitLengthYBaseValue(numbers[0].toDouble());
        else
            setKernelUnitLengthYBaseValue(numbers[1].toDouble());
    } else if (attr->name() == SVGNames::lighting_colorAttr)
        setLightingColorBaseValue(new SVGColor(value));
    else
        SVGFilterPrimitiveStandardAttributes::parseMappedAttribute(attr);
}

KCanvasFEDiffuseLighting *SVGFEDiffuseLightingElement::filterEffect() const
{
    if (!m_filterEffect) 
        m_filterEffect = static_cast<KCanvasFEDiffuseLighting *>(renderingDevice()->createFilterEffect(FE_DIFFUSE_LIGHTING));
    m_filterEffect->setIn(in().deprecatedString());
    setStandardAttributes(m_filterEffect);
    m_filterEffect->setDiffuseConstant((diffuseConstant()));
    m_filterEffect->setSurfaceScale((surfaceScale()));
    m_filterEffect->setKernelUnitLengthX((kernelUnitLengthX()));
    m_filterEffect->setKernelUnitLengthY((kernelUnitLengthY()));
    m_filterEffect->setLightingColor(lightingColor()->color());
    updateLights();
    return m_filterEffect;
}

void SVGFEDiffuseLightingElement::updateLights() const
{
    if (!m_filterEffect)
        return;
    
    KCLightSource *light = 0;
    for (Node *n = firstChild(); n; n = n->nextSibling()) {
        if (n->hasTagName(SVGNames::feDistantLightTag)||n->hasTagName(SVGNames::fePointLightTag)||n->hasTagName(SVGNames::feSpotLightTag)) {
            SVGFELightElement *lightNode = static_cast<SVGFELightElement *>(n); 
            light = lightNode->lightSource();
            break;
        }
    }
    m_filterEffect->setLightSource(light);
}

}

#endif // SVG_SUPPORT

