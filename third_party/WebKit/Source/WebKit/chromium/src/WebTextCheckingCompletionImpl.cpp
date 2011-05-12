/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "WebTextCheckingCompletionImpl.h"

#include "SpellChecker.h"
#include "TextCheckerClient.h"
#include "WebTextCheckingResult.h"
#include "WebVector.h"
#include <wtf/Assertions.h>

using namespace WebCore;

namespace WebKit {

static TextCheckingType toCoreCheckingType(WebTextCheckingResult::Error error)
{
    if (error == WebTextCheckingResult::ErrorSpelling)
        return TextCheckingTypeSpelling;
    ASSERT(error == WebTextCheckingResult::ErrorGrammar);
    return TextCheckingTypeGrammar;
}

static Vector<TextCheckingResult> toCoreResults(const WebVector<WebTextCheckingResult>& results)
{
    Vector<TextCheckingResult> coreResults;
    for (size_t i = 0; i < results.size(); ++i) { 
        TextCheckingResult coreResult;
        coreResult.type = toCoreCheckingType(results[i].error);
        coreResult.location = results[i].position;
        coreResult.length = results[i].length;
        coreResults.append(coreResult);
    }

    return coreResults;
}

void WebTextCheckingCompletionImpl::didFinishCheckingText(const WebVector<WebTextCheckingResult>& results)
{
    m_spellChecker->didCheck(m_identifier, toCoreResults(results));
    delete this;
}

} // namespace WebKit
