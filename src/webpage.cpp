/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 - 2010 Urs Wolfer <uwolfer @ kde.org>
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

#include "kwebkitpart.h"
#include "websslinfo.h"
#include "webview.h"
#include "sslinfodialog_p.h"
#include "networkaccessmanager.h"
#include "settings/webkitsettings.h"

#include <kparts/browseropenorsavequestion.h>
#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/BrowserRun>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KGlobalSettings>
#include <KDE/KGlobal>
#include <KDE/KLocale>
#include <KDE/KJobUiDelegate>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KStandardShortcut>
#include <KDE/KAuthorized>
#include <KIO/Job>
#include <KIO/AccessManager>
#include <KDE/KTemporaryFile>

#include <QtCore/QListIterator>
#include <QtGui/QTextDocument>
#include <QtGui/QApplication>
#include <QtNetwork/QNetworkReply>
#include <QtUiTools/QUiLoader>

#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebElement>
#include <QtWebKit/QWebSecurityOrigin>

#define QL1S(x)  QLatin1String(x)
#define QL1C(x)  QLatin1Char(x)

typedef QPair<QString, QString> StringPair;

// Sanitizes the "mailto:" url, e.g. strips out any "attach" parameters.
static QUrl sanitizeMailToUrl(const QUrl &url, QStringList& files) {
    QUrl sanitizedUrl;    

    // NOTE: This is necessary to ensure we can properly use QUrl's query
    // related APIs to process 'mailto:' urls of form 'mailto:foo@bar.com'.
    if (url.hasQuery())
      sanitizedUrl = url;
    else
      sanitizedUrl = QUrl(url.scheme() + QL1S(":?") + url.path());

    QListIterator<StringPair> it (sanitizedUrl.queryItems());
    sanitizedUrl.setEncodedQuery(QByteArray());    // clear out the query componenet

    while (it.hasNext()) {
        StringPair queryItem = it.next();
        if (queryItem.first.contains(QL1C('@')) && queryItem.second.isEmpty()) {
            queryItem.second = queryItem.first;
            queryItem.first = "to";
        } else if (QString::compare(queryItem.first, QL1S("attach"), Qt::CaseInsensitive) == 0) {
            files << queryItem.second;
            continue;
        }
        sanitizedUrl.addQueryItem(queryItem.first, queryItem.second);
    }

    return sanitizedUrl;
}

static int errorCodeFromReply(QNetworkReply* reply)
{
    // First check if there is a KIO error code sent back and use that,
    // if not attempt to convert QNetworkReply's NetworkError to KIO::Error.
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
            break;
    }

    return 0;
}

// Returns true if the scheme and domain of the two urls match...
static bool domainSchemeMatch(const QUrl& u1, const QUrl& u2)
{
    if (u1.scheme() != u2.scheme())
        return false;

    QStringList u1List = u1.host().split(QL1C('.'), QString::SkipEmptyParts);
    QStringList u2List = u2.host().split(QL1C('.'), QString::SkipEmptyParts);

    if (qMin(u1List.count(), u2List.count()) < 2)
        return false;  // better safe than sorry...

    while (u1List.count() > 2)
        u1List.removeFirst();

    while (u2List.count() > 2)
        u2List.removeFirst();

    return (u1List == u2List);
}

static void restoreStateFor(QWebFrame *frame, const WebFrameState &frameState)
{
    Q_ASSERT(frame);

    frame->setScrollPosition(QPoint(frameState.scrollPosX, frameState.scrollPosY));
    QHashIterator<QString, QString> it (frameState.formData);
    while (it.hasNext()) {
        it.next();
        QWebElement element = frame->documentElement().findFirst(it.key());
        if (element.isNull())
            kWarning() << "Found no element that matches:" << it.key();
        else
            element.evaluateJavaScript(QString::fromLatin1("if(this.value.length == 0) this.value=\"%1\";")
                                       .arg(it.value()));           
    }
}


class WebPage::WebPagePrivate
{
public:
    WebPagePrivate() : ignoreError(false), kioErrorCode(0) {}

    enum WebPageSecurity { PageUnencrypted, PageEncrypted, PageMixed };

    WebSslInfo sslInfo;
    QHash<QString, WebFrameState> frameStateContainer;
    // Holds list of requests including those from children frames
    QList<QUrl> requestQueue;
    QPointer<KWebKitPart> part;
    bool ignoreError;
    int kioErrorCode;
};

