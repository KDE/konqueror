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

#ifndef WEBENGINEPAGE_H
#define WEBENGINEPAGE_H

#include "websslinfo.h"

#include <KParts/BrowserExtension>
#include <QWebEnginePage>

#include <QUrl>
#include <QMultiHash>
#include <QPointer>
#include <QScopedPointer>

class QAuthenticator;
class QUrl;
class WebSslInfo;
class WebEnginePart;
class QWebEngineDownloadItem;
class KPasswdServerClient;
class WebEngineWallet;

class WebEnginePage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit WebEnginePage(WebEnginePart *wpart, QWidget *parent = nullptr);
    ~WebEnginePage() override;

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

    void download(const QUrl &url, bool newWindow = false);

    WebEngineWallet* wallet() const {return m_wallet;}
    
    /**
    * @brief Tells the page that the part has requested to load the given URL
    * 
    * @note Calling this function doesn't cause the page to be loaded: you still need to call load() to do so.
    * @see m_urlLoadedByPart
    * @param url the requested URL
    */
    void setLoadUrlCalledByPart(const QUrl &url){m_urlLoadedByPart = url;}

Q_SIGNALS:
    /**
     * This signal is emitted whenever a user cancels/aborts a load resource
     * request.
     */
    void loadAborted(const QUrl &url);

    void navigationRequested(WebEnginePage* page, const QUrl& url);

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
    QWebEnginePage* createWindow(WebWindowType type) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    bool acceptNavigationRequest(const QUrl& request, NavigationType type, bool isMainFrame) override;
    
    /**
    * @brief Override of `QWebEnginePage::certificateError`
    * 
    * If the error is overridable, asks the user whether to ignore the error or not and returns `true` or `false` accordingly. 
    * If the error is not overridable, it always returns `false`.
    * 
    * @internal
    * A problem arises if the certificate error happens while loading a page requested by WebEnginePart::load() (rather than from the
    * user's interaction with the WebEnginePage itself). The problem is that when WebEnginePart::load() is called, any certificate error
    * will have already been caught by the `KParts` mechanism and the user will already have been asked about it. so it doesn't make sense
    * to ask him again. To avoid doing so, this function checks m_urlLoadedByPart: if it is the same url as the one the certificate
    * refers to, `true` is returned (and m_urlLoadedByPart is reset).
    * @endinternal
    * 
    * @param ce the certificate error
    * @return `true` if the error can be ignored and `false` otherwise
    */
    bool certificateError(const QWebEngineCertificateError &ce) override;

protected Q_SLOTS:
    void slotLoadFinished(bool ok);
    void slotUnsupportedContent(QNetworkReply* reply);
    virtual void slotGeometryChangeRequested(const QRect& rect);
    void slotFeaturePermissionRequested(const QUrl& url, QWebEnginePage::Feature feature);
    void slotAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);

private:
    bool checkLinkSecurity(const QNetworkRequest& req, NavigationType type) const;
    bool checkFormData(const QUrl& url) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl& url);

private:
    enum WebEnginePageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    int m_kioErrorCode;
    bool m_ignoreError;

    WebSslInfo m_sslInfo;
    QPointer<WebEnginePart> m_part;

    QScopedPointer<KPasswdServerClient> m_passwdServerClient;
    WebEngineWallet *m_wallet;
    
    /**
    * @brief The last URL that the part requested to be loaded
    * 
    * Before calling `load()`, the part needs to call setLoadUrlCalledByPart() passing the URL which will be loaded. This variable
    * will be reset either the first time acceptNavigationRequest() is called with a different URL or when certificateError() is called.
    * 
    * This variable is needed to implement certificateError().
    * 
    */
    QUrl m_urlLoadedByPart;
};


/**
 * This is a fake implementation of WebEnginePage to workaround the ugly API used
 * to request for the creation of a new window from javascript in QtWebEngine. PORTING_TODO
 *
 * The KPart API for creating new windows requires all the information about the
 * new window up front. Unfortunately QWebEnginePage::createWindow function does not
 * provide any of these necessary information except for the window type. All
 * the other necessary information is emitted as signals instead! Hence, the
 * need for this class to collect all of the necessary information, such as
 * window name, size and position, before calling KPart's createNewWindow
 * function.
 */
class NewWindowPage : public WebEnginePage
{
    Q_OBJECT
public:
    NewWindowPage(WebWindowType windowType, WebEnginePart* part,
                  QWidget* parent = nullptr);
    ~NewWindowPage() override;

protected:
    bool acceptNavigationRequest(const QUrl& request, NavigationType type, bool isMainFrame) override;

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
    WebEngineWallet* m_wallet; 
};

#endif // WEBENGINEPAGE_H
