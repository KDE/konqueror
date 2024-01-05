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
#include "qtwebengine6compat.h"
#include "browserextension.h"
#include "webenginepartdownloadmanager.h"

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
class WebEngineDownloadJob;

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

    void requestDownload(QWebEngineDownloadRequest *item, bool newWindow, WebEnginePartDownloadManager::DownloadObjective objective);

    void setStatusBarText(const QString &text);

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

#if QT_VERSION_MAJOR == 6
    QWidget* view() const;
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

#if QT_VERSION_MAJOR < 6
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
#else
signals:
protected Q_SLOTS:
    void handleCertificateError(const QWebEngineCertificateError &ce);
#endif

protected Q_SLOTS:
    void slotLoadFinished(bool ok);
    virtual void slotGeometryChangeRequested(const QRect& rect);
    void slotFeaturePermissionRequested(const QUrl& url, QWebEnginePage::Feature feature);
    void slotAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);
    void changeFullScreenMode(QWebEngineFullScreenRequest req);
    void changeLifecycleState(QWebEnginePage::LifecycleState recommendedState);

    /**
     * @brief Updates the stylesheet applied to the page
     *
     * Since `QtWebEngine` doesn't directly support custom stylesheets, this is done using javascript
     * @param script the script to run to update the stylesheet
     * @see WebEnginePartControls::updateUserStyleSheet
     * @see WebEnginePartControls::updateStyleSheet
     */
    void updateUserStyleSheet(const QString &script);

private:
    bool checkFormData(const QUrl& url) const;
    bool handleMailToUrl (const QUrl& , NavigationType type) const;
    void setPageJScriptPolicy(const QUrl& url);
    bool askBrowserToOpenUrl(const QUrl &url, const QString &mimetype=QString(), const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(), const BrowserArguments &bargs = BrowserArguments());
    bool downloadWithExternalDonwloadManager(const QUrl &url);

    //Whether a local URL should be opened by this part or by another part. This takes into account the user preferences
    //and it's needed to avoid, for example, that the link "Home Folder" in the intro page is displayed in WebEnginePart
    //and not by the part selected by the user to display directories.
    bool shouldOpenLocalUrl(const QUrl &url) const;

    /**
     * @brief Saves the given remote URL to disk and asks the browser to display it
     *
     * This is used in response to the "Save URL as" entry in the context menu
     *
     * The URL is saved using the \link KonqInterfaces::DownloaderInterface DownloaderInterface\endlink.
     * The browser is asked to display the downloaded URL in a new tab and only if it
     * can be embedded. This is to minimize disturbance for the user (which has already
     * had to choose the download destination).
     *
     * @param req the object describing the download request
     * @param args the `KParts::OpenUrlArguments` to pass when asking the browser to display the downloaded URL
     * @param bArgs the `BrowserArguments` arguments to pass when asking the browser to display the downloaded URL
     * @todo Instead of asking the browser to display the downloaded URL, show a message widget at the bottom with
     * a button allowing the user to display the file and which disappears automatically after a time. It's similar
     * to the way Chrome allows to quickly open a file after downloading it, but it is only done when the user
     * explicitly asks to save the URL.
     */
     void saveUrlToDiskAndDisplay(QWebEngineDownloadRequest *req, const KParts::OpenUrlArguments &args, const BrowserArguments &bArgs);

     /**
      * @brief Saves the page currently displayed and opens the local copy
      *
      * This is used in response to the "Save as" menu entry
      * @param req the object describing the download request
      */
     void saveAs(QWebEngineDownloadRequest *req);

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

    QMultiHash<QUrl, QWebEngineDownloadRequest*> m_downloadItems;
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
    WindowArgs m_windowArgs;
    WebWindowType m_type;
    bool m_createNewWindow;
};

#endif // WEBENGINEPAGE_H

