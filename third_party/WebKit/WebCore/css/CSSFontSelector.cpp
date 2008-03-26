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
    ASSERT(m_document);
}

CSSFontSelector::~CSSFontSelector()
{}

bool CSSFontSelector::isEmpty() const
{
    return m_fonts.isEmpty();
}

DocLoader* CSSFontSelector::docLoader() const
{
    return m_document->docLoader();
}

static String hashForFont(const String& familyName, bool bold, bool italic)
{
    String familyHash(familyName);
    if (bold)
        familyHash += "-webkit-bold";
    if (italic)
        familyHash += "-webkit-italic";
    return AtomicString(familyHash);
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

    // Create a FontDescription for this font and set up bold/italic info properly.
    FontDescription fontDescription;

    if (RefPtr<CSSValue> fontStyle = style->getPropertyCSSValue(CSSPropertyFontStyle))
        fontDescription.setItalic(static_cast<CSSPrimitiveValue*>(fontStyle.get())->getIdent() != CSSValueNormal);

    if (RefPtr<CSSValue> fontWeight = style->getPropertyCSSValue(CSSPropertyFontWeight)) {
        // FIXME: Need to support weights for real, since we're effectively limiting the number of supported weights to two.
        // This behavior could also result in the "last kinda bold variant" described winning even if it isn't the best match for bold.
        switch (static_cast<CSSPrimitiveValue*>(fontWeight.get())->getIdent()) {
            case CSSValueBold:
            case CSSValueBolder:
            case CSSValue600:
            case CSSValue700:
            case CSSValue800:
            case CSSValue900:
                fontDescription.setWeight(cBoldWeight);
            default:
                break;
        }
    }

    if (RefPtr<CSSValue> fontVariant = style->getPropertyCSSValue(CSSPropertyFontVariant))
        fontDescription.setSmallCaps(static_cast<CSSPrimitiveValue*>(fontVariant.get())->getIdent() == CSSValueSmallCaps);

    // Each item in the src property's list is a single CSSFontFaceSource. Put them all into a CSSFontFace.
    CSSFontFace* fontFace = 0;

    int i;
    int srcLength = srcList->length();

    bool foundLocal = false;

#if ENABLE(SVG_FONTS)
    bool foundSVGFont = false;
#endif

    for (i = 0; i < srcLength; i++) {
        // An item in the list either specifies a string (local font name) or a URL (remote font to download).
        CSSFontFaceSrcValue* item = static_cast<CSSFontFaceSrcValue*>(srcList->itemWithoutBoundsCheck(i));
        CSSFontFaceSource* source = 0;

#if ENABLE(SVG_FONTS)
        foundSVGFont = item->isSVGFontFaceSrc() || item->svgFontFaceElement();
#endif

        if (!item->isLocal()) {
            if (item->isSupportedFormat()) {
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
            String family = item->resource();

            // Test the validity of the local font now.  We don't want to include this font if it does not exist
            // on the system.  If it *does* exist on the system, then we don't need to look any further.
            if (FontCache::fontExists(fontDescription, family)
#if ENABLE(SVG_FONTS)
                || foundSVGFont
#endif
               ) {
                source = new CSSFontFaceSource(family);
                foundLocal = true;
            }
        }

        if (!fontFace)
            fontFace = new CSSFontFace();

        if (source) {
#if ENABLE(SVG_FONTS)
            source->setSVGFontFaceElement(item->svgFontFaceElement());
#endif
            fontFace->addSource(source);
        }

        // We can just break if we see a local font that is valid.
        if (foundLocal)
            break;
    }

    ASSERT(fontFace);

    if (fontFace && !fontFace->isValid()) {
        delete fontFace;
        return;
    }

    // Hash under every single family name.
    int familyLength = familyList->length();
    for (i = 0; i < familyLength; i++) {
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
        if (foundSVGFont && fontDescription.smallCaps())
            familyName += "-webkit-svg-small-caps";
#endif

        String hash = hashForFont(familyName.lower(), fontDescription.bold(), fontDescription.italic());
        CSSSegmentedFontFace* segmentedFontFace = m_fonts.get(hash).get();
        if (!segmentedFontFace) {
            segmentedFontFace = new CSSSegmentedFontFace(this);
            m_fonts.set(hash, segmentedFontFace);
        }
        if (rangeList) {
            // A local font matching the font description should come first, so that it gets used for
            // any character not overlaid by explicit @font-face rules for the family.
            if (!segmentedFontFace->numRanges() && FontCache::fontExists(fontDescription, familyName)) {
                CSSFontFace* implicitFontFace = new CSSFontFace();
                implicitFontFace->addSource(new CSSFontFaceSource(familyName));
                ASSERT(implicitFontFace->isValid());
                segmentedFontFace->overlayRange(0, 0x7FFFFFFF, implicitFontFace);
            }

            unsigned numRanges = rangeList->length();
            for (unsigned i = 0; i < numRanges; i++) {
                CSSUnicodeRangeValue* range = static_cast<CSSUnicodeRangeValue*>(rangeList->itemWithoutBoundsCheck(i));
                segmentedFontFace->overlayRange(range->from(), range->to(), fontFace);
            }
        } else
            segmentedFontFace->overlayRange(0, 0x7FFFFFFF, fontFace);
    }
}

void CSSFontSelector::fontLoaded(CSSSegmentedFontFace*)
{
    if (m_document->inPageCache() || !m_document->renderer())
        return;
    m_document->recalcStyle(Document::Force);
    m_document->renderer()->setNeedsLayoutAndPrefWidthsRecalc();
}

FontData* CSSFontSelector::getFontData(const FontDescription& fontDescription, const AtomicString& familyName)
{
    if (m_fonts.isEmpty() && !familyName.startsWith("-webkit-"))
        return 0;
    
    bool bold = fontDescription.bold();
    bool italic = fontDescription.italic();
    
    bool syntheticBold = false;
    bool syntheticItalic = false;

    String family = familyName.string().lower();

#if ENABLE(SVG_FONTS)
    RefPtr<CSSSegmentedFontFace> face;

    if (fontDescription.smallCaps()) {
        String testFamily = family + "-webkit-svg-small-caps";
        face = m_fonts.get(hashForFont(testFamily, bold, italic));
    } else
        face = m_fonts.get(hashForFont(family, bold, italic));
#else
    RefPtr<CSSSegmentedFontFace> face = m_fonts.get(hashForFont(family, bold, italic));
#endif

    // If we don't find a face, and if bold/italic are set, we should try other variants.
    // Bold/italic should try bold first, then italic, then normal (on the assumption that we are better at synthesizing italic than we are
    // at synthesizing bold).
    if (!face) {
        if (bold && italic) {
            syntheticItalic = true;
            face = m_fonts.get(hashForFont(family, bold, false));
            if (!face) {
                syntheticBold = true;
                face = m_fonts.get(hashForFont(family, false, italic));
            }
        }
        
        // Bold should try normal.
        // Italic should try normal.
        if (!face && (bold || italic)) {
            syntheticBold = bold;
            syntheticItalic = italic;
            face = m_fonts.get(hashForFont(family, false, false));
        }
    }

#if ENABLE(SVG_FONTS)
    // If no face was found, and if we're a SVG Font we may have hit following case:
    // <font-face> specified font-weight and/or font-style to be ie. bold and italic.
    // And the font-family requested is non-bold & non-italic. For SVG Fonts we still
    // have to return the defined font, and not fallback to the system default.
    if (!face && !bold)
        face = m_fonts.get(hashForFont(family, true, italic));

    if (!face && !italic)
        face = m_fonts.get(hashForFont(family, bold, true));

    if (!face && !bold && !italic)
        face = m_fonts.get(hashForFont(family, true, true));
#endif

    // If no face was found, then return 0 and let the OS come up with its best match for the name.
    if (!face) {
        // If we were handed a generic family, but there was no match, go ahead and return the correct font based off our
        // settings.
        const Settings* settings = m_document->frame()->settings();
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

    // We have a face.  Ask it for a font data.  If it cannot produce one, it will fail, and the OS will take over.
    return face->getFontData(fontDescription, syntheticBold, syntheticItalic);
}

}
