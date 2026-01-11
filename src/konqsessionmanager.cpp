/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2025 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqsessionmanager.h"
#include "konqmisc.h"
#include "konqmainwindow.h"
#include "konqsessionmanager_interface.h"
#include "konqsessionmanageradaptor.h"
#include "konqviewmanager.h"
#include "konqsettings.h"
#include "konqapplication.h"
#include "konqsessiondialog.h"

#ifdef KActivities_FOUND
#include "activitymanager.h"
#include <PlasmaActivities/Consumer>
#include <KX11Extras>
#endif

#include "konqdebug.h"
#include <kio/deletejob.h>
#include <KLocalizedString>
#include <KWindowInfo>
#include <KX11Extras>
#include <KSharedConfig>

#include <QUrl>

#include <QFileInfo>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QtAlgorithms>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QSize>
#include <QApplication>
#include <QStandardPaths>
#include <QSessionManager>
#include <KConfigGroup>

#include <QMessageBox>

class KonqSessionManagerPrivate
{
public:
    KonqSessionManagerPrivate()
        : instance(nullptr)
    {
    }

    ~KonqSessionManagerPrivate()
    {
        delete instance;
    }

    KonqSessionManager *instance;
};

Q_GLOBAL_STATIC(KonqSessionManagerPrivate, myKonqSessionManagerPrivate)

KonqSessionManager::KonqSessionManager()
    : m_autosaveDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + "autosave")
    , m_autosaveEnabled(false) // so that enableAutosave works
    , m_createdOwnedByDir(false)
    , m_sessionConfig(nullptr)
#ifdef KActivities_FOUND
    , m_activityManager(new ActivityManager(this))
#endif
{
    // Initialize dbus interfaces
    new KonqSessionManagerAdaptor(this);

    const QString dbusPath = QStringLiteral("/KonqSessionManager");
    const QString dbusInterface = QStringLiteral("org.kde.Konqueror.SessionManager");

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(dbusPath, this);
    m_baseService = KonqMisc::encodeFilename(dbus.baseService());
    dbus.connect(QString(), dbusPath, dbusInterface, QStringLiteral("saveCurrentSession"), this, SLOT(slotSaveCurrentSession(QString)));

    // Initialize the timer
    const int interval = Konq::Settings::autoSaveInterval();
    if (interval > 0) {
        m_autoSaveTimer.setInterval(interval * 1000);
        connect(&m_autoSaveTimer, &QTimer::timeout, this, &KonqSessionManager::autoSaveSession);
    }
    enableAutosave();

    connect(qApp, &QGuiApplication::commitDataRequest, this, &KonqSessionManager::slotCommitData);
    connect(qApp, &QGuiApplication::lastWindowClosed, this, &KonqSessionManager::saveSessionAtExit);
}

KonqSessionManager::~KonqSessionManager()
{
    if (m_sessionConfig) {
        QFile::remove(m_sessionConfig->name());
    }
    delete m_sessionConfig;
}

QString KonqSessionManager::fullWindowId(const QString& sessionFile, const QString& windowId)
{
    return sessionFile + windowId;
}

const QList<KConfigGroup> KonqSessionManager::windowConfigGroups(KConfig &config)
{
    QList<KConfigGroup> groups;
    KConfigGroup generalGroup(&config, "General");
    const int size = generalGroup.readEntry("Number of Windows", 0);
    for (int i = 0; i < size; i++) {
        groups << KConfigGroup(&config, "Window" + QString::number(i));
    }
    return groups;
}

void KonqSessionManager::restoreSessionSavedAtLogout()
{
    askUserToRestoreAutosavedAbandonedSessions();

    int n = 1;
    while (KonqMainWindow::canBeRestored(n)) {
        const QString className = KXmlGuiWindow::classNameOfToplevel(n);

        //The !m_preloadedWindowsNumber.contains(n) check avoid restoring preloaded windows
        if (className == QLatin1String("KonqMainWindow")) {
            KonqMainWindow * mw = new KonqMainWindow();
            mw->restore(n);
        } else  {
            qCWarning(KONQUEROR_LOG) << "Unknown class" << className << "in session saved data!";
        }
        ++n;
    }
}


