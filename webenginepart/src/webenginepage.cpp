/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2008 Dirk Mueller <mueller@kde.org>
    SPDX-FileCopyrightText: 2008-2010 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepage.h"

#include "webenginepart.h"
#include "webengineview.h"
#include "settings/webenginesettings.h"
#include "webenginepartdownloadmanager.h"
#include "webenginewallet.h"
#include <webenginepart_debug.h>
#include "webenginepartcontrols.h"
#include "navigationrecorder.h"
#include "profile.h"
#include "webenginepart_ext.h"

#include "libkonq_utils.h"
#include "interfaces/browser.h"

#include <QWebEngineCertificateError>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <KDialogJobUiDelegate>
#include <QWebEngineView>

#include <KMessageBox>
#include <KLocalizedString>
#include <KShell>
#include <KAuthorized>
#include <KStringHandler>
#include <KUrlAuthorized>
#include <KSharedConfig>
#include <KIO/AuthInfo>
#include <KIO/Job>
#include <KIO/CommandLauncherJob>
#include <KJobTrackerInterface>
#include <KUserTimestamp>
#include <KPasswdServerClient>
#include <KJobWidgets>
#include <KPluginMetaData>
#include <KIO/JobUiDelegateFactory>

#include <QStandardPaths>
#include <QScreen>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QMimeDatabase>

#include <QFile>
#include <QAuthenticator>
#include <QApplication>
#include <QNetworkReply>
#include <QTimer>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QUrlQuery>
#include <QWebEngineDownloadRequest>
#include <KConfigGroup>
#include <KToggleFullScreenAction>
//#include <QWebSecurityOrigin>
#include "utils.h"
#include "htmlextension.h"

using namespace KonqInterfaces;

WebEnginePage::WebEnginePage(WebEnginePart *part, QWidget *parent)
    : QWebEnginePage(KonqWebEnginePart::Profile::defaultProfile(), parent),
        m_kioErrorCode(0),
        m_ignoreError(false),
        m_part(part),
        m_passwdServerClient(new KPasswdServerClient),
        m_dropOperationTimer(new QTimer(this))
{
    if (view()) {
        WebEngineSettings::self()->computeFontSizes(view()->logicalDpiY());
    }

    //setForwardUnsupportedContent(true);

    connect(this, &QWebEnginePage::geometryChangeRequested,
            this, &WebEnginePage::slotGeometryChangeRequested);
    //    connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
    //            this, SLOT(slotUnsupportedContent(QNetworkReply*)));
    connect(this, &QWebEnginePage::featurePermissionRequested,
            this, &WebEnginePage::slotFeaturePermissionRequested);
    connect(this, &QWebEnginePage::loadFinished,
            this, &WebEnginePage::slotLoadFinished);
    connect(this, &QWebEnginePage::authenticationRequired,
            this, &WebEnginePage::slotAuthenticationRequired);
    connect(this, &QWebEnginePage::fullScreenRequested, this, &WebEnginePage::changeFullScreenMode);
    connect(this, &QWebEnginePage::recommendedStateChanged, this, &WebEnginePage::changeLifecycleState);

    connect(this, &QWebEnginePage::loadStarted, this, [this](){m_dropOperationTimer->stop();});
    m_dropOperationTimer->setSingleShot(true);

    connect(this, &QWebEnginePage::certificateError, this, &WebEnginePage::handleCertificateError);

//If this part is displaying the developer tools for another part, inform the other page it's not displaying the developer tools anymore.
//I'm not sure this is needed, but I think it's better to do it, just to be on the safe side
    auto unsetInspectedPageIfNeeded = [this](bool ok) {
        if (ok && inspectedPage() && url().scheme() != QLatin1String("devtools")) {
            setInspectedPage(nullptr);
        }
    };
    connect(this, &QWebEnginePage::loadFinished, this, unsetInspectedPageIfNeeded);

    WebEnginePartControls::self()->navigationRecorder()->registerPage(this);
    m_part->downloadManager()->addPage(this);

    setBackgroundColor(WebEngineSettings::self()->customBackgroundColor());
    connect(WebEnginePartControls::self(), &WebEnginePartControls::updateBackgroundColor, this, [this](const QColor &color){setBackgroundColor(color);});
    connect(WebEnginePartControls::self(), &WebEnginePartControls::updateStyleSheet, this, &WebEnginePage::updateUserStyleSheet);
}

WebEnginePage::~WebEnginePage()
{
//qCDebug(WEBENGINEPART_LOG) << this;
}

const WebSslInfo& WebEnginePage::sslInfo() const
{
    return m_sslInfo;
}

QWidget *WebEnginePage::view() const
{
    return QWebEngineView::forPage(this);
}

void WebEnginePage::setSslInfo (const WebSslInfo& info)
{
    m_sslInfo = info;
}

