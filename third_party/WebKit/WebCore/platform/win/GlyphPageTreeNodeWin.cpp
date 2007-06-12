/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
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
#include "FontData.h"
#include <windows.h>

namespace WebCore
{

bool GlyphPage::fill(UChar* buffer, unsigned bufferLength, const FontData* fontData)
{
    // bufferLength will be greater than the glyph page size if the buffer has Unicode supplementary characters.
    // GetGlyphIndices doesn't support this so ScriptGetCMap should be used instead. It seems that supporting this
    // would require modifying the registry (see http://www.i18nguy.com/surrogates.html) so we won't support this for now.
    if (bufferLength > GlyphPage::size)
        return false;

    HDC dc = GetDC((HWND)0);
    SaveDC(dc);
    SelectObject(dc, fontData->m_font.hfont());

    TEXTMETRIC tm;
    GetTextMetrics(dc, &tm);
    WORD localGlyphBuffer[GlyphPage::size];
    GetGlyphIndices(dc, buffer, bufferLength, localGlyphBuffer, 0);
    for (unsigned i = 0; i < GlyphPage::size; i++)
        setGlyphDataForIndex(i, localGlyphBuffer[i], fontData);
    RestoreDC(dc, -1);
    ReleaseDC(0, dc);
    return true;
}

}

