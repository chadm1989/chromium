/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/html/BaseButtonInputType.h"

#include "HTMLNames.h"
#include "core/dom/Text.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/html/HTMLInputElement.h"
#include "core/rendering/RenderButton.h"
#include "core/rendering/RenderTextFragment.h"

namespace WebCore {

using namespace HTMLNames;

class NonSelectableText : public Text {
    inline NonSelectableText(Document* document, const String& data)
        : Text(document, data, CreateText)
    {
    }

    virtual RenderText* createTextRenderer(RenderStyle*) OVERRIDE
    {
        return new RenderTextFragment(this, dataImpl());
    }

public:
    static inline PassRefPtr<NonSelectableText> create(Document* document, const String& data)
    {
        return adoptRef(new NonSelectableText(document, data));
    }
};

// ----------------------------

void BaseButtonInputType::createShadowSubtree()
{
    ASSERT(element()->userAgentShadowRoot());
    RefPtr<Text> text = NonSelectableText::create(element()->document(), element()->valueWithDefault());
    element()->userAgentShadowRoot()->appendChild(text, ASSERT_NO_EXCEPTION, DeprecatedAttachNow);
}

void BaseButtonInputType::valueAttributeChanged()
{
    toText(element()->userAgentShadowRoot()->firstChild())->setData(element()->valueWithDefault());
}

bool BaseButtonInputType::shouldSaveAndRestoreFormControlState() const
{
    return false;
}

bool BaseButtonInputType::appendFormData(FormDataList&, bool) const
{
    // Buttons except overridden types are never successful.
    return false;
}

RenderObject* BaseButtonInputType::createRenderer(RenderStyle*) const
{
    return new RenderButton(element());
}

bool BaseButtonInputType::storesValueSeparateFromAttribute()
{
    return false;
}

void BaseButtonInputType::setValue(const String& sanitizedValue, bool, TextFieldEventBehavior)
{
    element()->setAttribute(valueAttr, sanitizedValue);
}

} // namespace WebCore
