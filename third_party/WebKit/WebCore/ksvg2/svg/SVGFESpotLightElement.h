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

#ifndef SVGFESpotLightElement_h
#define SVGFESpotLightElement_h
#if ENABLE(SVG) && ENABLE(SVG_EXPERIMENTAL_FEATURES)

#include "SVGFELightElement.h"

namespace WebCore
{
    class SVGFESpotLightElement : public SVGFELightElement
    {
    public:
        SVGFESpotLightElement(const QualifiedName&, Document*);
        virtual ~SVGFESpotLightElement();
        virtual SVGLightSource *lightSource() const;
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
