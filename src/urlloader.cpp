/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "urlloader.h"
#include "konqembedsettings.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include "konqurl.h"
#include "konqdebug.h"
#include "konqmainwindowfactory.h"

#include "libkonq_utils.h"

#include "interfaces/downloadjob.h"

#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KIO/FileCopyJob>
#include <KIO/MimeTypeFinderJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/CopyJob>
#include <KIO/JobTracker>
#include <KMessageBox>
#include <KParts/ReadOnlyPart>
#include <KParts/NavigationExtension>
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
using namespace Konq;

UrlLoader::UrlLoader(KonqMainWindow *mainWindow, KonqView *view, const QUrl &url, const QString &mimeType, const KonqOpenURLRequest &req, bool trustedSource):
    QObject(mainWindow),
    m_mainWindow(mainWindow),
    m_url(url),
    m_mimeType(mimeType),
    m_request(req),
    m_view(view),
    m_trustedSource(trustedSource),
    m_protocolAllowsReading(KProtocolManager::supportsReading(m_url)), // If the protocol is unknown, assume it doesn't allow reading
    m_useDownloadJob(req.browserArgs.downloadJob()),
    m_downloadJob(req.browserArgs.downloadJob()),
    m_allowedActions{m_request.urlActions()}
{
    m_request.suggestedFileName = m_request.browserArgs.suggestedDownloadName();

    QString requestedEmbeddingPart = m_request.browserArgs.embedWith();
    if (!requestedEmbeddingPart.isEmpty()) {
        m_request.serviceName = requestedEmbeddingPart;
    }

    if (m_useDownloadJob) {
        m_originalUrl = m_url;
        m_downloadJob->setParent(this);
    }
    determineStartingMimetype();
    m_ignoreDefaultHtmlPart = m_request.browserArgs.ignoreDefaultHtmlPart();
}

UrlLoader::~UrlLoader()
{
}

void UrlLoader::updateAllowedActions()
{
    if (m_allowedActions.isAllowed(UrlAction::Execute) && !isUrlExecutable()) {
        m_allowedActions.allow(UrlAction::Execute, false);
    }

    if (m_allowedActions.isAllowed(UrlAction::Embed)) {
        if (!m_part.isValid()) {
            m_allowedActions.allow(UrlAction::Embed, false);
        } else if (isViewLocked()) {
        //If the view is locked, assume, we need to embed the URL. Ideally, this should
        //be done
            m_request.forceEmbed();
        }
    }

    if (!m_protocolAllowsReading && !isMimeTypeKnown(m_mimeType) && m_allowedActions.isAllowed(Konq::UrlAction::Open)) {
        m_allowedActions.force(Konq::UrlAction::Open);
    }

    if (serviceIsKonqueror(m_service)) {
        m_allowedActions.allow(UrlAction::Open, false);
    }

}

bool UrlLoader::can(OpenUrlAction action) const
{
    return m_allowedActions.isAllowed(action);
}

