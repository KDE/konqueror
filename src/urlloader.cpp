/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "urlloader.h"
#include "konqsettings.h"
#include "konqmainwindow.h"
#include "konqview.h"

#include <KIO/OpenUrlJob>
#include <KIO/JobUiDelegate>
#include <KMimeTypeTrader>
#include <KMessageBox>
#include <KParts/ReadOnlyPart>
#include <KParts/BrowserInterface>
#include <KParts/BrowserExtension>
#include <KParts/PartLoader>
#include <KParts/BrowserOpenOrSaveQuestion>
#include <KIO/FileCopyJob>
#include <KJobWidgets>
#include <KProtocolManager>
#include <KDesktopFile>
#include <KApplicationTrader>

#include <QDebug>
#include <QArgument>
#include <QWebEngineProfile>
#include <QMimeDatabase>
#include <QWebEngineProfile>
#include <QFileDialog>

bool UrlLoader::isExecutable(const QString& mimeType)
{
    return KParts::BrowserRun::isExecutable(mimeType);
}

UrlLoader::UrlLoader(KonqMainWindow *mainWindow, KonqView *view, const QUrl &url, const QString &mimeType, const KonqOpenURLRequest &req, bool trustedSource, bool dontEmbed):
    QObject(mainWindow), m_mainWindow(mainWindow), m_url(url), m_mimeType(mimeType), m_request(req), m_view(view), m_trustedSource(trustedSource), m_dontEmbed(dontEmbed)
{
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
    m_request.browserArgs.setNewTab(false);
}


void UrlLoader::start()
{
    if (m_url.isLocalFile()) {
        detectSettingsForLocalFiles();
    } else {
        detectSettingsForRemoteFiles();
    }
    qDebug()<< m_url << m_mimeType;

    if (!m_mimeType.isEmpty()) {
        KService::Ptr preferredService = KApplicationTrader::preferredService(m_mimeType);
        if (serviceIsKonqueror(preferredService)) {
            m_request.forceAutoEmbed = true;
        }
    }

    qDebug() << "Should embed" << m_url << "?" << shouldEmbedThis();
    qDebug() << "Is mimetype for" << m_url << "kown?" << isMimeTypeKnown(m_mimeType);

    if (isMimeTypeKnown(m_mimeType)) {
        if (shouldEmbedThis()) {
            decideEmbedOrSave();
        } else {
            qDebug() << "Calling decideOpenOrSave for" << m_url;
            decideOpenOrSave();
        }
    } else {
        m_isAsync = true;
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
    if (m_ready) {
        performAction();
    } else {
        launchOpenUrlJob(true);
    }
}

void UrlLoader::decideEmbedOrSave()
{
    KPluginMetaData md = KParts::PartLoader::partsForMimeType(m_mimeType).first();
    m_action = OpenUrlAction::Embed;

    //Ask whether to save or embed, except in the following cases:
    //- it's a web page: always embed
    //- there's no part to open it: always save
    //NOTE: action is set to Embed on creation, so there's no need to change it when embedding
    //TODO Remove KonqRun: is there a way to do this without hardcoding mimetypes?
    if (m_mimeType != QLatin1String("text/html") && m_mimeType != QLatin1String("application/xhtml+xml")) {
        if (md.isValid()) {
            KParts::BrowserOpenOrSaveQuestion::Result answer = askSaveOrOpen(OpenEmbedMode::Embed).first;
            if (answer == KParts::BrowserOpenOrSaveQuestion::Cancel) {
                m_action = OpenUrlAction::DoNothing;
            } else if (answer == KParts::BrowserOpenOrSaveQuestion::Save) {
                m_action = OpenUrlAction::Save;
            }
        } else {
            m_action = OpenUrlAction::Save;
        }
    }

    if (m_action == OpenUrlAction::Embed) {
        if (m_view && m_request.typedUrl.isEmpty() && m_view->supportsMimeType(m_mimeType)) {
            m_service = m_view->service();
        } else {
            //TODO Remove KonqRun: replace the following line with the commented out line below it as soon as I can find out how to create a service from a KPluginMetaData
            m_service = KMimeTypeTrader::self()->preferredService(m_mimeType, QStringLiteral("KParts/ReadOnlyPart"));
            //service = KService::serviceByStorageId(md.pluginId());
        }
        if (m_service) {
            m_request.serviceName = m_service->desktopEntryName();
        }
    }
    m_ready = m_service || m_action != OpenUrlAction::Embed;
}

void UrlLoader::decideOpenOrSave()
{
    m_ready = true;
    m_action = OpenUrlAction::Open;

    QString protClass = KProtocolInfo::protocolClass(m_url.scheme());
    bool alwaysOpen = m_url.isLocalFile() || protClass == QLatin1String(":local") || KProtocolInfo::isHelperProtocol(m_url);
    if (!alwaysOpen) {
        OpenSaveAnswer answerWithService = askSaveOrOpen(OpenEmbedMode::Open);
        KParts::BrowserOpenOrSaveQuestion::Result answer = answerWithService.first;
        qDebug() << "ANSWER" << answer;
        if (answer == KParts::BrowserOpenOrSaveQuestion::Open) {
            m_service = answerWithService.second;
        } else if (answer == KParts::BrowserOpenOrSaveQuestion::Save) {
            m_action = OpenUrlAction::Save;
        } else {
            m_action = OpenUrlAction::DoNothing;
        }
    } else {
        m_service = KApplicationTrader::preferredService(m_mimeType);
    }
    if (m_action == OpenUrlAction::Open) {
        const bool allowExecution = m_trustedSource || KParts::BrowserRun::allowExecution(m_mimeType, m_url);
        if (allowExecution) {
            if (KParts::BrowserRun::isExecutable(m_mimeType)) {
                m_action = OpenUrlAction::Execute;
            }
        } else {
            m_action = OpenUrlAction::DoNothing;
        }
    }
//     if (m_openUrlJob && m_action != OpenUrlAction::Execute) {
//         m_openUrlJob->kill();
//     }
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
            done();
            break;
    }
}

