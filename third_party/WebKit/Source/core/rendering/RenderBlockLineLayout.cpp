/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All right reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include "core/rendering/FastTextAutosizer.h"
#include "core/rendering/LayoutRectRecorder.h"
#include "core/rendering/RenderCounter.h"
#include "core/rendering/RenderFlowThread.h"
#include "core/rendering/RenderLayer.h"
#include "core/rendering/RenderListMarker.h"
#include "core/rendering/RenderRegion.h"
#include "core/rendering/RenderRubyRun.h"
#include "core/rendering/RenderView.h"
#include "core/rendering/TrailingFloatsRootInlineBox.h"
#include "core/rendering/VerticalPositionCache.h"
#include "core/rendering/line/BreakingContextInlineHeaders.h"
#include "core/rendering/svg/SVGRootInlineBox.h"
#include "platform/text/BidiResolver.h"
#include "wtf/RefCountedLeakCounter.h"
#include "wtf/StdLibExtras.h"
#include "wtf/Vector.h"
#include "wtf/unicode/CharacterNames.h"

namespace WebCore {

static IndentTextOrNot requiresIndent(bool isFirstLine, bool isAfterHardLineBreak, RenderStyle* style)
{
    if (isFirstLine)
        return IndentText;
    if (isAfterHardLineBreak && style->textIndentLine() == TextIndentEachLine)
        return IndentText;

    return DoNotIndentText;
}

class LineBreaker {
public:
    friend class BreakingContext;
    LineBreaker(RenderBlockFlow* block)
        : m_block(block)
    {
        reset();
    }

    InlineIterator nextLineBreak(InlineBidiResolver&, LineInfo&, RenderTextInfo&, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements&);

    bool lineWasHyphenated() { return m_hyphenated; }
    const Vector<RenderBox*>& positionedObjects() { return m_positionedObjects; }
    EClear clear() { return m_clear; }
private:
    void reset();

    InlineIterator nextSegmentBreak(InlineBidiResolver&, LineInfo&, RenderTextInfo&, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements&);
    void skipLeadingWhitespace(InlineBidiResolver&, LineInfo&, FloatingObject* lastFloatFromPreviousLine, LineWidth&);

    RenderBlockFlow* m_block;
    bool m_hyphenated;
    EClear m_clear;
    Vector<RenderBox*> m_positionedObjects;
};

static RenderObject* firstRenderObjectForDirectionalityDetermination(RenderObject* root, RenderObject* current = 0)
{
    RenderObject* next = current;
    while (current) {
        if (isIsolated(current->style()->unicodeBidi())
            && (current->isRenderInline() || current->isRenderBlock())) {
            if (current != root)
                current = 0;
            else
                current = next;
            break;
        }
        current = current->parent();
    }

    if (!current)
        current = root->firstChild();

    while (current) {
        next = 0;
        if (isIteratorTarget(current) && !(current->isText() && toRenderText(current)->isAllCollapsibleWhitespace()))
            break;

        if (!isIteratorTarget(current) && !isIsolated(current->style()->unicodeBidi()))
            next = current->firstChild();

        if (!next) {
            while (current && current != root) {
                next = current->nextSibling();
                if (next)
                    break;
                current = current->parent();
            }
        }

        if (!next)
            break;

        current = next;
    }

    return current;
}

static TextDirection determinePlaintextDirectionality(RenderObject* root, RenderObject* current = 0, unsigned pos = 0)
{
    InlineIterator iter(root, firstRenderObjectForDirectionalityDetermination(root, current), pos);
    InlineBidiResolver observer;
    observer.setStatus(BidiStatus(root->style()->direction(), isOverride(root->style()->unicodeBidi())));
    observer.setPositionIgnoringNestedIsolates(iter);
    return observer.determineParagraphDirectionality();
}

static inline InlineBox* createInlineBoxForRenderer(RenderObject* obj, bool isRootLineBox, bool isOnlyRun = false)
{
    if (isRootLineBox)
        return toRenderBlock(obj)->createAndAppendRootInlineBox();

    if (obj->isText()) {
        InlineTextBox* textBox = toRenderText(obj)->createInlineTextBox();
        // We only treat a box as text for a <br> if we are on a line by ourself or in strict mode
        // (Note the use of strict mode.  In "almost strict" mode, we don't treat the box for <br> as text.)
        if (obj->isBR())
            textBox->setIsText(isOnlyRun || obj->document().inNoQuirksMode());
        return textBox;
    }

    if (obj->isBox())
        return toRenderBox(obj)->createInlineBox();

    return toRenderInline(obj)->createAndAppendInlineFlowBox();
}

static inline void dirtyLineBoxesForRenderer(RenderObject* o, bool fullLayout)
{
    if (o->isText()) {
        RenderText* renderText = toRenderText(o);
        renderText->dirtyLineBoxes(fullLayout);
    } else
        toRenderInline(o)->dirtyLineBoxes(fullLayout);
}

static bool parentIsConstructedOrHaveNext(InlineFlowBox* parentBox)
{
    do {
        if (parentBox->isConstructed() || parentBox->nextOnLine())
            return true;
        parentBox = parentBox->parent();
    } while (parentBox);
    return false;
}

InlineFlowBox* RenderBlockFlow::createLineBoxes(RenderObject* obj, const LineInfo& lineInfo, InlineBox* childBox, bool startNewSegment)
{
    // See if we have an unconstructed line box for this object that is also
    // the last item on the line.
    unsigned lineDepth = 1;
    InlineFlowBox* parentBox = 0;
    InlineFlowBox* result = 0;
    bool hasDefaultLineBoxContain = style()->lineBoxContain() == RenderStyle::initialLineBoxContain();
    do {
        ASSERT_WITH_SECURITY_IMPLICATION(obj->isRenderInline() || obj == this);

        RenderInline* inlineFlow = (obj != this) ? toRenderInline(obj) : 0;

        // Get the last box we made for this render object.
        parentBox = inlineFlow ? inlineFlow->lastLineBox() : toRenderBlock(obj)->lastLineBox();

        // If this box or its ancestor is constructed then it is from a previous line, and we need
        // to make a new box for our line.  If this box or its ancestor is unconstructed but it has
        // something following it on the line, then we know we have to make a new box
        // as well.  In this situation our inline has actually been split in two on
        // the same line (this can happen with very fancy language mixtures).
        bool constructedNewBox = false;
        bool allowedToConstructNewBox = !hasDefaultLineBoxContain || !inlineFlow || inlineFlow->alwaysCreateLineBoxes();
        bool mustCreateBoxesToRoot = startNewSegment && !(parentBox && parentBox->isRootInlineBox());
        bool canUseExistingParentBox = parentBox && !parentIsConstructedOrHaveNext(parentBox) && !mustCreateBoxesToRoot;
        if (allowedToConstructNewBox && !canUseExistingParentBox) {
            // We need to make a new box for this render object.  Once
            // made, we need to place it at the end of the current line.
            InlineBox* newBox = createInlineBoxForRenderer(obj, obj == this);
            ASSERT_WITH_SECURITY_IMPLICATION(newBox->isInlineFlowBox());
            parentBox = toInlineFlowBox(newBox);
            parentBox->setFirstLineStyleBit(lineInfo.isFirstLine());
            parentBox->setIsHorizontal(isHorizontalWritingMode());
            if (!hasDefaultLineBoxContain)
                parentBox->clearDescendantsHaveSameLineHeightAndBaseline();
            constructedNewBox = true;
        }

        if (constructedNewBox || canUseExistingParentBox) {
            if (!result)
                result = parentBox;

            // If we have hit the block itself, then |box| represents the root
            // inline box for the line, and it doesn't have to be appended to any parent
            // inline.
            if (childBox)
                parentBox->addToLine(childBox);

            if (!constructedNewBox || obj == this)
                break;

            childBox = parentBox;
        }

        // If we've exceeded our line depth, then jump straight to the root and skip all the remaining
        // intermediate inline flows.
        obj = (++lineDepth >= cMaxLineDepth) ? this : obj->parent();

    } while (true);

    return result;
}

template <typename CharacterType>
static inline bool endsWithASCIISpaces(const CharacterType* characters, unsigned pos, unsigned end)
{
    while (isASCIISpace(characters[pos])) {
        pos++;
        if (pos >= end)
            return true;
    }
    return false;
}

static bool reachedEndOfTextRenderer(const BidiRunList<BidiRun>& bidiRuns)
{
    BidiRun* run = bidiRuns.logicallyLastRun();
    if (!run)
        return true;
    unsigned pos = run->stop();
    RenderObject* r = run->m_object;
    if (!r->isText() || r->isBR())
        return false;
    RenderText* renderText = toRenderText(r);
    unsigned length = renderText->textLength();
    if (pos >= length)
        return true;

    if (renderText->is8Bit())
        return endsWithASCIISpaces(renderText->characters8(), pos, length);
    return endsWithASCIISpaces(renderText->characters16(), pos, length);
}

RootInlineBox* RenderBlockFlow::constructLine(BidiRunList<BidiRun>& bidiRuns, const LineInfo& lineInfo)
{
    ASSERT(bidiRuns.firstRun());

    bool rootHasSelectedChildren = false;
    InlineFlowBox* parentBox = 0;
    int runCount = bidiRuns.runCount() - lineInfo.runsFromLeadingWhitespace();
    for (BidiRun* r = bidiRuns.firstRun(); r; r = r->next()) {
        // Create a box for our object.
        bool isOnlyRun = (runCount == 1);
        if (runCount == 2 && !r->m_object->isListMarker())
            isOnlyRun = (!style()->isLeftToRightDirection() ? bidiRuns.lastRun() : bidiRuns.firstRun())->m_object->isListMarker();

        if (lineInfo.isEmpty())
            continue;

        InlineBox* box = createInlineBoxForRenderer(r->m_object, false, isOnlyRun);
        r->m_box = box;

        ASSERT(box);
        if (!box)
            continue;

        if (!rootHasSelectedChildren && box->renderer()->selectionState() != RenderObject::SelectionNone)
            rootHasSelectedChildren = true;

        // If we have no parent box yet, or if the run is not simply a sibling,
        // then we need to construct inline boxes as necessary to properly enclose the
        // run's inline box. Segments can only be siblings at the root level, as
        // they are positioned separately.
        bool runStartsSegment = r->m_startsSegment;

        if (!parentBox || parentBox->renderer() != r->m_object->parent() || runStartsSegment)
            // Create new inline boxes all the way back to the appropriate insertion point.
            parentBox = createLineBoxes(r->m_object->parent(), lineInfo, box, runStartsSegment);
        else {
            // Append the inline box to this line.
            parentBox->addToLine(box);
        }

        bool visuallyOrdered = r->m_object->style()->rtlOrdering() == VisualOrder;
        box->setBidiLevel(r->level());

        if (box->isInlineTextBox()) {
            InlineTextBox* text = toInlineTextBox(box);
            text->setStart(r->m_start);
            text->setLen(r->m_stop - r->m_start);
            text->setDirOverride(r->dirOverride(visuallyOrdered));
            if (r->m_hasHyphen)
                text->setHasHyphen(true);
        }
    }

    // We should have a root inline box.  It should be unconstructed and
    // be the last continuation of our line list.
    ASSERT(lastLineBox() && !lastLineBox()->isConstructed());

    // Set the m_selectedChildren flag on the root inline box if one of the leaf inline box
    // from the bidi runs walk above has a selection state.
    if (rootHasSelectedChildren)
        lastLineBox()->root()->setHasSelectedChildren(true);

    // Set bits on our inline flow boxes that indicate which sides should
    // paint borders/margins/padding.  This knowledge will ultimately be used when
    // we determine the horizontal positions and widths of all the inline boxes on
    // the line.
    bool isLogicallyLastRunWrapped = bidiRuns.logicallyLastRun()->m_object && bidiRuns.logicallyLastRun()->m_object->isText() ? !reachedEndOfTextRenderer(bidiRuns) : true;
    lastLineBox()->determineSpacingForFlowBoxes(lineInfo.isLastLine(), isLogicallyLastRunWrapped, bidiRuns.logicallyLastRun()->m_object);

    // Now mark the line boxes as being constructed.
    lastLineBox()->setConstructed();

    // Return the last line.
    return lastRootBox();
}

ETextAlign RenderBlockFlow::textAlignmentForLine(bool endsWithSoftBreak) const
{
    ETextAlign alignment = style()->textAlign();
    if (endsWithSoftBreak)
        return alignment;

    if (!RuntimeEnabledFeatures::css3TextEnabled())
        return (alignment == JUSTIFY) ? TASTART : alignment;

    TextAlignLast alignmentLast = style()->textAlignLast();
    switch (alignmentLast) {
    case TextAlignLastStart:
        return TASTART;
    case TextAlignLastEnd:
        return TAEND;
    case TextAlignLastLeft:
        return LEFT;
    case TextAlignLastRight:
        return RIGHT;
    case TextAlignLastCenter:
        return CENTER;
    case TextAlignLastJustify:
        return JUSTIFY;
    case TextAlignLastAuto:
        if (alignment != JUSTIFY)
            return alignment;
        if (style()->textJustify() == TextJustifyDistribute)
            return JUSTIFY;
        return TASTART;
    }

    return alignment;
}

static void updateLogicalWidthForLeftAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // The direction of the block should determine what happens with wide lines.
    // In particular with RTL blocks, wide lines should still spill out to the left.
    if (isLeftToRightDirection) {
        if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun)
            trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        return;
    }

    if (trailingSpaceRun)
        trailingSpaceRun->m_box->setLogicalWidth(0);
    else if (totalLogicalWidth > availableLogicalWidth)
        logicalLeft -= (totalLogicalWidth - availableLogicalWidth);
}

static void updateLogicalWidthForRightAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    // Wide lines spill out of the block based off direction.
    // So even if text-align is right, if direction is LTR, wide lines should overflow out of the right
    // side of the block.
    if (isLeftToRightDirection) {
        if (trailingSpaceRun) {
            totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
            trailingSpaceRun->m_box->setLogicalWidth(0);
        }
        if (totalLogicalWidth < availableLogicalWidth)
            logicalLeft += availableLogicalWidth - totalLogicalWidth;
        return;
    }

    if (totalLogicalWidth > availableLogicalWidth && trailingSpaceRun) {
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceRun->m_box->logicalWidth() - totalLogicalWidth + availableLogicalWidth));
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
    } else
        logicalLeft += availableLogicalWidth - totalLogicalWidth;
}