static std::optional<QString> checkForDownloadManager(QWidget* widget)
{
    KConfigGroup cfg (KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals), "HTML Settings");
    const QString fileName (cfg.readPathEntry("DownloadManager", QString()));
    if (fileName.isEmpty()) {
        return std::nullopt;
    }

    const QString exeName = QStandardPaths::findExecutable(fileName);
    if (exeName.isEmpty()) {
        KMessageBox::detailedError(widget,
                                    i18n("The download manager (%1) could not be found in your installation.", fileName),
                                    i18n("Try to reinstall it and make sure that it is available in $PATH. \n\nThe integration will be disabled."));
        cfg.writePathEntry("DownloadManager", QString());
        cfg.sync();
        return std::nullopt;
    }
    return exeName;
}

bool WebEnginePage::downloadWithExternalDonwloadManager(const QUrl &url)
{
    if (url.isLocalFile()) {
        return false;
    }

    auto useDlManager = checkForDownloadManager(view());
    if (!useDlManager) {
        return false;
    }
    QString downloadManagerExe = useDlManager.value();

    //qCDebug(WEBENGINEPART_LOG) << "Calling command" << cmd;
    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(downloadManagerExe, {url.toString()});
    job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, view()));
    job->start();
    return true;
}

void WebEnginePage::requestDownload(QWebEngineDownloadRequest *item, bool newWindow, WebEnginePartDownloadManager::DownloadObjective objective)
{
    QUrl url = item->url();
    if (downloadWithExternalDonwloadManager(url)) {
        item->cancel();
        item->deleteLater();
        return;
    }

    WebEngineDownloaderExtension *downloader = m_part->downloader();
    Q_ASSERT(downloader);

    downloader->addDownloadRequest(item);

    BrowserArguments bArgs;
    bArgs.setForcesNewWindow(newWindow);
    bArgs.setSuggestedDownloadName(item->suggestedFileName());
    if (Konq::Settings::alwaysEmbedInNewTab()) {
        bArgs.setNewTab(true);
    }
    KParts::OpenUrlArguments args;

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(item->mimeType());
    if (!mime.isValid() || mime.isDefault()) {
        mime = db.mimeTypeForFile(item->suggestedFileName(), QMimeDatabase::MatchExtension);
    }
    args.setMimeType(mime.name());

    bArgs.setIgnoreDefaultHtmlPart(true);

    if (objective == WebEnginePartDownloadManager::DownloadObjective::SaveOnly) {
        bArgs.setForcedAction(BrowserArguments::Action::Save);
        saveUrlToDiskAndDisplay(item, args, bArgs);
        return;
    } else if (objective == WebEnginePartDownloadManager::DownloadObjective::SaveAs) {
        saveAs(item);
    } else {
        bArgs.setDownloadId(item->id());
        emit downloader->downloadAndOpenUrl(url, args, bArgs, true);
        if (item->state() == QWebEngineDownloadRequest::DownloadRequested) {
            qCDebug(WEBENGINEPART_LOG()) << "Automatically accepting download for" << item->url() << "This shouldn't happen";
            item->accept();
        }
    }
}

void WebEnginePage::saveUrlToDiskAndDisplay(QWebEngineDownloadRequest* req, const KParts::OpenUrlArguments& args, const BrowserArguments& bArgs)
{
    QWidget *window = view() ? view()->window() : nullptr;

    QString suggestedName = !req->suggestedFileName().isEmpty() ? req->suggestedFileName() : req->url().fileName();
    QString downloadPath = Konq::askDownloadLocation(suggestedName, window);
    if (downloadPath.isEmpty()) {
        req->cancel();
        return;
    }

    WebEngineDownloaderExtension *downloader = m_part->downloader();
    DownloaderJob *job = downloader->downloadJob(req->url(), req->id(), this);
    if (!job) {
        return;
    }

    auto lambda = [this, args, bArgs](DownloaderJob *, const QUrl &url) {
        emit m_part->browserExtension()->browserOpenUrlRequest(url, args, bArgs);
    };
    connect(job, &DownloaderJob::downloadResult, m_part, &WebEnginePart::displayActOnDownloadedFileBar);
    job->startDownload(downloadPath, window, this, lambda);
}

