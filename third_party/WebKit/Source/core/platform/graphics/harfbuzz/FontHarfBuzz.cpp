/*
 * Copyright (c) 2007, 2008, 2010 Google Inc. All rights reserved.
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
#include "core/platform/graphics/Font.h"

#include "platform/NotImplemented.h"
#include "core/platform/graphics/GraphicsContext.h"
#include "core/platform/graphics/SimpleFontData.h"
#include "core/platform/graphics/harfbuzz/HarfBuzzShaper.h"
#include "platform/fonts/GlyphBuffer.h"
#include "platform/geometry/FloatRect.h"

#include "SkPaint.h"
#include "SkTemplates.h"

#include "wtf/unicode/Unicode.h"

namespace WebCore {

bool Font::canReturnFallbackFontsForComplexText()
{
    return false;
}

bool Font::canExpandAroundIdeographsInComplexText()
{
    return false;
}


static void paintGlyphs(GraphicsContext* gc, const SimpleFontData* font,
    const GlyphBufferGlyph* glyphs, int numGlyphs,
    SkPoint* pos, const FloatRect& textRect)
{
    TextDrawingModeFlags textMode = gc->textDrawingMode();

    // We draw text up to two times (once for fill, once for stroke).
    if (textMode & TextModeFill) {
        SkPaint paint;
        gc->setupPaintForFilling(&paint);
        font->platformData().setupPaint(&paint, gc);
        gc->adjustTextRenderMode(&paint);
        paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

        gc->drawPosText(glyphs, numGlyphs << 1, pos, textRect, paint);
    }

    if ((textMode & TextModeStroke)
        && gc->strokeStyle() != NoStroke
        && gc->strokeThickness() > 0) {

        SkPaint paint;
        gc->setupPaintForStroking(&paint);
        font->platformData().setupPaint(&paint, gc);
        gc->adjustTextRenderMode(&paint);
        paint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

        if (textMode & TextModeFill) {
            // If we also filled, we don't want to draw shadows twice.
            // See comment in FontChromiumWin.cpp::paintSkiaText() for more details.
            // Since we use the looper for shadows, we remove it (if any) now.
            paint.setLooper(0);
        }

        gc->drawPosText(glyphs, numGlyphs << 1, pos, textRect, paint);
    }
}

void Font::drawGlyphs(GraphicsContext* gc, const SimpleFontData* font,
    const GlyphBuffer& glyphBuffer,  int from, int numGlyphs,
    const FloatPoint& point, const FloatRect& textRect) const
{
    SkASSERT(sizeof(GlyphBufferGlyph) == sizeof(uint16_t)); // compile-time assert

    const GlyphBufferGlyph* glyphs = glyphBuffer.glyphs(from);
    SkScalar x = SkFloatToScalar(point.x());
    SkScalar y = SkFloatToScalar(point.y());

    // FIXME: text rendering speed:
    // Android has code in their WebCore fork to special case when the
    // GlyphBuffer has no advances other than the defaults. In that case the
    // text drawing can proceed faster. However, it's unclear when those
    // patches may be upstreamed to WebKit so we always use the slower path
    // here.
    const GlyphBufferAdvance* adv = glyphBuffer.advances(from);
    SkAutoSTMalloc<32, SkPoint> storage(numGlyphs);
    SkPoint* pos = storage.get();

    const OpenTypeVerticalData* verticalData = font->verticalData();
    if (font->platformData().orientation() == Vertical && verticalData) {
        AffineTransform savedMatrix = gc->getCTM();
        gc->concatCTM(AffineTransform(0, -1, 1, 0, point.x(), point.y()));
        gc->concatCTM(AffineTransform(1, 0, 0, 1, -point.x(), -point.y()));

        const int kMaxBufferLength = 256;
        Vector<FloatPoint, kMaxBufferLength> translations;

        const FontMetrics& metrics = font->fontMetrics();
        SkScalar verticalOriginX = SkFloatToScalar(point.x() + metrics.floatAscent() - metrics.floatAscent(IdeographicBaseline));
        float horizontalOffset = point.x();

        int glyphIndex = 0;
        while (glyphIndex < numGlyphs) {
            int chunkLength = std::min(kMaxBufferLength, numGlyphs - glyphIndex);

            const Glyph* glyphs = glyphBuffer.glyphs(from + glyphIndex);
            translations.resize(chunkLength);
            verticalData->getVerticalTranslationsForGlyphs(font, &glyphs[0], chunkLength, reinterpret_cast<float*>(&translations[0]));

            x = verticalOriginX;
            y = SkFloatToScalar(point.y() + horizontalOffset - point.x());

            float currentWidth = 0;
            for (int i = 0; i < chunkLength; ++i, ++glyphIndex) {
                pos[i].set(
                    x + SkIntToScalar(lroundf(translations[i].x())),
                    y + -SkIntToScalar(-lroundf(currentWidth - translations[i].y())));
                currentWidth += glyphBuffer.advanceAt(from + glyphIndex);
            }
            horizontalOffset += currentWidth;
            paintGlyphs(gc, font, glyphs, numGlyphs, pos, textRect);
        }

        gc->setCTM(savedMatrix);
        return;
    }

    for (int i = 0; i < numGlyphs; i++) {
        pos[i].set(x, y);
        x += SkFloatToScalar(adv[i].width());
        y += SkFloatToScalar(adv[i].height());
    }

    paintGlyphs(gc, font, glyphs, numGlyphs, pos, textRect);
}

void Font::drawComplexText(GraphicsContext* gc, const TextRunPaintInfo& runInfo, const FloatPoint& point) const
{
    if (!runInfo.run.length())
        return;

    TextDrawingModeFlags textMode = gc->textDrawingMode();
    bool fill = textMode & TextModeFill;
    bool stroke = (textMode & TextModeStroke)
        && gc->strokeStyle() != NoStroke
        && gc->strokeThickness() > 0;

    if (!fill && !stroke)
        return;

    GlyphBuffer glyphBuffer;
    HarfBuzzShaper shaper(this, runInfo.run);
    shaper.setDrawRange(runInfo.from, runInfo.to);
    if (!shaper.shape(&glyphBuffer))
        return;
    FloatPoint adjustedPoint = shaper.adjustStartPoint(point);
    drawGlyphBuffer(gc, runInfo, glyphBuffer, adjustedPoint);
}

void Font::drawEmphasisMarksForComplexText(GraphicsContext* /* context */, const TextRunPaintInfo& /* runInfo */, const AtomicString& /* mark */, const FloatPoint& /* point */) const
{
    notImplemented();
}

float Font::floatWidthForComplexText(const TextRun& run, HashSet<const SimpleFontData*>* /* fallbackFonts */, GlyphOverflow* /* glyphOverflow */) const
{
    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape())
        return 0;
    return shaper.totalWidth();
}

// Return the code point index for the given |x| offset into the text run.
int Font::offsetForPositionForComplexText(const TextRun& run, float xFloat,
                                          bool includePartialGlyphs) const
{
    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape())
        return 0;
    return shaper.offsetForPosition(xFloat);
}

// Return the rectangle for selecting the given range of code-points in the TextRun.
FloatRect Font::selectionRectForComplexText(const TextRun& run,
                                            const FloatPoint& point, int height,
                                            int from, int to) const
{
    HarfBuzzShaper shaper(this, run);
    if (!shaper.shape())
        return FloatRect();
    return shaper.selectionRect(point, height, from, to);
}

} // namespace WebCore
