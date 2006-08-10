/*
 *  Copyright (C) 2006 Apple Computer, Inc.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef SVG_SUPPORT

#include "JSSVGElementWrapperFactory.h"

#include "JSSVGSVGElement.h"

#include "SVGNames.h"

#include "SVGSVGElement.h"

using namespace KJS;

// FIXME: Eventually this file should be autogenerated, just like SVGNames, SVGElementFactory, etc.

namespace WebCore {

using namespace SVGNames;

typedef DOMNode* (*CreateSVGElementWrapperFunction)(ExecState*, PassRefPtr<SVGElement>);

#define FOR_EACH_TAG(macro) \
    macro(svg, SVG) \
    // end of macro

#define CREATE_WRAPPER_FUNCTION(tag, name) \
static DOMNode* create##name##Wrapper(ExecState* exec, PassRefPtr<SVGElement> element) \
{ \
    return new JSSVG##name##Element(exec, static_cast<SVG##name##Element*>(element.get())); \
}
FOR_EACH_TAG(CREATE_WRAPPER_FUNCTION)
#undef CREATE_WRAPPER_FUNCTION

DOMNode* createJSSVGWrapper(ExecState* exec, PassRefPtr<SVGElement> element)
{
    static HashMap<WebCore::AtomicStringImpl*, CreateSVGElementWrapperFunction> map;
    if (map.isEmpty()) {
#define ADD_TO_HASH_MAP(tag, name) map.set(tag##Tag.localName().impl(), create##name##Wrapper);
FOR_EACH_TAG(ADD_TO_HASH_MAP)
#undef ADD_TO_HASH_MAP
    }
    CreateSVGElementWrapperFunction createWrapperFunction = map.get(element->localName().impl());
    if (createWrapperFunction)
        return createWrapperFunction(exec, element);
    return new JSSVGElement(exec, element.get());
}

}

#endif // SVG_SUPPORT
