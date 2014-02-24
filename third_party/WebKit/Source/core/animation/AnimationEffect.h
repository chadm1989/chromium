/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef AnimationEffect_h
#define AnimationEffect_h

#include "CSSPropertyNames.h"
#include "heap/Handle.h"
#include "wtf/HashMap.h"
#include "wtf/PassOwnPtr.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class AnimatableValue;

class AnimationEffect : public RefCountedWillBeGarbageCollectedFinalized<AnimationEffect> {
    DECLARE_GC_INFO;
public:
    enum CompositeOperation {
        CompositeReplace,
        CompositeAdd,
    };
    // Encapsulates the value which results from applying a set of composition operations onto an
    // underlying value. It is used to represent the output of the effect phase of the Web
    // Animations model.
    class CompositableValue : public RefCounted<CompositableValue> {
    public:
        virtual ~CompositableValue() { }
        virtual bool dependsOnUnderlyingValue() const = 0;
        virtual PassRefPtr<AnimatableValue> compositeOnto(const AnimatableValue*) const = 0;
    };

    virtual ~AnimationEffect() { }
    typedef HashMap<CSSPropertyID, RefPtr<CompositableValue> > CompositableValueMap;
    typedef Vector<std::pair<CSSPropertyID, RefPtr<CompositableValue> > > CompositableValueList;
    virtual PassOwnPtr<CompositableValueList> sample(int iteration, double fraction) const = 0;

    virtual bool affects(CSSPropertyID) { return false; };
    virtual bool isKeyframeEffectModel() const { return false; }

    virtual void trace(Visitor*) = 0;
};

} // namespace WebCore

#endif // AnimationEffect_h
