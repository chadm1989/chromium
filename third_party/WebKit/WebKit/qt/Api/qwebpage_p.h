/*
    Copyright (C) 2007 Trolltech ASA

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/
#ifndef QWEBPAGE_P_H
#define QWEBPAGE_P_H

#include <qnetworkproxy.h>
#include <qpointer.h>

#include "qwebpage.h"
#include "qwebframe.h"

#include <wtf/RefPtr.h>

namespace WebCore
{
    class ChromeClientQt;
    class ContextMenuClientQt;
    class ContextMenuItem;
    class ContextMenu;
    class EditorClientQt;
    class Node;
    class Page;
}

class QUndoStack;
class QMenu;

class QWebPageContextPrivate
{
public:
    QPoint pos;
    QString text;
    QUrl linkUrl;
    QUrl imageUrl;
    QPixmap image;
    QPointer<QWebFrame> targetFrame;
    RefPtr<WebCore::Node> innerNonSharedNode;
};

class QWebPageContext
{
public:
    QWebPageContext();
    QWebPageContext(const QWebPageContext &other);
    QWebPageContext &operator=(const QWebPageContext &other);
    ~QWebPageContext();

    QPoint pos() const;
    QString text() const;
    QUrl linkUrl() const;
    QUrl imageUrl() const;
    // ### we have a pixmap internally, should this be called pixmap() instead?
    QPixmap image() const;

    QWebFrame *targetFrame() const;

private:
    QWebPageContext(const WebCore::HitTestResult &hitTest);
    QWebPageContextPrivate *d;

    friend class QWebPage;
};

class QWebPagePrivate
{
public:
    QWebPagePrivate(QWebPage *);
    ~QWebPagePrivate();
    void createMainFrame();
    QMenu *createContextMenu(const WebCore::ContextMenu *webcoreMenu, const QList<WebCore::ContextMenuItem> *items);

    QWebFrame *frameAt(const QPoint &pos) const;

    void _q_onLoadProgressChanged(int);
    void _q_webActionTriggered(bool checked);
    void updateAction(QWebPage::WebAction action);
    void updateNavigationActions();
    void updateEditorActions();

    WebCore::ChromeClientQt *chromeClient;
    WebCore::ContextMenuClientQt *contextMenuClient;
    WebCore::EditorClientQt *editorClient;
    WebCore::Page *page;

    QPointer<QWebFrame> mainFrame;
    QPointer<QWebFrame> frameUnderMouse;

    inline QWebFrame *currentFrame(const QPoint &pos) const
    { return frameUnderMouse ? static_cast<QWebFrame *>(frameUnderMouse) : frameAt(pos); }

    QWebPage *q;
    QUndoStack *undoStack;
    QWidget *view;

    QWebNetworkInterface *networkInterface;

    bool modified;

    bool insideOpenCall;
    quint64 m_totalBytes;
    quint64 m_bytesReceived;

    QWebPage::NavigationRequestResponse navigationRequested(QWebFrame *frame, const QWebNetworkRequest &request, QWebPage::NavigationType type);
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy networkProxy;
#endif

    QWebPageHistory history;
    QWebPageContext currentContext;
    QWebSettings *settings;

    QAction *actions[QWebPage::WebActionCount];
};

#endif
