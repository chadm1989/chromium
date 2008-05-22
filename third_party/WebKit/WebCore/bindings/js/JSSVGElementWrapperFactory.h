/*
 *  Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef JSSVGElementWrapperFactory_h
#define JSSVGElementWrapperFactory_h

#if ENABLE(SVG)

#include <wtf/Forward.h>

namespace KJS {
    class ExecState;
}

namespace WebCore {

    class JSNode;
    class SVGElement;

    JSNode* createJSSVGWrapper(KJS::ExecState*, PassRefPtr<SVGElement>);

}

#endif // ENABLE(SVG)

#endif // JSSVGElementWrapperFactory_h
