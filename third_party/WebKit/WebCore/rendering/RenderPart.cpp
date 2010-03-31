/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
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
#include "RenderPart.h"

#include "RenderView.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameElement.h"

namespace WebCore {

RenderPart::RenderPart(Element* node)
    : RenderWidget(node)
    , m_hasFallbackContent(false)
{
    // init RenderObject attributes
    setInline(false);
}

RenderPart::~RenderPart()
{
    clearWidget();
}

void RenderPart::setWidget(PassRefPtr<Widget> widget)
{
    if (widget == this->widget())
        return;

    RenderWidget::setWidget(widget);

    // make sure the scrollbars are set correctly for restore
    // ### find better fix
    viewCleared();
}

void RenderPart::layoutWithFlattening(bool fixedWidth, bool fixedHeight)
{
    FrameView* childFrameView = static_cast<FrameView*>(widget());
    RenderView* childRoot = childFrameView ? static_cast<RenderView*>(childFrameView->frame()->contentRenderer()) : 0;
    HTMLFrameElement* element = static_cast<HTMLFrameElement*>(node());

    // Do not expand frames which has zero width or height
    if (!width() || !height() || !childRoot) {
        updateWidgetPosition();
        if (childFrameView)
            childFrameView->layout();
        setNeedsLayout(false);
        return;
    }

    // need to update to calculate min/max correctly
    updateWidgetPosition();
    if (childRoot->prefWidthsDirty())
        childRoot->calcPrefWidths();

    // if scrollbars are off, and the width or height are fixed
    // we obey them and do not expand. With frame flattening
    // no subframe much ever become scrollable.

    bool isScrollable = element->scrollingMode() != ScrollbarAlwaysOff;

    // make sure minimum preferred width is enforced
    if (isScrollable || !fixedWidth)
        setWidth(max(width(), childRoot->minPrefWidth()));

    // update again to pass the width to the child frame
    updateWidgetPosition();
    childFrameView->layout();

    // expand the frame by setting frame height = content height
    if (isScrollable || !fixedHeight || childRoot->isFrameSet())
        setHeight(max(height(), childFrameView->contentsHeight()));
    if (isScrollable || !fixedWidth || childRoot->isFrameSet())
        setWidth(max(width(), childFrameView->contentsWidth()));

    // make room for the inset border
    setWidth(width() + borderLeft() + borderRight());
    setHeight(height() + borderTop() + borderBottom());

    updateWidgetPosition();

    ASSERT(!childFrameView->layoutPending());
    ASSERT(!childRoot->needsLayout());
    ASSERT(!childRoot->firstChild() || !childRoot->firstChild()->firstChild() || !childRoot->firstChild()->firstChild()->needsLayout());

    setNeedsLayout(false);
}


void RenderPart::viewCleared()
{
}

}