void WebEnginePage::saveAs(QWebEngineDownloadRequest* req)
{
    QWidget *window = view() ? view()->window() : nullptr;

    QString suggestedName = !req->suggestedFileName().isEmpty() ? req->suggestedFileName() : req->url().fileName();
    QString downloadPath = Konq::askDownloadLocation(suggestedName, window);
    if (downloadPath.isEmpty()) {
        req->cancel();
        return;
    }

    WebEngineDownloaderExtension *downloader = m_part->downloader();
    DownloaderJob *job = downloader->downloadJob(req->url(), req->id(), this);
    if (!job) {
        return;
    }

    auto lambda = [this](DownloaderJob *dj, const QUrl &url) {
        if (dj->error() == 0) {
            m_part->openUrl(url);
            return;
        }
        BrowserArguments bArgs;
        bArgs.setForcesNewWindow(true);
        emit m_part->browserExtension()->browserOpenUrlRequest(url, {}, bArgs);
    };
    job->startDownload(downloadPath, window, this, lambda);
}

void WebEnginePage::setDropOperationStarted()
{
    m_dropOperationTimer->start(100);
}

QWebEnginePage *WebEnginePage::createWindow(WebWindowType type)
{
    if (m_dropOperationTimer->isActive()) {
        m_dropOperationTimer->stop();
        return this;
    }

    //qCDebug(WEBENGINEPART_LOG) << "window type:" << type;
    // Crete an instance of NewWindowPage class to capture all the
    // information we need to create a new window. See documentation of
    // the class for more information...
    NewWindowPage* page = new NewWindowPage(type, part());
    return page;
}

bool WebEnginePage::askBrowserToOpenUrl(const QUrl& url, const QString& mimetype, const KParts::OpenUrlArguments &_args, const BrowserArguments &bargs)
{
    KParts::OpenUrlArguments args(_args);
    args.setMimeType(mimetype);
    args.metaData().insert("DontSendToDefaultHTMLPart", "");
    emit m_part->browserExtension()->browserOpenUrlRequest(url, args, bargs);
    return false;
}

bool WebEnginePage::shouldOpenUrl(const QUrl& url) const
{
    static const QStringList s_forcedSchemes{QStringLiteral("remote"), QStringLiteral("trash")};
    if (s_forcedSchemes.contains(url.scheme())) {
        return false;
    }
    if (!url.isLocalFile()) {
        return true;
    }
    BrowserInterface *bi = m_part->browserExtension()->browserInterface();
    bool useThisPart = false;
    //We don't check whether bi is valid, as invokeMethod will fail if it's nullptr
    //If invokeMethod fails, useThisPart will keep its default value (false) which is what we need to return, so there's no
    //need to check the return value of invokeMethod
    QMetaObject::invokeMethod(bi, "isCorrectPartForLocalFile", Q_RETURN_ARG(bool, useThisPart), Q_ARG(KParts::ReadOnlyPart*, part()), Q_ARG(QString, url.path()));
    return useThisPart;
}

bool WebEnginePage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    //Ask the browser for permission to navigate away. In Konqueror, if a view is locked, it can't navigate to somewhere else
    if (isMainFrame) {
        KonqInterfaces::Browser *browser = KonqInterfaces::Browser::browser(qApp);
        if (browser && !browser->canNavigateTo(part(), url)) {
            return false;
        }
    }

    if (isMainFrame) {
        if (!shouldOpenUrl(url)) {
            return askBrowserToOpenUrl(url);
        }
    }

    QUrl reqUrl(url);

    // Handle "mailto:" url here...
    if (handleMailToUrl(reqUrl, type))
        return false;

    const bool isTypedUrl = property("NavigationTypeUrlEntered").toBool();

    /*
      NOTE: We use a dynamic QObject property called "NavigationTypeUrlEntered"
      to distinguish between requests generated by user entering a url vs those
      that were generated programmatically through javascript (AJAX requests).
    */
    if (isMainFrame && isTypedUrl)
      setProperty("NavigationTypeUrlEntered", QVariant());

    // inPage requests are those generarted within the current page through
    // link clicks, javascript queries, and button clicks (form submission).
    bool inPageRequest = true;
    switch (type) {
        case QWebEnginePage::NavigationTypeFormSubmitted:
            if (!checkFormData(url))
               return false;
            if (part() && part()->wallet()) {
                part()->wallet()->saveFormsInPage(this);
            }

            break;
#if 0
        case QWebEnginePage::NavigationTypeFormResubmitted:
            if (!checkFormData(request))
                return false;
            if (KMessageBox::warningContinueCancel(view(),
                            i18n("<qt><p>To display the requested web page again, "
                                  "the browser needs to resend information you have "
                                  "previously submitted.</p>"
                                  "<p>If you were shopping online and made a purchase, "
                                  "click the Cancel button to prevent a duplicate purchase."
                                  "Otherwise, click the Continue button to display the web"
                                  "page again.</p>"),
                            i18n("Resubmit Information")) == KMessageBox::Cancel) {
                return false;
            }
            break;
#endif
        case QWebEnginePage::NavigationTypeBackForward:
            // If history navigation is locked, ignore all such requests...
            if (property("HistoryNavigationLocked").toBool()) {
                setProperty("HistoryNavigationLocked", QVariant());
                qCDebug(WEBENGINEPART_LOG) << "Rejected history navigation because 'HistoryNavigationLocked' property is set!";
                return false;
            }
            //qCDebug(WEBENGINEPART_LOG) << "Navigating to item (" << history()->currentItemIndex()
            //         << "of" << history()->count() << "):" << history()->currentItem().url();
            inPageRequest = false;
            break;
        case QWebEnginePage::NavigationTypeReload:
//            setRequestMetaData(QL1S("cache"), QL1S("reload"));
            inPageRequest = false;
            break;
        case QWebEnginePage::NavigationTypeOther: // triggered by javascript
            qCDebug(WEBENGINEPART_LOG) << "Triggered by javascript";
            inPageRequest = !isTypedUrl;
            break;
        default:
            break;
    }

    if (inPageRequest) {
        // if (!checkLinkSecurity(request, type))
        //      return false;

        //  if (m_sslInfo.isValid())
        //      setRequestMetaData(QL1S("ssl_was_in_use"), QL1S("TRUE"));
    }


    // Honor the enabling/disabling of plugins per host.
    settings()->setAttribute(QWebEngineSettings::PluginsEnabled, WebEngineSettings::self()->isPluginsEnabled(reqUrl.host()));

    if (isMainFrame) {
        //Setting the javascript policy after the page has been loaded can be too late
        //(see bug #490321), so we also do it here
        setPageJScriptPolicy(url);
        emit mainFrameNavigationRequested(this, url);
    }
    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

