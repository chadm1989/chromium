/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * All rights reserved.
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
#include "ScrollView.h"
#include "FrameView.h"
#include "FloatRect.h"
#include "IntPoint.h"
#include "PlatformMouseEvent.h"
#include "PlatformWheelEvent.h"
#include "NotImplemented.h"
#include "Frame.h"
#include "Page.h"
#include "GraphicsContext.h"
#include "PlatformScrollBar.h"

#include <QDebug>
#include <QWidget>
#include <QPainter>

#include "qwebframe.h"
#include "qwebpage.h"

// #define DEBUG_SCROLLVIEW

namespace WebCore {

class ScrollView::ScrollViewPrivate : public ScrollbarClient
{
public:
    ScrollViewPrivate(ScrollView* view)
      : m_view(view)
      , m_hasStaticBackground(false)
      , m_scrollbarsSuppressed(false)
      , m_inUpdateScrollbars(false)
      , m_scrollbarsAvoidingResizer(0)
      , m_vScrollbarMode(ScrollbarAuto)
      , m_hScrollbarMode(ScrollbarAuto)
    {
        setHasHorizontalScrollbar(true);
        setHasVerticalScrollbar(true);
    }

    ~ScrollViewPrivate()
    {
        setHasHorizontalScrollbar(false);
        setHasVerticalScrollbar(false);
    }

    void setHasHorizontalScrollbar(bool hasBar);
    void setHasVerticalScrollbar(bool hasBar);

    virtual void valueChanged(Scrollbar*);
    virtual IntRect windowClipRect() const;

    void scrollBackingStore(const IntSize& scrollDelta);

