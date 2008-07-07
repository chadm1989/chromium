/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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
#include "CSSFontSelector.h"

#include "AtomicString.h"
#include "CachedFont.h"
#include "CSSFontFace.h"
#include "CSSFontFaceRule.h"
#include "CSSFontFaceSource.h"
#include "CSSFontFaceSrcValue.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSSegmentedFontFace.h"
#include "CSSUnicodeRangeValue.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "DocLoader.h"
#include "Document.h"
#include "FontCache.h"
#include "FontFamilyValue.h"
#include "Frame.h"
#include "NodeList.h"
#include "RenderObject.h"
#include "Settings.h"
#include "SimpleFontData.h"

#if ENABLE(SVG)
#include "SVGFontFaceElement.h"
#include "SVGNames.h"
#endif

namespace WebCore {

CSSFontSelector::CSSFontSelector(Document* document)
    : m_document(document)
{
    // FIXME: An old comment used to say there was no need to hold a reference to m_document
    // because "we are guaranteed to be destroyed before the document". But there does not
    // seem to be any such guarantee.

    ASSERT(m_document);
    FontCache::addClient(this);
}

CSSFontSelector::~CSSFontSelector()
{
    FontCache::removeClient(this);
    deleteAllValues(m_fontFaces);
    deleteAllValues(m_locallyInstalledFontFaces);
    deleteAllValues(m_fonts);
}

bool CSSFontSelector::isEmpty() const
{
    return m_fonts.isEmpty();
}

DocLoader* CSSFontSelector::docLoader() const
{
    return m_document ? m_document->docLoader() : 0;
}

void CSSFontSelector::addFontFaceRule(const CSSFontFaceRule* fontFaceRule)
{
    // Obtain the font-family property and the src property.  Both must be defined.
    const CSSMutableStyleDeclaration* style = fontFaceRule->style();
    RefPtr<CSSValue> fontFamily = style->getPropertyCSSValue(CSSPropertyFontFamily);
    RefPtr<CSSValue> src = style->getPropertyCSSValue(CSSPropertySrc);
    RefPtr<CSSValue> unicodeRange = style->getPropertyCSSValue(CSSPropertyUnicodeRange);
    if (!fontFamily || !src || !fontFamily->isValueList() || !src->isValueList() || unicodeRange && !unicodeRange->isValueList())
        return;

    CSSValueList* familyList = static_cast<CSSValueList*>(fontFamily.get());
    if (!familyList->length())
        return;

    CSSValueList* srcList = static_cast<CSSValueList*>(src.get());
    if (!srcList->length())
        return;

    CSSValueList* rangeList = static_cast<CSSValueList*>(unicodeRange.get());

    unsigned traitsMask = 0;

    if (RefPtr<CSSValue> fontStyle = style->getPropertyCSSValue(CSSPropertyFontStyle)) {
        if (fontStyle->isPrimitiveValue()) {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            list->append(fontStyle);
            fontStyle = list;
        } else if (!fontStyle->isValueList())
            return;

        CSSValueList* styleList = static_cast<CSSValueList*>(fontStyle.get());
        unsigned numStyles = styleList->length();
        if (!numStyles)
            return;

        for (unsigned i = 0; i < numStyles; ++i) {
            switch (static_cast<CSSPrimitiveValue*>(styleList->itemWithoutBoundsCheck(i))->getIdent()) {
                case CSSValueAll:
                    traitsMask |= FontStyleMask;
                    break;
                case CSSValueNormal:
                    traitsMask |= FontStyleNormalMask;
                    break;
                case CSSValueItalic:
                case CSSValueOblique:
                    traitsMask |= FontStyleItalicMask;
                    break;
                default:
                    break;
            }
        }
    } else
        traitsMask |= FontStyleMask;

    if (RefPtr<CSSValue> fontWeight = style->getPropertyCSSValue(CSSPropertyFontWeight)) {
        if (fontWeight->isPrimitiveValue()) {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            list->append(fontWeight);
            fontWeight = list;
        } else if (!fontWeight->isValueList())
            return;

        CSSValueList* weightList = static_cast<CSSValueList*>(fontWeight.get());
        unsigned numWeights = weightList->length();
        if (!numWeights)
            return;

        for (unsigned i = 0; i < numWeights; ++i) {
            switch (static_cast<CSSPrimitiveValue*>(weightList->itemWithoutBoundsCheck(i))->getIdent()) {
                case CSSValueAll:
                    traitsMask |= FontWeightMask;
                    break;
                case CSSValueBolder:
                case CSSValueBold:
                case CSSValue700:
                    traitsMask |= FontWeight700Mask;
                    break;
                case CSSValueNormal:
                case CSSValue400:
                    traitsMask |= FontWeight400Mask;
                    break;
                case CSSValue900:
                    traitsMask |= FontWeight900Mask;
                    break;
                case CSSValue800:
                    traitsMask |= FontWeight800Mask;
                    break;
                case CSSValue600:
                    traitsMask |= FontWeight600Mask;
                    break;
                case CSSValue500:
                    traitsMask |= FontWeight500Mask;
                    break;
                case CSSValue300:
                    traitsMask |= FontWeight300Mask;
                    break;
                case CSSValueLighter:
                case CSSValue200:
                    traitsMask |= FontWeight200Mask;
                    break;
                case CSSValue100:
                    traitsMask |= FontWeight100Mask;
                    break;
                default:
                    break;
            }
        }
    } else
        traitsMask |= FontWeightMask;

    if (RefPtr<CSSValue> fontVariant = style->getPropertyCSSValue(CSSPropertyFontVariant)) {
        if (fontVariant->isPrimitiveValue()) {
            RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
            list->append(fontVariant);
            fontVariant = list;
        } else if (!fontVariant->isValueList())
            return;

        CSSValueList* variantList = static_cast<CSSValueList*>(fontVariant.get());
        unsigned numVariants = variantList->length();
        if (!numVariants)
            return;

        for (unsigned i = 0; i < numVariants; ++i) {
            switch (static_cast<CSSPrimitiveValue*>(variantList->itemWithoutBoundsCheck(i))->getIdent()) {
                case CSSValueAll:
                    traitsMask |= FontVariantMask;
                    break;
                case CSSValueNormal:
                    traitsMask |= FontVariantNormalMask;
                    break;
                case CSSValueSmallCaps:
                    traitsMask |= FontVariantSmallCapsMask;
                    break;
                default:
                    break;
            }
        }
    } else
        traitsMask |= FontVariantNormalMask;

    // Each item in the src property's list is a single CSSFontFaceSource. Put them all into a CSSFontFace.
    RefPtr<CSSFontFace> fontFace;

    int srcLength = srcList->length();

    bool foundLocal = false;

#if ENABLE(SVG_FONTS)
    bool foundSVGFont = false;
#endif

    for (int i = 0; i < srcLength; i++) {
        // An item in the list either specifies a string (local font name) or a URL (remote font to download).
        CSSFontFaceSrcValue* item = static_cast<CSSFontFaceSrcValue*>(srcList->itemWithoutBoundsCheck(i));
        CSSFontFaceSource* source = 0;

#if ENABLE(SVG_FONTS)
        foundSVGFont = item->isSVGFontFaceSrc() || item->svgFontFaceElement();
#endif

        if (!item->isLocal()) {
            if (item->isSupportedFormat() && m_document) {
                CachedFont* cachedFont = m_document->docLoader()->requestFont(item->resource());
                if (cachedFont) {
#if ENABLE(SVG_FONTS)
                    if (foundSVGFont)
                        cachedFont->setSVGFont(true);
#endif
                    source = new CSSFontFaceSource(item->resource(), cachedFont);
                }
            }
        } else {
            source = new CSSFontFaceSource(item->resource());
            foundLocal = true;
        }

        if (!fontFace)
            fontFace = CSSFontFace::create(static_cast<FontTraitsMask>(traitsMask));

        if (source) {
#if ENABLE(SVG_FONTS)
            source->setSVGFontFaceElement(item->svgFontFaceElement());
#endif
            fontFace->addSource(source);
        }
    }

    ASSERT(fontFace);

    if (fontFace && !fontFace->isValid())
        return;

    if (rangeList) {
        unsigned numRanges = rangeList->length();
        for (unsigned i = 0; i < numRanges; i++) {
            CSSUnicodeRangeValue* range = static_cast<CSSUnicodeRangeValue*>(rangeList->itemWithoutBoundsCheck(i));
            fontFace->addRange(range->from(), range->to());
        }
    }

    // Hash under every single family name.
    int familyLength = familyList->length();
    for (int i = 0; i < familyLength; i++) {
        CSSPrimitiveValue* item = static_cast<CSSPrimitiveValue*>(familyList->itemWithoutBoundsCheck(i));
        String familyName;
        if (item->primitiveType() == CSSPrimitiveValue::CSS_STRING)
            familyName = static_cast<FontFamilyValue*>(item)->familyName();
        else if (item->primitiveType() == CSSPrimitiveValue::CSS_IDENT) {
            // We need to use the raw text for all the generic family types, since @font-face is a way of actually
            // defining what font to use for those types.
            String familyName;
            switch (item->getIdent()) {
                case CSSValueSerif:
                    familyName = "-webkit-serif";
                    break;
                case CSSValueSansSerif:
                    familyName = "-webkit-sans-serif";
                    break;
                case CSSValueCursive:
                    familyName = "-webkit-cursive";
                    break;
                case CSSValueFantasy:
                    familyName = "-webkit-fantasy";
                    break;
                case CSSValueMonospace:
                    familyName = "-webkit-monospace";
                    break;
                default:
                    break;
            }
        }

        if (familyName.isEmpty())
            continue;

#if ENABLE(SVG_FONTS)
        // SVG allows several <font> elements with the same font-family, differing only
        // in ie. font-variant. Be sure to pick up the right one - in getFontData below.
        if (foundSVGFont && (traitsMask & FontVariantSmallCapsMask))
            familyName += "-webkit-svg-small-caps";
#endif

        familyName = familyName.lower();
        Vector<RefPtr<CSSFontFace> >* familyFontFaces = m_fontFaces.get(familyName);
        if (!familyFontFaces) {
            familyFontFaces = new Vector<RefPtr<CSSFontFace> >;
            m_fontFaces.set(familyName, familyFontFaces);

            ASSERT(!m_locallyInstalledFontFaces.contains(familyName));
            Vector<RefPtr<CSSFontFace> >* familyLocallyInstalledFaces;

            Vector<unsigned> locallyInstalledFontsTraitsMasks;
            FontCache::getTraitsInFamily(familyName, locallyInstalledFontsTraitsMasks);
            unsigned numLocallyInstalledFaces = locallyInstalledFontsTraitsMasks.size();
            if (numLocallyInstalledFaces) {
                familyLocallyInstalledFaces = new Vector<RefPtr<CSSFontFace> >;
                m_locallyInstalledFontFaces.set(familyName, familyLocallyInstalledFaces);

                for (unsigned i = 0; i < numLocallyInstalledFaces; ++i) {
                    RefPtr<CSSFontFace> locallyInstalledFontFace = CSSFontFace::create(static_cast<FontTraitsMask>(locallyInstalledFontsTraitsMasks[i]));
                    locallyInstalledFontFace->addSource(new CSSFontFaceSource(familyName));
                    ASSERT(locallyInstalledFontFace->isValid());
                    familyLocallyInstalledFaces->append(locallyInstalledFontFace);
                }
            }
        }

        familyFontFaces->append(fontFace);
    }
}

void CSSFontSelector::fontLoaded(CSSSegmentedFontFace*)
{
    if (!m_document || m_document->inPageCache() || !m_document->renderer())
        return;
    m_document->recalcStyle(Document::Force);
    m_document->renderer()->setNeedsLayoutAndPrefWidthsRecalc();
}

void CSSFontSelector::fontCacheInvalidated()
{
    if (!m_document || m_document->inPageCache() || !m_document->renderer())
        return;
    m_document->recalcStyle(Document::Force);
    m_document->renderer()->setNeedsLayoutAndPrefWidthsRecalc();
}

static FontData* fontDataForGenericFamily(Document* document, const FontDescription& fontDescription, const AtomicString& familyName)
{
    if (!document)
        return 0;

    const Settings* settings = document->frame()->settings();
    AtomicString genericFamily;
    if (familyName == "-webkit-serif")
        genericFamily = settings->serifFontFamily();
    else if (familyName == "-webkit-sans-serif")
        genericFamily = settings->sansSerifFontFamily();
    else if (familyName == "-webkit-cursive")
        genericFamily = settings->cursiveFontFamily();
    else if (familyName == "-webkit-fantasy")
        genericFamily = settings->fantasyFontFamily();
    else if (familyName == "-webkit-monospace")
        genericFamily = settings->fixedFontFamily();
    else if (familyName == "-webkit-standard")
        genericFamily = settings->standardFontFamily();

    if (!genericFamily.isEmpty())
        return FontCache::getCachedFontData(FontCache::getCachedFontPlatformData(fontDescription, genericFamily));

    return 0;
}

static FontTraitsMask desiredTraitsMaskForComparison;

static inline bool compareFontFaces(CSSFontFace* first, CSSFontFace* second)
{
    FontTraitsMask firstTraitsMask = first->traitsMask();
    FontTraitsMask secondTraitsMask = second->traitsMask();

    bool firstHasDesiredVariant = firstTraitsMask & desiredTraitsMaskForComparison & FontVariantMask;
    bool secondHasDesiredVariant = secondTraitsMask & desiredTraitsMaskForComparison & FontVariantMask;

    if (firstHasDesiredVariant != secondHasDesiredVariant)
        return firstHasDesiredVariant;

    bool firstHasDesiredStyle = firstTraitsMask & desiredTraitsMaskForComparison & FontStyleMask;
    bool secondHasDesiredStyle = secondTraitsMask & desiredTraitsMaskForComparison & FontStyleMask;

    if (firstHasDesiredStyle != secondHasDesiredStyle)
        return firstHasDesiredStyle;

    if (secondTraitsMask & desiredTraitsMaskForComparison & FontWeightMask)
        return false;
    if (firstTraitsMask & desiredTraitsMaskForComparison & FontWeightMask)
        return true;

    // http://www.w3.org/TR/2002/WD-css3-webfonts-20020802/#q46 says: "If there are fewer then 9 weights in the family, the default algorithm
    // for filling the "holes" is as follows. If '500' is unassigned, it will be assigned the same font as '400'. If any of the values '600',
    // '700', '800', or '900' remains unassigned, they are assigned to the same face as the next darker assigned keyword, if any, or the next
    // lighter one otherwise. If any of '300', '200', or '100' remains unassigned, it is assigned to the next lighter assigned keyword, if any,
    // or the next darker otherwise."
    // For '400', we made up our own rule (which then '500' follows).

    static const FontTraitsMask weightFallbackRules[9][8] = {
        { FontWeight200Mask, FontWeight300Mask, FontWeight400Mask, FontWeight500Mask, FontWeight600Mask, FontWeight700Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight100Mask, FontWeight300Mask, FontWeight400Mask, FontWeight500Mask, FontWeight600Mask, FontWeight700Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight200Mask, FontWeight100Mask, FontWeight400Mask, FontWeight500Mask, FontWeight600Mask, FontWeight700Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight500Mask, FontWeight300Mask, FontWeight600Mask, FontWeight200Mask, FontWeight700Mask, FontWeight100Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight400Mask, FontWeight300Mask, FontWeight600Mask, FontWeight200Mask, FontWeight700Mask, FontWeight100Mask, FontWeight800Mask, FontWeight900Mask },
        { FontWeight700Mask, FontWeight800Mask, FontWeight900Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask },
        { FontWeight800Mask, FontWeight900Mask, FontWeight600Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask },
        { FontWeight900Mask, FontWeight700Mask, FontWeight600Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask },
        { FontWeight800Mask, FontWeight700Mask, FontWeight600Mask, FontWeight500Mask, FontWeight400Mask, FontWeight300Mask, FontWeight200Mask, FontWeight100Mask }
    };

    const FontTraitsMask* weightFallbackRule = weightFallbackRules[0];
    unsigned w = FontWeight100Bit;
    while (!(desiredTraitsMaskForComparison & (1 << w))) {
        w++;
        weightFallbackRule += 8;
    }

    for (unsigned i = 0; i < 8; ++i) {
        if (secondTraitsMask & weightFallbackRule[i])
            return false;
        if (firstTraitsMask & weightFallbackRule[i])
            return true;
    }

    return false;
}

FontData* CSSFontSelector::getFontData(const FontDescription& fontDescription, const AtomicString& familyName)
{
    if (m_fontFaces.isEmpty()) {
        if (familyName.startsWith("-webkit-"))
            return fontDataForGenericFamily(m_document, fontDescription, familyName);
        return 0;
    }

    String family = familyName.string().lower();

#if ENABLE(SVG_FONTS)
    if (fontDescription.smallCaps())
        family += "-webkit-svg-small-caps";
#endif

    Vector<RefPtr<CSSFontFace> >* familyFontFaces = m_fontFaces.get(family);
    // If no face was found, then return 0 and let the OS come up with its best match for the name.
    if (!familyFontFaces || familyFontFaces->isEmpty()) {
        // If we were handed a generic family, but there was no match, go ahead and return the correct font based off our
        // settings.
        return fontDataForGenericFamily(m_document, fontDescription, familyName);
    }

    HashMap<unsigned, RefPtr<CSSSegmentedFontFace> >* segmentedFontFaceCache = m_fonts.get(family);
    if (!segmentedFontFaceCache) {
        segmentedFontFaceCache = new HashMap<unsigned, RefPtr<CSSSegmentedFontFace> >;
        m_fonts.set(family, segmentedFontFaceCache);
    }

    FontTraitsMask traitsMask = fontDescription.traitsMask();

    RefPtr<CSSSegmentedFontFace> face = segmentedFontFaceCache->get(traitsMask);

    if (!face) {
        face = CSSSegmentedFontFace::create(this);
        segmentedFontFaceCache->set(traitsMask, face);
        // Collect all matching faces and sort them in order of preference.
        Vector<CSSFontFace*, 32> candidateFontFaces;
        for (int i = familyFontFaces->size() - 1; i >= 0; --i) {
            CSSFontFace* candidate = familyFontFaces->at(i).get();
            unsigned candidateTraitsMask = candidate->traitsMask();
            if ((traitsMask & FontStyleNormalMask) && !(candidateTraitsMask & FontStyleNormalMask))
                continue;
            if ((traitsMask & FontVariantNormalMask) && !(candidateTraitsMask & FontVariantNormalMask))
                continue;
            candidateFontFaces.append(candidate);
        }

        if (Vector<RefPtr<CSSFontFace> >* familyLocallyInstalledFontFaces = m_locallyInstalledFontFaces.get(family)) {
            unsigned numLocallyInstalledFontFaces = familyLocallyInstalledFontFaces->size();
            for (unsigned i = 0; i < numLocallyInstalledFontFaces; ++i) {
                CSSFontFace* candidate = familyLocallyInstalledFontFaces->at(i).get();
                unsigned candidateTraitsMask = candidate->traitsMask();
                if ((traitsMask & FontStyleNormalMask) && !(candidateTraitsMask & FontStyleNormalMask))
                    continue;
                if ((traitsMask & FontVariantNormalMask) && !(candidateTraitsMask & FontVariantNormalMask))
                    continue;
                candidateFontFaces.append(candidate);
            }
        }

        desiredTraitsMaskForComparison = traitsMask;
        std::stable_sort(candidateFontFaces.begin(), candidateFontFaces.end(), compareFontFaces);
        unsigned numCandidates = candidateFontFaces.size();
        for (unsigned i = 0; i < numCandidates; ++i)
            face->appendFontFace(candidateFontFaces[i]);
    }

    // We have a face.  Ask it for a font data.  If it cannot produce one, it will fail, and the OS will take over.
    return face->getFontData(fontDescription);
}

}
