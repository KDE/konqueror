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

#include <KParts/BrowserExtension>
#include <QtWebEngineWidgets/QWebEnginePage>

#include <QUrl>
#include <QDebug>
#include <QMultiHash>
#include <QPointer>

class QUrl;
class WebSslInfo;
class WebEnginePart;
class QWebEngineDownloadItem;


class WebPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit WebPage(WebEnginePart *wpart, QWidget *parent = Q_NULLPTR);
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
    void downloadRequest(QWebEngineDownloadItem* request);

Q_SIGNALS:
    /**
     * This signal is emitted whenever a user cancels/aborts a load resource
     * request.
     */
    void loadAborted(const QUrl &url);

protected:
    /**
     * Returns the webengine part in use by this object.
     * @internal
     */
    WebEnginePart* part() const;

    /**
     * Sets the webengine part to be used by this object.
     * @internal
     */
    void setPart(WebEnginePart*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    QWebEnginePage* createWindow(WebWindowType type) Q_DECL_OVERRIDE;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    bool acceptNavigationRequest(const QUrl& request, NavigationType type, bool isMainFrame)  Q_DECL_OVERRIDE;

protected Q_SLOTS:
    void slotRequestFinished(QNetworkReply* reply);
    void slotUnsupportedContent(QNetworkReply* reply);
    virtual void slotGeometryChangeRequested(const QRect& rect);
    void slotFeaturePermissionRequested(const QUrl& url, QWebEnginePage::Feature feature);

private:
    bool checkLinkSecurity(const QNetworkRequest& req, NavigationType type) const;
    bool checkFormData(const QNetworkRequest& req) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl& url);

private:
    enum WebPageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    int m_kioErrorCode;
    bool m_ignoreError;

    WebSslInfo m_sslInfo;
    QPointer<WebEnginePart> m_part;
};


/**
 * This is a fake implementation of WebPage to workaround the ugly API used
 * to request for the creation of a new window from javascript in QtWebEngine. PORTING_TODO
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
    NewWindowPage(WebWindowType windowType, WebEnginePart* part,
                  QWidget* parent = Q_NULLPTR);
    virtual ~NewWindowPage();

protected:
    bool acceptNavigationRequest(const QUrl& request, NavigationType type, bool isMainFrame)  Q_DECL_OVERRIDE;

private Q_SLOTS:
    void slotGeometryChangeRequested(const QRect& rect) override;
    void slotMenuBarVisibilityChangeRequested(bool visible);
    void slotStatusBarVisibilityChangeRequested(bool visible);
    void slotToolBarVisibilityChangeRequested(bool visible);
    void slotLoadFinished(bool);

private:
    KParts::WindowArgs m_windowArgs;
    WebWindowType m_type;
    bool m_createNewWindow;
};

#endif // WEBPAGE_H