void UrlLoader::determineStartingMimetype()
{
    QMimeDatabase db;
    if (!m_mimeType.isEmpty()) {
        if (m_useDownloadJob || !db.mimeTypeForName(m_mimeType).isDefault()) {
            return;
        }
    }

    m_mimeType = m_request.args.mimeType();
    //In theory, this should never happen, as parts requesting to download the URL
    //by themselves should already have set the mimetype. However, let's check, just in case
    if (m_useDownloadJob && m_mimeType.isEmpty()) {
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


void UrlLoader::findService()
{
    if (!can(UrlAction::Open)) {
        m_service = nullptr;
        return;
    }
    QString openWith = m_request.browserArgs.openWith();
    if (!openWith.isEmpty()) {
        m_service = KService::serviceByStorageId(openWith);
    }
    if (!m_service) {
        m_service = KApplicationTrader::preferredService(m_mimeType);
    }
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

    m_isAsync = m_protocolAllowsReading && (!isMimeTypeKnown(m_mimeType) || m_useDownloadJob);
}

bool UrlLoader::isViewLocked() const
{
    return m_view && m_view->isLockedLocation();
}

void UrlLoader::decideAction()
{
    if (hasError()) {
        m_request.forceEmbed();
        return;
    }

    if (isMimeTypeKnown(m_mimeType)) {
        m_part = findEmbeddingPart();
        findService();
    }
    updateAllowedActions();

    decideExecute();
    switch (m_request.chosenAction) {
        case OpenUrlAction::Execute:
            m_ready = true;
            break;
        case OpenUrlAction::DoNothing:
            m_ready = true;
            return;
        default:
            decideEmbedOpenOrSave();
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
        m_ready = !m_useDownloadJob;
        performAction();
    }
}

KPluginMetaData UrlLoader::findEmbeddingPart(bool forceServiceName) const
{
    const QLatin1String webEngineName("webenginepart");

    // Use WebEnginePart for konq: URLs even if it's not the default HTML engine
    if (KonqUrl::hasKonqScheme(m_url)) {
        return findPartById(webEngineName);
    }

    KPluginMetaData part;

    // If the service name has been set by the "--part" command line
    // argument via KonquerorApplication::createWindowsForUrlArguments(),
    // then use it as is.  It will only be set for the first call with
    // that command invocation.
    //
    // This test must be performed before the 'existing view' test below,
    // because at this point the view showing the "konq:blank" page already
    // exists.
    if (!m_request.serviceName.isEmpty()) {
        part = findPartById(m_request.serviceName);
    }

    // Check whether the current view can display the MIME type, but only if
    // the URL hasn't been explicitly typed by the user: in this case, use the
    // preferred service. This is needed to avoid the situation where m_view
    // is a Kate part, the user enters the URL of a web page, and the page is
    // opened within the Kate part because it can handle HTML files.
    if (!part.isValid() && m_view && m_request.typedUrl.isEmpty() && m_view->supportsMimeType(m_mimeType)) {
        part = m_view->service();
    }

    // If the part does not support the required MIME type, then it cannot be
    // used. The only exception is when we are forced to use the given service.
    if (part.isValid() && !forceServiceName && !part.supportsMimeType(m_mimeType)) {
        part = KPluginMetaData();
    }

    if (!part.isValid()) {
        // Now, if no part has yet been resolved, then use the preferred part
        // for the MIME type.
        part = preferredPart(m_mimeType);
    }

    /* Corner case: webenginepart can't determine mimetype (gives application/octet-stream) but
     * OpenUrlJob determines a mimetype supported by WebEnginePart (for example application/xml):
     * if the preferred part is webenginepart, we'd get an endless loop because webenginepart will
     * call again this. To avoid this, if the preferred service is webenginepart and m_ignoreDefaultHtmlPart
     * is true, use the second preferred service (if any); otherwise return false. This will offer the user
     * the option to open or save, instead.
     *
     * This can also happen if the URL was opened from a link with the "download" attribute or with a
     * "CONTENT-DISPOSITION: attachment" header. In these cases, WebEnginePart will always refuse to open
     * the URL and will ask Konqueror to download it. However, if the preferred part for the URL mimetype is
     * WebEnginePart itself, this would lead to an endless loop. This check avoids it
     */
    if (m_ignoreDefaultHtmlPart && m_part.pluginId() == webEngineName) {
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

static UrlLoader::OpenUrlAction actionFromDialogResult(DownloadActionQuestion::Action res)
{
    switch (res) {
        case DownloadActionQuestion::Action::Cancel:
            return UrlLoader::OpenUrlAction::DoNothing;
        case DownloadActionQuestion::Action::Save:
            return UrlLoader::OpenUrlAction::Save;
        case DownloadActionQuestion::Action::Embed:
            return UrlLoader::OpenUrlAction::Embed;
        case DownloadActionQuestion::Action::Open:
            return UrlLoader::OpenUrlAction::Open;
    }
    //We can't really reach this point, but it's needed to avoid a compiler warning
    return UrlLoader::OpenUrlAction::UnknownAction;
}

void UrlLoader::decideEmbedOpenOrSave()
{
    m_request.chosenAction = m_allowedActions.forcedAction();
    if (m_request.chosenAction == OpenUrlAction::UnknownAction) {
        askEmbedSaveOrOpen();
    }

    //If the URL should be opened in an external application and it should be downloaded using a download job
    //ensure it's not downloaded in Konqueror's temporary directory but in the global temporary directory,
    //otherwise if the user closes Konqueror before closing the application, there could be issues because the
    //external application wouldn't find the file anymore.
    //Problem: if the application doesn't support the --tempfile switch, the file will remain even after both Konqueror
    //and the external application are closed and will only be removed automatically if the temporary directory is deleted
    //or emptied.
    DownloadJob *job = m_request.browserArgs.downloadJob();
    if (m_useDownloadJob && m_request.chosenAction == OpenUrlAction::Open && job) {
        QString fileName = QFileInfo(job->downloadPath()).fileName();
        fileName = generateUniqueFileName(fileName, QDir::temp().path());
        job->setDownloadPath(QDir::temp().filePath(fileName));
    }

    m_ready = m_request.chosenAction != OpenUrlAction::Embed || m_part.isValid();
}

void UrlLoader::askEmbedSaveOrOpen()
{
    using Result = DownloadActionQuestion::Action;
    DownloadActionQuestion dlg(m_mainWindow, m_url, m_mimeType, m_ignoreDefaultHtmlPart);
    dlg.setSuggestedFileName(m_request.suggestedFileName);
    Result saveMode = can(OpenUrlAction::Save) ? Result::Save : Result::Cancel;
    Result openMode = can(OpenUrlAction::Open) ? Result::Open : Result::Cancel;
    Result embedMode = can(OpenUrlAction::Embed) ? Result::Embed : Result::Cancel;

    DownloadActionQuestion::EmbedFlags flag = m_request.browserArgs.forceShowActionDialog() ? DownloadActionQuestion::ForceDialog : DownloadActionQuestion::InlineDisposition;
    DownloadActionQuestion::Action ans = dlg.ask(saveMode|embedMode|openMode, flag);

    m_request.chosenAction = actionFromDialogResult(ans);
    if (m_request.chosenAction == OpenUrlAction::Embed && (m_request.browserArgs.embedWith().isEmpty() || dlg.dialogShown())) {
        //Override the part in browserArgs only if the user was shown a dialog (meaning he explicitly chose a part)
        m_part = dlg.selectedPart();
        m_request.serviceName = m_part.pluginId();
    } else if (m_request.chosenAction == OpenUrlAction::Open && (!m_request.browserArgs.openWith().isEmpty() || dlg.dialogShown())) {
        //Override the service in browserArgs only if the user was shown a dialog (meaning he explicitly chose a service)
        m_service = dlg.selectedService();
    }
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


void UrlLoader::decideExecute()
{
    if (!can(UrlAction::Execute)) {
        return;
    }

    //We don't want to execute when we are reloading the file is visible in the current part
    //(the file is visible in the current part, so we know the user wanted to display it) unless
    //for, some reasons, we're forced to do so.
    if (m_request.args.reload() && !m_allowedActions.isForced(UrlAction::Execute)) {
        return;
    }

    UrlAction alternativeAction;
    if (can(UrlAction::Embed)) {
        alternativeAction = UrlAction::Embed;
    } else if (can(UrlAction::Open)) {
        alternativeAction = UrlAction::Open;
    } else {
        alternativeAction = UrlAction::DoNothing;
    }

    KMessageBox::ButtonCode code;
    KGuiItem executeGuiItem(i18nc("Execute an executable file", "Execute"),
                            QIcon::fromTheme(QStringLiteral("system-run")));
    QString displayIconName = alternativeAction == UrlAction::Embed ? QStringLiteral("document-preview") : QStringLiteral("document-open");
    KGuiItem displayGuiItem(i18nc("Display an executable file inside Konqueror", "Display"),
                            QIcon::fromTheme(displayIconName));
    QString dontShowAgainId(QLatin1String("AskExecuting")+m_mimeType);

    if (alternativeAction != UrlAction::DoNothing) {
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
            m_request.chosenAction = OpenUrlAction::Execute;
            break;
        case KMessageBox::Cancel:
            m_request.chosenAction = OpenUrlAction::DoNothing;
            break;
        case KMessageBox::SecondaryAction:
            //The "No" button actually corresponds to the "Cancel" action if the file can't be displayed
            m_request.chosenAction = alternativeAction;
        default: //This point can't be reached. This is here only to avoid a compiler warning
            return;
    }
}

void UrlLoader::performAction()
{
    //If we still aren't ready, it means that we need to use a download job.
    //Do this only when opening or embedding, however, since when
    //saving we first need to ask the user where to save. save() will launch the
    //job itself.
    //When the job has finished downloading the URL, the slot connected with its
    //result() signal will set m_redy to true, then call this function again
    if (!m_ready && (m_request.chosenAction == OpenUrlAction::Embed || m_request.chosenAction == OpenUrlAction::Open)) {
        downloadForEmbeddingOrOpening();
        return;
    }
    switch (m_request.chosenAction) {
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
        case OpenUrlAction::UnknownAction: //This should never happen
            done();
            break;
    }
}

void UrlLoader::downloadForEmbeddingOrOpening()
{
    //This shouldn't happen
    if (!m_downloadJob) {
        done();
        return;
    }
    m_downloadJob->setIntent(m_request.chosenAction == UrlAction::Embed ? DownloadJob::Embed : DownloadJob::Open);
    connect(m_downloadJob, &DownloadJob::downloadResult, this, &UrlLoader::jobFinished);
    m_downloadJob->startDownload(m_mainWindow, this, &UrlLoader::downloadForEmbeddingOrOpeningDone);
}

void UrlLoader::downloadForEmbeddingOrOpeningDone(KonqInterfaces::DownloadJob *job, const QUrl &url)
{
    if (job && job->error() == 0) {
        m_url = url;
        m_ready = true;
        m_request.tempFile = true;
    } else if (!job || job->error() == KIO::ERR_USER_CANCELED) {
        m_request.chosenAction = OpenUrlAction::DoNothing;
        m_ready = true;
    }
    checkDownloadedMimetype();
    performAction();
}

void UrlLoader::checkDownloadedMimetype()
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(m_url.path());
    QString typeName = type.name();
    //Don't override the mimetype with the default one
    if (type.isDefault() || typeName == m_mimeType) {
        return;
    }
    m_mimeType = typeName;
    //The URL is a local file now, so there are no problems in opening it with WebEnginePart
    m_ignoreDefaultHtmlPart = false;
    if (shouldEmbedThis()) {
        m_part = findEmbeddingPart(false);
        if (m_part.isValid()) {
            m_request.chosenAction = OpenUrlAction::Embed;
            return;
        }
    }
    m_request.chosenAction = OpenUrlAction::Open;
    if (m_service && m_service->hasMimeType(m_mimeType)) {
        return;
    }
    m_service = KApplicationTrader::preferredService(m_mimeType);
}

void UrlLoader::done(KJob *job)
{
    //Ensure that m_mimeType and m_request.args.mimeType are equal, since it's not clear what will be used
    m_request.args.setMimeType(m_mimeType);

    //Update the allowed actions
    m_request.browserArgs.setAllowedUrlActions(m_allowedActions);

    if (job) {
        jobFinished(job);
    }
    emit finished(this);
    //If we reach here and m_partDownloadJob->finished() is false, it means the job hasn't been started in the first place,
    //(because the user canceled the download), so kill it
    if (m_downloadJob && !m_downloadJob->finished()) {
        m_downloadJob->kill();
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
        m_url = makeErrorUrl(m_jobErrorCode, m_mimeTypeFinderJob->errorString(), m_url);
        m_mimeType = QStringLiteral("text/html");
        m_request.chosenAction = OpenUrlAction::Embed;
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
    if (m_ignoreDefaultHtmlPart || isMimeTypeKnown(m_mimeType)) {
        return false;
    }
    const QVector<QString> webengineSchemes = {QStringLiteral("error"), QStringLiteral("konq"), QStringLiteral("data")};
    return m_url.scheme().startsWith(QStringLiteral("http")) || webengineSchemes.contains(m_url.scheme());
}

void UrlLoader::detectSettingsForRemoteFiles()
{
    if (m_url.isLocalFile()) {
        return;
    }

    if (m_url.scheme() == QLatin1String("error")) {
        m_useDownloadJob = false; //error URLs can never be downloaded
        m_mimeType = QLatin1String("text/html");
        m_request.args.setMimeType(QStringLiteral("text/html"));
    }
    else if (shouldUseDefaultHttpMimeype()) {
        // If a part which supports html asked to download the URL, it means it's not html, so don't change its mimetype
        if (m_useDownloadJob && m_part.supportsMimeType(QStringLiteral("text/html"))) {
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

    m_useDownloadJob = false; //If the file is local, there's no need to download it

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
    if (!can(OpenUrlAction::Embed)) {
        return false;
    } else {
        return  m_allowedActions.isForced(OpenUrlAction::Embed) || KonqFMSettings::settings()->shouldEmbed(m_mimeType);
    }
}

void UrlLoader::embed()
{
    if (m_jobErrorCode) {
        QUrl url = m_url;
        m_url = makeErrorUrl(m_jobErrorCode, m_url.scheme(), m_url);
        m_mimeType = QStringLiteral("text/html");
        m_part = findPartById(QStringLiteral("webenginepart"));
    }
    bool embedded = m_mainWindow->openView(m_mimeType, m_url, m_view, m_request, m_originalUrl);
    if (embedded) {
        done();
    } else {
        m_allowedActions.allow(UrlAction::Embed, false);
        m_request.serviceName = {};
        //This is an unexpected situation, so enable open and save action even if they hadn't been originally requested
        m_allowedActions.allow(Konq::UrlAction::Open, true);
        m_allowedActions.allow(Konq::UrlAction::Save, true);
        m_part = {};
        decideEmbedOpenOrSave();
        performAction();
    }
}

QString UrlLoader::startingSaveDir() const
{
    QString startDir = m_mainWindow->saveDir();
    if (startDir.isEmpty()) {
        //TODO: find a better way to store the last save dir
        QString lastSaveDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        QVariant lastSaveDirData = m_mainWindow->property(s_lastSaveDirProperty);
        if (lastSaveDirData.isValid()) {
            startDir = lastSaveDirData.toString();
        }
    }
    return startDir;
}

void UrlLoader::save()
{
    QFileDialog dlg(m_mainWindow);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setWindowTitle(i18n("Save As"));
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    QString suggestedName = !m_request.suggestedFileName.isEmpty() ? m_request.suggestedFileName : m_url.fileName();
    dlg.selectFile(suggestedName);
    dlg.setDirectory(startingSaveDir());

    //NOTE: we can't use QDialog::open() because the dialog needs to be synchronous so that we can call
    //DownloadJob::setDownloadPath()
    if (dlg.exec() == QDialog::Rejected) {
        done();
        return;
    }
    QUrl chosenUrl = dlg.selectedUrls().value(0);
    m_mainWindow->setProperty(s_lastSaveDirProperty, QFileInfo(chosenUrl.path()).absoluteDir().path());
    performSave(m_url, chosenUrl);
}

void UrlLoader::performSave(const QUrl& orig, const QUrl& dest)
{
    if (m_useDownloadJob) {
        KonqInterfaces::DownloadJob *dj = m_request.browserArgs.downloadJob();
        if (dj) {
            dj->setIntent(DownloadJob::Save);
            dj->startDownload(dest.path(), m_mainWindow, this, [this](DownloadJob *j){done(j);});
            return;
        }
    }

    KJob *job = KIO::file_copy(orig, dest, -1, KIO::Overwrite);
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
    //If the user chose to open the URL in Konqueror itself, check whether it can be
    //embedded and, if so, create a new window and force embedding the URL there.
    //This avoids an endless loop while ensuring that choosing Konqueror as external
    //application for a supported type works correctly.
    //TODO refactor this: there should be a way to tell the main window that the user chose to
    //open the URL in another instance of Konqueror and let the main window handle it
    if (m_service && serviceIsKonqueror(m_service) && m_mainWindow->activeViewsNotLockedCount() > 0) {
        if (preferredPart(mimeType()).isValid()) {
            m_request.forceEmbed();
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(m_url, m_request);
            mw->show();
        } else {
            KMessageBox::error(m_mainWindow, i18n("There appears to be a configuration error. You have associated Konqueror with %1, but it cannot handle this file type.", m_mimeType));
        }
        done();
        return;
    }
    if (m_jobErrorCode != 0) {
        done();
        return;
    }

    KJob *job = nullptr;
    if (m_service) {
        KIO::ApplicationLauncherJob *j = new KIO::ApplicationLauncherJob(m_service);
        j->setUrls({m_url});
        if (m_request.tempFile) {
            j->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
        }
        job = j;
    } else {
        KIO::OpenUrlJob *j = new KIO::OpenUrlJob(m_url, m_mimeType);
        j->setRunExecutables(false);
        job = j;
    }
    job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
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
