/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005, 2006 Rob Buis <buis@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KSVG_SVGLinearGradientElementImpl_H
#define KSVG_SVGLinearGradientElementImpl_H
#ifdef SVG_SUPPORT

#include <SVGGradientElement.h>

namespace WebCore
{
    class SVGLength;
    class SVGLinearGradientElement : public SVGGradientElement
    {
    public:
        SVGLinearGradientElement(const QualifiedName&, Document*);
        virtual ~SVGLinearGradientElement();

        // 'SVGLinearGradientElement' functions
        virtual void parseMappedAttribute(MappedAttribute*);

    protected:
        virtual void buildGradient(PassRefPtr<SVGPaintServerGradient>) const;
        virtual SVGPaintServerType gradientType() const { return PS_LINEAR_GRADIENT; }

    protected:
        virtual const SVGElement* contextElement() const { return this; }

    private:
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGLength*, RefPtr<SVGLength>, X1, x1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGLength*, RefPtr<SVGLength>, Y1, y1)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGLength*, RefPtr<SVGLength>, X2, x2)
        ANIMATED_PROPERTY_DECLARATIONS(SVGLinearGradientElement, SVGLength*, RefPtr<SVGLength>, Y2, y2)
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