#if 0
static int errorCodeFromReply(QNetworkReply* reply)
{
    // First check if there is a KIO error code sent back and use that,
    // if not attempt to convert QNetworkReply's NetworkError to KIO::Error.
    QVariant attr = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::KioError));
    if (attr.isValid() && attr.type() == QVariant::Int)
        return attr.toInt();

    switch (reply->error()) {
        case QNetworkReply::ConnectionRefusedError:
            return KIO::ERR_CANNOT_CONNECT;
        case QNetworkReply::HostNotFoundError:
            return KIO::ERR_UNKNOWN_HOST;
        case QNetworkReply::TimeoutError:
            return KIO::ERR_SERVER_TIMEOUT;
        case QNetworkReply::OperationCanceledError:
            return KIO::ERR_USER_CANCELED;
        case QNetworkReply::ProxyNotFoundError:
            return KIO::ERR_UNKNOWN_PROXY_HOST;
        case QNetworkReply::ContentAccessDenied:
            return KIO::ERR_ACCESS_DENIED;
        case QNetworkReply::ContentOperationNotPermittedError:
            return KIO::ERR_WRITE_ACCESS_DENIED;
        case QNetworkReply::ContentNotFoundError:
            return KIO::ERR_NO_CONTENT;
        case QNetworkReply::AuthenticationRequiredError:
            return KIO::ERR_CANNOT_AUTHENTICATE;
        case QNetworkReply::ProtocolUnknownError:
            return KIO::ERR_UNSUPPORTED_PROTOCOL;
        case QNetworkReply::ProtocolInvalidOperationError:
            return KIO::ERR_UNSUPPORTED_ACTION;
        case QNetworkReply::UnknownNetworkError:
            return KIO::ERR_UNKNOWN;
        case QNetworkReply::NoError:
        default:
            break;
    }

    return 0;
}
#endif

void WebEnginePage::handleCertificateError(const QWebEngineCertificateError &ce) {
    WebEnginePartControls::self()->handleCertificateError(ce, this);
}

WebEnginePart* WebEnginePage::part() const
{
    return m_part.data();
}

void WebEnginePage::setPart(WebEnginePart* part)
{
    m_part = part;
}

void WebEnginePage::slotLoadFinished(bool ok)
{
    QUrl requestUrl = url();
    requestUrl.setUserInfo(QString());
#if 0
    const bool shouldResetSslInfo = (m_sslInfo.isValid() && !domainSchemeMatch(requestUrl, m_sslInfo.url()));
    QWebFrame* frame = qobject_cast<QWebFrame *>(reply->request().originatingObject());
    if (!frame)
        return;
    const bool isMainFrameRequest = (frame == mainFrame());
#else
    // PORTING_TODO
    const bool isMainFrameRequest = true;
#endif

#if 0
    // Only deal with non-redirect responses...
    const QVariant redirectVar = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    if (isMainFrameRequest && redirectVar.isValid()) {
        m_sslInfo.restoreFrom(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)),
                              reply->url(), shouldResetSslInfo);
        return;
    }

    const int errCode = errorCodeFromReply(reply);
    qCDebug(WEBENGINEPART_LOG) << frame << "is main frame request?" << isMainFrameRequest << requestUrl;