void UrlLoader::done(KJob *job)
{
    if (job) {
        jobFinished(job);
    }
    emit finished(this);
    deleteLater();
}


bool UrlLoader::serviceIsKonqueror(KService::Ptr service)
{
    return service && (service->desktopEntryName() == QLatin1String("konqueror") || service->exec().trimmed().startsWith(QLatin1String("kfmclient")));
}

void UrlLoader::launchOpenUrlJob(bool pauseOnMimeTypeDetermined)
{
    m_openUrlJob = new KIO::OpenUrlJob(m_url, m_mimeType, this);
    m_openUrlJob->setEnableExternalBrowser(false);
    m_openUrlJob->setShowOpenOrExecuteDialog(true);
    m_openUrlJob->setRunExecutables(true);
    m_openUrlJob->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    m_openUrlJob->setSuggestedFileName(m_request.suggestedFileName);
    if (pauseOnMimeTypeDetermined) {
        //TODO Remove KonqRun: sometimes, this signal is emitted with mimetype application/octet-stream, even when the mimetype
        //should be known (and, indeed, clicking again on the link gives the correct mimetype).
        connect(m_openUrlJob, &KIO::OpenUrlJob::mimeTypeFound, this, &UrlLoader::mimetypeDeterminedByJob);
    }
    connect(m_openUrlJob, &KJob::finished, this, &UrlLoader::jobFinished);
    m_openUrlJob->start();
}

void UrlLoader::mimetypeDeterminedByJob(const QString &mimeType)
{
    m_mimeType=mimeType;
    if (shouldEmbedThis()) {
        m_openUrlJob->kill();
        decideEmbedOrSave();
    } else {
        m_openUrlJob->suspend();
        decideOpenOrSave();
        if (m_action != OpenUrlAction::Execute) {
            m_openUrlJob->kill();
        }
    }
    performAction();
}

void UrlLoader::detectSettingsForRemoteFiles()
{
    if (m_url.isLocalFile()) {
        return;
    }

    //TODO Remove KonqRun: determine this dynamically
    const QVector<QString> webengineSchemes = {"error", "konq", "tar"};

    if (m_mimeType.isEmpty() && (m_url.scheme().startsWith(QStringLiteral("http")) || webengineSchemes.contains(m_url.scheme()))) {
        m_mimeType = QLatin1String("text/html");
        m_request.args.setMimeType(QStringLiteral("text/html"));
    } else if (!m_trustedSource && isTextExecutable(m_mimeType)) {
        m_mimeType = QLatin1String("text/plain");
        m_request.args.setMimeType(QStringLiteral("text/plain"));
    }
}

