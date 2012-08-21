/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "websslinfo.h"

#include <KDE/KWebPage>
#include <KDE/KParts/BrowserExtension>

#include <QUrl>
#include <QDebug>
#include <QMultiHash>

class KUrl;
class WebSslInfo;
class KWebKitPart;
class QWebFrame;


class WebPage : public KWebPage
{
    Q_OBJECT
public:
    explicit WebPage(KWebKitPart *wpart, QWidget *parent = 0);
    ~WebPage();

    /**
     * Returns the SSL information for the current page.
     *
     * @see WebSslInfo.
     */
    const WebSslInfo& sslInfo() const;

    /**
     * Sets the page's SSL information to @p other.
     *
     * @see WebSslInfo
     */
    void setSslInfo (const WebSslInfo &other);

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

protected:
    /**
     * Returns the webkit part in use by this object.
     * @internal
     */
    KWebKitPart* part() const;

    /**
     * Sets the webkit part to be used by this object.
     * @internal
     */
    void setPart(KWebKitPart*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual QWebPage* createWindow(WebWindowType type);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    virtual QString userAgentForUrl(const QUrl& url) const;

protected Q_SLOTS:
    void slotRequestFinished(QNetworkReply* reply);
    void slotUnsupportedContent(QNetworkReply* reply);
    virtual void slotGeometryChangeRequested(const QRect& rect);

private:
    bool checkLinkSecurity(const QNetworkRequest& req, NavigationType type) const;
    bool checkFormData(const QNetworkRequest& req) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl& url);

private:
    enum WebPageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    int m_kioErrorCode;
    bool m_ignoreError;
    bool m_noJSOpenWindowCheck;

    WebSslInfo m_sslInfo;
    QList<QUrl> m_requestQueue;
    QPointer<KWebKitPart> m_part;
};


/**
 * This is a fake implementation of WebPage to workaround the ugly API used
 * to request for the creation of a new window from javascript in QtWebKit.
 *
 * The KPart API for creating new windows requires all the information about the
 * new window up front. Unfortunately QWebPage::createWindow function does not
 * provide any of these necessary information except for the window type. All
 * the other necessary information is emitted as signals instead! Hence, the
 * need for this class to collect all of the necessary information, such as
 * window name, size and position, before calling KPart's createNewWindow
 * function.
 */
class NewWindowPage : public WebPage
{
    Q_OBJECT
public:
    NewWindowPage(WebWindowType windowType, KWebKitPart* part,
                  bool disableJSWindowOpenCheck= false, QWidget* parent = 0);
    virtual ~NewWindowPage();

protected:
    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);

private Q_SLOTS:
    void slotGeometryChangeRequested(const QRect& rect);
    void slotMenuBarVisibilityChangeRequested(bool visible);
    void slotStatusBarVisibilityChangeRequested(bool visible);
    void slotToolBarVisibilityChangeRequested(bool visible);
    void slotLoadFinished(bool);

private:
    KParts::WindowArgs m_windowArgs;
    WebWindowType m_type;
    bool m_createNewWindow;
    bool m_disableJSOpenwindowCheck;
};

#endif // WEBPAGE_H