#endif

    if (ok) {
        if (isMainFrameRequest) {
#if 0
            m_sslInfo.restoreFrom(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)),
                    reply->url(), shouldResetSslInfo);
#endif
            setPageJScriptPolicy(url());
        }
    } else {
    // Handle any error...
#if 0
    switch (errCode) {
        case 0:
        case KIO::ERR_NO_CONTENT:
            break;
        case KIO::ERR_ABORTED:
        case KIO::ERR_USER_CANCELED: // Do nothing if request is cancelled/aborted
            //qCDebug(WEBENGINEPART_LOG) << "User aborted request!";
            m_ignoreError = true;
            emit loadAborted(QUrl());
            return;
        // Handle the user clicking on a link that refers to a directory
        // Since KIO cannot automatically convert a GET request to a LISTDIR one.
        case KIO::ERR_IS_DIRECTORY:
            m_ignoreError = true;
            emit loadAborted(reply->url());
            return;
        default:
            // Make sure the saveFrameStateRequested signal is emitted so
            // the page can restored properly.
            if (isMainFrameRequest)
                emit saveFrameStateRequested(frame, 0);

            m_ignoreError = (reply->attribute(QNetworkRequest::User).toInt() == QNetworkReply::ContentAccessDenied);
            m_kioErrorCode = errCode;
            break;
#endif
    }

    if (isMainFrameRequest) {
        const WebEnginePageSecurity security = (m_sslInfo.isValid() ? PageEncrypted : PageUnencrypted);
        emit m_part->navigationExtension()->setPageSecurity(security);
    }
}

void WebEnginePage::slotFeaturePermissionRequested(const QUrl& url, QWebEnginePage::Feature feature)
{
    //url.path() is always / (meaning that permissions should be granted site-wide and not per page)
    QUrl thisUrl(this->url());
    thisUrl.setPath("/");
    thisUrl.setQuery(QString());
    thisUrl.setFragment(QString());
    if (url == thisUrl) {
        part()->slotShowFeaturePermissionBar(url, feature);
        return;
    }
    switch(feature) {
    case QWebEnginePage::Notifications:
        // FIXME: We should have a setting to tell if this is enabled, but so far it is always enabled.
        setFeaturePermission(url, feature, QWebEnginePage::PermissionGrantedByUser);
        break;
    case QWebEnginePage::Geolocation:
        if (KMessageBox::warningContinueCancel(nullptr, i18n("This site is attempting to "
                                                       "access information about your "
                                                       "physical location.\n"
                                                       "Do you want to allow it access?"),
                                            i18n("Network Transmission"),
                                            KGuiItem(i18n("Allow access")),
                                            KStandardGuiItem::cancel(),
                                            QStringLiteral("WarnGeolocation")) == KMessageBox::Cancel) {
            setFeaturePermission(url, feature, QWebEnginePage::PermissionDeniedByUser);
        } else {
            setFeaturePermission(url, feature, QWebEnginePage::PermissionGrantedByUser);
        }
        break;
    default:
        setFeaturePermission(url, feature, QWebEnginePage::PermissionUnknown);
        break;
    }
}