void UrlLoader::detectSettingsForLocalFiles()
{
    if (!m_url.isLocalFile()) {
        return;
    }

    if (!m_mimeType.isEmpty()) {
        // Generic mechanism for redirecting to tar:/<path>/ when clicking on a tar file,
        // zip:/<path>/ when clicking on a zip file, etc.
        // The .protocol file specifies the mimetype that the kioslave handles.
        // Note that we don't use mimetype inheritance since we don't want to
        // open OpenDocument files as zip folders...
        // Also note that we do this here and not in openView anymore,
        // because in the case of foo.bz2 we don't know the final mimetype, we need a konqrun...
        const QString protocol = KProtocolManager::protocolForArchiveMimetype(m_mimeType);
        if (!protocol.isEmpty() && KonqFMSettings::settings()->shouldEmbed(m_mimeType)) {
            m_url.setScheme(protocol);
            if (m_mimeType == QLatin1String("application/x-webarchive")) {
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

        // Redirect to the url in Type=Link desktop files
        if (m_mimeType == QLatin1String("application/x-desktop")) {
            KDesktopFile df(m_url.toLocalFile());
            if (df.hasLinkType()) {
                m_url = QUrl(df.readUrl());
                m_mimeType.clear(); // to be determined again
            }
        }
    } else {
        QMimeDatabase db;
        m_mimeType = db.mimeTypeForFile(m_url.path()).name();
    }
    //TODO Remove KonqRun: what's the difference between m_mimeType and m_request.args.mimeType()?
    //   m_request.args.setMimeType(m_mimeType);
}


bool UrlLoader::shouldEmbedThis() const
{
    return !m_dontEmbed && (m_request.forceAutoEmbed || KonqFMSettings::settings()->shouldEmbed(m_mimeType));
}

void UrlLoader::embed()
{
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
    job->addMetaData(QStringLiteral("MaxCacheSize"), QStringLiteral("0")); // Don't store in http cache.
    job->addMetaData(QStringLiteral("cache"), QStringLiteral("cache")); // Use entry from cache if available.
    KJobWidgets::setWindow(job, m_mainWindow);
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    connect(job, &KJob::finished, this, [this, job](){done(job);});
    job->start();
}

void UrlLoader::open()
{
    if (m_service && serviceIsKonqueror(m_service) && m_mainWindow->refuseExecutingKonqueror(m_mimeType)) {
        qDebug() << "REFUSING EXECUTING";
        // Prevention against user stupidity : if the associated app for this mimetype
        // is konqueror/kfmclient, then we'll loop forever.
        return;
    }
    qDebug() << "Launching" << m_url << "with" << m_service;
    KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(m_service);
    job->setUrls({m_url});
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_mainWindow));
    if (m_request.tempFile) {
        job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
    }
    connect(job, &KJob::finished, this, [this, job](){done(job);});
    job->start();
}

void UrlLoader::execute()
{
    if (!m_openUrlJob) {
        launchOpenUrlJob(false);
    } else {
        disconnect(m_openUrlJob, &KJob::finished, this, nullptr); //Otherwise, jobFinished will be called twice
        connect(m_openUrlJob, &KJob::finished, this, [this](){done(m_openUrlJob);});
        m_openUrlJob->resume();
    }
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
    return qMakePair((mode == OpenEmbedMode::Open ? dlg.askOpenOrSave() : dlg.askEmbedOrSave()), dlg.selectedService());
}

QString UrlLoader::partForLocalFile(const QString& path)
{
    QMimeDatabase db;
    QString mimetype = db.mimeTypeForFile(path).name();
    //TODO Remove KonqRun: replace the following two lines with the commented out lines below them as soon as I can find out how to create a service from a KPluginMetaData
    KService::Ptr service = KMimeTypeTrader::self()->preferredService(mimetype, QStringLiteral("KParts/ReadOnlyPart"));
    return service ? service->name() : QString();
//     KPluginMetaData md = KParts::PartLoader::partsForMimeType(mimetype).first();
//     return md.pluginId();
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
    m_jobHadError = job->error();
}

QDebug operator<<(QDebug dbg, UrlLoader::OpenUrlAction action)
{
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    switch (action) {
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
