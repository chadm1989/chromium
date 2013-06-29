/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef StyleCustomFilterProgramCache_h
#define StyleCustomFilterProgramCache_h

#include "core/platform/graphics/filters/custom/CustomFilterProgramInfo.h"
#include <wtf/FastAllocBase.h>
#include <wtf/HashMap.h>

namespace WebCore {

class StyleCustomFilterProgram;
class CustomFilterProgramInfo;

class StyleCustomFilterProgramCache {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<StyleCustomFilterProgramCache> create();
    ~StyleCustomFilterProgramCache();

    // Lookups a StyleCustomFilterProgram that has similar parameters with the specified program.
    StyleCustomFilterProgram* lookup(StyleCustomFilterProgram*) const;
    StyleCustomFilterProgram* lookup(const CustomFilterProgramInfo&) const;

    void add(StyleCustomFilterProgram*);
    void remove(StyleCustomFilterProgram*);

private:
    StyleCustomFilterProgramCache() { }

    typedef HashMap<CustomFilterProgramInfo, StyleCustomFilterProgram*> CacheMap;
    CacheMap m_cache;
};

} // namespace WebCore


#endif // StyleCustomFilterProgramCache_h
