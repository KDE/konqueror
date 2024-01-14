/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "urlloader.h"
#include "konqsettings.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include "konqurl.h"
#include "konqdebug.h"

#include "libkonq_utils.h"

#include "interfaces/downloaderextension.h"

#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KIO/FileCopyJob>
#include <KIO/MimeTypeFinderJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/CopyJob>
#include <KIO/JobTracker>
#include <KMessageBox>
#include <KParts/ReadOnlyPart>
#include "kf5compat.h" //For NavigationExtension
#include <KParts/PartLoader>
#include <KJobWidgets>
#include <KProtocolManager>
#include <KDesktopFile>
#include <KApplicationTrader>
#include <KParts/PartLoader>
#include <KLocalizedString>
#include <KIO/JobUiDelegateFactory>
#include <KJobTrackerInterface>

#include <QDebug>
#include <QArgument>
#include <QWebEngineProfile>
#include <QMimeDatabase>
#include <QWebEngineProfile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLoggingCategory>

using namespace KonqInterfaces;

bool UrlLoader::embedWithoutAskingToSave(const QString &mimeType)
{
    static QStringList s_mimeTypes;
    if (s_mimeTypes.isEmpty()) {
        QStringList names{QStringLiteral("kfmclient_html"), QStringLiteral("kfmclient_dir"), QStringLiteral("kfmclient_war")};
        for (const QString &name : names) {
            KService::Ptr s = KService::serviceByStorageId(name);
            if (s) {
                s_mimeTypes.append(s->mimeTypes());
            } else {
                qCDebug(KONQUEROR_LOG()) << "Couldn't find service" << name;
            }
        }
        //The user may want to save xml files rather than embedding them
        //TODO: is there a better way to do this?
        s_mimeTypes.removeOne(QStringLiteral("application/xml"));
    }
    return s_mimeTypes.contains(mimeType);
}

UrlLoader::UrlLoader(KonqMainWindow *mainWindow, KonqView *view, const QUrl &url, const QString &mimeType, const KonqOpenURLRequest &req, bool trustedSource, bool dontEmbed):
    QObject(mainWindow),
    m_mainWindow(mainWindow),
    m_url(url),
    m_mimeType(mimeType),
    m_request(req),
    m_view(view),
    m_trustedSource(trustedSource),
    m_dontEmbed(dontEmbed),
    m_protocolAllowsReading(KProtocolManager::supportsReading(m_url) || !KProtocolInfo::isKnownProtocol(m_url)), // If the protocol is unknown, assume it allows reading
    m_letRequestingPartDownloadUrl(req.letPartPerformDownload)
{
    //TODO KF6: currently there's no way to pass the suggested file name. When dropping compatibility with KF5, have a field for this
    //in BrowserArguments
    m_request.suggestedFileName = m_request.args.metaData().value(QStringLiteral("SuggestedFileName"));

    //TODO KF6 after implementing a better way to allow the user to display a file after saving it locally
    //(see comment for WebEnginePage::saveUrlToDiskAndDisplay in webenginepage.cpp), remove all references
    //to embedOrNothing
    m_embedOrNothing = m_request.args.metaData().contains(QStringLiteral("EmbedOrNothing"));

    getDownloaderJobFromPart();
    if (m_letRequestingPartDownloadUrl) {
        m_originalUrl = m_url;
    }
    determineStartingMimetype();
    m_dontPassToWebEnginePart = m_request.args.metaData().contains("DontSendToDefaultHTMLPart");

    //Obviously this shoud never happen
    if (m_dontEmbed && m_embedOrNothing) {
        qCDebug(KONQUEROR_LOG()) << "Conflicting requests for loading" << m_url << ": to never embed and to only embed. Nothing will be done";
        m_action = OpenUrlAction::DoNothing;
    }
}

UrlLoader::~UrlLoader()
{
}

void UrlLoader::determineStartingMimetype()
{
    QMimeDatabase db;
    if (!m_mimeType.isEmpty()) {
        if (m_letRequestingPartDownloadUrl || !db.mimeTypeForName(m_mimeType).isDefault()) {
            return;
        }
    }

    m_mimeType = m_request.args.mimeType();
    //In theory, this should never happen, as parts requesting to download the URL
    //by themselves should already have set the mimetype. However, let's check, just in case
    if (m_letRequestingPartDownloadUrl && m_mimeType.isEmpty()) {
        m_mimeType = QStringLiteral("application/octet-stream");
    } else if (db.mimeTypeForName(m_mimeType).isDefault()) {
        m_mimeType.clear();
    }
}

