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
#include "browserextension.h"
#include "webenginepartdownloadmanager.h"

#include <QWebEnginePage>

#include <QUrl>
#include <QMultiHash>
#include <QPointer>
#include <QScopedPointer>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineDownloadRequest>
#include <QWebEngineNavigationRequest>
#include <QWebEngineLoadingInfo>

class QAuthenticator;
class WebSslInfo;
class WebEnginePart;
class KPasswdServerClient;
class WebEngineWallet;
class WebEngineDownloadJob;
class WebEnginePartControls;

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

    /**
     * @brief Informs the page that a drop operation has been started
     *
     * Calling this method changes the behavior of createNewWindow()
     * @note Since `QWebEngineView` doesn't provide a way to tell when a drop operation actually end,
     * there's no function to inform that the drop operation has ended. The page considers the
     * drop operation to have ended when one the following happens:
     * - createNewWindow() is called
     * - the `loadStarted` signal is emitted
     * - 100ms have elapsed since the last call to this method (this time interval has been chosen arbitrary: it should
     *   be enough for the drop operation to have actually ended but short enough to make it unlikely that
     *   the user has started another action)
     * @see m_dropOperationTimer
     * @see createWindow()
     * @see WebEngineView::dropEvent
     */
    void setDropOperationStarted();

    static bool allowLifeycleStateManagement(bool allow);

    QWidget* view() const;

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

protected:

    /**
     * Reimplemented for internal reasons, the API is not affected.
     * @internal
     */
    // bool acceptNavigationRequest(const QUrl& request, NavigationType type, bool isMainFrame) override;

    /**
     * @brief Helper function to easily access the single WebEnginePartControls instance
     *
     * This is a shortcut for `WebEnginePartControls::self()`
     *
     * @return the single WebEnginePartControls instance
     */
    WebEnginePartControls* controls() const;

protected Q_SLOTS:
    void slotLoadFinished(bool ok);
    virtual void slotGeometryChangeRequested(const QRect& rect);
    void slotFeaturePermissionRequested(const QUrl& url, QWebEnginePage::Feature feature);
    void slotAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);
    void changeFullScreenMode(QWebEngineFullScreenRequest req);

    /**
     * @brief Changes the life cycle state of the page
     *
     * This will make the page either `Active` or `Frozen` (it's never `Discarded`).
     * A page will be made `Frozen` only if:
     * - @p recommendedState is't `Active`
     * - the page is not visible
     * - the page is not loading (according to #m_loadStatus)
     * - life cycle state management is enabled (according to WebEnginePartControls::isPageLifecycleManagementEnabled()
     *
     * @param recommendedState the life cycle state recommended by `QWebEnginePage`
     */
    void changeLifecycleState(QWebEnginePage::LifecycleState recommendedState);
#ifdef MEDIAREQUEST_SUPPORTED
    void chooseDesktopMedia(const QWebEngineDesktopMediaRequest &request);
#endif
    void createNewWindow(QWebEngineNewWindowRequest &req);

    void print();
#ifdef WEBENGINE_FRAMES_SUPPORTED
    void printFrame(QWebEngineFrame frame);
#endif

    /**
    * @brief Handles a certificate error
    * @see CertificateErrorDialogManager::handleCertificateError
    *
    * @param _ce the certificate error
    */
    void handleCertificateError(const QWebEngineCertificateError &ce);

    void handleNavigationRequest(QWebEngineNavigationRequest &req);

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
    bool handleMailToUrl (const QUrl& , QWebEngineNavigationRequest::NavigationType type) const;
    void setPageJScriptPolicy(const QUrl& url);
    bool askBrowserToOpenUrl(const QUrl &url, const QString &mimetype=QString(), const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(), const BrowserArguments &bargs = BrowserArguments());
    bool downloadWithExternalDonwloadManager(const QUrl &url);
    /**
     * @brief Decide whether the page is allowed to open a new window requested by javascript or not
     * @param url the URL to load in the new window
     * @return `true` if the page is allowed to open the new window and `false` if it isn't
     */
    bool decideHandlingOfJavascripWindow(const QUrl url) const;

    /**
     * @brief Whether WebEnginePart should open an URL by itself or delegate the main application
     *
     * This tries to take into account the user preferences and it's needed, for example, to ensure that
     * the link "Home Folder" in the introduction page is displayed in the part the user choose for directories
     * rather than in WebEnginePart.
     *
     * This function is meant to be used by acceptNavigationRequest(), which unfortunately means it doesn't know
     * the URL mimetype. Because of this, only preferences related to the URL's scheme can be applied. In particular:
     * - if the URL has the `file` scheme, its mimetype is detected using QMimeTypeDatabase and the appropriate
     *  preferences are enforced
     * - if the URL has the `trash` or `remote` schemes, it is handled by the application
     * @param url the URL to open
     * @return `true` if WebEnginePart should open the URL and `false` if it should let the application open it
     */
    bool shouldOpenUrl(const QUrl &url) const;

private:
    enum WebEnginePageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    int m_kioErrorCode;
    bool m_ignoreError;

    WebSslInfo m_sslInfo;
    QPointer<WebEnginePart> m_part;

    QScopedPointer<KPasswdServerClient> m_passwdServerClient;

    /**
     * @brief Timer used to decide whether a drop operation is happening
     *
     * A drop operation is happening if this timer is active
     * @see setDropOperationStarted for more details
     */
    QTimer *m_dropOperationTimer;

    QMultiHash<QUrl, QWebEngineDownloadRequest*> m_downloadItems;

    /**
     * @brief The loading status of the page
     *
     * This is needed in changeLifecycleState() to know whether a page is being loaded,
     * since `QWebEnginePage::isLoading()` sometimes returns `false` even if the
     * page is loading
     */
    QWebEngineLoadingInfo::LoadStatus m_loadStatus;
};

#endif // WEBENGINEPAGE_H

