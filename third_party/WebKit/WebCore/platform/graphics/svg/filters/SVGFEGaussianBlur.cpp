/*
    Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>
                  2005 Eric Seidel <eric.seidel@kdemail.net>

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
    aint with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"

#if ENABLE(SVG) && ENABLE(SVG_EXPERIMENTAL_FEATURES)
#include "SVGFEGaussianBlur.h"
#include "TextStream.h"

namespace WebCore {

float SVGFEGaussianBlur::stdDeviationX() const
{
    return m_x;
}

void SVGFEGaussianBlur::setStdDeviationX(float x)
{
    m_x = x;
}

float SVGFEGaussianBlur::stdDeviationY() const
{
    return m_y;
}

void SVGFEGaussianBlur::setStdDeviationY(float y)
{
    m_y = y;
}

TextStream& SVGFEGaussianBlur::externalRepresentation(TextStream& ts) const
{
    ts << "[type=GAUSSIAN-BLUR] ";
    SVGFilterEffect::externalRepresentation(ts);
    ts << " [std dev. x=" << stdDeviationX() << " y=" << stdDeviationY() << "]";
    return ts;
}

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(SVG_EXPERIMENTAL_FEATURES)