QString UrlLoader::mimeType() const
{
    return m_mimeType;
}

bool UrlLoader::isMimeTypeKnown(const QString &mimeType)
{
    return !mimeType.isEmpty();
}

void UrlLoader::setView(KonqView* view)
{
    m_view = view;
}

void UrlLoader::setOldLocationBarUrl(const QString& old)
{
    m_oldLocationBarUrl = old;
}

void UrlLoader::setNewTab(bool newTab)
{
    m_request.browserArgs.setNewTab(newTab);
}

void UrlLoader::start()
{
    if (m_url.isLocalFile()) {
        detectSettingsForLocalFiles();
    } else {
        detectSettingsForRemoteFiles();
    }

    if (hasError()) {
        m_mimeType = QStringLiteral("text/html");
    }
    if (isMimeTypeKnown(m_mimeType)) {
        KService::Ptr preferredService = KApplicationTrader::preferredService(m_mimeType);
        if (serviceIsKonqueror(preferredService)) {
            m_request.forceAutoEmbed = true;
        }
    }

    m_isAsync = m_protocolAllowsReading && (!isMimeTypeKnown(m_mimeType) || m_letRequestingPartDownloadUrl);
}

bool UrlLoader::isViewLocked() const
{
    return m_view && m_view->isLockedLocation();
}

void UrlLoader::decideAction()
{
    if (hasError()) {
        m_action = OpenUrlAction::Embed;
        return;
    }
    if (!m_embedOrNothing) {
        m_action = decideExecute();
    }
    switch (m_action) {
        case OpenUrlAction::Execute:
            m_ready = true;
            break;
        case OpenUrlAction::DoNothing:
            m_ready = true;
            return;
        default:
            if (!isMimeTypeKnown(m_mimeType) && !m_protocolAllowsReading && !m_embedOrNothing) {
                //If the protocol doesn't allow reading and we don't have a mimetype associated with it,
                //use the Open action, as we most likely won't be able to find out the mimetype. This is
                //what happens, for example, for mailto URLs
                m_action = OpenUrlAction::Open;
                return;
            } else if (isViewLocked() || shouldEmbedThis() || m_embedOrNothing) {
                bool success = decideEmbedOrSave();
                if (success || m_embedOrNothing) {
                    if (m_embedOrNothing && m_action != OpenUrlAction::Embed) {
                        m_action = OpenUrlAction::DoNothing;
                    }
                    return;
                }
            }
            decideOpenOrSave();
    }
}

void UrlLoader::abort()
{
    if (m_openUrlJob) {
        m_openUrlJob->kill();
    }
    if (m_applicationLauncherJob) {
        m_applicationLauncherJob->kill();
    }
    deleteLater();
}


void UrlLoader::goOn()
{
    if (m_isAsync && !isMimeTypeKnown(m_mimeType)) {
        launchMimeTypeFinderJob();
    } else {
        decideAction();
        m_ready = !m_letRequestingPartDownloadUrl;
        performAction();
    }
}

