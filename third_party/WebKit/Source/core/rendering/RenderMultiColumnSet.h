/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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


#ifndef RenderMultiColumnSet_h
#define RenderMultiColumnSet_h

#include "core/rendering/RenderMultiColumnFlowThread.h"
#include "core/rendering/RenderRegion.h"
#include "wtf/Vector.h"

namespace WebCore {

// RenderMultiColumnSet represents a set of columns that all have the same width and height. By
// combining runs of same-size columns into a single object, we significantly reduce the number of
// unique RenderObjects required to represent columns.
//
// Column sets are inserted as anonymous children of the actual multicol container (i.e. the
// renderer whose style computes to non-auto column-count and/or column-width).
//
// Being a "region", a column set has no children on its own, but is merely used to slice a portion
// of the tall "single-column" flow thread into actual columns visually, to convert from flow thread
// coordinates to visual ones. It is in charge of both positioning columns correctly relatively to
// the parent multicol container, and to calculate the correct translation for each column's
// contents, and to paint any rules between them. RenderMultiColumnSet objects are used for
// painting, hit testing, and any other type of operation that requires mapping from flow thread
// coordinates to visual coordinates.
//
// Column spans result in the creation of new column sets, since a spanning renderer has to be
// placed in between the column sets that come before and after the span.
class RenderMultiColumnSet FINAL : public RenderRegion {
public:
    static RenderMultiColumnSet* createAnonymous(RenderFlowThread*, RenderStyle* parentStyle);

    virtual bool isRenderMultiColumnSet() const OVERRIDE { return true; }

    RenderBlockFlow* multiColumnBlockFlow() const { return toRenderBlockFlow(parent()); }
    RenderMultiColumnFlowThread* multiColumnFlowThread() const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(!flowThread() || flowThread()->isRenderMultiColumnFlowThread());
        return static_cast<RenderMultiColumnFlowThread*>(flowThread());
    }

    RenderMultiColumnSet* nextSiblingMultiColumnSet() const;

    LayoutUnit logicalBottomInFlowThread() const { return isHorizontalWritingMode() ? flowThreadPortionRect().maxY() : flowThreadPortionRect().maxX(); }

    unsigned computedColumnCount() const { return m_computedColumnCount; }
    LayoutUnit computedColumnWidth() const { return m_computedColumnWidth; }
    LayoutUnit computedColumnHeight() const { return m_computedColumnHeight; }

    void setComputedColumnWidthAndCount(LayoutUnit width, unsigned count)
    {
        m_computedColumnWidth = width;
        m_computedColumnCount = count;
    }

    // Find the column that contains the given block offset, and return the translation needed to
    // get from flow thread coordinates to visual coordinates.
    LayoutSize flowThreadTranslationAtOffset(LayoutUnit) const;

    LayoutUnit heightAdjustedForSetOffset(LayoutUnit height) const;

    void updateMinimumColumnHeight(LayoutUnit height) { m_minimumColumnHeight = std::max(height, m_minimumColumnHeight); }
    LayoutUnit minimumColumnHeight() const { return m_minimumColumnHeight; }

    unsigned forcedBreaksCount() const { return m_contentRuns.size(); }
    void clearForcedBreaks();
    void addForcedBreak(LayoutUnit offsetFromFirstPage);

    // (Re-)calculate the column height if it's auto. If 'initial' is set, guess an initial column
    // height; otherwise, stretch the column height a tad. Return true if column height changed and
    // another layout pass is required.
    bool recalculateColumnHeight(bool initial);

    // Record space shortage (the amount of space that would have been enough to prevent some
    // element from being moved to the next column) at a column break. The smallest amount of space
    // shortage we find is the amount with which we will stretch the column height, if it turns out
    // after layout that the columns weren't tall enough.
    void recordSpaceShortage(LayoutUnit spaceShortage);

    virtual void updateLogicalWidth() OVERRIDE;

    void prepareForLayout();

    // Expand this set's flow thread portion rectangle to contain all trailing flow thread
    // overflow. Only to be called on the last set.
    void expandToEncompassFlowThreadContentsIfNeeded();

private:
    RenderMultiColumnSet(RenderFlowThread*);

    virtual void computeLogicalHeight(LayoutUnit logicalHeight, LayoutUnit logicalTop, LogicalExtentComputedValues&) const OVERRIDE;

    virtual void paintObject(PaintInfo&, const LayoutPoint& paintOffset) OVERRIDE;