static void updateLogicalWidthForCenterAlignedBlock(bool isLeftToRightDirection, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float availableLogicalWidth)
{
    float trailingSpaceWidth = 0;
    if (trailingSpaceRun) {
        totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
        trailingSpaceWidth = min(trailingSpaceRun->m_box->logicalWidth(), (availableLogicalWidth - totalLogicalWidth + 1) / 2);
        trailingSpaceRun->m_box->setLogicalWidth(max<float>(0, trailingSpaceWidth));
    }
    if (isLeftToRightDirection)
        logicalLeft += max<float>((availableLogicalWidth - totalLogicalWidth) / 2, 0);
    else
        logicalLeft += totalLogicalWidth > availableLogicalWidth ? (availableLogicalWidth - totalLogicalWidth) : (availableLogicalWidth - totalLogicalWidth) / 2 - trailingSpaceWidth;
}

void RenderBlockFlow::setMarginsForRubyRun(BidiRun* run, RenderRubyRun* renderer, RenderObject* previousObject, const LineInfo& lineInfo)
{
    int startOverhang;
    int endOverhang;
    RenderObject* nextObject = 0;
    for (BidiRun* runWithNextObject = run->next(); runWithNextObject; runWithNextObject = runWithNextObject->next()) {
        if (!runWithNextObject->m_object->isOutOfFlowPositioned() && !runWithNextObject->m_box->isLineBreak()) {
            nextObject = runWithNextObject->m_object;
            break;
        }
    }
    renderer->getOverhang(lineInfo.isFirstLine(), renderer->style()->isLeftToRightDirection() ? previousObject : nextObject, renderer->style()->isLeftToRightDirection() ? nextObject : previousObject, startOverhang, endOverhang);
    setMarginStartForChild(renderer, -startOverhang);
    setMarginEndForChild(renderer, -endOverhang);
}