KPluginMetaData UrlLoader::findEmbeddingPart(bool forceServiceName) const
{
    const QLatin1String webEngineName("webenginepart");

    //Use WebEnginePart for konq: URLs even if it's not the default html engine
    if (KonqUrl::hasKonqScheme(m_url)) {
        return findPartById(webEngineName);
    }

    KPluginMetaData part;

    //Check whether the view can display the mimetype, but only if the URL hasn't been explicitly
    //typed by the user: in this case, use the preferred service. This is needed to avoid the situation
    //where m_view is a Kate part, the user enters the URL of a web page and the page is opened within
    //the Kate part because it can handle html files.
    if (m_view && m_request.typedUrl.isEmpty() && m_view->supportsMimeType(m_mimeType)) {
        part = m_view->service();
    } else if (!m_request.serviceName.isEmpty()) {
        // If the service name has been set by the "--part" command line argument
        // (detected in handleCommandLine() in konqmain.cpp), then use it as is.
        part = findPartById(m_request.serviceName);
        if (!forceServiceName && !part.supportsMimeType(m_mimeType)) {
            part = KPluginMetaData();
        }
    }

    if (!part.isValid()) {
        part = preferredPart(m_mimeType);
    }

    /* Corner case: webenginepart can't determine mimetype (gives application/octet-stream) but
     * OpenUrlJob determines a mimetype supported by WebEnginePart (for example application/xml):
     * if the preferred part is webenginepart, we'd get an endless loop because webenginepart will
     * call again this. To avoid this, if the preferred service is webenginepart and m_dontPassToWebEnginePart
     * is true, use the second preferred service (if any); otherwise return false. This will offer the user
     * the option to open or save, instead.
     *
     * This can also happen if the URL was opened from a link with the "download" attribute or with a
     * "CONTENT-DISPOSITION: attachment" header. In these cases, WebEnginePart will always refuse to open
     * the URL and will ask Konqueror to download it. However, if the preferred part for the URL mimetype is
     * WebEnginePart itself, this would lead to an endless loop. This check avoids it
     */
    if (m_dontPassToWebEnginePart && m_part.pluginId() == webEngineName) {
        QVector<KPluginMetaData> parts = KParts::PartLoader::partsForMimeType(m_mimeType);
        auto findPart = [&webEngineName](const KPluginMetaData &md){return md.pluginId() != webEngineName;};
        QVector<KPluginMetaData>::const_iterator partToUse = std::find_if(parts.constBegin(), parts.constEnd(), findPart);
        if (partToUse != parts.constEnd()) {
            part = *partToUse;
        } else {
            part = KPluginMetaData();
        }
    }
    return part;
}

bool UrlLoader::decideEmbedOrSave()
{
    m_part = findEmbeddingPart();

    //If we can't find a part, return false, so that the caller can use decideOpenOrSave to allow the
    //user the possibility of opening the file, since embedding wasn't possibile
    if (!m_part.isValid()) {
        return false;
    }

    //Ask whether to save or embed, except in the following cases:
    //- it's a web page: always embed
    //- it's a local file: always embed
    if (embedWithoutAskingToSave(m_mimeType) || m_url.isLocalFile() || m_embedOrNothing) {
        m_action = OpenUrlAction::Embed;
    } else {
        m_action = askSaveOrOpen(OpenEmbedMode::Embed).first;
    }

    if (m_action == OpenUrlAction::Embed) {
        m_request.serviceName = m_part.pluginId();
    }

    m_ready = m_part.isValid() || m_action != OpenUrlAction::Embed;
    return true;
}

void UrlLoader::decideOpenOrSave()
{
    m_ready = true;
    QString protClass = KProtocolInfo::protocolClass(m_url.scheme());
    bool isLocal = m_url.isLocalFile();
    bool alwaysOpen = isLocal || protClass == QLatin1String(":local") || KProtocolInfo::isHelperProtocol(m_url);
    OpenSaveAnswer answerWithService;
    if (!alwaysOpen) {
        answerWithService = askSaveOrOpen(OpenEmbedMode::Open);
    } else {
        answerWithService = qMakePair(OpenUrlAction::Open, KApplicationTrader::preferredService(m_mimeType));
    }

    m_action = answerWithService.first;

    //If the URL should be opened in an external application and it should be downloaded by the requesting part,
    //ensure it's not downloaded in Konqueror's temporary directory but in the global temporary directory,
    //otherwise if the user closes Konqueror before closing the application, there could be issues because the
    //external application wouldn't find the file anymore.
    //Problem: if the application doesn't support the --tempfile switch, the file will remain even after both Konqueror
    //and the external application are closed and will only be removed automatically if the temporary directory is deleted
    //or emptied.
    if (m_letRequestingPartDownloadUrl && m_action == OpenUrlAction::Open && m_partDownloaderJob) {
        QString fileName = QFileInfo(m_partDownloaderJob->downloadPath()).fileName();
        m_partDownloaderJob->setDownloadPath(QDir::temp().filePath(fileName));
    }
    m_service = answerWithService.second;
}