WebPage::WebPage(KWebKitPart *part, QWidget *parent)
        :KWebPage(parent, (KWebPage::KPartsIntegration|KWebPage::KWalletIntegration)),
         d (new WebPagePrivate)
{
    d->part = part;

    // Set our own internal network access manager...
    KDEPrivate::MyNetworkAccessManager *manager = new KDEPrivate::MyNetworkAccessManager(this);
    manager->setCache(0);
    if (parent && parent->window())
        manager->setCookieJarWindowId(parent->window()->winId());
    setNetworkAccessManager(manager);

    setSessionMetaData("ssl_activate_warnings", "TRUE");

    // Set font sizes accordingly...
    if (view())
        WebKitSettings::self()->computeFontSizes(view()->logicalDpiY());

    setForwardUnsupportedContent(true);

    // Tell QtWebKit to treat man:/ protocol as a local resource...
    QWebSecurityOrigin::addLocalScheme(QL1S("man"));

    // Override the 'Accept' header sent by QtWebKit which favors XML over HTML!
    // Setting the accept meta-data to null will force kio_http to use its own
    // default settings for this header.
    setSessionMetaData(QL1S("accept"), QString());

    connect(this, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(slotGeometryChangeRequested(const QRect &)));
    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
    connect(this, SIGNAL(statusBarMessage(const QString &)),
            this, SLOT(slotStatusBarMessage(const QString &)));
    connect(this, SIGNAL(downloadRequested(const QNetworkRequest &)),
            this, SLOT(downloadRequest(const QNetworkRequest &)));
    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
            this, SLOT(slotUnsupportedContent(QNetworkReply *)));
    connect(networkAccessManager(), SIGNAL(finished(QNetworkReply *)),
            this, SLOT(slotRequestFinished(QNetworkReply *)));
}

