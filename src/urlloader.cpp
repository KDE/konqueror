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

#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KMessageBox>
#include <KParts/ReadOnlyPart>
#include <KParts/BrowserInterface>
#include <KParts/BrowserExtension>
#include <KParts/PartLoader>
#include <KIO/FileCopyJob>
#include <KIO/MimeTypeFinderJob>
#include <KJobWidgets>
#include <KProtocolManager>
#include <KDesktopFile>
#include <KApplicationTrader>
#include <KParts/PartLoader>
#include <KLocalizedString>
#include <KIO/JobUiDelegateFactory>

#include <QDebug>
#include <QArgument>
#include <QWebEngineProfile>
#include <QMimeDatabase>
#include <QWebEngineProfile>
#include <QFileDialog>
#include <QFileInfo>

bool UrlLoader::embedWithoutAskingToSave(const QString &mimeType)
{
    static QStringList s_mimeTypes;
    if (s_mimeTypes.isEmpty()) {
        QStringList names{QStringLiteral("kfmclient_html"), QStringLiteral("kfmclient_dir"), QStringLiteral("kfmclient_war")};
        for (const QString &name : names) {
            KPluginMetaData md = findPartById(name);
            s_mimeTypes.append(md.mimeTypes());
        }
        //The user may want to save xml files rather than embedding them
        //TODO: is there a better way to do this?
        s_mimeTypes.removeOne(QStringLiteral("application/xml"));
    }
    return s_mimeTypes.contains(mimeType);
}

bool UrlLoader::isExecutable(const QString& mimeType)
{
    return KParts::BrowserRun::isExecutable(mimeType);
}

UrlLoader::UrlLoader(KonqMainWindow *mainWindow, KonqView *view, const QUrl &url, const QString &mimeType, const KonqOpenURLRequest &req, bool trustedSource, bool dontEmbed):
    QObject(mainWindow), m_mainWindow(mainWindow), m_url(url), m_mimeType(mimeType), m_request(req), m_view(view), m_trustedSource(trustedSource), m_dontEmbed(dontEmbed),
    m_jobErrorCode(0), m_protocolAllowsReading(KProtocolManager::supportsReading(m_url))
{
    if (!isMimeTypeKnown(m_mimeType)) {
        m_mimeType = m_request.args.mimeType();
    }
    m_dontPassToWebEnginePart = m_request.args.metaData().contains("DontSendToDefaultHTMLPart");
}

UrlLoader::~UrlLoader()
{
}

QString UrlLoader::mimeType() const
{
    return m_mimeType;
}