bool UrlLoader::isUrlExecutable() const
{
    if (!m_url.isLocalFile()) {
        return false;
    }

//Code copied from kio/krun.cpp (KF5.109) written by:
//- Torben Weis <weis@kde.org>
//- David Faure <faure@kde.org>
//- Michael Pyne <michael.pyne@kdemail.net>
//- Harald Sitter <sitter@kde.org>
    QMimeDatabase db;
    QMimeType mimeType = db.mimeTypeForName(m_mimeType);
    if (!(mimeType.inherits(QStringLiteral("application/x-desktop")) ||
        mimeType.inherits(QStringLiteral("application/x-executable")) ||
        /* See https://bugs.freedesktop.org/show_bug.cgi?id=97226 */
        mimeType.inherits(QStringLiteral("application/x-sharedlib")) ||
        mimeType.inherits(QStringLiteral("application/x-ms-dos-executable")) ||
        mimeType.inherits(QStringLiteral("application/x-shellscript")))) {
        return false;
    }

    return QFileInfo(m_url.path()).isExecutable();
}


UrlLoader::OpenUrlAction UrlLoader::decideExecute() const {
    //We don't want to execute files which aren't local, files which aren't executable (obviously)
    //and we don't want to execute when we are reloading (the file is visible in the current part,
    //so we know the user wanted to display it
    if (!isUrlExecutable() || m_request.args.reload()) {
        return OpenUrlAction::UnknwonAction;
    }
    bool canDisplay = !KParts::PartLoader::partsForMimeType(m_mimeType).isEmpty();

    KMessageBox::ButtonCode code;
    KGuiItem executeGuiItem(i18nc("Execute an executable file", "Execute"),
                            QIcon::fromTheme(QStringLiteral("system-run")));
    KGuiItem displayGuiItem(i18nc("Display an executable file", "Display"),
                            QIcon::fromTheme(QStringLiteral("document-preview")));
    QString dontShowAgainId(QLatin1String("AskExecuting")+m_mimeType);

    if (canDisplay) {
        code = KMessageBox::questionTwoActionsCancel(m_mainWindow,
                                                     xi18nc("@info The user has to decide whether to execute an executable file or display it",
                                                            "<filename>%1</filename> can be executed. Do you want to execute it or to display it?", m_url.path()),
                                                     QString(), executeGuiItem, displayGuiItem,
                                                     KStandardGuiItem::cancel(), dontShowAgainId, KMessageBox::Dangerous);
    } else {
        code = KMessageBox::questionTwoActions(m_mainWindow,
                                               xi18nc("@info The user has to decide whether to execute an executable file or not",
                                                      "<filename>%1</filename> can be executed. Do you want to execute it?", m_url.path()),
                                               QString(), executeGuiItem, KStandardGuiItem::cancel(),
                                               dontShowAgainId, KMessageBox::Dangerous);}
    switch (code) {
        case KMessageBox::PrimaryAction:
            return OpenUrlAction::Execute;
        case KMessageBox::Cancel:
            return OpenUrlAction::DoNothing;
        case KMessageBox::SecondaryAction:
            //The "No" button actually corresponds to the "Cancel" action if the file can't be displayed
            return canDisplay ? OpenUrlAction::UnknwonAction : OpenUrlAction::DoNothing;
        default: //This is here only to avoid a compiler warning
            return OpenUrlAction::UnknwonAction;
    }
}

void UrlLoader::performAction()
{
    //If we still aren't ready, it means that the part wants to download the URL
    //by itself. Do this only when opening or embedding, however, since when
    //saving we first need to ask the user where to save. save() will launch the
    //job itself.
    //When the part has finished downloading the URL, the slot connected with the
    //job's result() signal will call again this function, after setting m_ready
    //to true
    if (!m_ready && (m_action == OpenUrlAction::Embed || m_action == OpenUrlAction::Open)) {
        downloadForEmbeddingOrOpening();
        return;
    }
    switch (m_action) {
        case OpenUrlAction::Embed:
            embed();
            break;
        case OpenUrlAction::Open:
            open();
            break;
        case OpenUrlAction::Execute:
            execute();
            break;
        case OpenUrlAction::Save:
            save();
            break;
        case OpenUrlAction::DoNothing:
        case OpenUrlAction::UnknwonAction: //This should never happen
            done();
            break;
    }
}