// Don't restore preloaded konquerors
void KonqSessionManager::slotCommitData(QSessionManager &sm)
{
    QList<KonqMainWindow*> const *windows = KonqMainWindow::mainWindowList();
    if (std::all_of(windows->constBegin(), windows->constEnd(), [](KonqMainWindow *w){return w->isPreloaded();})) {
        sm.setRestartHint(QSessionManager::RestartNever);
    }
}

void KonqSessionManager::disableAutosave()
{
    if (!m_autosaveEnabled) {
        return;
    }

    m_autosaveEnabled = false;
    m_autoSaveTimer.stop();
    if (m_sessionConfig) {
        QFile::remove(m_sessionConfig->name());
        delete m_sessionConfig;
        m_sessionConfig = nullptr;
    }
}

void KonqSessionManager::enableAutosave()
{
    if (m_autosaveEnabled) {
        return;
    }

    // Create the config file for autosaving current session
    QString filename = QLatin1String("autosave/") + m_baseService;
    const QString filePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + filename;

    delete m_sessionConfig;
    m_sessionConfig = new KConfig(filePath, KConfig::SimpleConfig);
    //qCDebug(KONQUEROR_LOG) << "config filename:" << m_sessionConfig->name();

    m_autosaveEnabled = true;
    m_autoSaveTimer.start();
}

void KonqSessionManager::deleteOwnedSessions()
{
    // Not dealing with the sessions about to remove anymore
    if (m_createdOwnedByDir && QDir(dirForMyOwnedSessionFiles()).removeRecursively()) {
        m_createdOwnedByDir = false;
    }
}

KonqSessionManager *KonqSessionManager::self()
{
    if (!myKonqSessionManagerPrivate->instance) {
        myKonqSessionManagerPrivate->instance = new KonqSessionManager();
    }

    return myKonqSessionManagerPrivate->instance;
}

void KonqSessionManager::autoSaveSession()
{
    // FIXME: Should really be checking if the session has changed before saving, to prevent needless I/O activity
    if (!m_autosaveEnabled) {
        return;
    }

    const bool isActive = m_autoSaveTimer.isActive();
    if (isActive) {
        m_autoSaveTimer.stop();
    }

    saveCurrentSessionToFile(m_sessionConfig);
    m_sessionConfig->sync();
    m_sessionConfig->markAsClean();

    // Now that we have saved current session it's safe to remove our owned_by
    // directory
    deleteOwnedSessions();

    if (isActive) {
        m_autoSaveTimer.start();
    }
}

void KonqSessionManager::saveCurrentSessions(const QString &path)
{
    emit saveCurrentSession(path);
}

void KonqSessionManager::slotSaveCurrentSession(const QString &path)
{
    const QString filename = path + '/' + m_baseService;
    saveCurrentSessionToFile(filename);
}

void KonqSessionManager::saveCurrentSessionToFile(const QString &sessionConfigPath, KonqMainWindow *mainWindow)
{
    QFile::remove(sessionConfigPath);
    KConfig config(sessionConfigPath, KConfig::SimpleConfig);

    QList<KonqMainWindow *> mainWindows;
    if (mainWindow) {
        mainWindows << mainWindow;
    }
    saveCurrentSessionToFile(&config, mainWindows);
}