bool UrlLoader::isMimeTypeKnown(const QString &mimeType)
{
    return !mimeType.isEmpty() && mimeType != QLatin1String("application/octet-stream");
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

    m_isAsync = m_protocolAllowsReading && !isMimeTypeKnown(m_mimeType);
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
    m_action = decideExecute();
    switch (m_action) {
        case OpenUrlAction::Execute:
            m_ready = true;
            break;
        case OpenUrlAction::DoNothing:
            m_ready = true;
            return;
        default:
            if (m_mimeType.isEmpty() && !m_protocolAllowsReading) {
                //If the protocol doesn't allow reading and we don't have a mimetype associated with it,
                //use the Open action, as we most likely won't be able to find out the mimetype. This is
                //what happens, for example, for mailto URLs
                m_action = OpenUrlAction::Open;
                return;
            } else if (isViewLocked() || shouldEmbedThis()) {
                bool success = decideEmbedOrSave();
                if (success) {
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
    if (m_isAsync) {
        launchMimeTypeFinderJob();
    } else {
        decideAction();
        m_ready = true;
        performAction();
    }
}

bool UrlLoader::decideEmbedOrSave()
{
    const QLatin1String webEngineName("webenginepart");

    //Use WebEnginePart for konq: URLs even if it's not the default html engine
    if (KonqUrl::hasKonqScheme(m_url)) {
        m_part = findPartById(webEngineName);
    } else {
        //Check whether the view can display the mimetype, but only if the URL hasn't been explicitly
        //typed by the user: in this case, use the preferred service. This is needed to avoid the situation
        //where m_view is a Kate part, the user enters the URL of a web page and the page is opened within
        //the Kate part because it can handle html files.
        if (m_view && m_request.typedUrl.isEmpty() && m_view->supportsMimeType(m_mimeType)) {
            m_part = m_view->service();
        } else {
            if (!m_request.serviceName.isEmpty()) {
                // If the service name has been set by the "--part" command line argument
                // (detected in handleCommandLine() in konqmain.cpp), then use it as is.
                m_part = findPartById(m_request.serviceName);
            } else {
                // Otherwise, use the preferred service for the MIME type.
                m_part = preferredPart(m_mimeType);
            }
        }
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
            m_part = *partToUse;
        } else {
            m_part = KPluginMetaData();
        }
    }

    //If we can't find a service, return false, so that the caller can use decideOpenOrSave to allow the
    //user the possibility of opening the file, since embedding wasn't possibile
    if (!m_part.isValid()) {
        return false;
    }

    //Ask whether to save or embed, except in the following cases:
    //- it's a web page: always embed
    //- it's a local file: always embed
    if (embedWithoutAskingToSave(m_mimeType) || m_url.isLocalFile()) {
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
    m_service = answerWithService.second;
}

UrlLoader::OpenUrlAction UrlLoader::decideExecute() const {
    if (!m_url.isLocalFile() || !KRun::isExecutable(m_mimeType)) {
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

void UrlLoader::done(KJob *job)
{
    //Ensure that m_mimeType and m_request.args.mimeType are equal, since it's not clear what will be used
    m_request.args.setMimeType(m_mimeType);
    if (job) {
        jobFinished(job);
    }
    emit finished(this);
    deleteLater();
}


bool UrlLoader::serviceIsKonqueror(KService::Ptr service)
{
    return service && (service->desktopEntryName() == QLatin1String("konqueror") || service->exec().trimmed() == QLatin1String("konqueror") || service->exec().trimmed().startsWith(QLatin1String("kfmclient")));
}

void UrlLoader::launchMimeTypeFinderJob()
{
    m_mimeTypeFinderJob = new KIO::MimeTypeFinderJob(m_url, this);
    m_mimeTypeFinderJob->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    m_mimeTypeFinderJob->setSuggestedFileName(m_request.suggestedFileName);
    connect(m_mimeTypeFinderJob, &KIO::MimeTypeFinderJob::result, this, [this](KJob*){mimetypeDeterminedByJob();});
    m_mimeTypeFinderJob->start();
}

void UrlLoader::mimetypeDeterminedByJob()
{
    if (!m_mimeTypeFinderJob->error()) {
        m_mimeType=m_mimeTypeFinderJob->mimeType();
        //Only check whether the URL represents an archive when it is a local file. This can be either because
        //QUrl::isLocalFile returns true or because its scheme corresponds to a protocol with class :local
        //(for example, tar)
        if (m_url.isLocalFile() || KProtocolInfo::protocolClass(m_url.scheme()) == QLatin1String(":local")) {
            detectArchiveSettings();
        }
        decideAction();
    } else {
        m_jobErrorCode = m_mimeTypeFinderJob->error();
        m_url = KParts::BrowserRun::makeErrorUrl(m_jobErrorCode, m_mimeTypeFinderJob->errorString(), m_url);
        m_mimeType = QStringLiteral("text/html");
        m_action = OpenUrlAction::Embed;
    }
    performAction();
}

bool UrlLoader::shouldUseDefaultHttpMimeype() const
{
    const QVector<QString> webengineSchemes = {"error", "konq"};
    if (m_dontPassToWebEnginePart || isMimeTypeKnown(m_mimeType)) {
        return false;
    } else if (m_url.scheme().startsWith(QStringLiteral("http")) || webengineSchemes.contains(m_url.scheme())) {
        return true;
    } else {
        return false;
    }
}

void UrlLoader::detectSettingsForRemoteFiles()
{
    if (m_url.isLocalFile()) {
        return;
    }
    if (shouldUseDefaultHttpMimeype()) {
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
        m_url = KParts::BrowserRun::makeErrorUrl(m_jobErrorCode, m_url.scheme(), m_url);
        m_mimeType = QStringLiteral("text/html");
        m_part = findPartById(QStringLiteral("webenginepart"));
    }
    bool embedded = m_mainWindow->openView(m_mimeType, m_url, m_view, m_request);
    if (embedded) {
        done();
    } else {
        decideOpenOrSave();
        performAction();
    }
}

void UrlLoader::save()
{
    QFileDialog *dlg = new QFileDialog(m_mainWindow);
    dlg->setAcceptMode(QFileDialog::AcceptSave);
    dlg->setWindowTitle(i18n("Save As"));
    dlg->setOption(QFileDialog::DontConfirmOverwrite, false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    QString suggestedName = !m_request.suggestedFileName.isEmpty() ? m_request.suggestedFileName : m_url.fileName();
    dlg->selectFile(suggestedName);
    dlg->setDirectory(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    auto savePrc = [this, dlg](){
        QUrl dest = dlg->selectedUrls().value(0);
        if (dest.isValid()) {
            saveUrlUsingKIO(m_url, dest);
        }
    };
    connect(dlg, &QDialog::accepted, dlg, savePrc);
    dlg->show();
}

void UrlLoader::saveUrlUsingKIO(const QUrl& orig, const QUrl& dest)
{
    KIO::FileCopyJob *job = KIO::file_copy(orig, dest, -1, KIO::Overwrite);
    KJobWidgets::setWindow(job, m_mainWindow);
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
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
    KParts::BrowserOpenOrSaveQuestion dlg(m_mainWindow, m_url, m_mimeType);
    dlg.setSuggestedFileName(m_request.suggestedFileName);
    dlg.setFeatures(KParts::BrowserOpenOrSaveQuestion::ServiceSelection);
    KParts::BrowserOpenOrSaveQuestion::Result ans = mode == OpenEmbedMode::Open ? dlg.askOpenOrSave() : dlg.askEmbedOrSave();
    OpenUrlAction action;
    switch (ans) {
        case KParts::BrowserOpenOrSaveQuestion::Save:
            action = OpenUrlAction::Save;
            break;
        case KParts::BrowserOpenOrSaveQuestion::Open:
            action = OpenUrlAction::Open;
            break;
        case KParts::BrowserOpenOrSaveQuestion::Embed:
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