void UrlLoader::getDownloaderJobFromPart()
{
    if (!m_letRequestingPartDownloadUrl) {
        return;
    }
    DownloaderExtension *iface = downloaderInterface();
    if (iface) {
        m_partDownloaderJob = iface->downloadJob(m_url, m_request.downloadId, this);
    } else {
        qCDebug(KONQUEROR_LOG) << "Wanting to let part download" << m_url << "but part doesn't implement the DownloaderInterface";
    }

    //If we can't get a job for whatever reason (it shouldn't happen, but let's be sure)
    //try to open the URL in the usual way. Maybe it'll work (most likely, it will if
    //there's no need to have special cookies set to access it)
    if (!m_partDownloaderJob) {
        qCDebug(KONQUEROR_LOG) << "Couldn't get DownloadJob for" << m_url << "from part" << m_part;
        m_letRequestingPartDownloadUrl = false;
        m_request.tempFile = false; //Opening a remote URL with tempFile will fail
    }
}

void UrlLoader::downloadForEmbeddingOrOpening()
{
    //This shouldn't happen
    if (!m_partDownloaderJob) {
        done();
        return;
    }
    connect(m_partDownloaderJob, &DownloaderJob::downloadResult, this, &UrlLoader::jobFinished);
#if QT_VERSION_MAJOR < 6
    //In KF5, DownloaderJob::startDownload doesn't connect to the signal
    connect(m_partDownloaderJob, &DownloaderJob::downloadResult, this, &UrlLoader::downloadForEmbeddingOrOpeningDone);
#endif
    m_partDownloaderJob->startDownload(m_mainWindow, this, &UrlLoader::downloadForEmbeddingOrOpeningDone);
}

void UrlLoader::downloadForEmbeddingOrOpeningDone(KonqInterfaces::DownloaderJob *job, const QUrl &url)
{
    if (job && job->error() == 0) {
        m_url = url;
        m_ready = true;
        m_request.tempFile = true;
    } else if (!job || job->error() == KIO::ERR_USER_CANCELED) {
        m_action = OpenUrlAction::DoNothing;
        m_ready = true;
    }
    checkDownloadedMimetype();
    performAction();
}

void UrlLoader::checkDownloadedMimetype()
{
    QMimeDatabase db;
    QMimeType typeByContent = db.mimeTypeForFile(m_url.path(), QMimeDatabase::MatchContent);
    QMimeType typeByName = db.mimeTypeForFile(m_url.path(), QMimeDatabase::MatchExtension);
    QString type = (typeByName.inherits(typeByContent.name()) ? typeByName : typeByContent).name();
    if (type == m_mimeType) {
        return;
    }
    m_mimeType = type;
    //The URL is a local file now, so there are no problems in opening it with WebEnginePart
    m_dontPassToWebEnginePart = false;
    if (shouldEmbedThis()) {
        m_part = findEmbeddingPart(false);
        if (m_part.isValid()) {
            m_action = OpenUrlAction::Embed;
            return;
        }
    }
    m_action = OpenUrlAction::Open;
    if (m_service && m_service->hasMimeType(m_mimeType)) {
        return;
    }
    m_service = KApplicationTrader::preferredService(m_mimeType);
}

void UrlLoader::done(KJob *job)
{
    //Ensure that m_mimeType and m_request.args.mimeType are equal, since it's not clear what will be used
    m_request.args.setMimeType(m_mimeType);
    if (job) {
        jobFinished(job);
    }
    emit finished(this);
    //If we reach here and m_partDownloaderJob->finished() is false, it means the job hasn't been started in the first place,
    //(because the user canceled the download), so kill it
    if (m_partDownloaderJob && !m_partDownloaderJob->finished()) {
        m_partDownloaderJob->kill();
    }
    deleteLater();
}

bool UrlLoader::serviceIsKonqueror(KService::Ptr service)
{
    return service && (service->desktopEntryName() == QLatin1String("konqueror") || service->exec().trimmed() == QLatin1String("konqueror") || service->exec().trimmed().startsWith(QLatin1String("kfmclient")));
}

void UrlLoader::launchMimeTypeFinderJob()
{
    m_mimeTypeFinderJob = new KIO::MimeTypeFinderJob(m_url, this);
    m_mimeTypeFinderJob->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    m_mimeTypeFinderJob->setSuggestedFileName(m_request.suggestedFileName);
    connect(m_mimeTypeFinderJob, &KIO::MimeTypeFinderJob::result, this, [this](KJob*){mimetypeDeterminedByJob();});
    m_mimeTypeFinderJob->start();
}

