/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "konqrun.h"

// KDE
#include "konqdebug.h"
#include <kmessagebox.h>
#include <KLocalizedString>
#include <kio/job.h>
#include <QMimeDatabase>
#include <QMimeType>
#include <QHostInfo>
#include <QFileInfo>

#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegate>
#include <KUrlAuthorized>
#include <KIO/DesktopExecParser>
#include <KProtocolInfo>
#include <KProtocolManager>
#include <KApplicationTrader>
#include <KJobWidgets>
#include <KParts/PartLoader>

#include <KService>
#include <KMimeTypeTrader>

// Local
#include "konqview.h"
#include "konqframestatusbar.h"
#include "konqhistorymanager.h"
#include "konqsettings.h"

KonqRun::KonqRun(KonqMainWindow *mainWindow, KonqView *_childView,
                 const QUrl &_url, const KonqOpenURLRequest &req, bool trustedSource)
    : KParts::BrowserRun(_url, req.args, req.browserArgs, _childView ? _childView->part() : nullptr, mainWindow,
                         //remove referrer if request was typed in manually.
                         // ### TODO: turn this off optionally.
                         !req.typedUrl.isEmpty(), trustedSource,
                         // Don't use inline errors on reloading due to auto-refresh sites, but use them in all other cases
                         // (no reload or user-requested reload)
                         !req.args.reload() || req.userRequestedReload),
      m_pMainWindow(mainWindow), m_pView(_childView), m_bFoundMimeType(false), m_req(req), m_inlineErrors(!req.args.reload() || req.userRequestedReload)
{
    setEnableExternalBrowser(false);
    //qCDebug(KONQUEROR_LOG) << "KonqRun::KonqRun() " << this;
    Q_ASSERT(!m_pMainWindow.isNull());
    if (m_pView) {
        m_pView->setLoading(true);
    }
}

KonqRun::~KonqRun()
{
    //qCDebug(KONQUEROR_LOG) << "KonqRun::~KonqRun() " << this;
    if (m_pView && m_pView->run() == this) {
        m_pView->setRun(nullptr);
    }
}

void KonqRun::foundMimeType(const QString &_type)
{
    //qCDebug(KONQUEROR_LOG) << "KonqRun::foundMimeType " << _type << " m_req=" << m_req.debug();

    QString mimeType = _type; // this ref comes from the job, we lose it when using KIO again

    m_bFoundMimeType = true;

    if (m_pView) {
        m_pView->setLoading(false);    // first phase finished, don't confuse KonqView
    }

    // Check if the main window wasn't deleted meanwhile
    if (!m_pMainWindow) {
        setError(true);
        setFinished(true);
        return;
    }

    // Grab the args back from BrowserRun
    m_req.args = arguments();
    m_req.browserArgs = browserArguments();

    bool tryEmbed = true;
    // One case where we shouldn't try to embed, is when the server asks us to save
    if (serverSuggestsSave()) {
        tryEmbed = false;
    }

    const bool associatedAppIsKonqueror = KonqMainWindow::isMimeTypeAssociatedWithSelf(mimeType);

    if (tryEmbed && tryOpenView(mimeType, associatedAppIsKonqueror)) {
        return;
    }

    // If we were following another view, do nothing if opening didn't work.
    if (m_req.followMode) {
        setFinished(true);
    }

    if (!hasFinished()) {
        // If we couldn't embed the mimetype, call BrowserRun::handleNonEmbeddable()
        KService::Ptr selectedService;
        KParts::BrowserRun::NonEmbeddableResult res = handleNonEmbeddable(mimeType, &selectedService);
        if (res == KParts::BrowserRun::Delayed) {
            return;
        }
        setFinished(res == KParts::BrowserRun::Handled);
        if (hasFinished()) {
            // save or cancel -> nothing else will happen in m_pView, so clear statusbar (#163628)
            m_pView->frame()->statusbar()->slotClear();
        } else {
            if (!tryEmbed) {
                // "Open" selected for a serverSuggestsSave() file - let's open. #171869
                if (tryOpenView(mimeType, associatedAppIsKonqueror)) {
                    return;
                }
            }
            // "Open" selected, possible with a specific application
            if (selectedService) {
                KRun::setPreferredService(selectedService->desktopEntryName());
            } else {
                // Open-with dialog
                KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob();
                job->setUrls({url()});
                job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_pMainWindow));
                job->setSuggestedFileName(suggestedFileName());
                job->start();
                setFinished(true);
            }
        }
    }

    // make Konqueror think there was an error, in order to stop the spinning wheel
    // (we saved, canceled, or we're starting another app... in any case the current view should stop loading).
    setError(true);

    if (!hasFinished()) { // only if we're going to open
        if (associatedAppIsKonqueror && m_pMainWindow->refuseExecutingKonqueror(mimeType)) {
            setFinished(true);
        }
    }

    if (!hasFinished()) {
        qCDebug(KONQUEROR_LOG) << "Nothing special to do in KonqRun, falling back to KRun";
        KRun::foundMimeType(mimeType);
    }
}