static inline void setLogicalWidthForTextRun(RootInlineBox* lineBox, BidiRun* run, RenderText* renderer, float xPos, const LineInfo& lineInfo,
                                             GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache, WordMeasurements& wordMeasurements)
{
    HashSet<const SimpleFontData*> fallbackFonts;
    GlyphOverflow glyphOverflow;

    const Font& font = renderer->style(lineInfo.isFirstLine())->font();
    // Always compute glyph overflow if the block's line-box-contain value is "glyphs".
    if (lineBox->fitsToGlyphs()) {
        // If we don't stick out of the root line's font box, then don't bother computing our glyph overflow. This optimization
        // will keep us from computing glyph bounds in nearly all cases.
        bool includeRootLine = lineBox->includesRootLineBoxFontOrLeading();
        int baselineShift = lineBox->verticalPositionForBox(run->m_box, verticalPositionCache);
        int rootDescent = includeRootLine ? font.fontMetrics().descent() : 0;
        int rootAscent = includeRootLine ? font.fontMetrics().ascent() : 0;
        int boxAscent = font.fontMetrics().ascent() - baselineShift;
        int boxDescent = font.fontMetrics().descent() + baselineShift;
        if (boxAscent > rootDescent ||  boxDescent > rootAscent)
            glyphOverflow.computeBounds = true;
    }

    LayoutUnit hyphenWidth = 0;
    if (toInlineTextBox(run->m_box)->hasHyphen()) {
        const Font& font = renderer->style(lineInfo.isFirstLine())->font();
        hyphenWidth = measureHyphenWidth(renderer, font);
    }
    float measuredWidth = 0;

    bool kerningIsEnabled = font.typesettingFeatures() & Kerning;

#if OS(MACOSX)
    // FIXME: Having any font feature settings enabled can lead to selection gaps on
    // Chromium-mac. https://bugs.webkit.org/show_bug.cgi?id=113418
    bool canUseSimpleFontCodePath = renderer->canUseSimpleFontCodePath() && !font.fontDescription().featureSettings();
#else
    bool canUseSimpleFontCodePath = renderer->canUseSimpleFontCodePath();
#endif

    // Since we don't cache glyph overflows, we need to re-measure the run if
    // the style is linebox-contain: glyph.

    if (!lineBox->fitsToGlyphs() && canUseSimpleFontCodePath) {
        int lastEndOffset = run->m_start;
        for (size_t i = 0, size = wordMeasurements.size(); i < size && lastEndOffset < run->m_stop; ++i) {
            const WordMeasurement& wordMeasurement = wordMeasurements[i];
            if (wordMeasurement.width <=0 || wordMeasurement.startOffset == wordMeasurement.endOffset)
                continue;
            if (wordMeasurement.renderer != renderer || wordMeasurement.startOffset != lastEndOffset || wordMeasurement.endOffset > run->m_stop)
                continue;

            lastEndOffset = wordMeasurement.endOffset;
            if (kerningIsEnabled && lastEndOffset == run->m_stop) {
                int wordLength = lastEndOffset - wordMeasurement.startOffset;
                measuredWidth += renderer->width(wordMeasurement.startOffset, wordLength, xPos, lineInfo.isFirstLine());
                if (i > 0 && wordLength == 1 && renderer->characterAt(wordMeasurement.startOffset) == ' ')
                    measuredWidth += renderer->style()->wordSpacing();
            } else
                measuredWidth += wordMeasurement.width;
            if (!wordMeasurement.fallbackFonts.isEmpty()) {
                HashSet<const SimpleFontData*>::const_iterator end = wordMeasurement.fallbackFonts.end();
                for (HashSet<const SimpleFontData*>::const_iterator it = wordMeasurement.fallbackFonts.begin(); it != end; ++it)
                    fallbackFonts.add(*it);
            }
        }
        if (measuredWidth && lastEndOffset != run->m_stop) {
            // If we don't have enough cached data, we'll measure the run again.
            measuredWidth = 0;
            fallbackFonts.clear();
        }
    }

    if (!measuredWidth)
        measuredWidth = renderer->width(run->m_start, run->m_stop - run->m_start, xPos, lineInfo.isFirstLine(), &fallbackFonts, &glyphOverflow);

    run->m_box->setLogicalWidth(measuredWidth + hyphenWidth);
    if (!fallbackFonts.isEmpty()) {
        ASSERT(run->m_box->isText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(toInlineTextBox(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).iterator;
        ASSERT(it->value.first.isEmpty());
        copyToVector(fallbackFonts, it->value.first);
        run->m_box->parent()->clearDescendantsHaveSameLineHeightAndBaseline();
    }
    if ((glyphOverflow.top || glyphOverflow.bottom || glyphOverflow.left || glyphOverflow.right)) {
        ASSERT(run->m_box->isText());
        GlyphOverflowAndFallbackFontsMap::iterator it = textBoxDataMap.add(toInlineTextBox(run->m_box), make_pair(Vector<const SimpleFontData*>(), GlyphOverflow())).iterator;
        it->value.second = glyphOverflow;
        run->m_box->clearKnownToHaveNoOverflow();
    }
}

static inline void computeExpansionForJustifiedText(BidiRun* firstRun, BidiRun* trailingSpaceRun, Vector<unsigned, 16>& expansionOpportunities, unsigned expansionOpportunityCount, float& totalLogicalWidth, float availableLogicalWidth)
{
    if (!expansionOpportunityCount || availableLogicalWidth <= totalLogicalWidth)
        return;

    size_t i = 0;
    for (BidiRun* r = firstRun; r; r = r->next()) {
        // This method is called once per segment, do not move past the current segment.
        if (r->m_startsSegment)
            break;
        if (!r->m_box || r == trailingSpaceRun)
            continue;

        if (r->m_object->isText()) {
            unsigned opportunitiesInRun = expansionOpportunities[i++];

            ASSERT(opportunitiesInRun <= expansionOpportunityCount);

            // Only justify text if whitespace is collapsed.
            if (r->m_object->style()->collapseWhiteSpace()) {
                InlineTextBox* textBox = toInlineTextBox(r->m_box);
                int expansion = (availableLogicalWidth - totalLogicalWidth) * opportunitiesInRun / expansionOpportunityCount;
                textBox->setExpansion(expansion);
                totalLogicalWidth += expansion;
            }
            expansionOpportunityCount -= opportunitiesInRun;
            if (!expansionOpportunityCount)
                break;
        }
    }
}

void RenderBlockFlow::updateLogicalWidthForAlignment(const ETextAlign& textAlign, const RootInlineBox* rootInlineBox, BidiRun* trailingSpaceRun, float& logicalLeft, float& totalLogicalWidth, float& availableLogicalWidth, int expansionOpportunityCount)
{
    TextDirection direction;
    if (rootInlineBox && rootInlineBox->renderer()->style()->unicodeBidi() == Plaintext)
        direction = rootInlineBox->direction();
    else
        direction = style()->direction();

    // Armed with the total width of the line (without justification),
    // we now examine our text-align property in order to determine where to position the
    // objects horizontally. The total width of the line can be increased if we end up
    // justifying text.
    switch (textAlign) {
    case LEFT:
    case WEBKIT_LEFT:
        updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case RIGHT:
    case WEBKIT_RIGHT:
        updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case CENTER:
    case WEBKIT_CENTER:
        updateLogicalWidthForCenterAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case JUSTIFY:
        adjustInlineDirectionLineBounds(expansionOpportunityCount, logicalLeft, availableLogicalWidth);
        if (expansionOpportunityCount) {
            if (trailingSpaceRun) {
                totalLogicalWidth -= trailingSpaceRun->m_box->logicalWidth();
                trailingSpaceRun->m_box->setLogicalWidth(0);
            }
            break;
        }
        // Fall through
    case TASTART:
        if (direction == LTR)
            updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        else
            updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    case TAEND:
        if (direction == LTR)
            updateLogicalWidthForRightAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        else
            updateLogicalWidthForLeftAlignedBlock(style()->isLeftToRightDirection(), trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth);
        break;
    }
}

static void updateLogicalInlinePositions(RenderBlockFlow* block, float& lineLogicalLeft, float& lineLogicalRight, float& availableLogicalWidth, bool firstLine, IndentTextOrNot shouldIndentText, LayoutUnit boxLogicalHeight)
{
    LayoutUnit lineLogicalHeight = block->minLineHeightForReplacedRenderer(firstLine, boxLogicalHeight);
    lineLogicalLeft = block->logicalLeftOffsetForLine(block->logicalHeight(), shouldIndentText == IndentText, lineLogicalHeight);
    // FIXME: This shouldn't be pixel snapped once multicolumn layout has been updated to correctly carry over subpixel values.
    // https://bugs.webkit.org/show_bug.cgi?id=105461
    lineLogicalRight = block->pixelSnappedLogicalRightOffsetForLine(block->logicalHeight(), shouldIndentText == IndentText, lineLogicalHeight);
    availableLogicalWidth = lineLogicalRight - lineLogicalLeft;
}

void RenderBlockFlow::computeInlineDirectionPositionsForLine(RootInlineBox* lineBox, const LineInfo& lineInfo, BidiRun* firstRun, BidiRun* trailingSpaceRun, bool reachedEnd,
                                                         GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache, WordMeasurements& wordMeasurements)
{
    ETextAlign textAlign = textAlignmentForLine(!reachedEnd && !lineBox->endsWithBreak());

    // CSS 2.1: "'Text-indent' only affects a line if it is the first formatted line of an element. For example, the first line of an anonymous block
    // box is only affected if it is the first child of its parent element."
    // CSS3 "text-indent", "each-line" affects the first line of the block container as well as each line after a forced line break,
    // but does not affect lines after a soft wrap break.
    bool isFirstLine = lineInfo.isFirstLine() && !(isAnonymousBlock() && parent()->firstChild() != this);
    bool isAfterHardLineBreak = lineBox->prevRootBox() && lineBox->prevRootBox()->endsWithBreak();
    IndentTextOrNot shouldIndentText = requiresIndent(isFirstLine, isAfterHardLineBreak, style());
    float lineLogicalLeft;
    float lineLogicalRight;
    float availableLogicalWidth;
    updateLogicalInlinePositions(this, lineLogicalLeft, lineLogicalRight, availableLogicalWidth, isFirstLine, shouldIndentText, 0);
    bool needsWordSpacing;
    ShapeInsideInfo* shapeInsideInfo = layoutShapeInsideInfo();
    if (shapeInsideInfo && shapeInsideInfo->hasSegments()) {
        BidiRun* segmentStart = firstRun;
        const SegmentList& segments = shapeInsideInfo->segments();
        float logicalLeft = max<float>(segments[0].logicalLeft, lineLogicalLeft);
        float logicalRight = min<float>(segments[0].logicalRight, lineLogicalRight);
        float startLogicalLeft = logicalLeft;
        float endLogicalRight = logicalLeft;
        float minLogicalLeft = logicalLeft;
        float maxLogicalRight = logicalLeft;
        lineBox->beginPlacingBoxRangesInInlineDirection(logicalLeft);
        for (size_t i = 0; i < segments.size(); i++) {
            if (i) {
                logicalLeft = max<float>(segments[i].logicalLeft, lineLogicalLeft);
                logicalRight = min<float>(segments[i].logicalRight, lineLogicalRight);
            }
            availableLogicalWidth = logicalRight - logicalLeft;
            BidiRun* newSegmentStart = computeInlineDirectionPositionsForSegment(lineBox, lineInfo, textAlign, logicalLeft, availableLogicalWidth, segmentStart, trailingSpaceRun, textBoxDataMap, verticalPositionCache, wordMeasurements);
            needsWordSpacing = false;
            endLogicalRight = lineBox->placeBoxRangeInInlineDirection(segmentStart->m_box, newSegmentStart ? newSegmentStart->m_box : 0, logicalLeft, minLogicalLeft, maxLogicalRight, needsWordSpacing, textBoxDataMap);
            if (!newSegmentStart || !newSegmentStart->next())
                break;
            ASSERT(newSegmentStart->m_startsSegment);
            // Discard the empty segment start marker bidi runs
            segmentStart = newSegmentStart->next();
        }
        lineBox->endPlacingBoxRangesInInlineDirection(startLogicalLeft, endLogicalRight, minLogicalLeft, maxLogicalRight);
        return;
    }

    if (firstRun && firstRun->m_object->isReplaced()) {
        RenderBox* renderBox = toRenderBox(firstRun->m_object);
        updateLogicalInlinePositions(this, lineLogicalLeft, lineLogicalRight, availableLogicalWidth, isFirstLine, shouldIndentText, renderBox->logicalHeight());
    }

    computeInlineDirectionPositionsForSegment(lineBox, lineInfo, textAlign, lineLogicalLeft, availableLogicalWidth, firstRun, trailingSpaceRun, textBoxDataMap, verticalPositionCache, wordMeasurements);
    // The widths of all runs are now known. We can now place every inline box (and
    // compute accurate widths for the inline flow boxes).
    needsWordSpacing = false;
    lineBox->placeBoxesInInlineDirection(lineLogicalLeft, needsWordSpacing, textBoxDataMap);
}

BidiRun* RenderBlockFlow::computeInlineDirectionPositionsForSegment(RootInlineBox* lineBox, const LineInfo& lineInfo, ETextAlign textAlign, float& logicalLeft,
    float& availableLogicalWidth, BidiRun* firstRun, BidiRun* trailingSpaceRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap, VerticalPositionCache& verticalPositionCache,
    WordMeasurements& wordMeasurements)
{
    bool needsWordSpacing = false;
    float totalLogicalWidth = lineBox->getFlowSpacingLogicalWidth();
    unsigned expansionOpportunityCount = 0;
    bool isAfterExpansion = true;
    Vector<unsigned, 16> expansionOpportunities;
    RenderObject* previousObject = 0;
    TextJustify textJustify = style()->textJustify();

    BidiRun* r = firstRun;
    for (; r; r = r->next()) {
        // Once we have reached the start of the next segment, we have finished
        // computing the positions for this segment's contents.
        if (r->m_startsSegment)
            break;
        if (!r->m_box || r->m_object->isOutOfFlowPositioned() || r->m_box->isLineBreak())
            continue; // Positioned objects are only participating to figure out their
                      // correct static x position.  They have no effect on the width.
                      // Similarly, line break boxes have no effect on the width.
        if (r->m_object->isText()) {
            RenderText* rt = toRenderText(r->m_object);
            if (textAlign == JUSTIFY && r != trailingSpaceRun && textJustify != TextJustifyNone) {
                if (!isAfterExpansion)
                    toInlineTextBox(r->m_box)->setCanHaveLeadingExpansion(true);
                unsigned opportunitiesInRun;
                if (rt->is8Bit())
                    opportunitiesInRun = Font::expansionOpportunityCount(rt->characters8() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                else
                    opportunitiesInRun = Font::expansionOpportunityCount(rt->characters16() + r->m_start, r->m_stop - r->m_start, r->m_box->direction(), isAfterExpansion);
                expansionOpportunities.append(opportunitiesInRun);
                expansionOpportunityCount += opportunitiesInRun;
            }

            if (int length = rt->textLength()) {
                if (!r->m_start && needsWordSpacing && isSpaceOrNewline(rt->characterAt(r->m_start)))
                    totalLogicalWidth += rt->style(lineInfo.isFirstLine())->font().wordSpacing();
                needsWordSpacing = !isSpaceOrNewline(rt->characterAt(r->m_stop - 1)) && r->m_stop == length;
            }

            setLogicalWidthForTextRun(lineBox, r, rt, totalLogicalWidth, lineInfo, textBoxDataMap, verticalPositionCache, wordMeasurements);
        } else {
            isAfterExpansion = false;
            if (!r->m_object->isRenderInline()) {
                RenderBox* renderBox = toRenderBox(r->m_object);
                if (renderBox->isRubyRun())
                    setMarginsForRubyRun(r, toRenderRubyRun(renderBox), previousObject, lineInfo);
                r->m_box->setLogicalWidth(logicalWidthForChild(renderBox));
                totalLogicalWidth += marginStartForChild(renderBox) + marginEndForChild(renderBox);
            }
        }

        totalLogicalWidth += r->m_box->logicalWidth();
        previousObject = r->m_object;
    }

    if (isAfterExpansion && !expansionOpportunities.isEmpty()) {
        expansionOpportunities.last()--;
        expansionOpportunityCount--;
    }

    updateLogicalWidthForAlignment(textAlign, lineBox, trailingSpaceRun, logicalLeft, totalLogicalWidth, availableLogicalWidth, expansionOpportunityCount);

    computeExpansionForJustifiedText(firstRun, trailingSpaceRun, expansionOpportunities, expansionOpportunityCount, totalLogicalWidth, availableLogicalWidth);

    return r;
}

void RenderBlockFlow::computeBlockDirectionPositionsForLine(RootInlineBox* lineBox, BidiRun* firstRun, GlyphOverflowAndFallbackFontsMap& textBoxDataMap,
                                                        VerticalPositionCache& verticalPositionCache)
{
    setLogicalHeight(lineBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache));

    // Now make sure we place replaced render objects correctly.
    for (BidiRun* r = firstRun; r; r = r->next()) {
        ASSERT(r->m_box);
        if (!r->m_box)
            continue; // Skip runs with no line boxes.

        // Align positioned boxes with the top of the line box.  This is
        // a reasonable approximation of an appropriate y position.
        if (r->m_object->isOutOfFlowPositioned())
            r->m_box->setLogicalTop(logicalHeight());

        // Position is used to properly position both replaced elements and
        // to update the static normal flow x/y of positioned elements.
        if (r->m_object->isText())
            toRenderText(r->m_object)->positionLineBox(r->m_box);
        else if (r->m_object->isBox())
            toRenderBox(r->m_object)->positionLineBox(r->m_box);
    }
    // Positioned objects and zero-length text nodes destroy their boxes in
    // position(), which unnecessarily dirties the line.
    lineBox->markDirty(false);
}

static inline bool isCollapsibleSpace(UChar character, RenderText* renderer)
{
    if (character == ' ' || character == '\t' || character == softHyphen)
        return true;
    if (character == '\n')
        return !renderer->style()->preserveNewline();
    return false;
}

template <typename CharacterType>
static inline int findFirstTrailingSpace(RenderText* lastText, const CharacterType* characters, int start, int stop)
{
    int firstSpace = stop;
    while (firstSpace > start) {
        UChar current = characters[firstSpace - 1];
        if (!isCollapsibleSpace(current, lastText))
            break;
        firstSpace--;
    }

    return firstSpace;
}

inline BidiRun* RenderBlockFlow::handleTrailingSpaces(BidiRunList<BidiRun>& bidiRuns, BidiContext* currentContext)
{
    if (!bidiRuns.runCount()
        || !bidiRuns.logicallyLastRun()->m_object->style()->breakOnlyAfterWhiteSpace()
        || !bidiRuns.logicallyLastRun()->m_object->style()->autoWrap())
        return 0;

    BidiRun* trailingSpaceRun = bidiRuns.logicallyLastRun();
    RenderObject* lastObject = trailingSpaceRun->m_object;
    if (!lastObject->isText())
        return 0;

    RenderText* lastText = toRenderText(lastObject);
    int firstSpace;
    if (lastText->is8Bit())
        firstSpace = findFirstTrailingSpace(lastText, lastText->characters8(), trailingSpaceRun->start(), trailingSpaceRun->stop());
    else
        firstSpace = findFirstTrailingSpace(lastText, lastText->characters16(), trailingSpaceRun->start(), trailingSpaceRun->stop());

    if (firstSpace == trailingSpaceRun->stop())
        return 0;

    TextDirection direction = style()->direction();
    bool shouldReorder = trailingSpaceRun != (direction == LTR ? bidiRuns.lastRun() : bidiRuns.firstRun());
    if (firstSpace != trailingSpaceRun->start()) {
        BidiContext* baseContext = currentContext;
        while (BidiContext* parent = baseContext->parent())
            baseContext = parent;

        BidiRun* newTrailingRun = new BidiRun(firstSpace, trailingSpaceRun->m_stop, trailingSpaceRun->m_object, baseContext, OtherNeutral);
        trailingSpaceRun->m_stop = firstSpace;
        if (direction == LTR)
            bidiRuns.addRun(newTrailingRun);
        else
            bidiRuns.prependRun(newTrailingRun);
        trailingSpaceRun = newTrailingRun;
        return trailingSpaceRun;
    }
    if (!shouldReorder)
        return trailingSpaceRun;

    if (direction == LTR) {
        bidiRuns.moveRunToEnd(trailingSpaceRun);
        trailingSpaceRun->m_level = 0;
    } else {
        bidiRuns.moveRunToBeginning(trailingSpaceRun);
        trailingSpaceRun->m_level = 1;
    }
    return trailingSpaceRun;
}

void RenderBlockFlow::appendFloatingObjectToLastLine(FloatingObject* floatingObject)
{
    ASSERT(!floatingObject->originatingLine());
    floatingObject->setOriginatingLine(lastRootBox());
    lastRootBox()->appendFloat(floatingObject->renderer());
}

// FIXME: This should be a BidiStatus constructor or create method.
static inline BidiStatus statusWithDirection(TextDirection textDirection, bool isOverride)
{
    WTF::Unicode::Direction direction = textDirection == LTR ? LeftToRight : RightToLeft;
    RefPtr<BidiContext> context = BidiContext::create(textDirection == LTR ? 0 : 1, direction, isOverride, FromStyleOrDOM);

    // This copies BidiStatus and may churn the ref on BidiContext I doubt it matters.
    return BidiStatus(direction, direction, direction, context.release());
}

static inline void setupResolverToResumeInIsolate(InlineBidiResolver& resolver, RenderObject* root, RenderObject* startObject)
{
    if (root != startObject) {
        RenderObject* parent = startObject->parent();
        setupResolverToResumeInIsolate(resolver, root, parent);
        notifyObserverEnteredObject(&resolver, startObject);
    }
}

static void restoreIsolatedMidpointStates(InlineBidiResolver& topResolver, InlineBidiResolver& isolatedResolver)
{
    while (!isolatedResolver.isolatedRuns().isEmpty()) {
        BidiRun* run = isolatedResolver.isolatedRuns().last();
        isolatedResolver.isolatedRuns().removeLast();
        topResolver.setMidpointStateForIsolatedRun(run, isolatedResolver.midpointStateForIsolatedRun(run));
    }
}

// FIXME: BidiResolver should have this logic.
static inline void constructBidiRunsForSegment(InlineBidiResolver& topResolver, BidiRunList<BidiRun>& bidiRuns, const InlineIterator& endOfRuns, VisualDirectionOverride override, bool previousLineBrokeCleanly, bool isNewUBAParagraph)
{
    // FIXME: We should pass a BidiRunList into createBidiRunsForLine instead
    // of the resolver owning the runs.
    ASSERT(&topResolver.runs() == &bidiRuns);
    ASSERT(topResolver.position() != endOfRuns);
    RenderObject* currentRoot = topResolver.position().root();
    topResolver.createBidiRunsForLine(endOfRuns, override, previousLineBrokeCleanly);

    while (!topResolver.isolatedRuns().isEmpty()) {
        // It does not matter which order we resolve the runs as long as we resolve them all.
        BidiRun* isolatedRun = topResolver.isolatedRuns().last();
        topResolver.isolatedRuns().removeLast();

        RenderObject* startObj = isolatedRun->object();

        // Only inlines make sense with unicode-bidi: isolate (blocks are already isolated).
        // FIXME: Because enterIsolate is not passed a RenderObject, we have to crawl up the
        // tree to see which parent inline is the isolate. We could change enterIsolate
        // to take a RenderObject and do this logic there, but that would be a layering
        // violation for BidiResolver (which knows nothing about RenderObject).
        RenderInline* isolatedInline = toRenderInline(highestContainingIsolateWithinRoot(startObj, currentRoot));
        ASSERT(isolatedInline);

        InlineBidiResolver isolatedResolver;
        LineMidpointState& isolatedLineMidpointState = isolatedResolver.midpointState();
        isolatedLineMidpointState = topResolver.midpointStateForIsolatedRun(isolatedRun);
        EUnicodeBidi unicodeBidi = isolatedInline->style()->unicodeBidi();
        TextDirection direction = isolatedInline->style()->direction();
        if (unicodeBidi == Plaintext) {
            if (isNewUBAParagraph)
                direction = determinePlaintextDirectionality(isolatedInline, startObj);
            else
                direction = determinePlaintextDirectionality(isolatedInline);
        } else {
            ASSERT(unicodeBidi == Isolate || unicodeBidi == IsolateOverride);
            direction = isolatedInline->style()->direction();
        }
        isolatedResolver.setStatus(statusWithDirection(direction, isOverride(unicodeBidi)));

        setupResolverToResumeInIsolate(isolatedResolver, isolatedInline, startObj);

        // The starting position is the beginning of the first run within the isolate that was identified
        // during the earlier call to createBidiRunsForLine. This can be but is not necessarily the
        // first run within the isolate.
        InlineIterator iter = InlineIterator(isolatedInline, startObj, isolatedRun->m_start);
        isolatedResolver.setPositionIgnoringNestedIsolates(iter);
        // We stop at the next end of line; we may re-enter this isolate in the next call to constructBidiRuns().
        // FIXME: What should end and previousLineBrokeCleanly be?
        // rniwa says previousLineBrokeCleanly is just a WinIE hack and could always be false here?
        isolatedResolver.createBidiRunsForLine(endOfRuns, NoVisualOverride, previousLineBrokeCleanly);

        ASSERT(isolatedResolver.runs().runCount());
        if (isolatedResolver.runs().runCount())
            bidiRuns.replaceRunWithRuns(isolatedRun, isolatedResolver.runs());

        // If we encountered any nested isolate runs, just move them
        // to the top resolver's list for later processing.
        if (!isolatedResolver.isolatedRuns().isEmpty()) {
            topResolver.isolatedRuns().append(isolatedResolver.isolatedRuns());
            currentRoot = isolatedInline;
            restoreIsolatedMidpointStates(topResolver, isolatedResolver);
        }
    }
}

static inline bool segmentIsEmpty(const InlineIterator& segmentStart, const InlineIterator& segmentEnd)
{
    return segmentStart == segmentEnd;
}

static inline void constructBidiRunsForLine(const RenderBlockFlow* block, InlineBidiResolver& topResolver, BidiRunList<BidiRun>& bidiRuns, const InlineIterator& endOfLine, VisualDirectionOverride override, bool previousLineBrokeCleanly, bool isNewUBAParagraph)
{
    ShapeInsideInfo* shapeInsideInfo = block->layoutShapeInsideInfo();
    if (!shapeInsideInfo || !shapeInsideInfo->hasSegments()) {
        constructBidiRunsForSegment(topResolver, bidiRuns, endOfLine, override, previousLineBrokeCleanly, isNewUBAParagraph);
        return;
    }

    const SegmentRangeList& segmentRanges = shapeInsideInfo->segmentRanges();
    ASSERT(segmentRanges.size());

    for (size_t i = 0; i < segmentRanges.size(); i++) {
        LineSegmentIterator iterator = segmentRanges[i].start;
        InlineIterator segmentStart(iterator.root, iterator.object, iterator.offset);
        iterator = segmentRanges[i].end;
        InlineIterator segmentEnd(iterator.root, iterator.object, iterator.offset);
        if (i) {
            ASSERT(segmentStart.object());
            BidiRun* segmentMarker = createRun(segmentStart.m_pos, segmentStart.m_pos, segmentStart.object(), topResolver);
            segmentMarker->m_startsSegment = true;
            bidiRuns.addRun(segmentMarker);
            // Do not collapse midpoints between segments
            topResolver.midpointState().betweenMidpoints = false;
        }
        if (!segmentIsEmpty(segmentStart, segmentEnd)) {
            topResolver.setPosition(segmentStart, numberOfIsolateAncestors(segmentStart));
            constructBidiRunsForSegment(topResolver, bidiRuns, segmentEnd, override, previousLineBrokeCleanly, isNewUBAParagraph);
        }
    }
}

// This function constructs line boxes for all of the text runs in the resolver and computes their position.
RootInlineBox* RenderBlockFlow::createLineBoxesFromBidiRuns(unsigned bidiLevel, BidiRunList<BidiRun>& bidiRuns, const InlineIterator& end, LineInfo& lineInfo, VerticalPositionCache& verticalPositionCache, BidiRun* trailingSpaceRun, WordMeasurements& wordMeasurements)
{
    if (!bidiRuns.runCount())
        return 0;

    // FIXME: Why is this only done when we had runs?
    lineInfo.setLastLine(!end.object());

    RootInlineBox* lineBox = constructLine(bidiRuns, lineInfo);
    if (!lineBox)
        return 0;

    lineBox->setBidiLevel(bidiLevel);
    lineBox->setEndsWithBreak(lineInfo.previousLineBrokeCleanly());

    bool isSVGRootInlineBox = lineBox->isSVGRootInlineBox();

    GlyphOverflowAndFallbackFontsMap textBoxDataMap;

    // Now we position all of our text runs horizontally.
    if (!isSVGRootInlineBox)
        computeInlineDirectionPositionsForLine(lineBox, lineInfo, bidiRuns.firstRun(), trailingSpaceRun, end.atEnd(), textBoxDataMap, verticalPositionCache, wordMeasurements);

    // Now position our text runs vertically.
    computeBlockDirectionPositionsForLine(lineBox, bidiRuns.firstRun(), textBoxDataMap, verticalPositionCache);

    // SVG text layout code computes vertical & horizontal positions on its own.
    // Note that we still need to execute computeVerticalPositionsForLine() as
    // it calls InlineTextBox::positionLineBox(), which tracks whether the box
    // contains reversed text or not. If we wouldn't do that editing and thus
    // text selection in RTL boxes would not work as expected.
    if (isSVGRootInlineBox) {
        ASSERT(isSVGText());
        toSVGRootInlineBox(lineBox)->computePerCharacterLayoutInformation();
    }

    // Compute our overflow now.
    lineBox->computeOverflow(lineBox->lineTop(), lineBox->lineBottom(), textBoxDataMap);

    return lineBox;
}

// Like LayoutState for layout(), LineLayoutState keeps track of global information
// during an entire linebox tree layout pass (aka layoutInlineChildren).
class LineLayoutState {
public:
    LineLayoutState(bool fullLayout, LayoutUnit& repaintLogicalTop, LayoutUnit& repaintLogicalBottom, RenderFlowThread* flowThread)
        : m_lastFloat(0)
        , m_endLine(0)
        , m_floatIndex(0)
        , m_endLineLogicalTop(0)
        , m_endLineMatched(false)
        , m_checkForFloatsFromLastLine(false)
        , m_isFullLayout(fullLayout)
        , m_repaintLogicalTop(repaintLogicalTop)
        , m_repaintLogicalBottom(repaintLogicalBottom)
        , m_adjustedLogicalLineTop(0)
        , m_usesRepaintBounds(false)
        , m_flowThread(flowThread)
    { }

    void markForFullLayout() { m_isFullLayout = true; }
    bool isFullLayout() const { return m_isFullLayout; }

    bool usesRepaintBounds() const { return m_usesRepaintBounds; }

    void setRepaintRange(LayoutUnit logicalHeight)
    {
        m_usesRepaintBounds = true;
        m_repaintLogicalTop = m_repaintLogicalBottom = logicalHeight;
    }

    void updateRepaintRangeFromBox(RootInlineBox* box, LayoutUnit paginationDelta = 0)
    {
        m_usesRepaintBounds = true;
        m_repaintLogicalTop = min(m_repaintLogicalTop, box->logicalTopVisualOverflow() + min<LayoutUnit>(paginationDelta, 0));
        m_repaintLogicalBottom = max(m_repaintLogicalBottom, box->logicalBottomVisualOverflow() + max<LayoutUnit>(paginationDelta, 0));
    }

    bool endLineMatched() const { return m_endLineMatched; }
    void setEndLineMatched(bool endLineMatched) { m_endLineMatched = endLineMatched; }

    bool checkForFloatsFromLastLine() const { return m_checkForFloatsFromLastLine; }
    void setCheckForFloatsFromLastLine(bool check) { m_checkForFloatsFromLastLine = check; }

    LineInfo& lineInfo() { return m_lineInfo; }
    const LineInfo& lineInfo() const { return m_lineInfo; }

    LayoutUnit endLineLogicalTop() const { return m_endLineLogicalTop; }
    void setEndLineLogicalTop(LayoutUnit logicalTop) { m_endLineLogicalTop = logicalTop; }

    RootInlineBox* endLine() const { return m_endLine; }
    void setEndLine(RootInlineBox* line) { m_endLine = line; }

    FloatingObject* lastFloat() const { return m_lastFloat; }
    void setLastFloat(FloatingObject* lastFloat) { m_lastFloat = lastFloat; }

    Vector<RenderBlockFlow::FloatWithRect>& floats() { return m_floats; }

    unsigned floatIndex() const { return m_floatIndex; }
    void setFloatIndex(unsigned floatIndex) { m_floatIndex = floatIndex; }

    LayoutUnit adjustedLogicalLineTop() const { return m_adjustedLogicalLineTop; }
    void setAdjustedLogicalLineTop(LayoutUnit value) { m_adjustedLogicalLineTop = value; }

    RenderFlowThread* flowThread() const { return m_flowThread; }
    void setFlowThread(RenderFlowThread* thread) { m_flowThread = thread; }

private:
    Vector<RenderBlockFlow::FloatWithRect> m_floats;
    FloatingObject* m_lastFloat;
    RootInlineBox* m_endLine;
    LineInfo m_lineInfo;
    unsigned m_floatIndex;
    LayoutUnit m_endLineLogicalTop;
    bool m_endLineMatched;
    bool m_checkForFloatsFromLastLine;

    bool m_isFullLayout;

    // FIXME: Should this be a range object instead of two ints?
    LayoutUnit& m_repaintLogicalTop;
    LayoutUnit& m_repaintLogicalBottom;

    LayoutUnit m_adjustedLogicalLineTop;

    bool m_usesRepaintBounds;

    RenderFlowThread* m_flowThread;
};

static void deleteLineRange(LineLayoutState& layoutState, RootInlineBox* startLine, RootInlineBox* stopLine = 0)
{
    RootInlineBox* boxToDelete = startLine;
    while (boxToDelete && boxToDelete != stopLine) {
        layoutState.updateRepaintRangeFromBox(boxToDelete);
        // Note: deleteLineRange(firstRootBox()) is not identical to deleteLineBoxTree().
        // deleteLineBoxTree uses nextLineBox() instead of nextRootBox() when traversing.
        RootInlineBox* next = boxToDelete->nextRootBox();
        boxToDelete->deleteLine();
        boxToDelete = next;
    }
}

void RenderBlockFlow::layoutRunsAndFloats(LineLayoutState& layoutState, bool hasInlineChild)
{
    // We want to skip ahead to the first dirty line
    InlineBidiResolver resolver;
    RootInlineBox* startLine = determineStartPosition(layoutState, resolver);

    unsigned consecutiveHyphenatedLines = 0;
    if (startLine) {
        for (RootInlineBox* line = startLine->prevRootBox(); line && line->isHyphenated(); line = line->prevRootBox())
            consecutiveHyphenatedLines++;
    }

    // FIXME: This would make more sense outside of this function, but since
    // determineStartPosition can change the fullLayout flag we have to do this here. Failure to call
    // determineStartPosition first will break fast/repaint/line-flow-with-floats-9.html.
    if (layoutState.isFullLayout() && hasInlineChild && !selfNeedsLayout()) {
        // Mark as needing a full layout to force us to repaint. Allow regions
        // to reflow as needed.
        setNeedsLayout(MarkOnlyThis);

        if (RuntimeEnabledFeatures::repaintAfterLayoutEnabled()) {
            setShouldDoFullRepaintAfterLayout(true);
        } else {
            RenderView* v = view();
            if (v && !v->doingFullRepaint() && hasLayer()) {
                // Because we waited until we were already inside layout to discover
                // that the block really needed a full layout, we missed our chance to repaint the layer
                // before layout started. Luckily the layer has cached the repaint rect for its original
                // position and size, and so we can use that to make a repaint happen now.
                repaintUsingContainer(containerForRepaint(), pixelSnappedIntRect(layer()->repainter().repaintRect()));
            }
        }
    }

    if (containsFloats())
        layoutState.setLastFloat(m_floatingObjects->set().last());

    // We also find the first clean line and extract these lines.  We will add them back
    // if we determine that we're able to synchronize after handling all our dirty lines.
    InlineIterator cleanLineStart;
    BidiStatus cleanLineBidiStatus;
    if (!layoutState.isFullLayout() && startLine)
        determineEndPosition(layoutState, startLine, cleanLineStart, cleanLineBidiStatus);

    if (startLine) {
        if (!layoutState.usesRepaintBounds())
            layoutState.setRepaintRange(logicalHeight());
        deleteLineRange(layoutState, startLine);
    }

    if (!layoutState.isFullLayout() && lastRootBox() && lastRootBox()->endsWithBreak()) {
        // If the last line before the start line ends with a line break that clear floats,
        // adjust the height accordingly.
        // A line break can be either the first or the last object on a line, depending on its direction.
        if (InlineBox* lastLeafChild = lastRootBox()->lastLeafChild()) {
            RenderObject* lastObject = lastLeafChild->renderer();
            if (!lastObject->isBR())
                lastObject = lastRootBox()->firstLeafChild()->renderer();
            if (lastObject->isBR()) {
                EClear clear = lastObject->style()->clear();
                if (clear != CNONE)
                    clearFloats(clear);
            }
        }
    }

    layoutRunsAndFloatsInRange(layoutState, resolver, cleanLineStart, cleanLineBidiStatus, consecutiveHyphenatedLines);
    linkToEndLineIfNeeded(layoutState);
    repaintDirtyFloats(layoutState.floats());
}

RenderTextInfo::RenderTextInfo()
    : m_text(0)
    , m_font(0)
{
}

RenderTextInfo::~RenderTextInfo()
{
}

// Before restarting the layout loop with a new logicalHeight, remove all floats that were added and reset the resolver.
inline const InlineIterator& RenderBlockFlow::restartLayoutRunsAndFloatsInRange(LayoutUnit oldLogicalHeight, LayoutUnit newLogicalHeight,  FloatingObject* lastFloatFromPreviousLine, InlineBidiResolver& resolver,  const InlineIterator& oldEnd)
{
    removeFloatingObjectsBelow(lastFloatFromPreviousLine, oldLogicalHeight);
    setLogicalHeight(newLogicalHeight);
    resolver.setPositionIgnoringNestedIsolates(oldEnd);
    return oldEnd;
}

static inline LayoutUnit adjustLogicalLineTop(ShapeInsideInfo* shapeInsideInfo, InlineIterator start, InlineIterator end, const WordMeasurements& wordMeasurements)
{
    if (!shapeInsideInfo || end != start)
        return 0;

    float minWidth = firstPositiveWidth(wordMeasurements);
    ASSERT(minWidth || wordMeasurements.isEmpty());
    if (minWidth > 0 && shapeInsideInfo->adjustLogicalLineTop(minWidth))
        return shapeInsideInfo->logicalLineTop();

    return shapeInsideInfo->shapeLogicalBottom();
}

static inline void pushShapeContentOverflowBelowTheContentBox(RenderBlockFlow* block, ShapeInsideInfo* shapeInsideInfo, LayoutUnit lineTop, LayoutUnit lineHeight)
{
    ASSERT(shapeInsideInfo);

    LayoutUnit logicalLineBottom = lineTop + lineHeight;
    LayoutUnit shapeLogicalBottom = shapeInsideInfo->shapeLogicalBottom();
    LayoutUnit shapeContainingBlockHeight = shapeInsideInfo->shapeContainingBlockHeight();

    bool isOverflowPositionedAlready = (shapeContainingBlockHeight - shapeInsideInfo->owner()->borderAndPaddingAfter() + lineHeight) <= lineTop;

    // If the last line overlaps with the shape, we don't need the segments anymore
    if (lineTop < shapeLogicalBottom && shapeLogicalBottom < logicalLineBottom)
        shapeInsideInfo->clearSegments();
    if (logicalLineBottom <= shapeLogicalBottom || !shapeContainingBlockHeight || isOverflowPositionedAlready)
        return;

    LayoutUnit newLogicalHeight = block->logicalHeight() + (shapeContainingBlockHeight - (lineTop + shapeInsideInfo->owner()->borderAndPaddingAfter()));
    block->setLogicalHeight(newLogicalHeight);
}

void RenderBlockFlow::updateShapeAndSegmentsForCurrentLine(ShapeInsideInfo*& shapeInsideInfo, const LayoutSize& logicalOffsetFromShapeContainer, LineLayoutState& layoutState)
{
    if (layoutState.flowThread())
        return updateShapeAndSegmentsForCurrentLineInFlowThread(shapeInsideInfo, layoutState);

    if (!shapeInsideInfo)
        return;

    LayoutUnit lineTop = logicalHeight() + logicalOffsetFromShapeContainer.height();
    LayoutUnit lineLeft = logicalOffsetFromShapeContainer.width();
    LayoutUnit lineHeight = this->lineHeight(layoutState.lineInfo().isFirstLine(), isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);

    // FIXME: Bug 95361: It is possible for a line to grow beyond lineHeight, in which case these segments may be incorrect.
    shapeInsideInfo->updateSegmentsForLine(LayoutSize(lineLeft, lineTop), lineHeight);

    pushShapeContentOverflowBelowTheContentBox(this, shapeInsideInfo, lineTop, lineHeight);
}

void RenderBlockFlow::updateShapeAndSegmentsForCurrentLineInFlowThread(ShapeInsideInfo*& shapeInsideInfo, LineLayoutState& layoutState)
{
    ASSERT(layoutState.flowThread());

    RenderRegion* currentRegion = regionAtBlockOffset(logicalHeight());
    if (!currentRegion || !currentRegion->logicalHeight())
        return;

    shapeInsideInfo = currentRegion->shapeInsideInfo();

    RenderRegion* nextRegion = 0;
    if (!currentRegion->isLastRegion()) {
        RenderRegionList regionList = layoutState.flowThread()->renderRegionList();
        RenderRegionList::const_iterator it = regionList.find(currentRegion);
        nextRegion = *(++it);
    }

    // We only want to deal regions with shapes, so we check if the next region has a shape
    if (!shapeInsideInfo && nextRegion && !nextRegion->shapeInsideInfo())
        return;

    LayoutUnit lineHeight = this->lineHeight(layoutState.lineInfo().isFirstLine(), isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes);
    LayoutUnit logicalLineTopInFlowThread = logicalHeight() + offsetFromLogicalTopOfFirstPage();
    LayoutUnit logicalLineBottomInFlowThread = logicalLineTopInFlowThread + lineHeight;
    LayoutUnit logicalRegionTopInFlowThread = currentRegion->logicalTopForFlowThreadContent();
    LayoutUnit logicalRegionBottomInFlowThread = logicalRegionTopInFlowThread + currentRegion->logicalHeight() - currentRegion->borderAndPaddingBefore() - currentRegion->borderAndPaddingAfter();

    LayoutUnit shapeBottomInFlowThread = LayoutUnit::max();
    if (shapeInsideInfo)
        shapeBottomInFlowThread = shapeInsideInfo->shapeLogicalBottom() + currentRegion->logicalTopForFlowThreadContent();

    bool lineOverLapsWithShapeBottom = shapeBottomInFlowThread < logicalLineBottomInFlowThread;
    bool lineOverLapsWithRegionBottom = logicalLineBottomInFlowThread > logicalRegionBottomInFlowThread;
    bool overFlowsToNextRegion = nextRegion && (lineOverLapsWithShapeBottom || lineOverLapsWithRegionBottom);

    // If the line is between two shapes/regions we position the line to the top of the next shape/region
    if (overFlowsToNextRegion) {
        ASSERT(currentRegion != nextRegion);
        LayoutUnit deltaToNextRegion = logicalRegionBottomInFlowThread - logicalLineTopInFlowThread;
        setLogicalHeight(logicalHeight() + deltaToNextRegion);

        currentRegion = nextRegion;
        shapeInsideInfo = currentRegion->shapeInsideInfo();

        logicalLineTopInFlowThread = logicalHeight() + offsetFromLogicalTopOfFirstPage();
        logicalLineBottomInFlowThread = logicalLineTopInFlowThread + lineHeight;
        logicalRegionTopInFlowThread = currentRegion->logicalTopForFlowThreadContent();
        logicalRegionBottomInFlowThread = logicalRegionTopInFlowThread + currentRegion->logicalHeight() - currentRegion->borderAndPaddingBefore() - currentRegion->borderAndPaddingAfter();
    }

    if (!shapeInsideInfo)
        return;

    bool isFirstLineInRegion = logicalLineBottomInFlowThread <= (logicalRegionTopInFlowThread + lineHeight);
    bool isFirstLineAdjusted = (logicalLineTopInFlowThread - logicalRegionTopInFlowThread) < (layoutState.adjustedLogicalLineTop() - currentRegion->borderAndPaddingBefore());
    // We position the first line to the top of the shape in the region or to the previously adjusted position in the shape
    if (isFirstLineInRegion || isFirstLineAdjusted) {
        LayoutUnit shapeTopOffset = layoutState.adjustedLogicalLineTop();
        if (!shapeTopOffset && (shapeInsideInfo->shapeLogicalTop() > 0))
            shapeTopOffset = shapeInsideInfo->shapeLogicalTop();

        LayoutUnit shapePositionInFlowThread = currentRegion->logicalTopForFlowThreadContent() + shapeTopOffset;
        LayoutUnit shapeTopLineTopDelta = shapePositionInFlowThread - logicalLineTopInFlowThread - currentRegion->borderAndPaddingBefore();

        setLogicalHeight(logicalHeight() + shapeTopLineTopDelta);
        logicalLineTopInFlowThread += shapeTopLineTopDelta;
        layoutState.setAdjustedLogicalLineTop(0);
    }

    LayoutUnit lineTop = logicalLineTopInFlowThread - currentRegion->logicalTopForFlowThreadContent() + currentRegion->borderAndPaddingBefore();
    // FIXME: Shape inside on a region does not yet take into account its padding for nested flow blocks
    shapeInsideInfo->updateSegmentsForLine(LayoutSize(0, lineTop), lineHeight);

    if (currentRegion->isLastRegion())
        pushShapeContentOverflowBelowTheContentBox(this, shapeInsideInfo, lineTop, lineHeight);
}

bool RenderBlockFlow::adjustLogicalLineTopAndLogicalHeightIfNeeded(ShapeInsideInfo* shapeInsideInfo, LayoutUnit absoluteLogicalTop, LineLayoutState& layoutState, InlineBidiResolver& resolver, FloatingObject* lastFloatFromPreviousLine, InlineIterator& end, WordMeasurements& wordMeasurements)
{
    LayoutUnit adjustedLogicalLineTop = adjustLogicalLineTop(shapeInsideInfo, resolver.position(), end, wordMeasurements);

    if (shapeInsideInfo && containsFloats()) {
        lastFloatFromPreviousLine = m_floatingObjects->set().last();
        if (!wordMeasurements.size()) {
            LayoutUnit floatLogicalTopOffset = shapeInsideInfo->computeFirstFitPositionForFloat(logicalSizeForFloat(lastFloatFromPreviousLine));
            if (logicalHeight() < floatLogicalTopOffset)
                adjustedLogicalLineTop = floatLogicalTopOffset;
        }
    }

    if (!adjustedLogicalLineTop)
        return false;

    LayoutUnit newLogicalHeight = adjustedLogicalLineTop - absoluteLogicalTop;

    if (layoutState.flowThread()) {
        layoutState.setAdjustedLogicalLineTop(adjustedLogicalLineTop);
        newLogicalHeight = logicalHeight();
    }

    end = restartLayoutRunsAndFloatsInRange(logicalHeight(), newLogicalHeight, lastFloatFromPreviousLine, resolver, end);
    return true;
}

void RenderBlockFlow::layoutRunsAndFloatsInRange(LineLayoutState& layoutState, InlineBidiResolver& resolver, const InlineIterator& cleanLineStart, const BidiStatus& cleanLineBidiStatus, unsigned consecutiveHyphenatedLines)
{
    RenderStyle* styleToUse = style();
    bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
    LineMidpointState& lineMidpointState = resolver.midpointState();
    InlineIterator endOfLine = resolver.position();
    bool checkForEndLineMatch = layoutState.endLine();
    RenderTextInfo renderTextInfo;
    VerticalPositionCache verticalPositionCache;

    LineBreaker lineBreaker(this);

    LayoutSize logicalOffsetFromShapeContainer;
    ShapeInsideInfo* shapeInsideInfo = layoutShapeInsideInfo();
    if (shapeInsideInfo) {
        ASSERT(shapeInsideInfo->owner() == this || allowsShapeInsideInfoSharing(shapeInsideInfo->owner()));
        if (shapeInsideInfo != this->shapeInsideInfo()) {
            // FIXME Bug 100284: If subsequent LayoutStates are pushed, we will have to add
            // their offsets from the original shape-inside container.
            logicalOffsetFromShapeContainer = logicalOffsetFromShapeAncestorContainer(shapeInsideInfo->owner());
        }
        // Begin layout at the logical top of our shape inside.
        if (logicalHeight() + logicalOffsetFromShapeContainer.height() < shapeInsideInfo->shapeLogicalTop()) {
            LayoutUnit logicalHeight = shapeInsideInfo->shapeLogicalTop() - logicalOffsetFromShapeContainer.height();
            if (layoutState.flowThread())
                logicalHeight -= shapeInsideInfo->owner()->borderAndPaddingBefore();
            setLogicalHeight(logicalHeight);
        }
    }

    while (!endOfLine.atEnd()) {
        // FIXME: Is this check necessary before the first iteration or can it be moved to the end?
        if (checkForEndLineMatch) {
            layoutState.setEndLineMatched(matchedEndLine(layoutState, resolver, cleanLineStart, cleanLineBidiStatus));
            if (layoutState.endLineMatched()) {
                resolver.setPosition(InlineIterator(resolver.position().root(), 0, 0), 0);
                break;
            }
        }

        lineMidpointState.reset();

        layoutState.lineInfo().setEmpty(true);
        layoutState.lineInfo().resetRunsFromLeadingWhitespace();

        const InlineIterator previousEndofLine = endOfLine;
        bool isNewUBAParagraph = layoutState.lineInfo().previousLineBrokeCleanly();
        FloatingObject* lastFloatFromPreviousLine = (containsFloats()) ? m_floatingObjects->set().last() : 0;

        updateShapeAndSegmentsForCurrentLine(shapeInsideInfo, logicalOffsetFromShapeContainer, layoutState);

        WordMeasurements wordMeasurements;
        endOfLine = lineBreaker.nextLineBreak(resolver, layoutState.lineInfo(), renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);
        renderTextInfo.m_lineBreakIterator.resetPriorContext();
        if (resolver.position().atEnd()) {
            // FIXME: We shouldn't be creating any runs in nextLineBreak to begin with!
            // Once BidiRunList is separated from BidiResolver this will not be needed.
            resolver.runs().deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).
            layoutState.setCheckForFloatsFromLastLine(true);
            resolver.setPosition(InlineIterator(resolver.position().root(), 0, 0), 0);
            break;
        }

        if (adjustLogicalLineTopAndLogicalHeightIfNeeded(shapeInsideInfo, logicalOffsetFromShapeContainer.height(), layoutState, resolver, lastFloatFromPreviousLine, endOfLine, wordMeasurements))
            continue;

        ASSERT(endOfLine != resolver.position());

        // This is a short-cut for empty lines.
        if (layoutState.lineInfo().isEmpty()) {
            if (lastRootBox())
                lastRootBox()->setLineBreakInfo(endOfLine.object(), endOfLine.m_pos, resolver.status());
        } else {
            VisualDirectionOverride override = (styleToUse->rtlOrdering() == VisualOrder ? (styleToUse->direction() == LTR ? VisualLeftToRightOverride : VisualRightToLeftOverride) : NoVisualOverride);

            if (isNewUBAParagraph && styleToUse->unicodeBidi() == Plaintext && !resolver.context()->parent()) {
                TextDirection direction = determinePlaintextDirectionality(resolver.position().root(), resolver.position().object(), resolver.position().offset());
                resolver.setStatus(BidiStatus(direction, isOverride(styleToUse->unicodeBidi())));
            }
            // FIXME: This ownership is reversed. We should own the BidiRunList and pass it to createBidiRunsForLine.
            BidiRunList<BidiRun>& bidiRuns = resolver.runs();
            constructBidiRunsForLine(this, resolver, bidiRuns, endOfLine, override, layoutState.lineInfo().previousLineBrokeCleanly(), isNewUBAParagraph);
            ASSERT(resolver.position() == endOfLine);

            BidiRun* trailingSpaceRun = !layoutState.lineInfo().previousLineBrokeCleanly() ? handleTrailingSpaces(bidiRuns, resolver.context()) : 0;

            if (bidiRuns.runCount() && lineBreaker.lineWasHyphenated()) {
                bidiRuns.logicallyLastRun()->m_hasHyphen = true;
                consecutiveHyphenatedLines++;
            } else
                consecutiveHyphenatedLines = 0;

            // Now that the runs have been ordered, we create the line boxes.
            // At the same time we figure out where border/padding/margin should be applied for
            // inline flow boxes.

            LayoutUnit oldLogicalHeight = logicalHeight();
            RootInlineBox* lineBox = createLineBoxesFromBidiRuns(resolver.status().context->level(), bidiRuns, endOfLine, layoutState.lineInfo(), verticalPositionCache, trailingSpaceRun, wordMeasurements);

            bidiRuns.deleteRuns();
            resolver.markCurrentRunEmpty(); // FIXME: This can probably be replaced by an ASSERT (or just removed).

            if (lineBox) {
                lineBox->setLineBreakInfo(endOfLine.object(), endOfLine.m_pos, resolver.status());
                if (layoutState.usesRepaintBounds())
                    layoutState.updateRepaintRangeFromBox(lineBox);

                if (paginated) {
                    LayoutUnit adjustment = 0;
                    adjustLinePositionForPagination(lineBox, adjustment, layoutState.flowThread());
                    if (adjustment) {
                        LayoutUnit oldLineWidth = availableLogicalWidthForLine(oldLogicalHeight, layoutState.lineInfo().isFirstLine());
                        lineBox->adjustBlockDirectionPosition(adjustment);
                        if (layoutState.usesRepaintBounds())
                            layoutState.updateRepaintRangeFromBox(lineBox);

                        if (availableLogicalWidthForLine(oldLogicalHeight + adjustment, layoutState.lineInfo().isFirstLine()) != oldLineWidth) {
                            // We have to delete this line, remove all floats that got added, and let line layout re-run.
                            lineBox->deleteLine();
                            endOfLine = restartLayoutRunsAndFloatsInRange(oldLogicalHeight, oldLogicalHeight + adjustment, lastFloatFromPreviousLine, resolver, previousEndofLine);
                            continue;
                        }

                        setLogicalHeight(lineBox->lineBottomWithLeading());
                    }

                    if (layoutState.flowThread())
                        updateRegionForLine(lineBox);
                }
            }
        }

        for (size_t i = 0; i < lineBreaker.positionedObjects().size(); ++i)
            setStaticPositions(this, lineBreaker.positionedObjects()[i]);

        if (!layoutState.lineInfo().isEmpty()) {
            layoutState.lineInfo().setFirstLine(false);
            clearFloats(lineBreaker.clear());
        }

        if (m_floatingObjects && lastRootBox()) {
            const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
            FloatingObjectSetIterator it = floatingObjectSet.begin();
            FloatingObjectSetIterator end = floatingObjectSet.end();
            if (layoutState.lastFloat()) {
                FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(layoutState.lastFloat());
                ASSERT(lastFloatIterator != end);
                ++lastFloatIterator;
                it = lastFloatIterator;
            }
            for (; it != end; ++it) {
                FloatingObject* f = *it;
                appendFloatingObjectToLastLine(f);
                ASSERT(f->renderer() == layoutState.floats()[layoutState.floatIndex()].object);
                // If a float's geometry has changed, give up on syncing with clean lines.
                if (layoutState.floats()[layoutState.floatIndex()].rect != f->frameRect())
                    checkForEndLineMatch = false;
                layoutState.setFloatIndex(layoutState.floatIndex() + 1);
            }
            layoutState.setLastFloat(!floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0);
        }

        lineMidpointState.reset();
        resolver.setPosition(endOfLine, numberOfIsolateAncestors(endOfLine));
    }

    // In case we already adjusted the line positions during this layout to avoid widows
    // then we need to ignore the possibility of having a new widows situation.
    // Otherwise, we risk leaving empty containers which is against the block fragmentation principles.
    if (paginated && !style()->hasAutoWidows() && !didBreakAtLineToAvoidWidow()) {
        // Check the line boxes to make sure we didn't create unacceptable widows.
        // However, we'll prioritize orphans - so nothing we do here should create
        // a new orphan.

        RootInlineBox* lineBox = lastRootBox();

        // Count from the end of the block backwards, to see how many hanging
        // lines we have.
        RootInlineBox* firstLineInBlock = firstRootBox();
        int numLinesHanging = 1;
        while (lineBox && lineBox != firstLineInBlock && !lineBox->isFirstAfterPageBreak()) {
            ++numLinesHanging;
            lineBox = lineBox->prevRootBox();
        }

        // If there were no breaks in the block, we didn't create any widows.
        if (!lineBox || !lineBox->isFirstAfterPageBreak() || lineBox == firstLineInBlock)
            return;

        if (numLinesHanging < style()->widows()) {
            // We have detected a widow. Now we need to work out how many
            // lines there are on the previous page, and how many we need
            // to steal.
            int numLinesNeeded = style()->widows() - numLinesHanging;
            RootInlineBox* currentFirstLineOfNewPage = lineBox;

            // Count the number of lines in the previous page.
            lineBox = lineBox->prevRootBox();
            int numLinesInPreviousPage = 1;
            while (lineBox && lineBox != firstLineInBlock && !lineBox->isFirstAfterPageBreak()) {
                ++numLinesInPreviousPage;
                lineBox = lineBox->prevRootBox();
            }

            // If there was an explicit value for orphans, respect that. If not, we still
            // shouldn't create a situation where we make an orphan bigger than the initial value.
            // This means that setting widows implies we also care about orphans, but given
            // the specification says the initial orphan value is non-zero, this is ok. The
            // author is always free to set orphans explicitly as well.
            int orphans = style()->hasAutoOrphans() ? style()->initialOrphans() : style()->orphans();
            int numLinesAvailable = numLinesInPreviousPage - orphans;
            if (numLinesAvailable <= 0)
                return;

            int numLinesToTake = min(numLinesAvailable, numLinesNeeded);
            // Wind back from our first widowed line.
            lineBox = currentFirstLineOfNewPage;
            for (int i = 0; i < numLinesToTake; ++i)
                lineBox = lineBox->prevRootBox();

            // We now want to break at this line. Remember for next layout and trigger relayout.
            setBreakAtLineToAvoidWidow(lineCount(lineBox));
            markLinesDirtyInBlockRange(lastRootBox()->lineBottomWithLeading(), lineBox->lineBottomWithLeading(), lineBox);
        }
    }

    clearDidBreakAtLineToAvoidWidow();
}

void RenderBlockFlow::linkToEndLineIfNeeded(LineLayoutState& layoutState)
{
    if (layoutState.endLine()) {
        if (layoutState.endLineMatched()) {
            bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
            // Attach all the remaining lines, and then adjust their y-positions as needed.
            LayoutUnit delta = logicalHeight() - layoutState.endLineLogicalTop();
            for (RootInlineBox* line = layoutState.endLine(); line; line = line->nextRootBox()) {
                line->attachLine();
                if (paginated) {
                    delta -= line->paginationStrut();
                    adjustLinePositionForPagination(line, delta, layoutState.flowThread());
                }
                if (delta) {
                    layoutState.updateRepaintRangeFromBox(line, delta);
                    line->adjustBlockDirectionPosition(delta);
                }
                if (layoutState.flowThread())
                    updateRegionForLine(line);
                if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                    for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                        FloatingObject* floatingObject = insertFloatingObject(*f);
                        ASSERT(!floatingObject->originatingLine());
                        floatingObject->setOriginatingLine(line);
                        setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f) + delta);
                        positionNewFloats();
                    }
                }
            }
            setLogicalHeight(lastRootBox()->lineBottomWithLeading());
        } else {
            // Delete all the remaining lines.
            deleteLineRange(layoutState, layoutState.endLine());
        }
    }

    if (m_floatingObjects && (layoutState.checkForFloatsFromLastLine() || positionNewFloats()) && lastRootBox()) {
        // In case we have a float on the last line, it might not be positioned up to now.
        // This has to be done before adding in the bottom border/padding, or the float will
        // include the padding incorrectly. -dwh
        if (layoutState.checkForFloatsFromLastLine()) {
            LayoutUnit bottomVisualOverflow = lastRootBox()->logicalBottomVisualOverflow();
            LayoutUnit bottomLayoutOverflow = lastRootBox()->logicalBottomLayoutOverflow();
            TrailingFloatsRootInlineBox* trailingFloatsLineBox = new TrailingFloatsRootInlineBox(this);
            m_lineBoxes.appendLineBox(trailingFloatsLineBox);
            trailingFloatsLineBox->setConstructed();
            GlyphOverflowAndFallbackFontsMap textBoxDataMap;
            VerticalPositionCache verticalPositionCache;
            LayoutUnit blockLogicalHeight = logicalHeight();
            trailingFloatsLineBox->alignBoxesInBlockDirection(blockLogicalHeight, textBoxDataMap, verticalPositionCache);
            trailingFloatsLineBox->setLineTopBottomPositions(blockLogicalHeight, blockLogicalHeight, blockLogicalHeight, blockLogicalHeight);
            trailingFloatsLineBox->setPaginatedLineWidth(availableLogicalWidthForContent(blockLogicalHeight));
            LayoutRect logicalLayoutOverflow(0, blockLogicalHeight, 1, bottomLayoutOverflow - blockLogicalHeight);
            LayoutRect logicalVisualOverflow(0, blockLogicalHeight, 1, bottomVisualOverflow - blockLogicalHeight);
            trailingFloatsLineBox->setOverflowFromLogicalRects(logicalLayoutOverflow, logicalVisualOverflow, trailingFloatsLineBox->lineTop(), trailingFloatsLineBox->lineBottom());
            if (layoutState.flowThread())
                updateRegionForLine(trailingFloatsLineBox);
        }

        const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
        FloatingObjectSetIterator it = floatingObjectSet.begin();
        FloatingObjectSetIterator end = floatingObjectSet.end();
        if (layoutState.lastFloat()) {
            FloatingObjectSetIterator lastFloatIterator = floatingObjectSet.find(layoutState.lastFloat());
            ASSERT(lastFloatIterator != end);
            ++lastFloatIterator;
            it = lastFloatIterator;
        }
        for (; it != end; ++it)
            appendFloatingObjectToLastLine(*it);
        layoutState.setLastFloat(!floatingObjectSet.isEmpty() ? floatingObjectSet.last() : 0);
    }
}