void UrlLoader::mimetypeDeterminedByJob()
{
    if (m_mimeTypeFinderJob->error()) {
        m_jobErrorCode = m_mimeTypeFinderJob->error();
        m_url = Konq::makeErrorUrl(m_jobErrorCode, m_mimeTypeFinderJob->errorString(), m_url);
        m_mimeType = QStringLiteral("text/html");
        m_action = OpenUrlAction::Embed;
        performAction();
        return;
    }
    m_mimeType=m_mimeTypeFinderJob->mimeType();
    if (m_mimeType.isEmpty()) {
        //Ensure the mimetype is not empty so that goOn (called below) won't attempt to determine it again
        //In theory, KIO::MimeTypeFinderJob::mimeType() should never return an empty string
        m_mimeType = QStringLiteral("application/octet-stream");
    }
    //Only check whether the URL represents an archive when it is a local file. This can be either because
    //QUrl::isLocalFile returns true or because its scheme corresponds to a protocol with class :local
    //(for example, tar)
    if (m_url.isLocalFile() || KProtocolInfo::protocolClass(m_url.scheme()) == QLatin1String(":local")) {
        detectArchiveSettings();
    }
    goOn();
}

bool UrlLoader::shouldUseDefaultHttpMimeype() const
{
    if (m_dontPassToWebEnginePart || isMimeTypeKnown(m_mimeType)) {
        return false;
    }
    const QVector<QString> webengineSchemes = {QStringLiteral("error"), QStringLiteral("konq")};
    return m_url.scheme().startsWith(QStringLiteral("http")) || webengineSchemes.contains(m_url.scheme());
}

DownloaderExtension* UrlLoader::downloaderInterface() const
{
    if (!m_request.requestingPart) {
        return nullptr;
    }
    return DownloaderExtension::downloader(m_request.requestingPart);
}

void UrlLoader::detectSettingsForRemoteFiles()
{
    if (m_url.isLocalFile()) {
        return;
    }

    if (m_url.scheme() == QLatin1String("error")) {
        m_letRequestingPartDownloadUrl = false; //error URLs can never be downloaded
        m_mimeType = QLatin1String("text/html");
        m_request.args.setMimeType(QStringLiteral("text/html"));
    }
    else if (shouldUseDefaultHttpMimeype()) {
        // If a part which supports html asked to download the URL, it means it's not html, so don't change its mimetype
        if (m_letRequestingPartDownloadUrl && m_part.supportsMimeType(QStringLiteral("text/html"))) {
            return;
        }
        m_mimeType = QLatin1String("text/html");
        m_request.args.setMimeType(QStringLiteral("text/html"));
    } else if (!m_trustedSource && isTextExecutable(m_mimeType)) {
        m_mimeType = QLatin1String("text/plain");
        m_request.args.setMimeType(QStringLiteral("text/plain"));
    }
}

int UrlLoader::checkAccessToLocalFile(const QString& path)
{
    QFileInfo info(path);
    bool fileExists = info.exists();
    if (!info.isReadable()) {
        QFileInfo parentInfo(info.dir().path());
        if (parentInfo.isExecutable() && !fileExists) {
            return KIO::ERR_DOES_NOT_EXIST;
        } else {
            return KIO::ERR_CANNOT_OPEN_FOR_READING;
        }
    } else if (info.isDir() && !info.isExecutable()) {
        return KIO::ERR_CANNOT_ENTER_DIRECTORY;
    } else {
        return 0;
    }
}

