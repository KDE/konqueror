/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2008 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2008 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPAGE_H
#define WEBENGINEPAGE_H

#include "websslinfo.h"
#include "kwebenginepartlib_export.h"

#include <KParts/BrowserExtension>
#include <QWebEnginePage>

#include <QUrl>
#include <QMultiHash>
#include <QPointer>
#include <QScopedPointer>
#include <QWebEngineFullScreenRequest>

class QAuthenticator;
class WebSslInfo;
class WebEnginePart;
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

    void download(QWebEngineDownloadItem *it, bool newWindow = false);

    void requestOpenFileAsTemporary(const QUrl &url, const QString &mimeType = "", bool newWindow = false, bool newTab = false);

    void setStatusBarText(const QString &text);

    /**
    * @brief Tells the page that the part has requested to load the given URL
    *
    * @note Calling this function doesn't cause the page to be loaded: you still need to call load() to do so.
    * @see m_urlRequestedByApp
    * @param url the requested URL
    */
//     void markUrlAsRequestedByApp(const QUrl &url){m_urlRequestedByApp = url;}

//     void forceLoadingOfUrl(const QUrl &url){m_forcedUrl = url;}

    /**
     * @brief Sets the webengine part to be used by this object.
     * @param part the part
     */
    void setPart(WebEnginePart *part);

#ifndef REMOTE_DND_NOT_HANDLED_BY_WEBENGINE
    /**
     * @brief Informs the page that a drop operation has been started
     *
     * Calling this method changes the behavior of createWindow()
     * @note Since `QWebEngineView` doesn't provide a way to tell when a drop operation actually end,
     * there's no function to inform that the drop operation has ended. The page considers the
     * drop operation to have ended when one the following happens:
     * - createWindow() is called
     * - the `loadStarted` signal is emitted
     * - 100ms have elapsed since the last call to this method (this time interval has been chosen arbitrary: it should
     *   be enough for the drop operation to have actually ended but short enough to make it unlikely that
     *   the user has started another action)
     * @see m_dropOperationTimer
     * @see createWindow()
     * @see WebEngineView::dropEvent
     */
    void setDropOperationStarted();
#endif

Q_SIGNALS:
    /**
     * This signal is emitted whenever a user cancels/aborts a load resource
     * request.
     */
    void loadAborted(const QUrl &url);

    void mainFrameNavigationRequested(WebEnginePage *page, const QUrl);

protected:
    /**
     * Returns the webengine part in use by this object.
     * @internal
     */
    WebEnginePart* part() const;

    /**
     * @brief Override of `QWebEnginePage::createWindow`
     *
     * Reimplemented for internal reasons, the API is not affected.
     *
     * By default, a new NewWindowPage will be returned; however, calling setDropOperationStarted changes
     * this behavior: in this case, no pages will be created and the function returns `this`. The default
     * behavior is restored when the drop operation ends (see setDropOperationStarted()).
     * @param type the window type to create. This is ignored if setDropOperationStarted has been called with `true`
     * @return a new NewWindowPage or `this` if within a drop operation
     * @see setDropOperationStarted
     */
    QWebEnginePage* createWindow(WebWindowType type) override;
protected:

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    bool acceptNavigationRequest(const QUrl& request, NavigationType type, bool isMainFrame) override;

    /**
    * @brief Override of `QWebEnginePage::certificateError`
    *
    * If the error is overridable, it first checks whether the user has already choosen to permanently ignore the error, in which case it returns `true`.
    * If the user hasn't made such a choice, the error is deferred and a WebEnginePartCertificateErrorDlg is shown. The result is handled by handleCertificateError.
    *
    * @internal
    * A problem arises if the certificate error happens while loading a page opened from a part which is not a WebEnginePart (this also includes the case when the
    * URL is entered in the location bar when the part is active). In this case, the URL is first loaded by KIO, then by WebEnginePart. If there's a certificate error,
    * it will be reported twice: by KIO and by the WebEnginePart. The old trick of using `m_urlLoadedByPart` doesn't work anymore, because if the current part is
    * a WebEnginePart, WebEnginePart::openUrl will be called, but everything will be handled by WebEnginePart.
    *
    * @param _ce the certificate error
    * @return @b false if the error is not overridable and @true in all other cases. Note that if @b _ce is deferred, according to the documentation for `QWebEngineCertificateError
    * the return value is ignored.
    */
    bool certificateError(const QWebEngineCertificateError &_ce) override;

protected Q_SLOTS:
    void slotLoadFinished(bool ok);
    void slotUnsupportedContent(QNetworkReply* reply);
    virtual void slotGeometryChangeRequested(const QRect& rect);
    void slotFeaturePermissionRequested(const QUrl& url, QWebEnginePage::Feature feature);
    void slotAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);
    void changeFullScreenMode(QWebEngineFullScreenRequest req);

private:
    bool checkLinkSecurity(const QNetworkRequest& req, NavigationType type) const;
    bool checkFormData(const QUrl& url) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl& url);
    bool askBrowserToOpenUrl(const QUrl &url, const QString &mimetype=QString(), const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(), const KParts::BrowserArguments &bargs = KParts::BrowserArguments());
//     bool askBrowserToOpenUrlInPart(const QUrl &url, const QString &part);

    //Whether a local URL should be opened by this part or by another part. This takes into account the user preferences
    //and it's needed to avoid, for example, that the link "Home Folder" in the intro page is displayed in WebEnginePart
    //and not by the part selected by the user to display directories.
    bool shouldOpenLocalUrl(const QUrl &url) const;

    /**
     * @brief Function called when the part is forced to save an URL to disk.
     *
     * This function should never be needed because the URL should be handled by the application itself.
     * However, there can be cases in which this doesn't happen, in particular
     * if there's something wrong with the system configuration and the application asks the part to
     * handle something it can't display. In this case, it would work as follow:
     * - the application asks the part to display the URL
     * - the part can't display the URL, so a download is triggered, causing the openUrlRequest signal
     *   to be emitted
     * - the application receives the signal and decides that the part should handle the URL
     * - the part can't display the URL and triggers a download
     * - endless loop
     *
     * To avoid this situation, inside download(), the part checks whether it was asked to handle
     * the URL by the application. In that case, it doesn't emit the openUrlRequest signal
     * but calls this function to directly download the file to disk.
     *
     * @param it the item describing the download
     */
     void saveUrlToDisk(QWebEngineDownloadItem *it);
     void saveUrlUsingKIO(const QUrl &origUrl, const QUrl &destUrl);

private:
    enum WebEnginePageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    int m_kioErrorCode;
    bool m_ignoreError;

    WebSslInfo m_sslInfo;
    QPointer<WebEnginePart> m_part;

    QScopedPointer<KPasswdServerClient> m_passwdServerClient;

#ifndef REMOTE_DND_NOT_HANDLED_BY_WEBENGINE
    /**
     * @brief Timer used to decide whether a drop operation is happening
     *
     * A drop operation is happening if this timer is active
     * @see setDropOperationStarted for more details
     */
    QTimer *m_dropOperationTimer;
#endif
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
};

#endif // WEBENGINEPAGE_H