void RenderBlockFlow::repaintDirtyFloats(Vector<FloatWithRect>& floats)
{
    size_t floatCount = floats.size();
    // Floats that did not have layout did not repaint when we laid them out. They would have
    // painted by now if they had moved, but if they stayed at (0, 0), they still need to be
    // painted.
    for (size_t i = 0; i < floatCount; ++i) {
        if (!floats[i].everHadLayout) {
            RenderBox* f = floats[i].object;
            if (!f->x() && !f->y() && f->checkForRepaintDuringLayout()) {
                if (RuntimeEnabledFeatures::repaintAfterLayoutEnabled())
                    f->setShouldDoFullRepaintAfterLayout(true);
                else
                    f->repaint();
            }
        }
    }
}

void RenderBlockFlow::layoutInlineChildren(bool relayoutChildren, LayoutUnit& repaintLogicalTop, LayoutUnit& repaintLogicalBottom, LayoutUnit afterEdge)
{
    if (FastTextAutosizer* textAutosizer = document().fastTextAutosizer())
        textAutosizer->inflate(this);

    // Lay out our hypothetical grid line as though it occurs at the top of the block.
    if (view()->layoutState() && view()->layoutState()->lineGrid() == this)
        layoutLineGridBox();

    RenderFlowThread* flowThread = flowThreadContainingBlock();
    bool clearLinesForPagination = firstLineBox() && flowThread && !flowThread->hasRegions();

    // Figure out if we should clear out our line boxes.
    // FIXME: Handle resize eventually!
    bool isFullLayout = !firstLineBox() || selfNeedsLayout() || relayoutChildren || clearLinesForPagination;
    LineLayoutState layoutState(isFullLayout, repaintLogicalTop, repaintLogicalBottom, flowThread);

    if (isFullLayout)
        lineBoxes()->deleteLineBoxes();

    // Text truncation kicks in in two cases:
    //     1) If your overflow isn't visible and your text-overflow-mode isn't clip.
    //     2) If you're an anonymous block with a block parent that satisfies #1 that was created
    //        to accomodate a block that has inline and block children. This excludes parents where
    //        canCollapseAnonymousBlockChild is false, notabley flex items and grid items.
    // FIXME: CSS3 says that descendants that are clipped must also know how to truncate.  This is insanely
    // difficult to figure out in general (especially in the middle of doing layout), so we only handle the
    // simple case of an anonymous block truncating when it's parent is clipped.
    bool hasTextOverflow = (style()->textOverflow() && hasOverflowClip())
        || (isAnonymousBlock() && parent() && parent()->isRenderBlock() && toRenderBlock(parent())->canCollapseAnonymousBlockChild()
            && parent()->style()->textOverflow() && parent()->hasOverflowClip());

    // Walk all the lines and delete our ellipsis line boxes if they exist.
    if (hasTextOverflow)
         deleteEllipsisLineBoxes();

    if (firstChild()) {
        // In full layout mode, clear the line boxes of children upfront. Otherwise,
        // siblings can run into stale root lineboxes during layout. Then layout
        // the replaced elements later. In partial layout mode, line boxes are not
        // deleted and only dirtied. In that case, we can layout the replaced
        // elements at the same time.
        bool hasInlineChild = false;
        Vector<RenderBox*> replacedChildren;
        for (InlineWalker walker(this); !walker.atEnd(); walker.advance()) {
            RenderObject* o = walker.current();

            LayoutRectRecorder recorder(*o, !o->isText());

            if (!hasInlineChild && o->isInline())
                hasInlineChild = true;

            if (o->isReplaced() || o->isFloating() || o->isOutOfFlowPositioned()) {
                RenderBox* box = toRenderBox(o);

                if (relayoutChildren || box->hasRelativeDimensions())
                    o->setChildNeedsLayout(MarkOnlyThis);

                // If relayoutChildren is set and the child has percentage padding or an embedded content box, we also need to invalidate the childs pref widths.
                if (relayoutChildren && box->needsPreferredWidthsRecalculation())
                    o->setPreferredLogicalWidthsDirty(MarkOnlyThis);

                if (o->isOutOfFlowPositioned())
                    o->containingBlock()->insertPositionedObject(box);
                else if (o->isFloating())
                    layoutState.floats().append(FloatWithRect(box));
                else if (isFullLayout || o->needsLayout()) {
                    // Replaced element.
                    box->dirtyLineBoxes(isFullLayout);
                    if (isFullLayout)
                        replacedChildren.append(box);
                    else
                        o->layoutIfNeeded();
                }
            } else if (o->isText() || (o->isRenderInline() && !walker.atEndOfInline())) {
                if (!o->isText())
                    toRenderInline(o)->updateAlwaysCreateLineBoxes(layoutState.isFullLayout());
                if (layoutState.isFullLayout() || o->selfNeedsLayout())
                    dirtyLineBoxesForRenderer(o, layoutState.isFullLayout());
                o->clearNeedsLayout();
            }
        }

        for (size_t i = 0; i < replacedChildren.size(); i++)
            replacedChildren[i]->layoutIfNeeded();

        layoutRunsAndFloats(layoutState, hasInlineChild);
    }

    // Expand the last line to accommodate Ruby and emphasis marks.
    int lastLineAnnotationsAdjustment = 0;
    if (lastRootBox()) {
        LayoutUnit lowestAllowedPosition = max(lastRootBox()->lineBottom(), logicalHeight() + paddingAfter());
        if (!style()->isFlippedLinesWritingMode())
            lastLineAnnotationsAdjustment = lastRootBox()->computeUnderAnnotationAdjustment(lowestAllowedPosition);
        else
            lastLineAnnotationsAdjustment = lastRootBox()->computeOverAnnotationAdjustment(lowestAllowedPosition);
    }

    // Now add in the bottom border/padding.
    setLogicalHeight(logicalHeight() + lastLineAnnotationsAdjustment + afterEdge);

    if (!firstLineBox() && hasLineIfEmpty())
        setLogicalHeight(logicalHeight() + lineHeight(true, isHorizontalWritingMode() ? HorizontalLine : VerticalLine, PositionOfInteriorLineBoxes));

    // See if we have any lines that spill out of our block.  If we do, then we will possibly need to
    // truncate text.
    if (hasTextOverflow)
        checkLinesForTextOverflow();
}

