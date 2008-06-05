/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef AccessibilityListBox_h
#define AccessibilityListBox_h

#include "AccessibilityObject.h"
#include "AccessibilityRenderObject.h"

namespace WebCore {
    
class AccessibilityListBox : public AccessibilityRenderObject {

private:
    AccessibilityListBox(RenderObject*);
public:
    static PassRefPtr<AccessibilityListBox> create(RenderObject*);
    virtual ~AccessibilityListBox();
    
    virtual AccessibilityObject* doAccessibilityHitTest(const IntPoint&);
    virtual bool isListBox() const { return true; };
    
    virtual bool canSetFocusAttribute() const { return true; }
    virtual bool canSetSelectedChildrenAttribute() const;
    void setSelectedChildren(AccessibilityChildrenVector&);
    virtual AccessibilityRole roleValue() const { return ListBoxRole; }
    
    virtual bool accessibilityIsIgnored() const { return false; }
    
    virtual void selectedChildren(AccessibilityChildrenVector&);
    virtual void visibleChildren(AccessibilityChildrenVector&);
    
    virtual void addChildren();

private:    
    AccessibilityObject* listBoxOptionAccessibilityObject(HTMLElement*) const;
};
    
} // namespace WebCore

#endif // AccessibilityListBox_h
