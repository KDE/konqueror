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

#include "webpage.h"

#include "webkitpart.h"
#include "websslinfo.h"
#include "webview.h"
#include "sslinfodialog_p.h"
#include "settings/webkitsettings.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/BrowserRun>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KGlobalSettings>
#include <KDE/KJobUiDelegate>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KStandardShortcut>
#include <KDE/KAuthorized>
#include <KIO/Job>
#include <KIO/AccessManager>

#include <QtCore/QListIterator>
#include <QtGui/QTextDocument>
#include <QtNetwork/QNetworkReply>
#include <QtUiTools/QUiLoader>
#include <QtWebKit/QWebFrame>


#define QL1(x)  QLatin1String(x)

typedef QPair<QString, QString> StringPair;

// Sanitizes the "mailto:" url, e.g. strips out any "attach" parameters.
static QUrl sanitizeMailToUrl(const QUrl &url, QStringList& files) {
    QUrl sanitizedUrl;    
    
    // NOTE: This is necessary to ensure we can properly use QUrl's query
    // related APIs to process 'mailto:' urls of form 'mailto:foo@bar.com'.
    if (url.hasQuery())
      sanitizedUrl = url;
    else
      sanitizedUrl = QUrl(url.scheme() + QL1(":?") + url.path());

    QListIterator<StringPair> it (sanitizedUrl.queryItems());
    sanitizedUrl.setEncodedQuery(QByteArray());    // clear out the query componenet

    while (it.hasNext()) {
        StringPair queryItem = it.next();
        if (queryItem.first.contains(QChar('@')) && queryItem.second.isEmpty()) {
            queryItem.second = queryItem.first;
            queryItem.first = "to";
        } else if (QString::compare(queryItem.first, QL1("attach"), Qt::CaseInsensitive) == 0) {
            files << queryItem.second;
            continue;
        }        
        sanitizedUrl.addQueryItem(queryItem.first, queryItem.second);
    }

    return sanitizedUrl;
}

// Converts QNetworkReply::NetworkError codes to the KIO equivalent ones...
static int convertErrorCode(QNetworkReply* reply)
{
    // First check if there is a KIO error code sent back and use that,
    // if not attempt to convert the QNetworkReply::
    QVariant attr = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::KioError));
    if (attr.isValid() && attr.type() == QVariant::Int)
        return attr.toInt();

    switch (reply->error()) {
        case QNetworkReply::ConnectionRefusedError:
            return KIO::ERR_COULD_NOT_CONNECT;
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
            return KIO::ERR_COULD_NOT_AUTHENTICATE;
        case QNetworkReply::ProtocolUnknownError:
            return KIO::ERR_UNSUPPORTED_PROTOCOL;
        case QNetworkReply::ProtocolInvalidOperationError:
            return KIO::ERR_UNSUPPORTED_ACTION;
        case QNetworkReply::UnknownNetworkError:
            return KIO::ERR_UNKNOWN;
        case QNetworkReply::NoError:
        default:
            return 0;
    }
}

// Returns true if the scheme and domain of the two urls match...
static bool domainSchemeMatch(const QUrl& u1, const QUrl& u2)
{
    if (u1.scheme() != u2.scheme())
        return false;

    QStringList u1List = u1.host().split(QChar('.'), QString::SkipEmptyParts);
    QStringList u2List = u2.host().split(QChar('.'), QString::SkipEmptyParts);

    if (qMin(u1List.count(), u2List.count()) < 2)
        return false;  // better safe than sorry...

    while (u1List.count() > 2)
        u1List.removeFirst();

    while (u2List.count() > 2)
        u2List.removeFirst();

    return (u1List == u2List);
}

static QString getFileNameForDownload(const QNetworkRequest &request, QNetworkReply *reply)
{
    QString fileName = KUrl(request.url()).fileName();

    if (reply && reply->hasRawHeader("Content-Disposition")) { // based on code from arora, downloadmanger.cpp
        const QString value = QL1(reply->rawHeader("Content-Disposition"));
        const int pos = value.indexOf(QL1("filename="));
        if (pos != -1) {
            QString name = value.mid(pos + 9);
            if (name.startsWith(QLatin1Char('"')) && name.endsWith(QLatin1Char('"')))
                name = name.mid(1, name.size() - 2);
            fileName = name;
        }
    }

    return fileName;
}

