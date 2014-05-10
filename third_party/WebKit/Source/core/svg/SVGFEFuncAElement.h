/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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
 */

#ifndef SVGFEFuncAElement_h
#define SVGFEFuncAElement_h

#include "SVGNames.h"
#include "core/svg/SVGComponentTransferFunctionElement.h"

namespace WebCore {

class SVGFEFuncAElement FINAL : public SVGComponentTransferFunctionElement {
public:
    DEFINE_NODE_FACTORY(SVGFEFuncAElement);

private:
    explicit SVGFEFuncAElement(Document&);
};

} // namespace WebCore

#endif
