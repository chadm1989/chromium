/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSStyleSheetList.h"

#include "HTMLStyleElement.h"
#include "JSStyleSheet.h"
#include "StyleSheet.h"
#include "StyleSheetList.h"

using namespace JSC;

namespace WebCore {

bool JSStyleSheetListOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, MarkStack& markStack)
{
    JSStyleSheetList* jsStyleSheetList = static_cast<JSStyleSheetList*>(handle.get().asCell());
    if (!jsStyleSheetList->hasCustomProperties())
        return false;
    Document* document = jsStyleSheetList->impl()->document();
    if (!document)
        return false;
    return markStack.containsOpaqueRoot(document);
}

bool JSStyleSheetList::canGetItemsForName(ExecState*, StyleSheetList* styleSheetList, const Identifier& propertyName)
{
    return styleSheetList->getNamedItem(identifierToString(propertyName));
}

JSValue JSStyleSheetList::nameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSStyleSheetList* thisObj = static_cast<JSStyleSheetList*>(asObject(slotBase));
    HTMLStyleElement* element = thisObj->impl()->getNamedItem(identifierToString(propertyName));
    ASSERT(element);
    return toJS(exec, thisObj->globalObject(), element->sheet());
}

} // namespace WebCore