class WebPage::WebPagePrivate
{
public:
    WebPagePrivate() {}

    WebSslInfo sslInfo;
    QMap<QString, WebFrameState> frameStateContainer;
    // Holds list of requests including those from children frames
    QMap<QUrl, QWebFrame*> requestQueue;
    QPointer<WebKitPart> part;
};

WebPage::WebPage(WebKitPart *part, QWidget *parent)
        :KWebPage(parent), d (new WebPagePrivate)
{
    d->part = part;
    setSessionMetaData("ssl_activate_warnings", "TRUE");

    // Set font sizes accordingly...
    if (view())
        WebKitSettings::self()->computeFontSizes(view()->logicalDpiY());

    setForwardUnsupportedContent(true);

    connect(this, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(slotGeometryChangeRequested(const QRect &)));
    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
    connect(this, SIGNAL(statusBarMessage(const QString &)),
            this, SLOT(slotStatusBarMessage(const QString &)));
    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(slotDownloadRequest(const QNetworkRequest &)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotUnsupportedContent(QNetworkReply *)));
    connect(networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
            this, SLOT(slotRequestFinished(QNetworkReply *)));
}

WebPage::~WebPage()
{
    delete d;
}

bool WebPage::authorizedRequest(const QUrl &url) const
{
    // Check for ad filtering...
    return !(WebKitSettings::self()->isAdFilterEnabled() && WebKitSettings::self()->isAdFiltered(url.toString()));
}

const WebSslInfo& WebPage::sslInfo() const
{
    return d->sslInfo;
}

void WebPage::setSslInfo (const WebSslInfo& info)
{
    d->sslInfo = info;
}

WebFrameState WebPage::frameState(const QString& frameName) const
{
    return d->frameStateContainer.value(frameName);
}

void WebPage::saveFrameState (const QString &frameName, const WebFrameState &frameState)
{
    d->frameStateContainer[frameName] = frameState;
}

void WebPage::restoreFrameState(const QString &frameName)
{
    if (d->frameStateContainer.contains(frameName)) {
        WebFrameState frameState = d->frameStateContainer.take(frameName);
        if (mainFrame()->frameName() == frameName) {
            mainFrame()->setScrollPosition(QPoint(frameState.scrollPosX, frameState.scrollPosY));
            kDebug() << "Restoring main frame:" << frameName << frameState;
        } else {
            QListIterator<QWebFrame *> frameIt (mainFrame()->childFrames());
            while (frameIt.hasNext()) {
                QWebFrame *frame = frameIt.next();
                if (frame->frameName() == frameName) {
                    frame->setScrollPosition(QPoint(frameState.scrollPosX, frameState.scrollPosY));
                    kDebug() << "Restoring child frame:" << frameName << frameState;
                }
            }
        }
    }
}

void WebPage::restoreAllFrameState()
{
    QList<QWebFrame *> frames = mainFrame()->childFrames();
    frames.prepend(mainFrame());

    QListIterator<QWebFrame *> frameIt (frames);
    while (frameIt.hasNext()) {
        QWebFrame* frame = frameIt.next();
        if (d->frameStateContainer.contains(frame->frameName())) {
            WebFrameState frameState = d->frameStateContainer.take(frame->frameName());
            frame->setScrollPosition(QPoint(frameState.scrollPosX, frameState.scrollPosY));
        }
    }
}