    ScrollView* m_view;
    IntSize m_scrollOffset;
    IntSize m_contentsSize;
    bool m_hasStaticBackground;
    bool m_scrollbarsSuppressed;
    bool m_inUpdateScrollbars;
    int m_scrollbarsAvoidingResizer;
    ScrollbarMode m_vScrollbarMode;
    ScrollbarMode m_hScrollbarMode;
    RefPtr<PlatformScrollbar> m_vBar;
    RefPtr<PlatformScrollbar> m_hBar;
    QRegion m_dirtyRegion;
    HashSet<Widget*> m_children;
};

void ScrollView::ScrollViewPrivate::setHasHorizontalScrollbar(bool hasBar)
{
    if (Scrollbar::hasPlatformScrollbars()) {
        if (hasBar && !m_hBar) {
            m_hBar = new PlatformScrollbar(this, HorizontalScrollbar, RegularScrollbar);
            m_view->addChild(m_hBar.get());
        } else if (!hasBar && m_hBar) {
            m_view->removeChild(m_hBar.get());;
            m_hBar = 0;
        }
    }
}

void ScrollView::ScrollViewPrivate::setHasVerticalScrollbar(bool hasBar)
{
    if (Scrollbar::hasPlatformScrollbars()) {
        if (hasBar && !m_vBar) {
            m_vBar = new PlatformScrollbar(this, VerticalScrollbar, RegularScrollbar);
            m_view->addChild(m_vBar.get());
        } else if (!hasBar && m_vBar) {
            m_view->removeChild(m_vBar.get());
            m_vBar = 0;
        }
    }
}

void ScrollView::ScrollViewPrivate::valueChanged(Scrollbar* bar)
{
    // Figure out if we really moved.
    IntSize newOffset = m_scrollOffset;
    if (bar) {
        if (bar == m_hBar)
            newOffset.setWidth(bar->value());
        else if (bar == m_vBar)
            newOffset.setHeight(bar->value());
    }
    IntSize scrollDelta = newOffset - m_scrollOffset;
    if (scrollDelta == IntSize())
        return;
    m_scrollOffset = newOffset;

    if (m_scrollbarsSuppressed)
        return;

    scrollBackingStore(scrollDelta);
    static_cast<FrameView*>(m_view)->frame()->sendScrollEvent();
}

void ScrollView::ScrollViewPrivate::scrollBackingStore(const IntSize& scrollDelta)
{
    // Since scrolling is double buffered, we will be blitting the scroll view's intersection
    // with the clip rect every time to keep it smooth.
    IntRect clipRect = m_view->windowClipRect();
    IntRect scrollViewRect = m_view->convertToContainingWindow(IntRect(0, 0, m_view->visibleWidth(), m_view->visibleHeight()));

    IntRect updateRect = clipRect;
    updateRect.intersect(scrollViewRect);

    if (!m_hasStaticBackground) {
       m_view->scrollBackingStore(-scrollDelta.width(), -scrollDelta.height(),
                                  scrollViewRect, clipRect);
    } else  {
       // We need to go ahead and repaint the entire backing store.
       m_view->update();
    }

    m_view->geometryChanged();
}

IntRect ScrollView::ScrollViewPrivate::windowClipRect() const
{
    return static_cast<const FrameView*>(m_view)->windowClipRect(false);
}

ScrollView::ScrollView()
    : m_data(new ScrollViewPrivate(this))
{
}

ScrollView::~ScrollView()
{
    delete m_data;
}

PlatformScrollbar *ScrollView::horizontalScrollBar() const
{
    return m_data->m_hBar.get();
}

PlatformScrollbar *ScrollView::verticalScrollBar() const
{
    return m_data->m_vBar.get();
}

void ScrollView::updateContents(const IntRect& rect, bool now)
{
    if (rect.isEmpty())
        return;

    IntPoint windowPoint = contentsToWindow(rect.location());
    IntRect containingWindowRect = rect;
    containingWindowRect.setLocation(windowPoint);

    //In QWebPage::paintEvent we paint the ev->region().rects()
    //individually.  Unfortunately, webkit expects we'll draw the
    //boundingrect of all update rects instead.  This is unfortunate,
    //because if we want to draw the scrollbar rects along with the update
    //rects this results in redrawing the entire page for a 1px scroll.
    //In light of this we cache the update rects that webkit sends us here
    //and send the bound along to QWebPage.  The cache is cleared everytime
    //an actual paint occurs in ScrollView::paint...

    QRect r(containingWindowRect);
    QWebPage* page = qwebframe()->page();
    r = r.intersect(QRect(QPoint(0, 0), page->viewportSize()));
    if (r.isEmpty())
        return;
    // Cache the dirty spot.
    if (!m_data->m_dirtyRegion.isEmpty())
        m_data->m_dirtyRegion = m_data->m_dirtyRegion.united(QRegion(r));
    else
        m_data->m_dirtyRegion = QRegion(r);

#if 0
    // ### QWebPage
    bool painting = containingWindow()->testAttribute(Qt::WA_WState_InPaintEvent);
    if (painting && now) {
        QWebPage *page = qobject_cast<QWebPage*>(containingWindow());
        QPainter p(page);
        page->mainFrame()->render(&p, m_data->m_dirtyRegion.boundingRect());
    } else if (now) {
        containingWindow()->repaint(m_data->m_dirtyRegion.boundingRect());
    } else {
        containingWindow()->update(m_data->m_dirtyRegion.boundingRect());
    }
#endif
}

void ScrollView::update()
{
    QWidget* window = containingWindow();
    if (window)
        window->update(frameGeometry());
}

int ScrollView::visibleWidth() const
{
    return width() - (m_data->m_vBar ? m_data->m_vBar->width() : 0);
}

int ScrollView::visibleHeight() const
{
    return height() - (m_data->m_hBar ? m_data->m_hBar->height() : 0);
}

FloatRect ScrollView::visibleContentRect() const
{
    return FloatRect(contentsX(), contentsY(), visibleWidth(), visibleHeight());
}

FloatRect ScrollView::visibleContentRectConsideringExternalScrollers() const
{
    // external scrollers not supported for now
    return visibleContentRect();
}

void ScrollView::setContentsPos(int newX, int newY)
{
    int dx = newX - contentsX();
    int dy = newY - contentsY();
    scrollBy(dx, dy);
}

void ScrollView::resizeContents(int w, int h)
{
    IntSize newContentsSize(w, h);
    if (m_data->m_contentsSize != newContentsSize) {
        m_data->m_contentsSize = newContentsSize;
        updateScrollbars(m_data->m_scrollOffset);
    }
}

void ScrollView::setFrameGeometry(const IntRect& newGeometry)
{
    IntRect oldGeometry = frameGeometry();
    Widget::setFrameGeometry(newGeometry);

    if (newGeometry == oldGeometry)
        return;

    if (newGeometry.width() != oldGeometry.width() || newGeometry.height() != oldGeometry.height()) {
        updateScrollbars(m_data->m_scrollOffset);
        static_cast<FrameView*>(this)->setNeedsLayout();
    }

    geometryChanged();
}

HashSet<Widget*>* ScrollView::children()
{
    return &(m_data->m_children);
}

void ScrollView::geometryChanged() const
{
    HashSet<Widget*>::const_iterator end = m_data->m_children.end();
    for (HashSet<Widget*>::const_iterator current = m_data->m_children.begin(); current != end; ++current)
        (*current)->geometryChanged();
}


int ScrollView::contentsX() const
{
    return scrollOffset().width();
}

int ScrollView::contentsY() const
{
    return scrollOffset().height();
}

int ScrollView::contentsWidth() const
{
    return m_data->m_contentsSize.width();
}

int ScrollView::contentsHeight() const
{
    return m_data->m_contentsSize.height();
}

IntPoint ScrollView::windowToContents(const IntPoint& windowPoint) const
{
    IntPoint viewPoint = convertFromContainingWindow(windowPoint);
    return viewPoint + scrollOffset();
}

IntPoint ScrollView::contentsToWindow(const IntPoint& contentsPoint) const
{
    IntPoint viewPoint = contentsPoint - scrollOffset();
    return convertToContainingWindow(viewPoint);
}

IntPoint ScrollView::convertChildToSelf(const Widget* child, const IntPoint& point) const
{
    IntPoint newPoint = point;
    if (child != m_data->m_hBar && child != m_data->m_vBar)
        newPoint = point - scrollOffset();
    return Widget::convertChildToSelf(child, newPoint);
}

IntPoint ScrollView::convertSelfToChild(const Widget* child, const IntPoint& point) const
{
    IntPoint newPoint = point;
    if (child != m_data->m_hBar && child != m_data->m_vBar)
        newPoint = point + scrollOffset();
    return Widget::convertSelfToChild(child, newPoint);
}

IntSize ScrollView::scrollOffset() const
{
    return m_data->m_scrollOffset;
}

IntSize ScrollView::maximumScroll() const
{
    IntSize delta = (m_data->m_contentsSize - IntSize(visibleWidth(), visibleHeight())) - scrollOffset();
    delta.clampNegativeToZero();
    return delta;
}

void ScrollView::scrollBy(int dx, int dy)
{
    IntSize scrollOffset = m_data->m_scrollOffset;
    IntSize newScrollOffset = scrollOffset + IntSize(dx, dy).shrunkTo(maximumScroll());
    newScrollOffset.clampNegativeToZero();

    if (newScrollOffset == scrollOffset)
        return;

    updateScrollbars(newScrollOffset);
}

void ScrollView::scrollRectIntoViewRecursively(const IntRect& r)
{
    IntPoint p(max(0, r.x()), max(0, r.y()));
    ScrollView* view = this;
    ScrollView* oldView = view;
    while (view) {
        view->setContentsPos(p.x(), p.y());
        p.move(view->x() - view->scrollOffset().width(), view->y() - view->scrollOffset().height());
        view = static_cast<ScrollView*>(parent());
    }
}

WebCore::ScrollbarMode ScrollView::hScrollbarMode() const
{
    return m_data->m_hScrollbarMode;
}

WebCore::ScrollbarMode ScrollView::vScrollbarMode() const
{
    return m_data->m_vScrollbarMode;
}

void ScrollView::suppressScrollbars(bool suppressed, bool repaintOnSuppress)
{
    m_data->m_scrollbarsSuppressed = suppressed;
    if (repaintOnSuppress && !suppressed) {
        if (m_data->m_hBar)
            m_data->m_hBar->invalidate();
        if (m_data->m_vBar)
            m_data->m_vBar->invalidate();

        // Invalidate the scroll corner too on unsuppress.
        IntRect hCorner;
        if (m_data->m_hBar && width() - m_data->m_hBar->width() > 0) {
            hCorner = IntRect(m_data->m_hBar->width(),
                              height() - m_data->m_hBar->height(),
                              width() - m_data->m_hBar->width(),
                              m_data->m_hBar->height());
            invalidateRect(hCorner);
        }

        if (m_data->m_vBar && height() - m_data->m_vBar->height() > 0) {
            IntRect vCorner(width() - m_data->m_vBar->width(),
                            m_data->m_vBar->height(),
                            m_data->m_vBar->width(),
                            height() - m_data->m_vBar->height());
            if (vCorner != hCorner)
                invalidateRect(vCorner);
        }
    }
}

void ScrollView::setHScrollbarMode(ScrollbarMode newMode)
{
    if (m_data->m_hScrollbarMode != newMode) {
        m_data->m_hScrollbarMode = newMode;
        updateScrollbars(m_data->m_scrollOffset);
    }
}

void ScrollView::setVScrollbarMode(ScrollbarMode newMode)
{
    if (m_data->m_vScrollbarMode != newMode) {
        m_data->m_vScrollbarMode = newMode;
        updateScrollbars(m_data->m_scrollOffset);
    }
}

void ScrollView::setScrollbarsMode(ScrollbarMode newMode)
{
    if (m_data->m_hScrollbarMode != newMode ||
        m_data->m_vScrollbarMode != newMode) {
        m_data->m_hScrollbarMode = m_data->m_vScrollbarMode = newMode;
        updateScrollbars(m_data->m_scrollOffset);
    }
}

void ScrollView::setStaticBackground(bool flag)
{
    m_data->m_hasStaticBackground = flag;
}

bool ScrollView::inWindow() const
{
    return true;
}

void ScrollView::updateScrollbars(const IntSize& desiredOffset)
{
    // Don't allow re-entrancy into this function.
    if (m_data->m_inUpdateScrollbars)
        return;

    // FIXME: This code is here so we don't have to fork FrameView.h/.cpp.
    // In the end, FrameView should just merge with ScrollView.
    if (static_cast<const FrameView*>(this)->frame()->prohibitsScrolling())
        return;
    
    m_data->m_inUpdateScrollbars = true;

    bool hasVerticalScrollbar = m_data->m_vBar;
    bool hasHorizontalScrollbar = m_data->m_hBar;
    bool oldHasVertical = hasVerticalScrollbar;
    bool oldHasHorizontal = hasHorizontalScrollbar;
    ScrollbarMode hScroll = m_data->m_hScrollbarMode;
    ScrollbarMode vScroll = m_data->m_vScrollbarMode;
    
    const int cVerticalWidth = PlatformScrollbar::verticalScrollbarWidth();
    const int cHorizontalHeight = PlatformScrollbar::horizontalScrollbarHeight();

    for (int pass = 0; pass < 2; pass++) {
        bool scrollsVertically;
        bool scrollsHorizontally;

        if (!m_data->m_scrollbarsSuppressed && (hScroll == ScrollbarAuto || vScroll == ScrollbarAuto)) {
            // Do a layout if pending before checking if scrollbars are needed.
            if (hasVerticalScrollbar != oldHasVertical || hasHorizontalScrollbar != oldHasHorizontal)
                static_cast<FrameView*>(this)->layout();
             
            scrollsVertically = (vScroll == ScrollbarAlwaysOn) || (vScroll == ScrollbarAuto && contentsHeight() > height());
            if (scrollsVertically)
                scrollsHorizontally = (hScroll == ScrollbarAlwaysOn) || (hScroll == ScrollbarAuto && contentsWidth() + cVerticalWidth > width());
            else {
                scrollsHorizontally = (hScroll == ScrollbarAlwaysOn) || (hScroll == ScrollbarAuto && contentsWidth() > width());
                if (scrollsHorizontally)
                    scrollsVertically = (vScroll == ScrollbarAlwaysOn) || (vScroll == ScrollbarAuto && contentsHeight() + cHorizontalHeight > height());
            }
        }
        else {
            scrollsHorizontally = (hScroll == ScrollbarAuto) ? hasHorizontalScrollbar : (hScroll == ScrollbarAlwaysOn);
            scrollsVertically = (vScroll == ScrollbarAuto) ? hasVerticalScrollbar : (vScroll == ScrollbarAlwaysOn);
        }
        
        if (hasVerticalScrollbar != scrollsVertically) {
            m_data->setHasVerticalScrollbar(scrollsVertically);
            hasVerticalScrollbar = scrollsVertically;
        }

        if (hasHorizontalScrollbar != scrollsHorizontally) {
            m_data->setHasHorizontalScrollbar(scrollsHorizontally);
            hasHorizontalScrollbar = scrollsHorizontally;
        }
    }
    
    // Set up the range (and page step/line step).
    IntSize maxScrollPosition(contentsWidth() - visibleWidth(), contentsHeight() - visibleHeight());
    IntSize scroll = desiredOffset.shrunkTo(maxScrollPosition);
    scroll.clampNegativeToZero();
 
    if (m_data->m_hBar) {
        int clientWidth = visibleWidth();
        m_data->m_hBar->setEnabled(contentsWidth() > clientWidth);
        int pageStep = (clientWidth - PAGE_KEEP);
        if (pageStep < 0) pageStep = clientWidth;
        IntRect oldRect(m_data->m_hBar->frameGeometry());
        IntRect hBarRect = IntRect(0,
                                   height() - m_data->m_hBar->height(),
                                   width() - (m_data->m_vBar ? m_data->m_vBar->width() : 0),
                                   m_data->m_hBar->height());
        m_data->m_hBar->setRect(hBarRect);
        if (!m_data->m_scrollbarsSuppressed && oldRect != m_data->m_hBar->frameGeometry())
            m_data->m_hBar->invalidate();

        m_data->m_hBar->setSteps(LINE_STEP, pageStep);
        m_data->m_hBar->setProportion(clientWidth, contentsWidth());
        m_data->m_hBar->setValue(scroll.width());
    } 

    if (m_data->m_vBar) {
        int clientHeight = visibleHeight();
        m_data->m_vBar->setEnabled(contentsHeight() > clientHeight);
        int pageStep = (clientHeight - PAGE_KEEP);
        if (pageStep < 0) pageStep = clientHeight;
        IntRect oldRect(m_data->m_vBar->frameGeometry());
        IntRect vBarRect = IntRect(width() - m_data->m_vBar->width(), 
                                   0,
                                   m_data->m_vBar->width(),
                                   height() - (m_data->m_hBar ? m_data->m_hBar->height() : 0));
        m_data->m_vBar->setRect(vBarRect);
        if (!m_data->m_scrollbarsSuppressed && oldRect != m_data->m_vBar->frameGeometry())
            m_data->m_vBar->invalidate();

        m_data->m_vBar->setSteps(LINE_STEP, pageStep);
        m_data->m_vBar->setProportion(clientHeight, contentsHeight());
        m_data->m_vBar->setValue(scroll.height());
    }

    if (oldHasVertical != (m_data->m_vBar != 0) || oldHasHorizontal != (m_data->m_hBar != 0))
        geometryChanged();

    // See if our offset has changed in a situation where we might not have scrollbars.
    // This can happen when editing a body with overflow:hidden and scrolling to reveal selection.
    // It can also happen when maximizing a window that has scrollbars (but the new maximized result
    // does not).
    IntSize scrollDelta = scroll - m_data->m_scrollOffset;
    if (scrollDelta != IntSize()) {
       m_data->m_scrollOffset = scroll;
       m_data->scrollBackingStore(scrollDelta);
    }

    m_data->m_inUpdateScrollbars = false;
}

PlatformScrollbar* ScrollView::scrollbarUnderMouse(const PlatformMouseEvent& mouseEvent)
{
    IntPoint viewPoint = convertFromContainingWindow(mouseEvent.pos());
    if (m_data->m_hBar && m_data->m_hBar->frameGeometry().contains(viewPoint))
        return m_data->m_hBar.get();
    if (m_data->m_vBar && m_data->m_vBar->frameGeometry().contains(viewPoint))
        return m_data->m_vBar.get();
    return 0;
}

void ScrollView::addChild(Widget* child)
{
    QWidget* w = child->qwidget();
    if (w)
        m_data->m_children.add(child);

    child->setParent(this);
}

void ScrollView::removeChild(Widget* child)
{
    child->setParent(0);
    child->hide();
    QWidget* w = child->qwidget();
    if (w)
        m_data->m_children.remove(child);
}

void ScrollView::paint(GraphicsContext* context, const IntRect& rect)
{
    Q_ASSERT(isFrameView());

    m_data->m_dirtyRegion = QRegion(); //clear the cache...

    if (context->paintingDisabled())
        return;

    IntRect documentDirtyRect = rect;

    QPainter *p = context->platformContext();

    context->save();

    QRect canvasRect = frameGeometry();
    documentDirtyRect.intersect(canvasRect);
    context->translate(canvasRect.x(), canvasRect.y());
    documentDirtyRect.move(-canvasRect.x(), -canvasRect.y());

    context->translate(-contentsX(), -contentsY());
    documentDirtyRect.move(contentsX(), contentsY());

    IntRect clipRect = enclosingIntRect(visibleContentRect());

    context->clip(clipRect);

#ifdef DEBUG_SCROLLVIEW
    qDebug() << "ScrollView::paint --> "
             << "isSubframe" << (qwebframe() != qwebframe()->page()->mainFrame() ? "true" : "false")
             << "rect" << rect
             << "canvasRect" << canvasRect
             << "clipRect" << clipRect
             << "documentDirtyRect" << documentDirtyRect
             << endl;
#endif

    static_cast<const FrameView*>(this)->frame()->paint(context, documentDirtyRect);

    context->restore();

    // Now paint the scrollbars.
    if (!m_data->m_scrollbarsSuppressed && (m_data->m_hBar || m_data->m_vBar)) {
        context->save();

        IntRect scrollViewDirtyRect = rect;
        scrollViewDirtyRect.intersect(canvasRect);

        context->translate(canvasRect.x(), canvasRect.y());
        scrollViewDirtyRect.move(-canvasRect.x(), -canvasRect.y());

        if (m_data->m_hBar)
            m_data->m_hBar->paint(context, scrollViewDirtyRect);
        if (m_data->m_vBar)
            m_data->m_vBar->paint(context, scrollViewDirtyRect);

        // Fill the scroll corner with white.
        IntRect hCorner;
        if (m_data->m_hBar && width() - m_data->m_hBar->width() > 0) {
            hCorner = IntRect(m_data->m_hBar->width(),
                              height() - m_data->m_hBar->height(),
                              width() - m_data->m_hBar->width(),
                              m_data->m_hBar->height());
            if (hCorner.intersects(scrollViewDirtyRect))
                context->fillRect(hCorner, Color::white);
        }

        if (m_data->m_vBar && height() - m_data->m_vBar->height() > 0) {
            IntRect vCorner(width() - m_data->m_vBar->width(),
                            m_data->m_vBar->height(),
                            m_data->m_vBar->width(),
                            height() - m_data->m_vBar->height());
            if (vCorner != hCorner && vCorner.intersects(scrollViewDirtyRect))
                context->fillRect(vCorner, Color::white);
        }

        context->restore();
    }
}

void ScrollView::wheelEvent(PlatformWheelEvent& e)
{
    // Determine how much we want to scroll.  If we can move at all, we will accept the event.
    IntSize maxScrollDelta = maximumScroll();
    if ((e.deltaX() < 0 && maxScrollDelta.width() > 0) ||
        (e.deltaX() > 0 && scrollOffset().width() > 0) ||
        (e.deltaY() < 0 && maxScrollDelta.height() > 0) ||
        (e.deltaY() > 0 && scrollOffset().height() > 0))
        e.accept();

    scrollBy(-e.deltaX() * LINE_STEP, -e.deltaY() * LINE_STEP);
}

bool ScrollView::scroll(ScrollDirection direction, ScrollGranularity granularity)
{
    if (direction == ScrollUp || direction == ScrollDown) {
        if (m_data->m_vBar)
            return m_data->m_vBar->scroll(direction, granularity);
    } else {
        if (m_data->m_hBar)
            return m_data->m_hBar->scroll(direction, granularity);
    }
    return false;
}

IntRect ScrollView::windowResizerRect()
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return IntRect();
    return page->chrome()->windowResizerRect();
}