bool KonqRun::tryOpenView(const QString &mimeType, bool associatedAppIsKonqueror)
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    if (associatedAppIsKonqueror) {
        m_req.forceAutoEmbed = true;
    }

    // When text/html is associated with another browser,
    // we need to find out if we should keep browsing the web in konq,
    // or if we are clicking on an html file in a directory view (which should
    // then open the other browser)
    else if (mime.isValid() &&
             (mime.inherits(QStringLiteral("text/html"))
              || mime.name().startsWith(QLatin1String("image/"))) // #83513
             && (m_pView && !m_pView->showsDirectory())) {
        m_req.forceAutoEmbed = true;
    }

    const bool ok = m_pMainWindow->openView(mimeType, KRun::url(), m_pView, m_req);
    setFinished(ok);
    return ok;
}

void KonqRun::handleError(KJob *job)
{
    if (!m_mailto.isEmpty()) {
        setJob(nullptr);
        setFinished(true);
        return;
    }
    KParts::BrowserRun::handleError(job);
}

//Code copied from browserrun.cpp
void KonqRun::switchToErrorUrl(KIO::Error error, const QString &stringUrl)
{
    KRun::setUrl(makeErrorUrl(error, stringUrl, url()));
    setJob(nullptr);
    mimeTypeDetermined(QStringLiteral("text/html"));
}

//Most of the code in this function has been copied from krun.cpp and browserrun.cpp
void KonqRun::init()
{
    QUrl url = KRun::url();
    if (!url.isValid() || url.scheme().isEmpty()) {
        if (m_inlineErrors && !url.isValid()) {
            switchToErrorUrl(KIO::ERR_MALFORMED_URL, url.toString());
            return;
        }
        const QString error = !url.isValid() ? url.errorString() : url.toString();
        handleInitError(KIO::ERR_MALFORMED_URL, i18n("Malformed URL\n%1", error));
        qCWarning(KONQUEROR_LOG) << "Malformed URL:" << error;
        setError(true);
        setFinished(true);
        return;
    }

    if (!KUrlAuthorized::authorizeUrlAction(QStringLiteral("open"), QUrl(), url)) {
        QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, url.toDisplayString());
        handleInitError(KIO::ERR_ACCESS_DENIED, msg);
        setError(true);
        setFinished(true);
        return;
    }

    if (url.scheme().startsWith(QLatin1String("http")) && usingWebEngine()) {
        //This is a fake mimetype, needed only to ensure that the URL will be handled
        //by WebEnginePart which will then determine the real mimetype. If it's
        //a mimetype it can't handle, it'll emit the KParts::BrowserExtension::openUrlRequest
        //passing the real mimetype. Knowing the mimetype, KonqMainWindow::openUrl will handle
        //it correctly without needing to use KonqRun again.
        mimeTypeDetermined(QStringLiteral("text/html"));
    } else if (url.isLocalFile()
               && (url.host().isEmpty() || (url.host() == QLatin1String("localhost"))
                   || (url.host().compare(QHostInfo::localHostName(), Qt::CaseInsensitive) == 0))) {
        const QString localPath = url.toLocalFile();
        if (!QFile::exists(localPath)) {
            if (m_inlineErrors) {
                switchToErrorUrl(KIO::ERR_DOES_NOT_EXIST, localPath);
            } else {
                handleInitError(KIO::ERR_DOES_NOT_EXIST,
                                i18n("<qt>Unable to run the command specified. "
                                    "The file or folder <b>%1</b> does not exist.</qt>",
                                    localPath.toHtmlEscaped()));
                setError(true);
                setFinished(true);
            }
            return;
        }

        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForUrl(url);
        if (mime.isDefault() && !QFileInfo(localPath).isReadable()) {
            // Unknown MIME type because the file is unreadable, no point in showing an open-with dialog (#261002)
            const QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, localPath);
            handleInitError(KIO::ERR_ACCESS_DENIED, msg);
            setError(true);
            setFinished(true);
            return;
        } else {
            mimeTypeDetermined(mime.name());
            return;
        }
    } else if (KIO::DesktopExecParser::hasSchemeHandler(url) && !KProtocolInfo::isKnownProtocol(url)) {
        // looks for an application associated with x-scheme-handler/<protocol>
        KService::Ptr service = KApplicationTrader::preferredService(QLatin1String("x-scheme-handler/") + url.scheme());
        if (service) {
            //  if there's one...
            if (runApplication(*service, QList<QUrl>() << url, window(), RunFlags{}, QString(), QByteArray())) {
                setFinished(true);
                return;
            }
        } else {
            // fallback, look for associated helper protocol
            Q_ASSERT(KProtocolInfo::isHelperProtocol(url.scheme()));
            const auto exec = KProtocolInfo::exec(url.scheme());
            if (exec.isEmpty()) {
                // use default MIME type opener for file
                mimeTypeDetermined(KProtocolManager::defaultMimetype(url));
                return;
            } else {
                if (run(exec, QList<QUrl>() << url, window(), QString(), QString(), QByteArray())) {
                    setFinished(true);
                    return;
                }
            }
        }
    }

    // Let's see whether it is a directory

    if (!KProtocolManager::supportsListing(url)) {
        // No support for listing => it can't be a directory (example: http)

        if (!KProtocolManager::supportsReading(url)) {
            // No support for reading files either => we can't do anything (example: mailto URL, with no associated app)
            handleInitError(KIO::ERR_UNSUPPORTED_ACTION, i18n("Could not find any application or handler for %1", url.toDisplayString()));
            setError(true);
            setFinished(true);
            return;
        }
        scanFile();
        return;
    }

    // It may be a directory or a file, let's stat
    KIO::JobFlags flags = progressInfo() ? KIO::DefaultFlags : KIO::HideProgressInfo;
    KIO::StatJob *job = KIO::statDetails(url, KIO::StatJob::SourceSide, KIO::StatBasic, flags);
    KJobWidgets::setWindow(job, window());
    connect(job, &KJob::result, this, &KonqRun::slotStatResult);
    setJob(job);
    if (job && !job->error() && m_pView) {
        connect(job, &KIO::StatJob::infoMessage, m_pView, &KonqView::slotInfoMessage);
    }
}