void RenderBlockFlow::checkFloatsInCleanLine(RootInlineBox* line, Vector<FloatWithRect>& floats, size_t& floatIndex, bool& encounteredNewFloat, bool& dirtiedByFloat)
{
    Vector<RenderBox*>* cleanLineFloats = line->floatsPtr();
    if (!cleanLineFloats)
        return;

    Vector<RenderBox*>::iterator end = cleanLineFloats->end();
    for (Vector<RenderBox*>::iterator it = cleanLineFloats->begin(); it != end; ++it) {
        RenderBox* floatingBox = *it;
        floatingBox->layoutIfNeeded();
        LayoutSize newSize(floatingBox->width() + floatingBox->marginWidth(), floatingBox->height() + floatingBox->marginHeight());
        if (floats[floatIndex].object != floatingBox) {
            encounteredNewFloat = true;
            return;
        }

        if (floats[floatIndex].rect.size() != newSize) {
            LayoutUnit floatTop = isHorizontalWritingMode() ? floats[floatIndex].rect.y() : floats[floatIndex].rect.x();
            LayoutUnit floatHeight = isHorizontalWritingMode() ? max(floats[floatIndex].rect.height(), newSize.height())
                                                                 : max(floats[floatIndex].rect.width(), newSize.width());
            floatHeight = min(floatHeight, LayoutUnit::max() - floatTop);
            line->markDirty();
            markLinesDirtyInBlockRange(line->lineBottomWithLeading(), floatTop + floatHeight, line);
            floats[floatIndex].rect.setSize(newSize);
            dirtiedByFloat = true;
        }
        floatIndex++;
    }
}