void UrlLoader::detectArchiveSettings()
{
    // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
    // zip:/<path>/ when clicking on a zip file, etc.
    // The .protocol file specifies the mimetype that the kioslave handles.
    // Note that we don't use mimetype inheritance since we don't want to
    // open OpenDocument files as zip folders...
    const QString protocol = KProtocolManager::protocolForArchiveMimetype(m_mimeType);
    if (protocol.isEmpty() && !KProtocolInfo::archiveMimetypes(m_url.scheme()).isEmpty() && m_mimeType == QLatin1String("inode/directory")) {
        m_url.setScheme(QStringLiteral("file"));
    } else if (!protocol.isEmpty() && KonqFMSettings::settings()->shouldEmbed(m_mimeType)) {
        m_url.setScheme(protocol);
        //If the URL ends with /, we assume that it was the result of the user using the Up button while displaying the webarchive.
        //This means that we don't want to add the /index.html part, otherwise the effect will be to have the same URL but with
        //an increasing number of slashes before index.html
        if (m_mimeType == QLatin1String("application/x-webarchive") && !m_url.path().endsWith('/')) {
            m_url.setPath(m_url.path() + QStringLiteral("/index.html"));
            m_mimeType = QStringLiteral("text/html");
        } else {
            if (KProtocolManager::outputType(m_url) == KProtocolInfo::T_FILESYSTEM) {
                if (!m_url.path().endsWith('/')) {
                    m_url.setPath(m_url.path() + '/');
                }
                m_mimeType = QStringLiteral("inode/directory");
            } else {
                m_mimeType.clear();
            }
        }
    }
}

void UrlLoader::detectSettingsForLocalFiles()
{
    if (!m_url.isLocalFile()) {
        return;
    }

    m_letRequestingPartDownloadUrl = false; //If the file is local, there's no need to download it

    m_jobErrorCode = checkAccessToLocalFile(m_url.path());

    if (!m_mimeType.isEmpty()) {
        detectArchiveSettings();
        // Redirect to the url in Type=Link desktop files
        if (m_mimeType == QLatin1String("application/x-desktop")) {
            KDesktopFile df(m_url.toLocalFile());
            if (df.hasLinkType()) {
                m_url = QUrl(df.readUrl());
                m_mimeType.clear(); // to be determined again
            }
        }
    } else {
        if (QFile::exists(m_url.path())) {
            QMimeDatabase db;
            m_mimeType = db.mimeTypeForFile(m_url.path()).name();
        } else {
            //Treat the nonexisting file as a directory
            m_mimeType = QStringLiteral("inode/directory");
        }
    }
}

bool UrlLoader::shouldEmbedThis() const
{
    return !m_dontEmbed && (m_request.forceAutoEmbed || KonqFMSettings::settings()->shouldEmbed(m_mimeType));
}

void UrlLoader::embed()
{
    if (m_jobErrorCode) {
        QUrl url = m_url;
        m_url = Konq::makeErrorUrl(m_jobErrorCode, m_url.scheme(), m_url);
        m_mimeType = QStringLiteral("text/html");
        m_part = findPartById(QStringLiteral("webenginepart"));
    }
    bool embedded = m_mainWindow->openView(m_mimeType, m_url, m_view, m_request, m_originalUrl);
    if (embedded || m_embedOrNothing) {
        done();
    } else {
        decideOpenOrSave();
        performAction();
    }
}

