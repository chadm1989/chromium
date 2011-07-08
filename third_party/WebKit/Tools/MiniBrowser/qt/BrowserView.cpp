/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
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

#include "BrowserView.h"

#include <QGraphicsScene>
#include <qdesktopwebview.h>
#include <qtouchwebview.h>
#include <qtouchwebpage.h>

BrowserView::BrowserView(bool useTouchWebView, QWidget* parent)
    : QGraphicsView(parent)
    , m_item(0)
{
    if (useTouchWebView)
        m_item = new QTouchWebView;
    else
        m_item = new QDesktopWebView;

    setScene(new QGraphicsScene(this));
    scene()->addItem(m_item);

    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

BrowserView::~BrowserView()
{
    delete m_item;
    m_item = 0;
}

void BrowserView::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    QRectF rect(QPoint(0, 0), event->size());
    m_item->setGeometry(rect);
    scene()->setSceneRect(rect);
}

void BrowserView::load(const QString& urlString)
{
    QUrl url(QUrl::fromUserInput(urlString));

    if (desktopWebView())
        desktopWebView()->load(url);
    else if (touchWebView())
        touchWebView()->page()->load(url);
}

QGraphicsWidget* BrowserView::view() const
{
    return m_item;
}

QTouchWebView* BrowserView::touchWebView() const
{
    return qobject_cast<QTouchWebView*>(m_item);
}

QDesktopWebView* BrowserView::desktopWebView() const
{
    return qobject_cast<QDesktopWebView*>(m_item);
}

QAction* BrowserView::navigationAction(QtWebKit::NavigationAction which) const
{
    if (desktopWebView())
        return desktopWebView()->navigationAction(which);
    if (touchWebView())
        return touchWebView()->page()->navigationAction(which);
    Q_ASSERT(false);
    return 0;
}