RootInlineBox* RenderBlockFlow::determineStartPosition(LineLayoutState& layoutState, InlineBidiResolver& resolver)
{
    RootInlineBox* curr = 0;
    RootInlineBox* last = 0;

    // FIXME: This entire float-checking block needs to be broken into a new function.
    bool dirtiedByFloat = false;
    if (!layoutState.isFullLayout()) {
        // Paginate all of the clean lines.
        bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
        LayoutUnit paginationDelta = 0;
        size_t floatIndex = 0;
        for (curr = firstRootBox(); curr && !curr->isDirty(); curr = curr->nextRootBox()) {
            if (paginated) {
                if (lineWidthForPaginatedLineChanged(curr, 0, layoutState.flowThread())) {
                    curr->markDirty();
                    break;
                }
                paginationDelta -= curr->paginationStrut();
                adjustLinePositionForPagination(curr, paginationDelta, layoutState.flowThread());
                if (paginationDelta) {
                    if (containsFloats() || !layoutState.floats().isEmpty()) {
                        // FIXME: Do better eventually.  For now if we ever shift because of pagination and floats are present just go to a full layout.
                        layoutState.markForFullLayout();
                        break;
                    }

                    layoutState.updateRepaintRangeFromBox(curr, paginationDelta);
                    curr->adjustBlockDirectionPosition(paginationDelta);
                }
                if (layoutState.flowThread())
                    updateRegionForLine(curr);
            }

            // If a new float has been inserted before this line or before its last known float, just do a full layout.
            bool encounteredNewFloat = false;
            checkFloatsInCleanLine(curr, layoutState.floats(), floatIndex, encounteredNewFloat, dirtiedByFloat);
            if (encounteredNewFloat)
                layoutState.markForFullLayout();

            if (dirtiedByFloat || layoutState.isFullLayout())
                break;
        }
        // Check if a new float has been inserted after the last known float.
        if (!curr && floatIndex < layoutState.floats().size())
            layoutState.markForFullLayout();
    }

    if (layoutState.isFullLayout()) {
        // FIXME: This should just call deleteLineBoxTree, but that causes
        // crashes for fast/repaint tests.
        curr = firstRootBox();
        while (curr) {
            // Note: This uses nextRootBox() insted of nextLineBox() like deleteLineBoxTree does.
            RootInlineBox* next = curr->nextRootBox();
            curr->deleteLine();
            curr = next;
        }
        ASSERT(!firstLineBox() && !lastLineBox());
    } else {
        if (curr) {
            // We have a dirty line.
            if (RootInlineBox* prevRootBox = curr->prevRootBox()) {
                // We have a previous line.
                if (!dirtiedByFloat && (!prevRootBox->endsWithBreak() || !prevRootBox->lineBreakObj() || (prevRootBox->lineBreakObj()->isText() && prevRootBox->lineBreakPos() >= toRenderText(prevRootBox->lineBreakObj())->textLength())))
                    // The previous line didn't break cleanly or broke at a newline
                    // that has been deleted, so treat it as dirty too.
                    curr = prevRootBox;
            }
        } else {
            // No dirty lines were found.
            // If the last line didn't break cleanly, treat it as dirty.
            if (lastRootBox() && !lastRootBox()->endsWithBreak())
                curr = lastRootBox();
        }

        // If we have no dirty lines, then last is just the last root box.
        last = curr ? curr->prevRootBox() : lastRootBox();
    }

    unsigned numCleanFloats = 0;
    if (!layoutState.floats().isEmpty()) {
        LayoutUnit savedLogicalHeight = logicalHeight();
        // Restore floats from clean lines.
        RootInlineBox* line = firstRootBox();
        while (line != curr) {
            if (Vector<RenderBox*>* cleanLineFloats = line->floatsPtr()) {
                Vector<RenderBox*>::iterator end = cleanLineFloats->end();
                for (Vector<RenderBox*>::iterator f = cleanLineFloats->begin(); f != end; ++f) {
                    FloatingObject* floatingObject = insertFloatingObject(*f);
                    ASSERT(!floatingObject->originatingLine());
                    floatingObject->setOriginatingLine(line);
                    setLogicalHeight(logicalTopForChild(*f) - marginBeforeForChild(*f));
                    positionNewFloats();
                    ASSERT(layoutState.floats()[numCleanFloats].object == *f);
                    numCleanFloats++;
                }
            }
            line = line->nextRootBox();
        }
        setLogicalHeight(savedLogicalHeight);
    }
    layoutState.setFloatIndex(numCleanFloats);

    layoutState.lineInfo().setFirstLine(!last);
    layoutState.lineInfo().setPreviousLineBrokeCleanly(!last || last->endsWithBreak());

    if (last) {
        setLogicalHeight(last->lineBottomWithLeading());
        InlineIterator iter = InlineIterator(this, last->lineBreakObj(), last->lineBreakPos());
        resolver.setPosition(iter, numberOfIsolateAncestors(iter));
        resolver.setStatus(last->lineBreakBidiStatus());
    } else {
        TextDirection direction = style()->direction();
        if (style()->unicodeBidi() == Plaintext)
            direction = determinePlaintextDirectionality(this);
        resolver.setStatus(BidiStatus(direction, isOverride(style()->unicodeBidi())));
        InlineIterator iter = InlineIterator(this, bidiFirstSkippingEmptyInlines(this, &resolver), 0);
        resolver.setPosition(iter, numberOfIsolateAncestors(iter));
    }
    return curr;
}

