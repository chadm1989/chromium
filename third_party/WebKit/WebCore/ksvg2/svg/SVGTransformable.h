/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

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

#ifndef KSVG_SVGTransformableImpl_H
#define KSVG_SVGTransformableImpl_H
#ifdef SVG_SUPPORT

#include "SVGLocatable.h"

namespace WebCore {
    
    class AffineTransform;
    class AtomicString;
    class Attribute;
    class Node;
    class SVGAnimatedTransformList;
    class SVGMatrix;
    class SVGTransformList;

    class SVGTransformable : public SVGLocatable {
    public:
        SVGTransformable();
        virtual ~SVGTransformable();

        // 'SVGTransformable' functions
        virtual SVGAnimatedTransformList* transform() const = 0;
        virtual SVGMatrix* localMatrix() const = 0;
        
        virtual void updateLocalTransform(SVGTransformList*) = 0;
        
        static void parseTransformAttribute(SVGTransformList*, const AtomicString& transform);
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif // KSVG_SVGTransformableImpl_H

// vim:ts=4:noet