void WebEnginePage::slotGeometryChangeRequested(const QRect & rect)
{
    const QString host = url().host();

    // NOTE: If a new window was created from another window which is in
    // maximized mode and its width and/or height were not specified at the
    // time of its creation, which is always the case in QWebEnginePage::createWindow,
    // then any move operation will seem not to work. That is because the new
    // window will be in maximized mode where moving it will not be possible...
    if (WebEngineSettings::self()->windowMovePolicy(host) == HtmlSettingsInterface::JSWindowMoveAllow &&
        (view()->x() != rect.x() || view()->y() != rect.y()))
        emit m_part->navigationExtension()->moveTopLevelWidget(rect.x(), rect.y());

    const int height = rect.height();
    const int width = rect.width();

    // parts of following code are based on kjs_window.cpp
    // Security check: within desktop limits and bigger than 100x100 (per spec)
    if (width < 100 || height < 100) {
        qCWarning(WEBENGINEPART_LOG) << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = view()->screen()->virtualGeometry();

    if (width > sg.width() || height > sg.height()) {
        qCWarning(WEBENGINEPART_LOG) << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebEngineSettings::self()->windowResizePolicy(host) == HtmlSettingsInterface::JSWindowResizeAllow) {
        //qCDebug(WEBENGINEPART_LOG) << "resizing to " << width << "x" << height;
        emit m_part->navigationExtension()->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    const int right = view()->x() + view()->frameGeometry().width();
    const int bottom = view()->y() + view()->frameGeometry().height();
    int moveByX = 0, moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0

    if ((moveByX || moveByY) && WebEngineSettings::self()->windowMovePolicy(host) == HtmlSettingsInterface::JSWindowMoveAllow)
        emit m_part->navigationExtension()->moveTopLevelWidget(view()->x() + moveByX, view()->y() + moveByY);
}

bool WebEnginePage::checkFormData(const QUrl &url) const
{
    const QString scheme (url.scheme());

    if (m_sslInfo.isValid() &&
        !scheme.compare(QL1S("https")) && !scheme.compare(QL1S("mailto")) &&
        (KMessageBox::warningContinueCancel(nullptr,
                                           i18n("Warning: This is a secure form "
                                                "but it is attempting to send "
                                                "your data back unencrypted.\n"
                                                "A third party may be able to "
                                                "intercept and view this "
                                                "information.\nAre you sure you "
                                                "want to send the data unencrypted?"),
                                           i18n("Network Transmission"),
                                           KGuiItem(i18n("&Send Unencrypted")))  == KMessageBox::Cancel)) {

        return false;
    }


    if (scheme.compare(QL1S("mailto")) == 0 &&
        (KMessageBox::warningContinueCancel(nullptr, i18n("This site is attempting to "
                                                    "submit form data via email.\n"
                                                    "Do you want to continue?"),
                                            i18n("Network Transmission"),
                                            KGuiItem(i18n("&Send Email")),
                                            KStandardGuiItem::cancel(),
                                            QStringLiteral("WarnTriedEmailSubmit")) == KMessageBox::Cancel)) {
        return false;
    }

    return true;
}

// Sanitizes the "mailto:" url, e.g. strips out any "attach" parameters.
static QUrl sanitizeMailToUrl(const QUrl &url, QStringList& files) {
    QUrl sanitizedUrl;

    // NOTE: This is necessary to ensure we can properly use QUrl's query
    // related APIs to process 'mailto:' urls of form 'mailto:foo@bar.com'.
    if (url.hasQuery())
      sanitizedUrl = url;
    else
      sanitizedUrl = QUrl(url.scheme() + QL1S(":?") + url.path());

    QUrlQuery query(sanitizedUrl);
    const QList<QPair<QString, QString> > items (query.queryItems());

    QUrlQuery sanitizedQuery;
    for(auto queryItem : items) {
        if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty()) {
            // ### DF: this hack breaks mailto:faure@kde.org, kmail doesn't expect mailto:?to=faure@kde.org
            queryItem.second = queryItem.first;
            queryItem.first = QStringLiteral("to");
        } else if (QString::compare(queryItem.first, QL1S("attach"), Qt::CaseInsensitive) == 0) {
            files << queryItem.second;
            continue;
        }
        sanitizedQuery.addQueryItem(queryItem.first, queryItem.second);
    }

    sanitizedUrl.setQuery(sanitizedQuery);
    return sanitizedUrl;
}

bool WebEnginePage::handleMailToUrl (const QUrl &url, NavigationType type) const
{
    if (url.scheme() == QL1S("mailto")) {
        QStringList files;
        QUrl mailtoUrl (sanitizeMailToUrl(url, files));

        switch (type) {
            case QWebEnginePage::NavigationTypeLinkClicked:
                if (!files.isEmpty() && KMessageBox::warningContinueCancelList(nullptr,
                                                                               i18n("<qt>Do you want to allow this site to attach "
                                                                                    "the following files to the email message?</qt>"),
                                                                               files, i18n("Email Attachment Confirmation"),
                                                                               KGuiItem(i18n("&Allow attachments")),
                                                                               KGuiItem(i18n("&Ignore attachments")), QL1S("WarnEmailAttachment")) == KMessageBox::Continue) {

                   // Re-add the attachments...
                    QStringListIterator filesIt (files);
                    QUrlQuery query(mailtoUrl);
                    while (filesIt.hasNext()) {
                        query.addQueryItem(QL1S("attach"), filesIt.next());
                    }
                    mailtoUrl.setQuery(query);
                }
                break;
            case QWebEnginePage::NavigationTypeFormSubmitted:
            //case QWebEnginePage::NavigationTypeFormResubmitted:
                if (!files.isEmpty()) {
                    KMessageBox::information(nullptr, i18n("This site attempted to attach a file from your "
                                                     "computer in the form submission. The attachment "
                                                     "was removed for your protection."),
                                             i18n("Attachment Removed"), QStringLiteral("InfoTriedAttach"));
                }
                break;
            default:
                 break;
        }

        //qCDebug(WEBENGINEPART_LOG) << "Emitting openUrlRequest with " << mailtoUrl;
        emit m_part->navigationExtension()->openUrlRequest(mailtoUrl);
        return true;
    }

    return false;
}

void WebEnginePage::setPageJScriptPolicy(const QUrl &url)
{
    const QString hostname (url.host());
    settings()->setAttribute(QWebEngineSettings::JavascriptEnabled,
                             WebEngineSettings::self()->isJavaScriptEnabled(hostname));

    const HtmlSettingsInterface::JSWindowOpenPolicy policy = WebEngineSettings::self()->windowOpenPolicy(hostname);
    settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows,
                             (policy != HtmlSettingsInterface::JSWindowOpenDeny &&
                              policy != HtmlSettingsInterface::JSWindowOpenSmart));
}