void RenderBlockFlow::determineEndPosition(LineLayoutState& layoutState, RootInlineBox* startLine, InlineIterator& cleanLineStart, BidiStatus& cleanLineBidiStatus)
{
    ASSERT(!layoutState.endLine());
    size_t floatIndex = layoutState.floatIndex();
    RootInlineBox* last = 0;
    for (RootInlineBox* curr = startLine->nextRootBox(); curr; curr = curr->nextRootBox()) {
        if (!curr->isDirty()) {
            bool encounteredNewFloat = false;
            bool dirtiedByFloat = false;
            checkFloatsInCleanLine(curr, layoutState.floats(), floatIndex, encounteredNewFloat, dirtiedByFloat);
            if (encounteredNewFloat)
                return;
        }
        if (curr->isDirty())
            last = 0;
        else if (!last)
            last = curr;
    }

    if (!last)
        return;

    // At this point, |last| is the first line in a run of clean lines that ends with the last line
    // in the block.

    RootInlineBox* prev = last->prevRootBox();
    cleanLineStart = InlineIterator(this, prev->lineBreakObj(), prev->lineBreakPos());
    cleanLineBidiStatus = prev->lineBreakBidiStatus();
    layoutState.setEndLineLogicalTop(prev->lineBottomWithLeading());

    for (RootInlineBox* line = last; line; line = line->nextRootBox())
        line->extractLine(); // Disconnect all line boxes from their render objects while preserving
                             // their connections to one another.

    layoutState.setEndLine(last);
}

bool RenderBlockFlow::checkPaginationAndFloatsAtEndLine(LineLayoutState& layoutState)
{
    LayoutUnit lineDelta = logicalHeight() - layoutState.endLineLogicalTop();

    bool paginated = view()->layoutState() && view()->layoutState()->isPaginated();
    if (paginated && layoutState.flowThread()) {
        // Check all lines from here to the end, and see if the hypothetical new position for the lines will result
        // in a different available line width.
        for (RootInlineBox* lineBox = layoutState.endLine(); lineBox; lineBox = lineBox->nextRootBox()) {
            if (paginated) {
                // This isn't the real move we're going to do, so don't update the line box's pagination
                // strut yet.
                LayoutUnit oldPaginationStrut = lineBox->paginationStrut();
                lineDelta -= oldPaginationStrut;
                adjustLinePositionForPagination(lineBox, lineDelta, layoutState.flowThread());
                lineBox->setPaginationStrut(oldPaginationStrut);
            }
            if (lineWidthForPaginatedLineChanged(lineBox, lineDelta, layoutState.flowThread()))
                return false;
        }
    }

    if (!lineDelta || !m_floatingObjects)
        return true;

    // See if any floats end in the range along which we want to shift the lines vertically.
    LayoutUnit logicalTop = min(logicalHeight(), layoutState.endLineLogicalTop());

    RootInlineBox* lastLine = layoutState.endLine();
    while (RootInlineBox* nextLine = lastLine->nextRootBox())
        lastLine = nextLine;

    LayoutUnit logicalBottom = lastLine->lineBottomWithLeading() + absoluteValue(lineDelta);

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    FloatingObjectSetIterator end = floatingObjectSet.end();
    for (FloatingObjectSetIterator it = floatingObjectSet.begin(); it != end; ++it) {
        FloatingObject* floatingObject = *it;
        if (logicalBottomForFloat(floatingObject) >= logicalTop && logicalBottomForFloat(floatingObject) < logicalBottom)
            return false;
    }

    return true;
}

bool RenderBlockFlow::matchedEndLine(LineLayoutState& layoutState, const InlineBidiResolver& resolver, const InlineIterator& endLineStart, const BidiStatus& endLineStatus)
{
    if (resolver.position() == endLineStart) {
        if (resolver.status() != endLineStatus)
            return false;
        return checkPaginationAndFloatsAtEndLine(layoutState);
    }

    // The first clean line doesn't match, but we can check a handful of following lines to try
    // to match back up.
    static int numLines = 8; // The # of lines we're willing to match against.
    RootInlineBox* originalEndLine = layoutState.endLine();
    RootInlineBox* line = originalEndLine;
    for (int i = 0; i < numLines && line; i++, line = line->nextRootBox()) {
        if (line->lineBreakObj() == resolver.position().object() && line->lineBreakPos() == resolver.position().m_pos) {
            // We have a match.
            if (line->lineBreakBidiStatus() != resolver.status())
                return false; // ...but the bidi state doesn't match.

            bool matched = false;
            RootInlineBox* result = line->nextRootBox();
            layoutState.setEndLine(result);
            if (result) {
                layoutState.setEndLineLogicalTop(line->lineBottomWithLeading());
                matched = checkPaginationAndFloatsAtEndLine(layoutState);
            }

            // Now delete the lines that we failed to sync.
            deleteLineRange(layoutState, originalEndLine, result);
            return matched;
        }
    }

    return false;
}

bool RenderBlockFlow::generatesLineBoxesForInlineChild(RenderObject* inlineObj)

{
    ASSERT(inlineObj->parent() == this);

    InlineIterator it(this, inlineObj, 0);
    // FIXME: We should pass correct value for WhitespacePosition.
    while (!it.atEnd() && !requiresLineBox(it))
        it.increment();

    return !it.atEnd();
}

void LineBreaker::skipLeadingWhitespace(InlineBidiResolver& resolver, LineInfo& lineInfo,
                                                     FloatingObject* lastFloatFromPreviousLine, LineWidth& width)
{
    while (!resolver.position().atEnd() && !requiresLineBox(resolver.position(), lineInfo, LeadingWhitespace)) {
        RenderObject* object = resolver.position().object();
        if (object->isOutOfFlowPositioned()) {
            setStaticPositions(m_block, toRenderBox(object));
            if (object->style()->isOriginalDisplayInlineType()) {
                resolver.runs().addRun(createRun(0, 1, object, resolver));
                lineInfo.incrementRunsFromLeadingWhitespace();
            }
        } else if (object->isFloating()) {
            // The top margin edge of a self-collapsing block that clears a float intrudes up into it by the height of the margin,
            // so in order to place this first child float at the top content edge of the self-collapsing block add the margin back in before placement.
            LayoutUnit marginOffset = (!object->previousSibling() && m_block->isSelfCollapsingBlock() && m_block->style()->clear() && m_block->getClearDelta(m_block, LayoutUnit())) ? m_block->collapsedMarginBeforeForChild(m_block) : LayoutUnit();
            LayoutUnit oldLogicalHeight = m_block->logicalHeight();
            m_block->setLogicalHeight(oldLogicalHeight + marginOffset);
            m_block->positionNewFloatOnLine(m_block->insertFloatingObject(toRenderBox(object)), lastFloatFromPreviousLine, lineInfo, width);
            m_block->setLogicalHeight(oldLogicalHeight);
        } else if (object->isText() && object->style()->hasTextCombine() && object->isCombineText() && !toRenderCombineText(object)->isCombined()) {
            toRenderCombineText(object)->combineText();
            if (toRenderCombineText(object)->isCombined())
                continue;
        }
        resolver.position().increment(&resolver);
    }
    resolver.commitExplicitEmbedding();
}

