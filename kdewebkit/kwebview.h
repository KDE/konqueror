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
class KWebPage;

namespace KParts
{
    class OpenUrlArguments;
    class BrowserArguments;
}

/**
 * @short A reimplementation of QWebView with full KDE integration.
 *
 * This is a drop-in replacement for QWebView that provides full KDE integration
 * through @ref KWebPage as well as additional signals that capture middle,
 * shift and ctrl mouse clicks on links and url pasting from the selection
 * clipboard.
 *
 * @author Urs Wolfer <uwolfer @ kde.org>
 * @author Dawit Alemayehu <adawit @ kde.org>
 *
 * @since 4.4
 */
class KDEWEBKIT_EXPORT KWebView : public QWebView
{
    Q_OBJECT
public:
    /**
     * Constructs a KWebView object with parent @p parent.
     *
     * Set @p createCustomPage to false to prevent the creation of a custom
     * @ref KWebPage object for KDE integration. Doing so allows you to
     * avoid unnecessary object creation and deletion if you are going to
     * use your own custom implementation of KWebPage.
     *
     * @param parent            the parent object.
     * @param createCustomPage  if true, the default, creates a custom KWebPage object.
     */
    explicit KWebView(QWidget *parent = 0, bool createCustomPage = true);

    /**
     * Destroys the KWebView.
     */
    ~KWebView();

    /**
     * Returns true if access to remote content is allowed.
     *
     * By default access to remote content is allowed.
     *
     * @see setAllowExternalContent()
     * @see KWebPage::isExternalContentAllowed()
     */
    bool isExternalContentAllowed() const;

    /**
     * Set @p allow to false if you want to prevent access to remote content.
     *
     * If this function is set to false only resources on the local system
     * can be accessed through this class. By default fetching external content
     * is allowed.
     *
     * @see isExternalContentAllowed()
     * @see KWebPage::setAllowExternalContent(bool)
     */
    void setAllowExternalContent(bool allow);

Q_SIGNALS:
    /**
     * This signal is emitted when a url from the selection clipboard is pasted
     * on this view.
     *
     * @param url   the url of the clicked link.
     */
    void selectionClipboardUrlPasted(const KUrl &url);

    /**
     * This signal is emitted when a link is shift clicked with the left mouse
     * button.
     *
     * @param url   the url of the clicked link.
     */
    void linkShiftClicked(const KUrl &url);

    /**
     * This signal is emitted when a link is either clicked with the middle
     * mouse button or ctrl-clicked with the left mouse button.
     *
     * @param url   the url of the clicked link.
     */
    void linkMiddleOrCtrlClicked(const KUrl &url);

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::wheelEvent
     * @internal
     */
    void wheelEvent(QWheelEvent *event);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mousePressEvent
     * @internal
     */
    virtual void mousePressEvent(QMouseEvent *event);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mouseReleaseEvent
     * @internal
     */
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    class KWebViewPrivate;
    KWebViewPrivate* const d;
};

#endif // KWEBVIEW_H