void WebPage::saveUrl(const KUrl &url)
{
    slotDownloadRequest(QNetworkRequest(url));
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    // Handle "mailto:" url here...
    if (handleMailToUrl(request.url(), type))
      return false;

    if (frame) {
        /*
          NOTE: QtWebKit sends NavigationTypeOther instead of NavigationTypeLink
          or when users click on javascript links such as

          <a href="javascript:location.href='http://qt.nokia.com'">javascript link</a>
        */

        // inPage requests are those generarted within the current page
        // through link clicks, javascript queries, and button clicks
        // (form submission).
        bool inPageRequest = true;

        switch (type) {
            case QWebPage::NavigationTypeFormResubmitted:
            case QWebPage::NavigationTypeFormSubmitted:
                if (!checkFormData(request))
                    return false;
                break;
            case QWebPage::NavigationTypeReload:
            case QWebPage::NavigationTypeBackOrForward:
                inPageRequest = false;
            case QWebPage::NavigationTypeOther:
                // Set manual request user typed request...
                inPageRequest = (d->part->url() != request.url());
                if (frame != mainFrame() && d->frameStateContainer.contains(frame->frameName())) {
                    WebFrameState frameState = d->frameStateContainer.value(frame->frameName());
                    if (frameState.url != request.url()) {
                        frame->setUrl(frameState.url);
                        kDebug() << "Changing request for" << frame->frameName() << "to" << frameState.url;
                        return false;
                    }
                }
                break;
            default:
                break;
        }

        if (inPageRequest) {
            if (!checkLinkSecurity(request, type))
                return false;

            if (d->sslInfo.isValid())
                setRequestMetaData("ssl_was_in_use", "TRUE");
        }

        if (frame == mainFrame()) {
            setRequestMetaData("main_frame_request", "TRUE");
        } else {
            setRequestMetaData("main_frame_request", "FALSE");
        }

        // Insert the request into the queue...
        d->requestQueue.insert (request.url(), frame);
    }

    return KWebPage::acceptNavigationRequest(frame, request, type);
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
    kDebug() << type;
    KParts::ReadOnlyPart *part = 0;
    KParts::OpenUrlArguments args;
    args.metaData()["referrer"] = mainFrame()->url().toString();

    KParts::BrowserArguments bargs;
    bargs.setLockHistory(true);
    if (type == WebModalDialog)
        bargs.setForcesNewWindow(true);

    d->part->browserExtension()->createNewWindow(KUrl("about:blank"), args, bargs,
                                                 KParts::WindowArgs(), &part);

    WebKitPart *webKitPart = qobject_cast<WebKitPart*>(part);

    if (!webKitPart) {
        if (part)
            kDebug() << "Got a non WebKitPart" << part->metaObject()->className();
        else
            kDebug() << "part is null";

        return 0;
    }

    return webKitPart->view()->page();
}

void WebPage::slotUnsupportedContent(QNetworkReply *reply)
{
    KParts::OpenUrlArguments args;
    const KUrl url(reply->url());
    kDebug() << url;

    Q_FOREACH (const QByteArray &headerName, reply->rawHeaderList()) {
        args.metaData().insert(QString(headerName), QString(reply->rawHeader(headerName)));
    }

    emit d->part->browserExtension()->openUrlRequest(url, args, KParts::BrowserArguments());
}

void WebPage::slotDownloadRequest(const QNetworkRequest &request)
{
    const KUrl url(request.url());
    kDebug() << url;

    // Integration with a download manager...
    if (!url.isLocalFile()) {
        KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
        const QString downloadManger = cfg.readPathEntry("DownloadManager", QString());

        if (!downloadManger.isEmpty()) {
            // then find the download manager location
            kDebug() << "Using: " << downloadManger << " as Download Manager";
            QString cmd = KStandardDirs::findExe(downloadManger);
            if (cmd.isEmpty()) {
                QString errMsg = i18n("The Download Manager (%1) could not be found in your $PATH.", downloadManger);
                QString errMsgEx = i18n("Try to reinstall it. \n\nThe integration with Konqueror will be disabled.");
                KMessageBox::detailedSorry(view(), errMsg, errMsgEx);
                cfg.writePathEntry("DownloadManager", QString());
                cfg.sync();
            } else {
                cmd += ' ' + KShell::quoteArg(url.url());
                kDebug() << "Calling command" << cmd;
                KRun::runCommand(cmd, view());
                return;
            }
        }
    }

    KWebPage::downloadRequest(request);
}