void LineBreaker::reset()
{
    m_positionedObjects.clear();
    m_hyphenated = false;
    m_clear = CNONE;
}

InlineIterator LineBreaker::nextLineBreak(InlineBidiResolver& resolver, LineInfo& lineInfo, RenderTextInfo& renderTextInfo, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements& wordMeasurements)
{
    ShapeInsideInfo* shapeInsideInfo = m_block->layoutShapeInsideInfo();

    if (!shapeInsideInfo || !shapeInsideInfo->lineOverlapsShapeBounds())
        return nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);

    InlineIterator end = resolver.position();
    InlineIterator oldEnd = end;

    if (!shapeInsideInfo->hasSegments()) {
        end = nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);
        resolver.setPositionIgnoringNestedIsolates(oldEnd);
        return oldEnd;
    }

    const SegmentList& segments = shapeInsideInfo->segments();
    SegmentRangeList& segmentRanges = shapeInsideInfo->segmentRanges();

    for (unsigned i = 0; i < segments.size() && !end.atEnd(); i++) {
        const InlineIterator segmentStart = resolver.position();
        end = nextSegmentBreak(resolver, lineInfo, renderTextInfo, lastFloatFromPreviousLine, consecutiveHyphenatedLines, wordMeasurements);

        ASSERT(segmentRanges.size() == i);
        if (resolver.position().atEnd()) {
            segmentRanges.append(LineSegmentRange(segmentStart, end));
            break;
        }
        if (resolver.position() == end) {
            // Nothing fit this segment
            end = segmentStart;
            segmentRanges.append(LineSegmentRange(segmentStart, segmentStart));
            resolver.setPositionIgnoringNestedIsolates(segmentStart);
        } else {
            // Note that resolver.position is already skipping some of the white space at the beginning of the line,
            // so that's why segmentStart might be different than resolver.position().
            LineSegmentRange range(resolver.position(), end);
            segmentRanges.append(range);
            resolver.setPosition(end, numberOfIsolateAncestors(end));

            if (lineInfo.previousLineBrokeCleanly()) {
                // If we hit a new line break, just stop adding anything to this line.
                break;
            }
        }
    }
    resolver.setPositionIgnoringNestedIsolates(oldEnd);
    return end;
}

InlineIterator LineBreaker::nextSegmentBreak(InlineBidiResolver& resolver, LineInfo& lineInfo, RenderTextInfo& renderTextInfo, FloatingObject* lastFloatFromPreviousLine, unsigned consecutiveHyphenatedLines, WordMeasurements& wordMeasurements)
{
    reset();

    ASSERT(resolver.position().root() == m_block);

    bool appliedStartWidth = resolver.position().m_pos > 0;

    LineWidth width(*m_block, lineInfo.isFirstLine(), requiresIndent(lineInfo.isFirstLine(), lineInfo.previousLineBrokeCleanly(), m_block->style()));

    skipLeadingWhitespace(resolver, lineInfo, lastFloatFromPreviousLine, width);

    if (resolver.position().atEnd())
        return resolver.position();

    BreakingContext context(resolver, lineInfo, width, renderTextInfo, lastFloatFromPreviousLine, appliedStartWidth, m_block);

    while (context.currentObject()) {
        context.initializeForCurrentObject();
        if (context.currentObject()->isBR()) {
            context.handleBR(m_clear);
        } else if (context.currentObject()->isOutOfFlowPositioned()) {
            context.handleOutOfFlowPositioned(m_positionedObjects);
        } else if (context.currentObject()->isFloating()) {
            context.handleFloat();
        } else if (context.currentObject()->isRenderInline()) {
            context.handleEmptyInline();
        } else if (context.currentObject()->isReplaced()) {
            context.handleReplaced();
        } else if (context.currentObject()->isText()) {
            if (context.handleText(wordMeasurements, m_hyphenated)) {
                // We've hit a hard text line break. Our line break iterator is updated, so go ahead and early return.
                return context.lineBreak();
            }
        } else {
            ASSERT_NOT_REACHED();
        }

        if (context.atEnd())
            return context.handleEndOfLine();

        context.commitAndUpdateLineBreakIfNeeded();

        if (context.atEnd())
            return context.handleEndOfLine();

        context.increment();
    }

    context.clearLineBreakIfFitsOnLine();

    return context.handleEndOfLine();
}

void RenderBlockFlow::addOverflowFromInlineChildren()
{
    LayoutUnit endPadding = hasOverflowClip() ? paddingEnd() : LayoutUnit();
    // FIXME: Need to find another way to do this, since scrollbars could show when we don't want them to.
    if (hasOverflowClip() && !endPadding && node() && node()->isRootEditableElement() && style()->isLeftToRightDirection())
        endPadding = 1;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        addLayoutOverflow(curr->paddedLayoutOverflowRect(endPadding));
        LayoutRect visualOverflow = curr->visualOverflowRect(curr->lineTop(), curr->lineBottom());
        addContentsVisualOverflow(visualOverflow);
    }
}

void RenderBlockFlow::deleteEllipsisLineBoxes()
{
    ETextAlign textAlign = style()->textAlign();
    bool ltr = style()->isLeftToRightDirection();
    bool firstLine = true;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        if (curr->hasEllipsisBox()) {
            curr->clearTruncation();

            // Shift the line back where it belongs if we cannot accomodate an ellipsis.
            float logicalLeft = pixelSnappedLogicalLeftOffsetForLine(curr->lineTop(), firstLine);
            float availableLogicalWidth = logicalRightOffsetForLine(curr->lineTop(), false) - logicalLeft;
            float totalLogicalWidth = curr->logicalWidth();
            updateLogicalWidthForAlignment(textAlign, curr, 0, logicalLeft, totalLogicalWidth, availableLogicalWidth, 0);

            if (ltr)
                curr->adjustLogicalPosition((logicalLeft - curr->logicalLeft()), 0);
            else
                curr->adjustLogicalPosition(-(curr->logicalLeft() - logicalLeft), 0);
        }
        firstLine = false;
    }
}

void RenderBlockFlow::checkLinesForTextOverflow()
{
    // Determine the width of the ellipsis using the current font.
    // FIXME: CSS3 says this is configurable, also need to use 0x002E (FULL STOP) if horizontal ellipsis is "not renderable"
    const Font& font = style()->font();
    DEFINE_STATIC_LOCAL(AtomicString, ellipsisStr, (&horizontalEllipsis, 1));
    const Font& firstLineFont = firstLineStyle()->font();
    int firstLineEllipsisWidth = firstLineFont.width(constructTextRun(this, firstLineFont, &horizontalEllipsis, 1, firstLineStyle()));
    int ellipsisWidth = (font == firstLineFont) ? firstLineEllipsisWidth : font.width(constructTextRun(this, font, &horizontalEllipsis, 1, style()));

    // For LTR text truncation, we want to get the right edge of our padding box, and then we want to see
    // if the right edge of a line box exceeds that.  For RTL, we use the left edge of the padding box and
    // check the left edge of the line box to see if it is less
    // Include the scrollbar for overflow blocks, which means we want to use "contentWidth()"
    bool ltr = style()->isLeftToRightDirection();
    ETextAlign textAlign = style()->textAlign();
    bool firstLine = true;
    for (RootInlineBox* curr = firstRootBox(); curr; curr = curr->nextRootBox()) {
        // FIXME: Use pixelSnappedLogicalRightOffsetForLine instead of snapping it ourselves once the column workaround in said method has been fixed.
        // https://bugs.webkit.org/show_bug.cgi?id=105461
        float currLogicalLeft = curr->logicalLeft();
        int blockRightEdge = snapSizeToPixel(logicalRightOffsetForLine(curr->lineTop(), firstLine), currLogicalLeft);
        int blockLeftEdge = pixelSnappedLogicalLeftOffsetForLine(curr->lineTop(), firstLine);
        int lineBoxEdge = ltr ? snapSizeToPixel(currLogicalLeft + curr->logicalWidth(), currLogicalLeft) : snapSizeToPixel(currLogicalLeft, 0);
        if ((ltr && lineBoxEdge > blockRightEdge) || (!ltr && lineBoxEdge < blockLeftEdge)) {
            // This line spills out of our box in the appropriate direction.  Now we need to see if the line
            // can be truncated.  In order for truncation to be possible, the line must have sufficient space to
            // accommodate our truncation string, and no replaced elements (images, tables) can overlap the ellipsis
            // space.

            LayoutUnit width = firstLine ? firstLineEllipsisWidth : ellipsisWidth;
            LayoutUnit blockEdge = ltr ? blockRightEdge : blockLeftEdge;
            if (curr->lineCanAccommodateEllipsis(ltr, blockEdge, lineBoxEdge, width)) {
                float totalLogicalWidth = curr->placeEllipsis(ellipsisStr, ltr, blockLeftEdge, blockRightEdge, width);

                float logicalLeft = 0; // We are only intersted in the delta from the base position.
                float truncatedWidth = pixelSnappedLogicalRightOffsetForLine(curr->lineTop(), firstLine);
                updateLogicalWidthForAlignment(textAlign, curr, 0, logicalLeft, totalLogicalWidth, truncatedWidth, 0);
                if (ltr)
                    curr->adjustLogicalPosition(logicalLeft, 0);
                else
                    curr->adjustLogicalPosition(-(truncatedWidth - (logicalLeft + totalLogicalWidth)), 0);
            }
        }
        firstLine = false;
    }
}

bool RenderBlockFlow::positionNewFloatOnLine(FloatingObject* newFloat, FloatingObject* lastFloatFromPreviousLine, LineInfo& lineInfo, LineWidth& width)
{
    if (!positionNewFloats())
        return false;

    width.shrinkAvailableWidthForNewFloatIfNeeded(newFloat);

    // We only connect floats to lines for pagination purposes if the floats occur at the start of
    // the line and the previous line had a hard break (so this line is either the first in the block
    // or follows a <br>).
    if (!newFloat->paginationStrut() || !lineInfo.previousLineBrokeCleanly() || !lineInfo.isEmpty())
        return true;

    const FloatingObjectSet& floatingObjectSet = m_floatingObjects->set();
    ASSERT(floatingObjectSet.last() == newFloat);

    LayoutUnit floatLogicalTop = logicalTopForFloat(newFloat);
    int paginationStrut = newFloat->paginationStrut();

    if (floatLogicalTop - paginationStrut != logicalHeight() + lineInfo.floatPaginationStrut())
        return true;

    FloatingObjectSetIterator it = floatingObjectSet.end();
    --it; // Last float is newFloat, skip that one.
    FloatingObjectSetIterator begin = floatingObjectSet.begin();
    while (it != begin) {
        --it;
        FloatingObject* floatingObject = *it;
        if (floatingObject == lastFloatFromPreviousLine)
            break;
        if (logicalTopForFloat(floatingObject) == logicalHeight() + lineInfo.floatPaginationStrut()) {
            floatingObject->setPaginationStrut(paginationStrut + floatingObject->paginationStrut());
            RenderBox* floatBox = floatingObject->renderer();
            setLogicalTopForChild(floatBox, logicalTopForChild(floatBox) + marginBeforeForChild(floatBox) + paginationStrut);
            if (floatBox->isRenderBlock())
                floatBox->forceChildLayout();
            else
                floatBox->layoutIfNeeded();
            // Save the old logical top before calling removePlacedObject which will set
            // isPlaced to false. Otherwise it will trigger an assert in logicalTopForFloat.
            LayoutUnit oldLogicalTop = logicalTopForFloat(floatingObject);
            m_floatingObjects->removePlacedObject(floatingObject);
            setLogicalTopForFloat(floatingObject, oldLogicalTop + paginationStrut);
            m_floatingObjects->addPlacedObject(floatingObject);
        }
    }

    // Just update the line info's pagination strut without altering our logical height yet. If the line ends up containing
    // no content, then we don't want to improperly grow the height of the block.
    lineInfo.setFloatPaginationStrut(lineInfo.floatPaginationStrut() + paginationStrut);
    return true;
}

LayoutUnit RenderBlockFlow::startAlignedOffsetForLine(LayoutUnit position, bool firstLine)
{
    ETextAlign textAlign = style()->textAlign();

    if (textAlign == TASTART) // FIXME: Handle TAEND here
        return startOffsetForLine(position, firstLine);

    // updateLogicalWidthForAlignment() handles the direction of the block so no need to consider it here
    float totalLogicalWidth = 0;
    float logicalLeft = logicalLeftOffsetForLine(logicalHeight(), false);
    float availableLogicalWidth = logicalRightOffsetForLine(logicalHeight(), false) - logicalLeft;
    updateLogicalWidthForAlignment(textAlign, 0, 0, logicalLeft, totalLogicalWidth, availableLogicalWidth, 0);

    if (!style()->isLeftToRightDirection())
        return logicalWidth() - logicalLeft;
    return logicalLeft;
}

void RenderBlockFlow::layoutLineGridBox()
{
    if (style()->lineGrid() == RenderStyle::initialLineGrid()) {
        setLineGridBox(0);
        return;
    }

    setLineGridBox(0);

    RootInlineBox* lineGridBox = new RootInlineBox(this);
    lineGridBox->setHasTextChildren(); // Needed to make the line ascent/descent actually be honored in quirks mode.
    lineGridBox->setConstructed();
    GlyphOverflowAndFallbackFontsMap textBoxDataMap;
    VerticalPositionCache verticalPositionCache;
    lineGridBox->alignBoxesInBlockDirection(logicalHeight(), textBoxDataMap, verticalPositionCache);

    setLineGridBox(lineGridBox);

    // FIXME: If any of the characteristics of the box change compared to the old one, then we need to do a deep dirtying
    // (similar to what happens when the page height changes). Ideally, though, we only do this if someone is actually snapping
    // to this grid.
}

}