    virtual LayoutUnit pageLogicalWidth() const OVERRIDE { return m_computedColumnWidth; }
    virtual LayoutUnit pageLogicalHeight() const OVERRIDE { return m_computedColumnHeight; }

    virtual LayoutUnit pageLogicalTopForOffset(LayoutUnit offset) const OVERRIDE;

    // FIXME: This will change once we have column sets constrained by enclosing pages, etc.
    virtual LayoutUnit logicalHeightOfAllFlowThreadContent() const OVERRIDE { return m_computedColumnHeight; }

    virtual void repaintFlowThreadContent(const LayoutRect& repaintRect) const OVERRIDE;

    virtual void collectLayerFragments(LayerFragments&, const LayoutRect& layerBoundingBox, const LayoutRect& dirtyRect) OVERRIDE;

    virtual const char* renderName() const OVERRIDE;

    void paintColumnRules(PaintInfo&, const LayoutPoint& paintOffset);

    LayoutUnit columnGap() const;
    LayoutRect columnRectAt(unsigned index) const;
    unsigned columnCount() const;

    LayoutRect flowThreadPortionRectAt(unsigned index) const;
    LayoutRect flowThreadPortionOverflowRect(const LayoutRect& flowThreadPortion, unsigned index, unsigned colCount, LayoutUnit colGap) const;

    enum ColumnIndexCalculationMode {
        ClampToExistingColumns, // Stay within the range of already existing columns.
        AssumeNewColumns // Allow column indices outside the range of already existing columns.
    };
    unsigned columnIndexAtOffset(LayoutUnit, ColumnIndexCalculationMode = ClampToExistingColumns) const;

    void setAndConstrainColumnHeight(LayoutUnit);

    // Return the index of the content run with the currently tallest columns, taking all implicit
    // breaks assumed so far into account.
    unsigned findRunWithTallestColumns() const;

    // Given the current list of content runs, make assumptions about where we need to insert
    // implicit breaks (if there's room for any at all; depending on the number of explicit breaks),
    // and store the results. This is needed in order to balance the columns.
    void distributeImplicitBreaks();

    LayoutUnit calculateColumnHeight(bool initial) const;

    unsigned m_computedColumnCount; // Used column count (the resulting 'N' from the pseudo-algorithm in the multicol spec)
    LayoutUnit m_computedColumnWidth; // Used column width (the resulting 'W' from the pseudo-algorithm in the multicol spec)
    LayoutUnit m_computedColumnHeight;

    // The following variables are used when balancing the column set.
    LayoutUnit m_maxColumnHeight; // Maximum column height allowed.
    LayoutUnit m_minSpaceShortage; // The smallest amout of space shortage that caused a column break.
    LayoutUnit m_minimumColumnHeight;

    // A run of content without explicit (forced) breaks; i.e. a flow thread portion between two
    // explicit breaks, between flow thread start and an explicit break, between an explicit break
    // and flow thread end, or, in cases when there are no explicit breaks at all: between flow flow
    // thread start and flow thread end. We need to know where the explicit breaks are, in order to
    // figure out where the implicit breaks will end up, so that we get the columns properly
    // balanced. A content run starts out as representing one single column, and will represent one
    // additional column for each implicit break "inserted" there.
    class ContentRun {
    public:
        ContentRun(LayoutUnit breakOffset)
            : m_breakOffset(breakOffset)
            , m_assumedImplicitBreaks(0) { }

        unsigned assumedImplicitBreaks() const { return m_assumedImplicitBreaks; }
        void assumeAnotherImplicitBreak() { m_assumedImplicitBreaks++; }
        LayoutUnit breakOffset() const { return m_breakOffset; }

        // Return the column height that this content run would require, considering the implicit
        // breaks assumed so far.
        LayoutUnit columnLogicalHeight(LayoutUnit startOffset) const { return ceilf((m_breakOffset - startOffset).toFloat() / float(m_assumedImplicitBreaks + 1)); }

    private:
        LayoutUnit m_breakOffset; // Flow thread offset where this run ends.
        unsigned m_assumedImplicitBreaks; // Number of implicit breaks in this run assumed so far.
    };
    Vector<ContentRun, 1> m_contentRuns;
};

DEFINE_RENDER_OBJECT_TYPE_CASTS(RenderMultiColumnSet, isRenderMultiColumnSet());

} // namespace WebCore

#endif // RenderMultiColumnSet_h

