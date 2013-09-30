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

#ifndef AnimatableVisibility_h
#define AnimatableVisibility_h

#include "core/animation/AnimatableValue.h"
#include "core/rendering/style/RenderStyleConstants.h"

namespace WebCore {

class AnimatableVisibility : public AnimatableValue {
public:
    virtual ~AnimatableVisibility() { }
    static PassRefPtr<AnimatableVisibility> create(EVisibility visibility)
    {
        return adoptRef(new AnimatableVisibility(visibility));
    }

    EVisibility visibility() const { return m_visibility; }

protected:
    virtual PassRefPtr<AnimatableValue> interpolateTo(const AnimatableValue*, double fraction) const OVERRIDE;

private:
    explicit AnimatableVisibility(EVisibility visibility)
        : m_visibility(visibility)
    {
    }
    virtual AnimatableType type() const OVERRIDE { return TypeVisibility; }
    const EVisibility m_visibility;
};

inline const AnimatableVisibility* toAnimatableVisibility(const AnimatableValue* value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(value && value->isVisibility());
    return static_cast<const AnimatableVisibility*>(value);
}

} // namespace WebCore

#endif // AnimatableVisibility_h
