/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityAriaGridCell.h"

#include "AccessibilityObject.h"
#include "AccessibilityTableRow.h"

using namespace std;

namespace WebCore {
    
AccessibilityAriaGridCell::AccessibilityAriaGridCell(RenderObject* renderer)
    : AccessibilityTableCell(renderer)
{
}

AccessibilityAriaGridCell::~AccessibilityAriaGridCell()
{
}

PassRefPtr<AccessibilityAriaGridCell> AccessibilityAriaGridCell::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityAriaGridCell(renderer));
}

AccessibilityObject* AccessibilityAriaGridCell::parentTable() const
{
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent || !parent->isTableRow())
        return 0;
    
    parent = parent->parentObjectUnignored();
    if (!parent || !parent->isDataTable())
        return 0;
    
    return parent;
}
    
void AccessibilityAriaGridCell::rowIndexRange(pair<int, int>& rowRange)
{
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent || !parent->isTableRow())
        return;

    // as far as I can tell, grid cells cannot span rows
    rowRange.first = static_cast<AccessibilityTableRow*>(parent)->rowIndex();
    rowRange.second = 1;    
}

void AccessibilityAriaGridCell::columnIndexRange(pair<int, int>& columnRange)
{
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent || !parent->isTableRow())
        return;
    
    AccessibilityChildrenVector siblings = parent->children();
    unsigned childrenSize = siblings.size();
    for (unsigned k = 0; k < childrenSize; ++k) {
        if (siblings[k].get() == this) {
            columnRange.first = k;
            break;
        }
    }
    
    // as far as I can tell, grid cells cannot span columns
    columnRange.second = 1;    
}
  
} // namespace WebCore