WebPage::~WebPage()
{
    delete d;
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

void WebPage::restoreFrameStates()
{
    QList<QWebFrame *> frames = mainFrame()->childFrames();
    frames.prepend(mainFrame());

    QListIterator<QWebFrame *> frameIt (frames);
    while (frameIt.hasNext()) {
        QWebFrame* frame = frameIt.next();
        if (d->frameStateContainer.contains(frame->frameName())) {
            restoreStateFor(frame, d->frameStateContainer.take(frame->frameName()));
        }
    }
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    QUrl reqUrl (request.url());

    // Handle "mailto:" url here...
    if (handleMailToUrl(reqUrl, type))
      return false;

    if (frame) {
        // inPage requests are those generarted within the current page through
        // link clicks, javascript queries, and button clicks (form submission).
        bool inPageRequest = true;

        switch (type) {
            case QWebPage::NavigationTypeFormSubmitted:
            case QWebPage::NavigationTypeFormResubmitted:
                if (!checkFormData(request))
                    return false;
                break;
            case QWebPage::NavigationTypeReload:
            case QWebPage::NavigationTypeBackOrForward:
                inPageRequest = false;
                break;
            case QWebPage::NavigationTypeOther:
                /*
                  NOTE: Unfortunately QtWebKit sends a NavigationTypeOther
                  when users click on links that use javascript. For example,

                  <a href="javascript:location.href='http://qt.nokia.com'">javascript link</a>

                  This completely screws up the link security checks we attempt
                  to do below. There is currently no reliable way to discern link
                  clicks from other requests that are sent as NavigationTypeOther!
                  Perhaps a new type, NavigationTypeJavascript, will be added in
                  the future to QtWebKit ??!?
                */
                //if (d->part->url() == reqUrl)
                inPageRequest = false;

                /*
                  NOTE: It is impossible to marry QtWebKit's history handling
                  with that of Konqueror's. They simply do not mix like oil and
                  water! Anyhow, the code below is an attempt to work around
                  the issues associated with these problems.

                  It is not 100% perfect because this kpart will not and cannot share
                  its history with other browser components like khtml. That is not
                  the fault of these componenets, but rather how history management is
                  handled by KPart itself. Anyhow, almost everything else should work
                  as expected including the history being properly restored even if this part gets unloaded and loaded again...
                  **** Warning to future maintainers of this code... think 100x
                  before attempting to mess with this code. It took a full two
                  months to work out all the scenarios and come up with this solution.

                */
                if (d->frameStateContainer.contains(frame->frameName())) {
                    if (frame == mainFrame()) {
                        // Avoid reloading page when navigating back from an
                        // anchor or link that points to some place within the
                        // same page.
                        const QUrl frameUrl (frame->url());
                        const bool frmUrlHasFragment = frameUrl.hasFragment();
                        const bool reqUrlHasFragment = reqUrl.hasFragment();

                        //kDebug() << frame << ", url now:" << frameUrl << ", url requested:" << reqUrl;
                        if ((frmUrlHasFragment && frameUrl.toString(QUrl::RemoveFragment) == reqUrl.toString()) ||
                            (reqUrlHasFragment && reqUrl.toString(QUrl::RemoveFragment) == frameUrl.toString()) ||
                            (frmUrlHasFragment && reqUrlHasFragment && reqUrl == frameUrl)) {
                            //kDebug() << "Avoiding page reload!" << endl;
                            emit loadFinished(true);
                            return false;
                        }
                    } else {
                        // The following code does proper history navigation for
                        // websites that are composed of frames.
                        WebFrameState &frameState = d->frameStateContainer[frame->frameName()];
                        if (!frameState.handled && !frameState.url.isEmpty() &&
                            frameState.url.toString() != reqUrl.toString()) {
                            const bool signalsBlocked = frame->blockSignals(true);
                            frame->setUrl(frameState.url);
                            frame->blockSignals(signalsBlocked);
                            frameState.handled = true;
                            //kDebug() << "Changing request for" << frame << "from" << reqUrl << "to" << frameState.url;
                            return false;
                        }
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
        reqUrl.setUserInfo(QString());
        d->requestQueue << reqUrl;
    }

    return KWebPage::acceptNavigationRequest(frame, request, type);
}

QWebPage *WebPage::createWindow(WebWindowType type)
{
    KParts::ReadOnlyPart *part = 0;
    KParts::BrowserArguments bargs;
    if (type == WebModalDialog)
        bargs.setForcesNewWindow(true);

    d->part->browserExtension()->createNewWindow(KUrl("about:blank"), KParts::OpenUrlArguments(),
                                                 bargs, KParts::WindowArgs(), &part);

    KWebKitPart *webKitPart = qobject_cast<KWebKitPart*>(part);
    if (webKitPart)
        return webKitPart->view()->page();

    kWarning() << "Got a null or non kwebkitpart" << part;
    return 0;
}

void WebPage::slotUnsupportedContent(QNetworkReply *reply)
{
    Q_ASSERT (reply);

    const KIO::MetaData metaData = reply->attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData)).toMap();
    bool hasContentDisposition;
    if (metaData.isEmpty())
        hasContentDisposition = reply->hasRawHeader("Content-Disposition");
    else
        hasContentDisposition = metaData.contains("content-disposition-filename");

    if (hasContentDisposition) {
    // Workaround for no support for Content-Disposition in
    // QtWebkit < 2.0 (Qt 4.7).
#if KDE_IS_VERSION(4,4,75)
        downloadResponse(reply);
#else
        reply->abort();
        downloadRequest(reply->request());
#endif
        return;
    }

    if (reply->request().originatingObject() == this->mainFrame()) {
        reply->abort();
        KParts::OpenUrlArguments args;
        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        if (!contentType.isEmpty()) {
            if (contentType.contains(QL1C(';')))
                contentType.truncate(contentType.indexOf(QL1C(';')));
            args.setMimeType(contentType);
        }
        emit d->part->browserExtension()->openUrlRequest(reply->url(), args, KParts::BrowserArguments());
    }
}

void WebPage::downloadRequest(const QNetworkRequest &request)
{
    const KUrl url(request.url());

    // Integration with a download manager...
    if (!url.isLocalFile()) {
        KConfigGroup cfg = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals)->group("HTML Settings");
        const QString downloadManger = cfg.readPathEntry("DownloadManager", QString());

        if (!downloadManger.isEmpty()) {
            // then find the download manager location
            //kDebug() << "Using: " << downloadManger << " as Download Manager";
            QString cmd = KStandardDirs::findExe(downloadManger);
            if (cmd.isEmpty()) {
                QString errMsg = i18n("The download manager (%1) could not be found in your installation.", downloadManger);
                QString errMsgEx = i18n("Try to reinstall it and make sure that it is available in $PATH. \n\nThe integration will be disabled.");
                KMessageBox::detailedSorry(view(), errMsg, errMsgEx);
                cfg.writePathEntry("DownloadManager", QString());
                cfg.sync();
            } else {
                cmd += ' ' + KShell::quoteArg(url.url());
                //kDebug() << "Calling command" << cmd;
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
        //kDebug() << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = KGlobalSettings::desktopGeometry(view());

    if (width > sg.width() || height > sg.height()) {
        //kDebug() << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebKitSettings::self()->windowResizePolicy(host) == WebKitSettings::KJSWindowResizeAllow) {
        //kDebug() << "resizing to " << width << "x" << height;
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
                                   i18n("Close window?"), i18n("Confirmation Required"),
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
    QWebFrame* frame = qobject_cast<QWebFrame *>(reply->request().originatingObject());

    if (frame && d->requestQueue.removeOne(url)) {
        //kDebug() << url;
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode > 300 && statusCode < 304) {
            //kDebug() << "Redirected to" << reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        } else {
            const int errCode = errorCodeFromReply(reply);
            const bool isMainFrameRequest = (frame == mainFrame());

            // TODO: Perhaps look into supporting mixed mode, part secure part
            // not, sites at some point. For now we only deal with SSL information
            // for the main frame just like KHTML.
            if (isMainFrameRequest && d->sslInfo.isValid() &&
                !domainSchemeMatch(reply->url(), d->sslInfo.url())) {
                //kDebug() << "Reseting cached SSL info...";
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
                    //kDebug() << "User aborted request!";
                    d->ignoreError = true;
                    emit loadAborted(QUrl());
                    return;
                // Handle the user clicking on a link that refers to a directory
                // Since KIO cannot automatically convert a GET request to a LISTDIR one.
                case KIO::ERR_IS_DIRECTORY:
                    d->ignoreError = true;
                    emit loadAborted(reply->url());
                    return;
                default:
                    // Make sure the saveFrameStateRequested signal is emitted so
                    // the page can restored properly.
                    if (isMainFrameRequest)
                        emit saveFrameStateRequested(frame, 0);

                    d->ignoreError = false;
                    d->kioErrorCode = errCode;
                    break;
            }

            if (isMainFrameRequest) {
                WebPagePrivate::WebPageSecurity security;
                if (d->sslInfo.isValid())
                    security = WebPagePrivate::PageEncrypted;
                else
                    security = WebPagePrivate::PageUnencrypted;

                emit d->part->browserExtension()->setPageSecurity(security);
            }
        }
    }
}

bool WebPage::checkLinkSecurity(const QNetworkRequest &req, NavigationType type) const
{
    // Check whether the request is authorized or not...
    if (!KAuthorized::authorizeUrlAction("redirect", mainFrame()->url(), req.url())) {

        //kDebug() << "*** Failed security check: base-url=" << mainFrame()->url() << ", dest-url=" << req.url();
        QString buttonText, title, message;

        int response = KMessageBox::Cancel;
        KUrl linkUrl (req.url());

        if (type == QWebPage::NavigationTypeLinkClicked) {
            message = i18n("<qt>This untrusted page links to<br/><b>%1</b>."
                           "<br/>Do you want to follow the link?</qt>", linkUrl.url());
            title = i18n("Security Warning");
            buttonText = i18nc("follow link despite of security warning", "Follow");
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
    const QString scheme (req.url().scheme());

    if (d->sslInfo.isValid() &&
        !scheme.compare(QL1S("https")) && !scheme.compare(QL1S("mailto")) &&
        (KMessageBox::warningContinueCancel(0,
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

bool WebPage::handleMailToUrl (const QUrl &url, NavigationType type) const
{
    if (QString::compare(url.scheme(), QL1S("mailto"), Qt::CaseInsensitive) == 0) {
        QStringList files;
        QUrl mailtoUrl (sanitizeMailToUrl(url, files));

        switch (type) {
            case QWebPage::NavigationTypeLinkClicked:
                if (!files.isEmpty() && KMessageBox::warningContinueCancelList(0,
                                                                               i18n("<qt>Do you want to allow this site to attach "
                                                                                    "the following files to the email message ?</qt>"),
                                                                               files, i18n("Email Attachment Confirmation"),
                                                                               KGuiItem(i18n("&Allow attachments")),
                                                                               KGuiItem(i18n("&Ignore attachments")), QL1S("WarnEmailAttachment")) == KMessageBox::Continue) {

                   // Re-add the attachments...
                    QListIterator<QString> filesIt (files);
                    while (filesIt.hasNext()) {
                        mailtoUrl.addQueryItem(QL1S("attach"), filesIt.next());
                    }
                }
                break;
            case QWebPage::NavigationTypeFormSubmitted:
            case QWebPage::NavigationTypeFormResubmitted:
                if (!files.isEmpty()) {
                    KMessageBox::information(0, i18n("This site attempted to attach a file from your "
                                                     "computer in the form submission. The attachment "
                                                     "was removed for your protection."),
                                             i18n("Attachment Removed"), "InfoTriedAttach");
                }
                break;
            default:
                 break;
        }

        //kDebug() << "Emitting openUrlRequest with " << mailtoUrl;
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

bool WebPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
    if (extension == QWebPage::ErrorPageExtension && !d->ignoreError)
    {
        const QWebPage::ErrorPageExtensionOption *extOption = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
        kDebug() << extOption->domain << extOption->error << extOption->errorString;
        if (extOption->domain == QWebPage::QtNetwork) {
            QWebPage::ErrorPageExtensionReturn *extOutput = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);
            extOutput->content = errorPage(d->kioErrorCode, extOption->errorString, extOption->url).toUtf8();
            if (extOption->frame->parentFrame())
              extOutput->baseUrl = this->mainFrame()->url();
            else
              extOutput->baseUrl = extOption->frame->url();

            return true;
        }
    }

    return KWebPage::extension(extension, option, output);
}

bool WebPage::supportsExtension(Extension extension) const
{
    kDebug() << extension;
    if (extension == QWebPage::ErrorPageExtension)
        return true;

    return KWebPage::supportsExtension(extension);
}

QString WebPage::errorPage(int code, const QString& text, const KUrl& reqUrl) const
{
  QString errorName, techName, description;
  QStringList causes, solutions;

  QByteArray raw = KIO::rawErrorDetail( code, text, &reqUrl );
  QDataStream stream(raw);

  stream >> errorName >> techName >> description >> causes >> solutions;

  QString url, protocol, datetime;
  url = reqUrl.url();
  protocol = reqUrl.protocol();
  datetime = KGlobal::locale()->formatDateTime( QDateTime::currentDateTime(), KLocale::LongDate );

  QString filename( KStandardDirs::locate( "data", "kwebkitpart/error.html" ) );
  QFile file( filename );
  if ( !file.open( QIODevice::ReadOnly ) )
    return i18n("<html><body><h3>Unable to display error message!</h3>"
                "<p>The error template file <em>error.html</em> could not be "
                "found!</p></body></html>");

  QString html = QString( QL1S( file.readAll() ) );

  html.replace( QL1S( "TITLE" ), i18n( "Error: %1", errorName ) );
  html.replace( QL1S( "DIRECTION" ), QApplication::isRightToLeft() ? "rtl" : "ltr" );
  html.replace( QL1S( "ICON_PATH" ), KUrl(KIconLoader::global()->iconPath("dialog-warning", -KIconLoader::SizeHuge)).url() );

  QString doc = QL1S( "<h1>" );
  doc += i18n( "The requested operation could not be completed" );
  doc += QL1S( "</h1><h2>" );
  doc += errorName;
  doc += QL1S( "</h2>" );

  if ( !techName.isNull() ) {
    doc += QL1S( "<h2>" );
    doc += i18n( "Technical Reason: %1", techName );
    doc += QL1S( "</h2>" );
  }

  doc += QL1S( "<h3>" );
  doc += i18n( "Details of the Request:" );
  doc += QL1S( "</h3><ul><li>" );
  doc += i18n( "URL: %1" ,  url );
  doc += QL1S( "</li><li>" );

  if ( !protocol.isNull() ) {
    doc += i18n( "Protocol: %1", protocol );
    doc += QL1S( "</li><li>" );
  }

  doc += i18n( "Date and Time: %1" ,  datetime );
  doc += QL1S( "</li><li>" );
  doc += i18n( "Additional Information: %1" ,  text );
  doc += QL1S( "</li></ul><h3>" );
  doc += i18n( "Description:" );
  doc += QL1S( "</h3><p>" );
  doc += description;
  doc += QL1S( "</p>" );

  if ( causes.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Causes:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += causes.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  if ( solutions.count() ) {
    doc += QL1S( "<h3>" );
    doc += i18n( "Possible Solutions:" );
    doc += QL1S( "</h3><ul><li>" );
    doc += solutions.join( "</li><li>" );
    doc += QL1S( "</li></ul>" );
  }

  html.replace( QL1S("TEXT"), doc );

  return html;
}