void KonqSessionManager::saveCurrentSessionToFile(KConfig *config, const QList<KonqMainWindow *> &theMainWindows)
{
    QList<KonqMainWindow *> mainWindows = theMainWindows;

    if (mainWindows.isEmpty() && KonqMainWindow::mainWindowList()) {
        mainWindows = *KonqMainWindow::mainWindowList();
    }

    unsigned int counter = 0;

    if (mainWindows.isEmpty()) {
        return;
    }

    for (KonqMainWindow *window: mainWindows) {
        if (!window->isPreloaded()) {
            KConfigGroup configGroup(config, "Window" + QString::number(counter));
            window->saveProperties(configGroup);
            KWindowInfo info(window->winId(), NET::Properties(), NET::WM2Activities);
            configGroup.writeEntry("Activities", info.activities());
            counter++;
        }
    }

    KConfigGroup configGroup(config, "General");
    configGroup.writeEntry("Number of Windows", counter);
}

void KonqSessionManager::saveSessionAtExit()
{
    if (qApp->isSavingSession()) {
        return;
    }
    bool saveSessions = Konq::Settings::restoreLastState();
    if (!saveSessions) {
        return;
    }
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.isEmpty()) {
        const QList<KonqMainWindow*> windows = KonqMainWindow::mainWindows();
        if (windows.count() == 1 && windows.at(0)->isPreloaded())  {
            return;
        }
        saveCurrentSessionToFile(QDir(path).absoluteFilePath("last_session"));
    }
}

QString KonqSessionManager::autosaveDirectory() const
{
    return m_autosaveDir;
}

QStringList KonqSessionManager::takeSessionsOwnership()
{
    // Tell to other konqueror instances that we are the one dealing with
    // these sessions
    QDir dir(dirForMyOwnedSessionFiles());
    QDir parentDir(m_autosaveDir);

    if (!dir.exists()) {
        m_createdOwnedByDir = parentDir.mkdir("owned_by" + m_baseService);
    }

    QDirIterator it(m_autosaveDir, QDir::Writable | QDir::Files | QDir::Dirs |
                    QDir::NoDotAndDotDot);

    QStringList sessionFilePaths;
    QDBusConnectionInterface *idbus = QDBusConnection::sessionBus().interface();

    while (it.hasNext()) {
        it.next();
        // this is the case where another konq started to restore that session,
        // but crashed immediately. So we try to restore that session again
        if (it.fileInfo().isDir()) {
            // The remove() removes the "owned_by" part
            if (!idbus->isServiceRegistered(
                        KonqMisc::decodeFilename(it.fileName().remove(0, 8)))) {
                QDirIterator it2(it.filePath(), QDir::Writable | QDir::Files);
                while (it2.hasNext()) {
                    it2.next();
                    // take ownership of the abandoned file
                    const QString newFileName = dirForMyOwnedSessionFiles() +
                                                '/' + it2.fileName();
                    QFile::rename(it2.filePath(), newFileName);
                    sessionFilePaths.append(newFileName);
                }
                // Remove the old directory
                QDir(it.filePath()).removeRecursively();
            }
        } else { // it's a file
            if (!idbus->isServiceRegistered(KonqMisc::decodeFilename(it.fileName()))) {
                // and it's abandoned: take its ownership
                const QString newFileName = dirForMyOwnedSessionFiles() + '/' +
                                            it.fileName();
                QFile::rename(it.filePath(), newFileName);
                sessionFilePaths.append(newFileName);
            }
        }
    }

    return sessionFilePaths;
}