bool KonqRun::usingWebEngine() const
{
    //We need to find out if the user configured Konqueror to use WebEnginePart by default or not.
    //If the current part can display html files, it could be a WebEnginePart, so we can check it.
    //If the current part can't display html files, it will never be a webengine part, so it doesn't
    //tell anything about user configuration. In this case, always check the preferred part.
    KParts::ReadOnlyPart *part = m_pView ? m_pView->part() : nullptr;
    if (m_pView && m_pView->isWebBrowsingPart()) {
        return part->componentName() == "webenginepart";
    } else {
        QVector<KPluginMetaData> parts = KParts::PartLoader::partsForMimeType("text/html");
        return !parts.isEmpty() && parts.at(0).name() == "WebEngine";
    }
}


void KonqRun::scanFile()
{
    //Since QtWebEngine can't use the KIO framework, attempting to determine the mimetype here when
    //using QtWebEngine will lead to a double GET request. To avoid it, when using QtWebEngine, any URL with
    //http or https protocol will be treated as if it were text/html: this means it'll be opened with WebEnginePart,
    //which will determine its mimetype and proceed accordingly. However, this can lead to an endless loop when an http(s) URL
    //is of type application/octet-stream:
    // - WebEnginePart (indirectly) calls KonqMainWindow::openUrl with application/octet-stram as mimetype
    // - KonqMainWindow needs to know a more specific mimetype, so it creates a KonqRun
    // - KonqRun calls scanFile
    // - Since the protocol is http(s), scanFile delegates finding out the mimetype to WebEnginePart,
    //   which finds application/octet-stream starting an endless loop
    // To avoid this we assume that the creator of the KonqRun has set m_alreadyProcessedByWebEngine if the URL has
    // already been passed to WebEnginePart: it means that it couldn't find a suitable mimetype and we need to do it
    // by ourselves, even if it means doing a double GET request.
    if (m_req.args.mimeType().isEmpty() && (url().scheme() == "http" || url().scheme() == "https") && usingWebEngine()) {
        mimeTypeDetermined("text/html");
        return;
    }
    KParts::BrowserRun::scanFile();
    // could be a static cast as of now, but who would notify when
    // BrowserRun changes
    KIO::TransferJob *job = dynamic_cast<KIO::TransferJob *>(KRun::job());
    if (job && !job->error()) {
        connect(job, SIGNAL(redirection(KIO::Job*,QUrl)),
                SLOT(slotRedirection(KIO::Job*,QUrl)));
        if (m_pView && m_pView->service()->desktopEntryName() != QLatin1String("konq_sidebartng")) {
            connect(job, SIGNAL(infoMessage(KJob*,QString,QString)),
                    m_pView, SLOT(slotInfoMessage(KJob*,QString)));
        }
    }
}

void KonqRun::slotRedirection(KIO::Job *job, const QUrl &redirectedToURL)
{
    QUrl redirectFromURL = static_cast<KIO::TransferJob *>(job)->url();
    qCDebug(KONQUEROR_LOG) << redirectFromURL << "->" << redirectedToURL;
    KonqHistoryManager::kself()->confirmPending(redirectFromURL);

    if (redirectedToURL.scheme() == QLatin1String("mailto")) {
        m_mailto = redirectedToURL;
        return; // Error will follow
    }
    KonqHistoryManager::kself()->addPending(redirectedToURL);

    // Do not post data on reload if we were redirected to a new URL when
    // doing a POST request.
    if (redirectFromURL != redirectedToURL) {
        browserArguments().setDoPost(false);
    }
    browserArguments().setRedirectedRequest(true);
}

KonqView *KonqRun::childView() const
{
    return m_pView;
}

