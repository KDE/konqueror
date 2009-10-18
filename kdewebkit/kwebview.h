/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
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

class KUrl;

namespace KParts
{
    class OpenUrlArguments;
    class BrowserArguments;
}

/**
 * @short A re-implementation of QWebView to provide KDE integration.
 *
 * This is a convenience class that provides an implementation of QWebView with
 * full integration with KDE technologies for networking (KIO), cookie handling
 * (KCookieJar) and embeded non-html content (<embed>) handling (KPart apps).
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
     * Returns true if external content is fetched.
     * @see setAllowExternalContent().
     */
    bool isExternalContentAllowed() const;

    /**
     * Set @p allow to false if you don't want to allow showing external content,
     * so no external images for example. By default external content is fetched.
     * @see isExternalContentAllowed().
     */
    void setAllowExternalContent(bool allow);

Q_SIGNALS:
    /**
     * This signal is emitted when the user wants to navigate to the url @p url.
     */
    void openUrl(const KUrl &url);

    /**
     * This signal is emitted when the user clicks on a link with the middle
     * mouse button.
     */
    void openUrlInNewTab(const KUrl &url);

    /**
     * This signal is emitted when the user presses shift and clicks on a
     * link with the mouse button.
     */
    void saveUrl(const KUrl &url);

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::wheelEvent.
     * @internal
     */
    void wheelEvent(QWheelEvent *event);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mousePressEvent.
     * @internal
     */
    virtual void mousePressEvent(QMouseEvent *event);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mouseReleaseEvent.
     * @internal
     */
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    class KWebViewPrivate;
    KWebViewPrivate* const d;
};

#endif // KWEBVIEW_H
