/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
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
#include "wtf/text/TextCodec.h"

#include "wtf/StringExtras.h"

namespace WTF {

TextCodec::~TextCodec()
{
}

int TextCodec::getUnencodableReplacement(unsigned codePoint, UnencodableHandling handling, UnencodableReplacementArray replacement)
{
    switch (handling) {
        case QuestionMarksForUnencodables:
            replacement[0] = '?';
            replacement[1] = 0;
            return 1;
        case EntitiesForUnencodables:
            snprintf(replacement, sizeof(UnencodableReplacementArray), "&#%u;", codePoint);
            return static_cast<int>(strlen(replacement));
        case URLEncodedEntitiesForUnencodables:
            snprintf(replacement, sizeof(UnencodableReplacementArray), "%%26%%23%u%%3B", codePoint);
            return static_cast<int>(strlen(replacement));
    }
    ASSERT_NOT_REACHED();
    replacement[0] = 0;
    return 0;
}

} // namespace WTF