void WebPage::slotGeometryChangeRequested(const QRect &rect)
{
    const QString host = mainFrame()->url().host();

    if (WebKitSettings::self()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) { // Why doesn't this work?
        emit d->part->browserExtension()->moveTopLevelWidget(rect.x(), rect.y());
    }

    int height = rect.height();
    int width = rect.width();

    // parts of following code are based on kjs_window.cpp
    // Security check: within desktop limits and bigger than 100x100 (per spec)
    if (width < 100 || height < 100) {
        kDebug() << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = KGlobalSettings::desktopGeometry(view());

    if (width > sg.width() || height > sg.height()) {
        kDebug() << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebKitSettings::self()->windowResizePolicy(host) == WebKitSettings::KJSWindowResizeAllow) {
        kDebug() << "resizing to " << width << "x" << height;
        emit d->part->browserExtension()->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    const int right = view()->x() + view()->frameGeometry().width();
    const int bottom = view()->y() + view()->frameGeometry().height();
    int moveByX = 0;
    int moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0
    if ((moveByX || moveByY) &&
        WebKitSettings::self()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {

        emit d->part->browserExtension()->moveTopLevelWidget(view()->x() + moveByX, view()->y() + moveByY);
    }
}

void WebPage::slotWindowCloseRequested()
{
    emit d->part->browserExtension()->requestFocus(d->part);
    if (KMessageBox::questionYesNo(view(),
                                   i18n("Close window ?"), i18n("Confirmation Required"),
                                   KStandardGuiItem::close(), KStandardGuiItem::cancel())
        == KMessageBox::Yes) {
        d->part->deleteLater();
        d->part = 0;
    }
}

void WebPage::slotStatusBarMessage(const QString &message)
{
    if (WebKitSettings::self()->windowStatusPolicy(mainFrame()->url().host()) == WebKitSettings::KJSWindowStatusAllow) {
        emit jsStatusBarMessage(message);
    }
}

void WebPage::slotRequestFinished(QNetworkReply *reply)
{
    Q_ASSERT(reply);
    QUrl url (reply->request().url());

    if (d->requestQueue.contains(url)) {

        kDebug() << url;
        QWebFrame* frame = d->requestQueue.take(url);
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode > 300 && statusCode < 304) {            
            kDebug() << "Redirected to " << reply->url();
        } else {
            const int errCode = convertErrorCode(reply);
            const bool isMainFrameRequest = (frame == mainFrame());

            // TODO: Perhaps look into supporting mixed mode, part secure and
            // part not, sites at some point. For now we only deal with SSL
            // information for the main page like most browsers.
            if (isMainFrameRequest && d->sslInfo.isValid() &&
                !domainSchemeMatch(reply->url(), d->sslInfo.url())) {
                kDebug() << "Reseting cached SSL info...";
                d->sslInfo = WebSslInfo();
            }

            // Handle any error...
            switch (errCode) {
                case 0:
                    if (isMainFrameRequest) {
                        // Obtain and set the SSL information if any...
                        if (!d->sslInfo.isValid()) {
                            d->sslInfo.fromMetaData(reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)));
                            d->sslInfo.setUrl(reply->url());
                        }

                        setPageJScriptPolicy(reply->url());
                    }
                    break;
                case KIO::ERR_ABORTED:
                case KIO::ERR_USER_CANCELED: // Do nothing if request is cancelled/aborted
                    kDebug() << "User aborted request!";
                    emit loadAborted(QUrl());
                    return;
                // Handle the user clicking on a link that refers to a directory
                // Since KIO cannot automatically convert a GET request to a LISTDIR one.
                case KIO::ERR_IS_DIRECTORY:
                    emit loadAborted(reply->url());
                    return;
                default:
#if QT_VERSION < 0x040600
                    // WORKAROUND:
                    // This code is necessary because QWebFrame::setHtml does
                    // not work properly in Qt < 4.6. Hence, we emit this signal
                    // here to get the webkitpart to properly save frame states.
                    if (isMainFrameRequest)
                        emit saveFrameStateRequested(frame, 0);
#endif
                    url = QString ("error:/?error=%1&errText=%2#%3")
                          .arg(errCode).arg(reply->errorString()).arg(reply->url().toString());
                    break;
            }

            emit navigationRequestFinished(url, frame);
        }
    }
}