void KonqSessionManager::restoreSessions(const QStringList &sessionFilePathsList,
        bool openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    for (const QString &sessionFilePath: sessionFilePathsList) {
        restoreSession(sessionFilePath, openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSessions(const QString &sessionsDir, bool
        openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    QDirIterator it(sessionsDir, QDir::Readable | QDir::Files);

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        restoreSession(fi.filePath(), openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSession(const QString &sessionFilePath, bool
                                        openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    if (!QFile::exists(sessionFilePath)) {
        return;
    }

    KConfig config(sessionFilePath, KConfig::SimpleConfig);
    const QList<KConfigGroup> groups = windowConfigGroups(config);
    for (const KConfigGroup &configGroup: groups) {
        if (!configGroup.exists()) { // because if the session file has the wrong number of windows, it would launch empty windows for the missing number of windows
            // qWarning() << "restoreSession cleaning empty window group"; // should a warning be issued?
            continue;
        }
        if (!openTabsInsideCurrentWindow) {
            KonqViewManager::openSavedWindow(configGroup)->show();
        } else {
            parent->viewManager()->openSavedWindow(configGroup, true);
        }
    }
}

bool KonqSessionManager::restoreSessionSavedAtExit()
{
    if (!Konq::Settings::restoreLastState()) {
        return false;
    }

    QString lastSessionPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("last_session"));
    if (lastSessionPath.isEmpty()) {
        return false;
    }

    restoreSession(lastSessionPath);
    QFile(lastSessionPath).remove();

    return !KonqMainWindow::mainWindows().isEmpty();
}

static void removeDiscardedWindows(const QStringList &sessionFiles, const QStringList &discardedWindows)
{
    if (discardedWindows.isEmpty()) {
        return;
    }

    for (const QString &sessionFile: sessionFiles) {
        KConfig config(sessionFile, KConfig::SimpleConfig);
        QList<KConfigGroup> groups = KonqSessionManager::windowConfigGroups(config);
        for (int i = 0, count = groups.count(); i < count; ++i) {
            KConfigGroup &group = groups[i];
            const QString windowName = group.name();
            const QString windowId = KonqSessionManager::fullWindowId(sessionFile, windowName);
            if (discardedWindows.contains(windowId)) {
                group.deleteGroup();
            }
        }
    }
}

bool KonqSessionManager::askUserToRestoreAutosavedAbandonedSessions()
{
    const QStringList sessionFilePaths = takeSessionsOwnership();
    if (sessionFilePaths.isEmpty()) {
        return false;
    }

    disableAutosave();

    int result;
    QStringList discardedWindowList;
    const QLatin1String dontAskAgainName("Restore session when konqueror didn't close correctly");

    if (SessionRestoreDialog::shouldBeShown(dontAskAgainName, &result)) {
        SessionRestoreDialog *restoreDlg = new SessionRestoreDialog(sessionFilePaths);
        if (restoreDlg->isEmpty()) {
            result = QDialogButtonBox::No;
        } else {
#ifdef KActivities_FOUND
            KX11Extras::setOnActivities(restoreDlg->winId(), {});
#endif
            result = restoreDlg->exec();
            discardedWindowList = restoreDlg->discardedWindowList();
            if (restoreDlg->isDontShowChecked()) {
                SessionRestoreDialog::saveDontShow(dontAskAgainName, result);
            }
        }
        delete restoreDlg;
    }

    switch (result) {
    case QDialogButtonBox::Yes:
        // Remove the discarded session list files.
        removeDiscardedWindows(sessionFilePaths, discardedWindowList);
        restoreSessions(sessionFilePaths);
        enableAutosave();
        return true;
    case QDialogButtonBox::No:
        deleteOwnedSessions();
        enableAutosave();
        return false;
    default:
        // Remove the ownership of the currently owned files
        QDirIterator it(dirForMyOwnedSessionFiles(),
                        QDir::Writable | QDir::Files);

        while (it.hasNext()) {
            it.next();
            // remove ownership of the abandoned file
            QFile::rename(it.filePath(), m_autosaveDir + '/' + it.fileName());
        }
        // Remove the owned_by directory
        QDir(dirForMyOwnedSessionFiles()).removeRecursively();
        enableAutosave();
        return false;
    }
}

void KonqSessionManager::registerMainWindow(KonqMainWindow* window)
{
#ifdef KActivities_FOUND
    m_activityManager->registerMainWindow(window);
#else
    Q_UNUSED(window);
#endif
}

#ifdef KActivities_FOUND
ActivityManager * KonqSessionManager::activityManager()
{
    return m_activityManager;
}
#endif