bool ScrollView::resizerOverlapsContent() const
{
    return !m_data->m_scrollbarsAvoidingResizer;
}

void ScrollView::adjustOverlappingScrollbarCount(int overlapDelta)
{
    int oldCount = m_data->m_scrollbarsAvoidingResizer;
    m_data->m_scrollbarsAvoidingResizer += overlapDelta;
    if (parent() && parent()->isFrameView())
        static_cast<FrameView*>(parent())->adjustOverlappingScrollbarCount(overlapDelta);
    else if (!m_data->m_scrollbarsSuppressed) {
        // If we went from n to 0 or from 0 to n and we're the outermost view,
        // we need to invalidate the windowResizerRect(), since it will now need to paint
        // differently.
        if (oldCount > 0 && m_data->m_scrollbarsAvoidingResizer == 0 ||
            oldCount == 0 && m_data->m_scrollbarsAvoidingResizer > 0)
            invalidateRect(windowResizerRect());
    }
}

void ScrollView::setParent(ScrollView* parentView)
{
    if (!parentView && m_data->m_scrollbarsAvoidingResizer && parent() && parent()->isFrameView())
        static_cast<FrameView*>(parent())->adjustOverlappingScrollbarCount(false);
    Widget::setParent(parentView);
}

void ScrollView::addToDirtyRegion(const IntRect& containingWindowRect)
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return;
    page->chrome()->addToDirtyRegion(containingWindowRect);
}

void ScrollView::scrollBackingStore(int dx, int dy, const IntRect& scrollViewRect, const IntRect& clipRect)
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return;
    page->chrome()->scrollBackingStore(dx, dy, scrollViewRect, clipRect);
}

void ScrollView::updateBackingStore()
{
    ASSERT(isFrameView());
    const FrameView* frameView = static_cast<const FrameView*>(this);
    Page* page = frameView->frame() ? frameView->frame()->page() : 0;
    if (!page)
        return;
    page->chrome()->updateBackingStore();
}

}

// vim: ts=4 sw=4 et
