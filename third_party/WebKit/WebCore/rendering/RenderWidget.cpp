/**
 * This file is part of the HTML widget for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2006 Apple Computer, Inc.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "RenderWidget.h"

#include "AXObjectCache.h"
#include "Document.h"
#include "Element.h"
#include "Event.h"
#include "EventNames.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "RenderLayer.h"
#include "RenderView.h"

using namespace std;

namespace WebCore {

using namespace EventNames;

RenderWidget::RenderWidget(Node* node)
      : RenderReplaced(node)
      , m_widget(0)
      , m_refCount(0)
{
    // a replaced element doesn't support being anonymous
    ASSERT(node);
    m_view = node->document()->view();

    view()->addWidget(this);

    // this is no real reference counting, its just there
    // to make sure that we're not deleted while we're recursed
    // in an eventFilter of the widget
    ref();
}

void RenderWidget::destroy()
{
    // We can't call the base class's destroy because we don't
    // want to unconditionally delete ourselves (we're ref-counted).
    // So the code below includes copied and pasted contents of
    // both RenderBox::destroy() and RenderObject::destroy().
    // Fix originally made for <rdar://problem/4228818>.

    if (RenderView* v = view())
        v->removeWidget(this);

    document()->axObjectCache()->remove(this);

    remove();

    if (m_widget) {
        if (m_view)
            m_view->removeChild(m_widget);
        m_widget->setClient(0);
    }

    RenderLayer* layer = m_layer;
    RenderArena* arena = renderArena();

    if (layer)
        layer->clearClipRect();

    setNode(0);
    deref(arena);

    if (layer)
        layer->destroy(arena);
}

RenderWidget::~RenderWidget()
{
    ASSERT(m_refCount <= 0);
    deleteWidget();
}

void RenderWidget::resizeWidget(Widget* widget, int w, int h)
{
    if (element() && (widget->width() != w || widget->height() != h)) {
        RenderArena* arena = ref();
        element()->ref();
        widget->resize(w, h);
        element()->deref();
        deref(arena);
    }
}

void RenderWidget::setWidget(Widget* widget)
{
    if (widget != m_widget) {
        if (m_widget) {
            m_widget->setClient(0);
            deleteWidget();
        }
        m_widget = widget;
        if (m_widget) {
            m_widget->setClient(this);
            // if we've already received a layout, apply the calculated space to the
            // widget immediately, but we have to have really been full constructed (with a non-null
            // style pointer).
            if (!needsLayout() && style())
                resizeWidget(m_widget,
                    m_width - borderLeft() - borderRight() - paddingLeft() - paddingRight(),
                    m_height - borderTop() - borderBottom() - paddingTop() - paddingBottom());
            if (style()) {
                if (style()->visibility() != VISIBLE)
                    m_widget->hide();
                else
                    m_widget->show();
            }
            m_view->addChild(m_widget);
        }
    }
}

void RenderWidget::layout()
{
    ASSERT(needsLayout());
    ASSERT(minMaxKnown());

    setNeedsLayout(false);
}

void RenderWidget::sendConsumedMouseUp(Widget*)
{
    RenderArena* arena = ref();
    EventTargetNodeCast(node())->dispatchSimulatedMouseEvent(mouseupEvent);
    deref(arena);
}

void RenderWidget::setStyle(RenderStyle* newStyle)
{
    RenderReplaced::setStyle(newStyle);
    if (m_widget) {
        m_widget->setFont(style()->font());
        if (style()->visibility() != VISIBLE)
            m_widget->hide();
        else
            m_widget->show();
    }
}

void RenderWidget::paint(PaintInfo& paintInfo, int tx, int ty)
{
    if (!shouldPaint(paintInfo, tx, ty))
        return;

    tx += m_x;
    ty += m_y;

    if (hasBoxDecorations() && paintInfo.phase != PaintPhaseOutline && paintInfo.phase != PaintPhaseSelfOutline)
        paintBoxDecorations(paintInfo, tx, ty);

    if (!m_view || paintInfo.phase != PaintPhaseForeground || style()->visibility() != VISIBLE)
        return;

#if PLATFORM(MAC)
    if (style()->highlight() != nullAtom && !paintInfo.context->paintingDisabled())
        paintCustomHighlight(tx - m_x, ty - m_y, style()->highlight(), true);
#endif

    if (m_widget) {
        // Move the widget if necessary.  We normally move and resize widgets during layout, but sometimes
        // widgets can move without layout occurring (most notably when you scroll a document that
        // contains fixed positioned elements).
        m_widget->move(tx + borderLeft() + paddingLeft(), ty + borderTop() + paddingTop());

        // Tell the widget to paint now.  This is the only time the widget is allowed
        // to paint itself.  That way it will composite properly with z-indexed layers.
        m_widget->paint(paintInfo.context, paintInfo.rect);
    }

    // Paint a partially transparent wash over selected widgets.
    if (isSelected() && !document()->printing())
        paintInfo.context->fillRect(selectionRect(), selectionBackgroundColor());
}

void RenderWidget::focusIn(Widget*)
{
    RenderArena* arena = ref();
    RefPtr<Node> elem = element();
    if (elem)
        elem->document()->setFocusedNode(elem);
    deref(arena);
}

void RenderWidget::focusOut(Widget*)
{
    RenderArena* arena = ref();
    RefPtr<Node> elem = element();
    if (elem && elem == elem->document()->focusedNode())
        elem->document()->setFocusedNode(0);
    deref(arena);
}

void RenderWidget::scrollToVisible(Widget* widget)
{
    if (RenderLayer* layer = enclosingLayer())
        layer->scrollRectToVisible(absoluteBoundingBoxRect());
}

bool RenderWidget::isVisible(Widget* widget)
{
    return style()->visibility() == VISIBLE;
}

Element* RenderWidget::element(Widget* widget)
{
    Node* n = node();
    return n->isElementNode() ? static_cast<Element*>(n) : 0;
}

void RenderWidget::deref(RenderArena *arena)
{
    if (--m_refCount <= 0)
        arenaDelete(arena, this);
}

void RenderWidget::updateWidgetPosition()
{
    if (!m_widget)
        return;

    int x;
    int y;
    absolutePosition(x, y);
    x += borderLeft() + paddingLeft();
    y += borderTop() + paddingTop();

    int width = m_width - borderLeft() - borderRight() - paddingLeft() - paddingRight();
    int height = m_height - borderTop() - borderBottom() - paddingTop() - paddingBottom();

    IntRect newBounds(x, y, width, height);
    IntRect oldBounds(m_widget->frameGeometry());
    if (newBounds != oldBounds) {
        // The widget changed positions.  Update the frame geometry.
        if (checkForRepaintDuringLayout()) {
            RenderView* v = view();
            if (!v->printingMode()) {
                v->repaintViewRectangle(oldBounds);
                v->repaintViewRectangle(newBounds);
            }
        }

        RenderArena* arena = ref();
        element()->ref();
        m_widget->setFrameGeometry(newBounds);
        element()->deref();
        deref(arena);
    }
}

void RenderWidget::setSelectionState(SelectionState state)
{
    if (selectionState() != state) {
        RenderReplaced::setSelectionState(state);
        if (m_widget)
            m_widget->setIsSelected(isSelected());
    }
}

void RenderWidget::deleteWidget()
{
    delete m_widget;
}

} // namespace WebCore
