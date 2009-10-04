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

#include <kdewebkit_export.h>

#include <QtWebKit/QWebView>
#include "kwebpage.h"

class KUrl;
class QMouseEvent;
class QWheelEvent;
namespace KParts
{
    class OpenUrlArguments;
    class BrowserArguments;
}

/**
 * An enhanced QWebView with integration into the KDE environment.
 *
 * @author Urs Wolfer <uwolfer @ kde.org>
 * @since 4.4
 */

class KDEWEBKIT_EXPORT KWebView : public QWebView
{
    Q_OBJECT
public:
    /**
     * Constructs an empty KWebView with parent @p parent.
     */
    explicit KWebView(QWidget *parent = 0);

    /**
     * Destroys the KWebView.
     */
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

    /**
     * Set @p allow to false if you don't want to allow showing external content,
     * so no external images for example. By default external content is fetched.
     */
    void setAllowExternalContent(bool allow);

    /**
     * Returns true if external content is fetched, @see setAllowExternalContent().
     */
    bool isExternalContentAllowed() const;

public Q_SLOTS:
    /**
     * similar to load(const QNetworkRequest&, QNetworkAccessManager::Operation), but for KParts-style arguments instead.
     */
    void loadUrl(const KUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &bargs);

Q_SIGNALS:
    void openUrl(const KUrl &url);
    void openUrlInNewTab(const KUrl &url);
    void saveUrl(const KUrl &url);

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

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    void wheelEvent(QWheelEvent *event);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    void mousePressEvent(QMouseEvent *event);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
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