void UrlLoader::save()
{
    QFileDialog dlg(m_mainWindow);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setWindowTitle(i18n("Save As"));
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    QString suggestedName = !m_request.suggestedFileName.isEmpty() ? m_request.suggestedFileName : m_url.fileName();
    dlg.selectFile(suggestedName);
    dlg.setDirectory(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    //NOTE: we can't use QDialog::open() because the dialog needs to be synchronous so that we can call
    //DownloaderJob::setDownloadPath()
    if (dlg.exec() == QDialog::Rejected) {
        done();
        return;
    }
    performSave(m_url, dlg.selectedUrls().value(0));
}

void UrlLoader::performSave(const QUrl& orig, const QUrl& dest)
{
    KJob *job = nullptr;
    if (m_letRequestingPartDownloadUrl) {
        getDownloaderJobFromPart();
        if (m_partDownloaderJob) {
#if QT_VERSION_MAJOR < 6
            //In KF5, DownloaderJob::startDownload doesn't connect to the signal
            connect(m_partDownloaderJob, &DownloaderJob::downloadResult, this, [this](DownloaderJob *j){done(j);});
#endif
            m_partDownloaderJob->startDownload(dest.path(), m_mainWindow, this, [this](DownloaderJob *j){done(j);});
            return;
        }
    }
    job = KIO::file_copy(orig, dest, -1, KIO::Overwrite);
    if (!job) {
        done();
        return;
    }
    KJobWidgets::setWindow(job, m_mainWindow);
    KJobTrackerInterface *t = KIO::getJobTracker();
    if (t) {
        t->registerJob(job);
    }
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    connect(job, &KJob::finished, this, [this, job](){done(job);});
    job->start();
}

void UrlLoader::open()
{
    // Prevention against user stupidity : if the associated app for this mimetype
    // is konqueror/kfmclient, then we'll loop forever.
    if (m_service && serviceIsKonqueror(m_service) && m_mainWindow->refuseExecutingKonqueror(m_mimeType)) {
        return;
    }
    if (m_jobErrorCode != 0) {
        done();
        return;
    }

    KJob *job = nullptr;
    KIO::ApplicationLauncherJob *j = new KIO::ApplicationLauncherJob(m_service);
    j->setUrls({m_url});
    j->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    if (m_request.tempFile) {
        j->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
    }
    job = j;
    connect(job, &KJob::finished, this, [this, job](){done(job);});
    job->start();
}

void UrlLoader::execute()
{
    m_openUrlJob = new KIO::OpenUrlJob(m_url, m_mimeType, this);
    m_openUrlJob->setEnableExternalBrowser(false);
    m_openUrlJob->setRunExecutables(true);
    m_openUrlJob->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    m_openUrlJob->setSuggestedFileName(m_request.suggestedFileName);
    m_openUrlJob->setDeleteTemporaryFile(m_request.tempFile);
    connect(m_openUrlJob, &KJob::finished, this, [this]{done(m_openUrlJob);});
    m_openUrlJob->start();
}

//Copied from KParts::BrowserRun::isTextExecutable
bool UrlLoader::isTextExecutable(const QString &mimeType)
{
    return ( mimeType == QLatin1String("application/x-desktop") || mimeType == QLatin1String("application/x-shellscript"));
}

UrlLoader::OpenSaveAnswer UrlLoader::askSaveOrOpen(OpenEmbedMode mode) const
{
    BrowserOpenOrSaveQuestion dlg(m_mainWindow, m_url, m_mimeType);
    dlg.setSuggestedFileName(m_request.suggestedFileName);
    dlg.setFeatures(BrowserOpenOrSaveQuestion::ServiceSelection);
    BrowserOpenOrSaveQuestion::Result ans = mode == OpenEmbedMode::Open ? dlg.askOpenOrSave() : dlg.askEmbedOrSave();
    OpenUrlAction action;
    switch (ans) {
        case BrowserOpenOrSaveQuestion::Save:
            action = OpenUrlAction::Save;
            break;
        case BrowserOpenOrSaveQuestion::Open:
            action = OpenUrlAction::Open;
            break;
        case BrowserOpenOrSaveQuestion::Embed:
            action = OpenUrlAction::Embed;
            break;
        default:
            action = OpenUrlAction::DoNothing;
    }
    return qMakePair(action, dlg.selectedService());
}

QString UrlLoader::partForLocalFile(const QString& path)
{
    QMimeDatabase db;
    QString mimetype = db.mimeTypeForFile(path).name();

    KPluginMetaData plugin = preferredPart(mimetype);
    return plugin.pluginId();
}

UrlLoader::ViewToUse UrlLoader::viewToUse() const
{
    if (m_view && m_view->isFollowActive()) {
        return ViewToUse::CurrentView;
    }

    if (!m_view && !m_request.browserArgs.newTab()) {
        return ViewToUse::CurrentView;
    } else if (!m_view && m_request.browserArgs.newTab()) {
        return ViewToUse::NewTab;
    }
    return ViewToUse::View;
}

void UrlLoader::jobFinished(KJob* job)
{
    m_jobErrorCode = job->error();
}

QDebug operator<<(QDebug dbg, UrlLoader::OpenUrlAction action)
{
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    switch (action) {
        case UrlLoader::OpenUrlAction::UnknwonAction:
            dbg << "UnknownAction";
            break;
        case UrlLoader::OpenUrlAction::DoNothing:
            dbg << "DoNothing";
            break;
        case UrlLoader::OpenUrlAction::Save:
            dbg << "Save";
            break;
        case UrlLoader::OpenUrlAction::Embed:
            dbg << "Embed";
            break;
        case UrlLoader::OpenUrlAction::Open:
            dbg << "Open";
            break;
        case UrlLoader::OpenUrlAction::Execute:
            dbg << "Execute";
            break;
    }
    return dbg;
}