void WebEnginePage::slotAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth)
{
    KIO::AuthInfo info;
    info.url = requestUrl;
    info.username = auth->user();
    info.realmValue = auth->realm();
    // If no realm metadata, then make sure path matching is turned on.
    info.verifyPath = info.realmValue.isEmpty();

    const QString errorMsg = QString();
    const int ret = m_passwdServerClient->queryAuthInfo(&info, errorMsg, view()->window()->winId(), KUserTimestamp::userTimestamp());
    if (ret == KJob::NoError) {
        auth->setUser(info.username);
        auth->setPassword(info.password);
    } else {
        // Set authenticator null if dialog is cancelled
        // or if we couldn't communicate with kpasswdserver
        *auth = QAuthenticator();
    }
}

void WebEnginePage::changeFullScreenMode(QWebEngineFullScreenRequest req)
{
        BrowserInterface *iface = part()->browserExtension()->browserInterface();
        if (iface) {
            req.accept();
            iface->callMethod("toggleCompleteFullScreen", req.toggleOn());
        } else {
            req.reject();
        }
}


void WebEnginePage::setStatusBarText(const QString& text)
{
    if (m_part) {
        emit m_part->setStatusBarText(text);
    }
}

void WebEnginePage::changeLifecycleState(QWebEnginePage::LifecycleState recommendedState)
{
    if (recommendedState != QWebEnginePage::LifecycleState::Active && !isVisible()) {
        setLifecycleState(QWebEnginePage::LifecycleState::Frozen);
    } else {
        setLifecycleState(QWebEnginePage::LifecycleState::Active);
    }
}

void WebEnginePage::updateUserStyleSheet(const QString& script)
{
    runJavaScript(script, QWebEngineScript::ApplicationWorld);
}

/************************************* Begin NewWindowPage ******************************************/

NewWindowPage::NewWindowPage(WebWindowType type, WebEnginePart* part, QWidget* parent)
              :WebEnginePage(part, parent) , m_type(type) , m_createNewWindow(true)
{
    Q_ASSERT_X (part, "NewWindowPage", "Must specify a valid KPart");

    // FIXME: are these 3 signals actually defined or used?
    connect(this, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            this, SLOT(slotMenuBarVisibilityChangeRequested(bool)));
    connect(this, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            this, SLOT(slotToolBarVisibilityChangeRequested(bool)));
    connect(this, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            this, SLOT(slotStatusBarVisibilityChangeRequested(bool)));
    connect(this, &QWebEnginePage::loadFinished, this, &NewWindowPage::slotLoadFinished);
    if (m_type == WebBrowserBackgroundTab) {
        m_windowArgs.setLowerWindow(true);
    }
}

NewWindowPage::~NewWindowPage()
{
}

bool NewWindowPage::decideHandlingOfJavascripWindow(const QUrl url) const
{
    const HtmlSettingsInterface::JSWindowOpenPolicy policy = WebEngineSettings::self()->windowOpenPolicy(url.host());
    switch (policy) {
    case HtmlSettingsInterface::JSWindowOpenDeny:
        // TODO: Implement support for dealing with blocked pop up windows.
        return false;
    case HtmlSettingsInterface::JSWindowOpenAsk: {
        const QString message = (url.isEmpty() ?
                                    i18n("This site is requesting to open a new popup window.\n"
                                        "Do you want to allow this?") :
                                    i18n("<qt>This site is requesting to open a popup window to"
                                        "<p>%1</p><br/>Do you want to allow this?</qt>",
                                        KStringHandler::rsqueeze(url.toDisplayString().toHtmlEscaped(), 100)));
        return KMessageBox::questionTwoActions(view(), message, i18n("Javascript Popup Confirmation"),
                                        KGuiItem(i18n("Allow")), KGuiItem(i18n("Do Not Allow"))) == KMessageBox::PrimaryAction;
        // TODO: Implement support for dealing with blocked pop up windows.
    }
    default:
        break;
    }
    return true;
}

