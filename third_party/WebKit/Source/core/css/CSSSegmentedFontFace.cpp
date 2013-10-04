/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "core/css/CSSSegmentedFontFace.h"

#include "RuntimeEnabledFeatures.h"
#include "core/css/CSSFontFace.h"
#include "core/platform/graphics/FontCache.h"
#include "core/platform/graphics/FontDescription.h"
#include "core/platform/graphics/SegmentedFontData.h"
#include "core/platform/graphics/SimpleFontData.h"

namespace WebCore {

CSSSegmentedFontFace::CSSSegmentedFontFace(CSSFontSelector* fontSelector, FontTraitsMask traitsMask, bool isLocalFallback)
    : m_fontSelector(fontSelector)
    , m_traitsMask(traitsMask)
    , m_isLocalFallback(isLocalFallback)
{
}

CSSSegmentedFontFace::~CSSSegmentedFontFace()
{
    pruneTable();
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++)
        m_fontFaces[i]->clearSegmentedFontFace();
}

void CSSSegmentedFontFace::pruneTable()
{
    // Make sure the glyph page tree prunes out all uses of this custom font.
    if (m_fontDataTable.isEmpty())
        return;

    m_fontDataTable.clear();
}

bool CSSSegmentedFontFace::isValid() const
{
    // Valid if at least one font face is valid.
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->isValid())
            return true;
    }
    return false;
}

void CSSSegmentedFontFace::fontLoaded(CSSFontFace*)
{
    pruneTable();

    if (RuntimeEnabledFeatures::fontLoadEventsEnabled() && !isLoading()) {
        Vector<RefPtr<LoadFontCallback> > callbacks;
        m_callbacks.swap(callbacks);
        for (size_t index = 0; index < callbacks.size(); ++index) {
            if (checkFont())
                callbacks[index]->notifyLoaded(this);
            else
                callbacks[index]->notifyError(this);
        }
    }
}

void CSSSegmentedFontFace::appendFontFace(PassRefPtr<CSSFontFace> fontFace)
{
    pruneTable();
    fontFace->setSegmentedFontFace(this);
    m_fontFaces.append(fontFace);
}

static void appendFontData(SegmentedFontData* newFontData, PassRefPtr<SimpleFontData> prpFaceFontData, const Vector<CSSFontFace::UnicodeRange>& ranges)
{
    RefPtr<SimpleFontData> faceFontData = prpFaceFontData;
    unsigned numRanges = ranges.size();
    if (!numRanges) {
        newFontData->appendRange(FontDataRange(0, 0x7FFFFFFF, faceFontData));
        return;
    }

    for (unsigned j = 0; j < numRanges; ++j)
        newFontData->appendRange(FontDataRange(ranges[j].from(), ranges[j].to(), faceFontData));
}

PassRefPtr<FontData> CSSSegmentedFontFace::getFontData(const FontDescription& fontDescription)
{
    if (!isValid())
        return 0;

    float fontSize;
    if (RuntimeEnabledFeatures::subpixelFontScalingEnabled())
        fontSize = fontDescription.computedSize();
    else
        fontSize = fontDescription.computedPixelSize();

    FontTraitsMask desiredTraitsMask = fontDescription.traitsMask();
    unsigned hashKey = ((static_cast<unsigned>(fontSize * FontCache::s_fontSizePrecisionMultiplier) + 1) << (FontTraitsMaskWidth + FontWidthVariantWidth + 1))
        | ((fontDescription.orientation() == Vertical ? 1 : 0) << (FontTraitsMaskWidth + FontWidthVariantWidth))
        | fontDescription.widthVariant() << FontTraitsMaskWidth
        | desiredTraitsMask;

    RefPtr<SegmentedFontData>& fontData = m_fontDataTable.add(hashKey, 0).iterator->value;
    if (fontData && fontData->numRanges())
        return fontData; // No release, we have a reference to an object in the cache which should retain the ref count it has.

    if (!fontData)
        fontData = SegmentedFontData::create();

    bool syntheticBold = !(m_traitsMask & (FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask)) && (desiredTraitsMask & (FontWeight600Mask | FontWeight700Mask | FontWeight800Mask | FontWeight900Mask));
    bool syntheticItalic = !(m_traitsMask & FontStyleItalicMask) && (desiredTraitsMask & FontStyleItalicMask);

    for (int i = m_fontFaces.size() - 1; i >= 0; --i) {
        if (!m_fontFaces[i]->isValid())
            continue;
        if (RefPtr<SimpleFontData> faceFontData = m_fontFaces[i]->getFontData(fontDescription, syntheticBold, syntheticItalic)) {
            ASSERT(!faceFontData->isSegmented());
            appendFontData(fontData.get(), faceFontData.release(), m_fontFaces[i]->ranges());
        }
    }
    if (fontData->numRanges())
        return fontData; // No release, we have a reference to an object in the cache which should retain the ref count it has.

    return 0;
}

bool CSSSegmentedFontFace::hasSVGFontFaceSource() const
{
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->hasSVGFontFaceSource())
            return true;
    }
    return false;
}

bool CSSSegmentedFontFace::isLoading() const
{
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->loadStatus() == FontFace::Loading)
            return true;
    }
    return false;
}

void CSSSegmentedFontFace::willUseFontData(const FontDescription& fontDescription)
{
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++)
        m_fontFaces[i]->willUseFontData(fontDescription);
}

bool CSSSegmentedFontFace::checkFont() const
{
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        if (m_fontFaces[i]->loadStatus() != FontFace::Loaded)
            return false;
    }
    return true;
}

void CSSSegmentedFontFace::loadFont(const FontDescription& fontDescription, PassRefPtr<LoadFontCallback> callback)
{
    RefPtr<SegmentedFontData> fontData = toSegmentedFontData(getFontData(fontDescription).get());
    unsigned numRanges = fontData->numRanges();
    for (unsigned i = 0; i < numRanges; i++)
        fontData->rangeAt(i).fontData()->beginLoadIfNeeded();

    if (callback) {
        if (isLoading())
            m_callbacks.append(callback);
        else if (checkFont())
            callback->notifyLoaded(this);
        else
            callback->notifyError(this);
    }
}

Vector<RefPtr<FontFace> > CSSSegmentedFontFace::fontFaces() const
{
    Vector<RefPtr<FontFace> > fontFaces;
    unsigned size = m_fontFaces.size();
    for (unsigned i = 0; i < size; i++) {
        RefPtr<FontFace> face = m_fontFaces[i]->fontFace();
        if (face)
            fontFaces.append(face);
    }
    return fontFaces;
}

}