bool WebPage::checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const
{
    // Check whether the request is authorized or not...
    if (!KAuthorized::authorizeUrlAction("redirect", mainFrame()->url(), req.url())) {
        QString buttonText, title, message;

        int response = KMessageBox::Cancel;
        KUrl linkUrl (req.url());

        if (type == QWebPage::NavigationTypeLinkClicked) {
            message = i18n("<qt>This untrusted page links to<br/><b>%1</b>."
                           "<br/>Do you want to follow the link?</qt>", linkUrl.url());
            title = i18n("Security Warning");
            buttonText = i18n("Follow");
        } else {
            title = i18n("Security Alert");
            message = i18n("<qt>Access by untrusted page to<br/><b>%1</b><br/> denied.</qt>",
                           Qt::escape(linkUrl.prettyUrl()));
        }

        if (buttonText.isEmpty()) {
            KMessageBox::error( 0, message, title);
        } else {
            // Dangerous flag makes the Cancel button the default
            response = KMessageBox::warningContinueCancel(0, message, title,
                                                          KGuiItem(buttonText),
                                                          KStandardGuiItem::cancel(),
                                                          QString(), // no don't ask again info
                                                          KMessageBox::Notify | KMessageBox::Dangerous);
        }

        return (response == KMessageBox::Continue);
    }

    return true;
}

bool WebPage::checkFormData(const QNetworkRequest &req) const
{
    const QString scheme (req.url().scheme().toLower());

    if (d->sslInfo.isValid() && scheme != "https" && scheme != "mailto" &&
        (KMessageBox::warningContinueCancel(0,
                                           i18n("Warning: This is a secure form "
                                                "but it is attempting to send "
                                                "your data back unencrypted.\n"
                                                "A third party may be able to "
                                                "intercept and view this "
                                                "information.\nAre you sure you "
                                                "wish to continue?"),
                                           i18n("Network Transmission"),
                                           KGuiItem(i18n("&Send Unencrypted")))  == KMessageBox::Cancel)) {

        return false;
    }


    if ((scheme == QL1("mailto")) &&
        (KMessageBox::warningContinueCancel(0, i18n("This site is attempting to "
                                                    "submit form data via email.\n"
                                                    "Do you want to continue?"),
                                            i18n("Network Transmission"),
                                            KGuiItem(i18n("&Send Email")),
                                            KStandardGuiItem::cancel(),
                                            "WarnTriedEmailSubmit") == KMessageBox::Cancel)) {
        return false;
    }

    return true;
}

bool WebPage::handleMailToUrl (const QUrl& url, NavigationType type) const
{
    if (QString::compare(url.scheme(), QL1("mailto"), Qt::CaseInsensitive) == 0) {
        QStringList files;
        QUrl mailtoUrl (sanitizeMailToUrl(url, files));

        switch (type) {
            case QWebPage::NavigationTypeLinkClicked:
                if (!files.isEmpty() && KMessageBox::warningContinueCancelList(0,
                                                                               i18n("<qt>Do you want to allow this site to attach "
                                                                                    "the following files to the email message ?</qt>"),
                                                                               files, i18n("Email Attachment Confirmation"),
                                                                               KGuiItem(i18n("&Allow attachments")),
                                                                               KGuiItem(i18n("&Ignore attachments")), QL1("WarnEmailAttachment")) == KMessageBox::Continue) {

                    Q_FOREACH(const QString& file, files) {
                        mailtoUrl.addQueryItem(QL1("attach"), file); // Re-add the attachments...
                    }
                }
                break;
            case QWebPage::NavigationTypeFormSubmitted:
            case QWebPage::NavigationTypeFormResubmitted:
                if (!files.isEmpty()) {
                    KMessageBox::information(0, i18n("This site attempted to attach a file from your "
                                                     "computer in the form submission. The attachment "
                                                     "was removed for your protection."),
                                             i18n("KDE"), "InfoTriedAttach");
                }
                break;
            default:
                 break;
        }

        kDebug() << "Emitting openUrlRequest with " << mailtoUrl;
        emit d->part->browserExtension()->openUrlRequest(mailtoUrl);
        return true;
    }

    return false;
}

void WebPage::setPageJScriptPolicy(const QUrl &url)
{
    const QString hostname (url.host());
    settings()->setAttribute(QWebSettings::JavascriptEnabled,
                             WebKitSettings::self()->isJavaScriptEnabled(hostname));

    const WebKitSettings::KJSWindowOpenPolicy policy = WebKitSettings::self()->windowOpenPolicy(hostname);
    settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,
                             (policy != WebKitSettings::KJSWindowOpenDeny &&
                              policy != WebKitSettings::KJSWindowOpenSmart));
}