bool NewWindowPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    //qCDebug(WEBENGINEPART_LOG) << "url:" << url << ", type:" << type << ", isMainFrame:" << isMainFrame << "m_createNewWindow=" << m_createNewWindow;
    if (!m_createNewWindow) {
        return WebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

    const QUrl reqUrl (url);
    const bool actionRequestedByUser = type != QWebEnginePage::NavigationTypeOther;
    const bool actionRequestsNewTab = m_type == QWebEnginePage::WebBrowserBackgroundTab || m_type == QWebEnginePage::WebBrowserTab;

    if (actionRequestedByUser && !actionRequestsNewTab) {
        if (!part() && !isMainFrame) {
            return false;
        }
        if (!decideHandlingOfJavascripWindow(reqUrl)) {
            deleteLater();
            return false;
        }
    }

    // Browser args...
    BrowserArguments bargs;
    //Don't set forcesNewWindow for if m_type is WebDialog because it include popups, which the user may want to open in a new tab
    bargs.setForcesNewWindow(m_type == WebBrowserWindow);

    // OpenUrl args...
    KParts::OpenUrlArguments uargs;
    uargs.setMimeType(QL1S("text/html"));
    uargs.setActionRequestedByUser(actionRequestedByUser);

    // Window args...
    WindowArgs wargs (m_windowArgs);

    KParts::ReadOnlyPart* newWindowPart = nullptr;
    emit part()->browserExtension()->browserCreateNewWindow(url, uargs, bargs, wargs, &newWindowPart);
    qCDebug(WEBENGINEPART_LOG) << "Created new window" << newWindowPart;

    deleteLater();
    return false;
}

void NewWindowPage::slotGeometryChangeRequested(const QRect & rect)
{
    if (!rect.isValid())
        return;

    if (!m_createNewWindow) {
        WebEnginePage::slotGeometryChangeRequested(rect);
        return;
    }

    m_windowArgs.setX(rect.x());
    m_windowArgs.setY(rect.y());
    m_windowArgs.setWidth(qMax(rect.width(), 100));
    m_windowArgs.setHeight(qMax(rect.height(), 100));
}

void NewWindowPage::slotMenuBarVisibilityChangeRequested(bool visible)
{
    //qCDebug(WEBENGINEPART_LOG) << visible;
    m_windowArgs.setMenuBarVisible(visible);
}

void NewWindowPage::slotStatusBarVisibilityChangeRequested(bool visible)
{
    //qCDebug(WEBENGINEPART_LOG) << visible;
    m_windowArgs.setStatusBarVisible(visible);
}

void NewWindowPage::slotToolBarVisibilityChangeRequested(bool visible)
{
    //qCDebug(WEBENGINEPART_LOG) << visible;
    m_windowArgs.setToolBarsVisible(visible);
}

// When is this called? (and acceptNavigationRequest is not called?)
// The only case I found is Ctrl+click on link to data URL (like in konqviewmgrtest), that's quite specific...
// Everything else seems to work with this method being commented out...
void NewWindowPage::slotLoadFinished(bool ok)
{
    Q_UNUSED(ok)
    if (!m_createNewWindow)
        return;

    const bool actionRequestedByUser = true; // ### we don't have the information here, unlike in acceptNavigationRequest

    // Browser args...
    BrowserArguments bargs;
    //Don't set forcesNewWindow for if m_type is WebDialog because it include popups, which the user may want to open in a new tab
    bargs.setForcesNewWindow(m_type == WebBrowserWindow);

    // OpenUrl args...
    KParts::OpenUrlArguments uargs;
    uargs.setMimeType(QL1S("text/html"));
    uargs.setActionRequestedByUser(actionRequestedByUser);

    // Window args...
    WindowArgs wargs (m_windowArgs);

    KParts::ReadOnlyPart* newWindowPart =nullptr;

    emit part()->browserExtension()->browserCreateNewWindow(QUrl(), uargs, bargs, wargs, &newWindowPart);

    qCDebug(WEBENGINEPART_LOG) << "Created new window or tab" << newWindowPart;

    // Get the webview...
    WebEnginePart* webenginePart = newWindowPart ? qobject_cast<WebEnginePart*>(newWindowPart) : nullptr;
    WebEngineView* webView = webenginePart ? qobject_cast<WebEngineView*>(webenginePart->view()) : nullptr;

    if (webView) {
        // if a new window is created, set a new window meta-data flag.
        if (newWindowPart->widget()->topLevelWidget() != part()->widget()->topLevelWidget()) {
            KParts::OpenUrlArguments args;
            args.metaData().insert(QL1S("new-window"), QL1S("true"));
            newWindowPart->setArguments(args);
        }
        // Reparent this page to the new webview to prevent memory leaks.
        setParent(webView);
        // Replace the webpage of the new webview with this one. Nice trick...
        webView->setPage(this);
        // Set the new part as the one this page will use going forward.
        setPart(webenginePart);
        // Connect all the signals from this page to the slots in the new part.
        webenginePart->connectWebEnginePageSignals(this);
    }

    //Set the create new window flag to false...
    m_createNewWindow = false;
}

/****************************** End NewWindowPage *************************************************/

