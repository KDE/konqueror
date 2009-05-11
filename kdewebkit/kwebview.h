/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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
#ifndef KWEBVIEW_H
#define KWEBVIEW_H

#include <kdemacros.h>

#include <QtWebKit/QWebView>
#include "kwebpage.h"

class QWebHitTestResult;
class KUrl;
class QMouseEvent;
class QWheelEvent;
namespace KParts
{
class OpenUrlArguments;
class BrowserArguments;
}

class KDE_EXPORT KWebView : public QWebView
{
    Q_OBJECT
public:
    explicit KWebView(QWidget *parent = 0);
    ~KWebView();
    /**
     * This method returns the current KWebPage, if there is none, one will be created.
     * It calles virtual method setNewPage() to create new (K)WebPage, so
     * of you reimplements KWebPage ypu should reimplement this setNewPage()
     * @see setPage()
     * @see setNewPage()
     * @short Getter for KWebPage
     * @return current KWebPage
     */
    KWebPage *page() const;
    void setPage(KWebPage *page);
    QWidget *searchBar();

public Q_SLOTS:
    void setCustomContextMenu(bool show);

    /**
     * similar to load(const QNetworkRequest&, QNetworkAccessManager::Operation), but for KParts-style arguments instead.
     */
    void load(const KUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &bargs);

Q_SIGNALS:
    void showContextMenu(QContextMenuEvent *event);
    void openUrl(const KUrl &url);
    void openUrlInNewTab(const KUrl &url);

protected:
    /**
     * Creates new (K)WebPage. This virtual method is called by page() to create new (K)WebPage if necessary.
     * Reimplement this method if you reimplement KWebPage, e.g:
     * @code
     * void MyWebView::setNewPage()
     * {
     *     setPage(new MyWebPage(this));
     * }
     * @endcode
     * @see page()
     */
    virtual void setNewPage();
    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

protected Q_SLOTS:
    virtual void slotFindNextClicked();
    virtual void slotFindPreviousClicked();
    virtual void slotSearchChanged(const QString &);
    virtual void resultSearch(KWebPage::FindFlags flags);

private:
    class KWebViewPrivate;
    KWebViewPrivate* const d;
};

#endif // KWEBVIEW_H
