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

#include <KDE/KWebPage>

#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QMultiHash>

class KUrl;
class WebSslInfo;
class KWebKitPart;
class QVariant;
class QWebFrame;


class WebPage : public KWebPage
{
    Q_OBJECT
public:
    WebPage(KWebKitPart *wpart, QWidget *parent);
    ~WebPage();

    /**
     * Returns the SSL information for the current page.
     *
     * @see WebSslInfo.
     */
    const WebSslInfo& sslInfo() const;

    /**
     * Sets the cached page SSL information to @p info.
     *
     * @see WebSslInfo
     */
    void setSslInfo (const WebSslInfo &info);

    /**
     * Reimplemented for internal reasons. The API is not affected.
     *
     * @internal
     * @see KWebPage::downloadRequest.
     */
    void downloadRequest(const QNetworkRequest &request);

    /**
     * Returns the error page associated with the KIO error @p code.
     *
     * @param text the error message.
     * @param url the url where the error was encountered.
     *
     * @return html error page.
     */
    QString errorPage(int code, const QString& text, const KUrl& url) const;

    /**
     * Re-implemented to handle ErrorPageExtension.
     *
     * @see QWebPage::extension()
     */
    bool extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output);

    /**
     * Re-implemented to handle ErrorPageExtension.
     *
     * @see QWebPage::supportsExtension()
     */
    bool supportsExtension(Extension extension) const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever a user cancels/aborts a load resource
     * request.
     */
    void loadAborted(const KUrl &url);

    /**
     * This signal is emitted whenever status message is received from javascript
     * and the user's configuration allows it to be set.
     */
    void jsStatusBarMessage(const QString &);

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
    void slotUnsupportedContent(QNetworkReply *reply);
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
