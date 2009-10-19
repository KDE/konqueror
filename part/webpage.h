/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
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
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "webkitkde_export.h"

#include <kdewebkit/kwebpage.h>

class KUrl;
class WebSslInfo;
class WebKitPart;
class QVariant;

class WEBKITKDE_EXPORT WebPage : public KWebPage
{
    Q_OBJECT
public:
    WebPage(WebKitPart *wpart, QWidget *parent);
    ~WebPage();

    /**
     * Re-implemented for internal reasons. API is unaffected.
     *
     * @see KWebPage::authorizedRequest.
     */
    bool authorizedRequest(const QUrl &) const;

    /**
     * Returns the SSL information for the current page.
     *
     * @see WebSslInfo.
     */
    const WebSslInfo& sslInfo() const;

Q_SIGNALS:
    void updateHistory();

    /**
     * This signal is emitted whenever a navigation request by the main frame completes.
     */
    void navigationRequestFinished();

    /**
     * This signal is emitted under the same condition as QWebPage::loadStarted()
     * except it also sends the url being loaded.
     */
    void loadStarted(const QUrl& url);

    /**
     * This signal is emitted whenever a user cancels/aborts a load resource request.
     */
    void loadAborted(const QUrl& newUrl);

    /**
     * This signal is emitted whenever an error is encoutered while loading the requested resource.
     */
    void loadError(int, const QString&, const QString& frameName = QString());

public Q_SLOTS:
    /**
     * Prompts the user to saves the contents of the specified @p url.
     */
    void saveUrl(const KUrl &url);


protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual QWebPage* createWindow(WebWindowType type);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual bool acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type);

protected Q_SLOTS:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see KWebPage::slotUnsupportedContent
     * @internal
     */
    void slotUnsupportedContent(QNetworkReply *reply);
    void slotDownloadRequest(const QNetworkRequest &request);

    void slotRequestFinished(QNetworkReply *reply);
    void slotGeometryChangeRequested(const QRect &rect);
    void slotWindowCloseRequested();
    void slotStatusBarMessage(const QString &message);

private:
    bool checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const;
    bool checkFormData(const QNetworkRequest &req) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl &url);

private:
    class WebPagePrivate;
    WebPagePrivate* const d;
};

#endif // WEBPAGE_H
