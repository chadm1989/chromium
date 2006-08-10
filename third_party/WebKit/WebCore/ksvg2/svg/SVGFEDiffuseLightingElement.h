/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Oliver Hunt <ojh16@student.canterbury.ac.nz>

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

#ifndef KSVG_SVGFEDiffuseLightingElementImpl_H
#define KSVG_SVGFEDiffuseLightingElementImpl_H
#ifdef SVG_SUPPORT

#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    class KCanvasFEDiffuseLighting;
    class SVGAnimatedNumber;
    class SVGAnimatedString;
    class SVGAnimatedColor;
    
    class SVGFEDiffuseLightingElement : public SVGFilterPrimitiveStandardAttributes
    {
    public:
        SVGFEDiffuseLightingElement(const QualifiedName&, Document*);
        virtual ~SVGFEDiffuseLightingElement();

        // 'SVGFEDiffuseLightingElement' functions
        SVGAnimatedString *in1() const;
        SVGAnimatedNumber *diffuseConstant() const;
        SVGAnimatedNumber *surfaceScale() const;
        SVGAnimatedNumber *kernelUnitLengthX() const;
        SVGAnimatedNumber *kernelUnitLengthY() const;
        SVGAnimatedColor  *lightingColor() const;

        // Derived from: 'Element'
        virtual void parseMappedAttribute(MappedAttribute *attr);

        virtual KCanvasFEDiffuseLighting *filterEffect() const;

    private:
        mutable RefPtr<SVGAnimatedString> m_in1;
        mutable RefPtr<SVGAnimatedNumber> m_diffuseConstant;
        mutable RefPtr<SVGAnimatedNumber> m_surfaceScale;
        mutable RefPtr<SVGAnimatedColor>  m_lightingColor;
        mutable RefPtr<SVGAnimatedNumber> m_kernelUnitLengthX;
        mutable RefPtr<SVGAnimatedNumber> m_kernelUnitLengthY;
        //need other properties here...
        mutable KCanvasFEDiffuseLighting *m_filterEffect;
        
        //light management
        void updateLights() const;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif
