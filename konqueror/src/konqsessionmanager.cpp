/*
   This file is part of the KDE project
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqsessionmanager.h"
#include "konqmisc.h"
#include "konqmainwindow.h"
#include "konqsessionmanager_interface.h"
#include "konqsessionmanageradaptor.h"
#include "konqviewmanager.h"
#include "konqsettingsxt.h"

#include <kglobal.h>
#include <kdebug.h>
#include <kio/deletejob.h>
#include <kstandarddirs.h>
#include <kvbox.h>
#include <khbox.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <ktempdir.h>

#include <QPushButton>
#include <QtCore/QFileInfo>
#include <QPoint>
#include <QtDBus/QtDBus>
#include <QtAlgorithms>
#include <QDirIterator>
#include <QtCore/QDir>
#include <QDBusArgument>
#include <QFile>
#include <QSize>

class KonqSessionManagerPrivate
{
public:
    KonqSessionManagerPrivate()
        : instance(0)
    {
    }

    ~KonqSessionManagerPrivate()
    {
        delete instance;
    }

    KonqSessionManager *instance;
};

K_GLOBAL_STATIC(KonqSessionManagerPrivate, myKonqSessionManagerPrivate)

KonqSessionManager::KonqSessionManager()
    : m_autosaveDir(KStandardDirs::locateLocal("appdata", "autosave"))
{
    // Initialize dbus interfaces
    new KonqSessionManagerAdaptor ( this );

    const QString dbusPath = "/KonqSessionManager";
    const QString dbusInterface = "org.kde.Konqueror.SessionManager";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    m_baseService = KonqMisc::encodeFilename(dbus.baseService());
    dbus.connect(QString(), dbusPath, dbusInterface, "saveCurrentSession", this, SLOT(slotSaveCurrentSession(QString)));

    // Initialize the timer
    const int interval = KonqSettings::autoSaveInterval();
    if (interval > 0) {
        m_autoSaveTimer.setInterval(interval*1000);
        connect( &m_autoSaveTimer, SIGNAL(timeout()), this,
            SLOT(autoSaveSession()) );
    }
    m_autosaveEnabled = false; // so that enableAutosave works
    enableAutosave();
}

KonqSessionManager::~KonqSessionManager()
{
}

void KonqSessionManager::disableAutosave()
{
    if(!m_autosaveEnabled)
        return;

    m_autosaveEnabled = false;
    m_autoSaveTimer.stop();
    QFile::remove(m_autoSavedSessionConfig);
}

void KonqSessionManager::enableAutosave()
{
    if(m_autosaveEnabled)
        return;

    // Create the config file for autosaving current session
    QString filename = "autosave/" + m_baseService;
    m_autoSavedSessionConfig = KStandardDirs::locateLocal("appdata", filename);
    QFile::remove(m_autoSavedSessionConfig);

    m_autosaveEnabled = true;
    m_autoSaveTimer.start();
}

void KonqSessionManager::deleteOwnedSessions()
{
    // Not dealing with the sessions about to remove anymore
    KTempDir::removeDir(dirForMyOwnedSessionFiles());
}

KonqSessionManager* KonqSessionManager::self()
{
    if(!myKonqSessionManagerPrivate->instance)
        myKonqSessionManagerPrivate->instance = new KonqSessionManager();

    return myKonqSessionManagerPrivate->instance;
}

void KonqSessionManager::autoSaveSession()
{
    if(!m_autosaveEnabled)
        return;

    const bool isActive = m_autoSaveTimer.isActive();
    if(isActive)
        m_autoSaveTimer.stop();

    saveCurrentSessionToFile(m_autoSavedSessionConfig);

    // Now that we have saved current session it's safe to remove our owned_by
    // directory
    deleteOwnedSessions();

    if(isActive)
        m_autoSaveTimer.start();
}

void KonqSessionManager::saveCurrentSessions(const QString & path)
{
    emit saveCurrentSession(path);
}

void KonqSessionManager::slotSaveCurrentSession(const QString & path)
{
    const QString filename = path + '/' + m_baseService;
    saveCurrentSessionToFile(filename);
}

void KonqSessionManager::saveCurrentSessionToFile(const QString& sessionConfigPath)
{
    QFile::remove(sessionConfigPath);
    KConfig sessionConfig(sessionConfigPath, KConfig::SimpleConfig, "appdata");

    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    unsigned int counter = 0;

    if(!mainWindows || mainWindows->isEmpty())
        return;

    foreach ( KonqMainWindow* window, *mainWindows )
    {
        KConfigGroup configGroup(&sessionConfig, "Window" +
            QString::number(counter));
        window->saveProperties(configGroup);
        counter++;
    }
    KConfigGroup configGroup(&sessionConfig, "General");
    configGroup.writeEntry("Number of Windows", counter);
}

QStringList KonqSessionManager::takeSessionsOwnership()
{
    // Tell to other konqueror instances that we are the one dealing with
    // these sessions
    QDir dir(dirForMyOwnedSessionFiles());
    QDir parentDir(m_autosaveDir);

    if(!dir.exists())
        parentDir.mkdir("owned_by" + m_baseService);

    QDirIterator it(m_autosaveDir, QDir::Writable|QDir::Files|QDir::Dirs|
        QDir::NoDotAndDotDot);

    QStringList sessionFilePaths;
    QDBusConnectionInterface *idbus = QDBusConnection::sessionBus().interface();

    while (it.hasNext())
    {
        it.next();
        // this is the case where another konq started to restore that session,
        // but crashed immediately. So we try to restore that session again
        if(it.fileInfo().isDir())
        {
            // The remove() removes the "owned_by" part
            if(!idbus->isServiceRegistered(
                KonqMisc::decodeFilename(it.fileName().remove(0, 8))))
            {
                QDirIterator it2(it.filePath(), QDir::Writable|QDir::Files);
                while (it2.hasNext())
                {
                    it2.next();
                    // take ownership of the abandoned file
                    const QString newFileName = dirForMyOwnedSessionFiles() +
                                                '/' + it2.fileName();
                    QFile::rename(it2.filePath(), newFileName);
                    sessionFilePaths.append(newFileName);
                }
                // Remove the old directory
                KTempDir::removeDir(it.filePath());
            }
        } else { // it's a file
            if(!idbus->isServiceRegistered(KonqMisc::decodeFilename(it.fileName())))
            {
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
    foreach ( const QString& sessionFilePath, sessionFilePathsList )
    {
        restoreSession(sessionFilePath, openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSessions(const QString &sessionsDir, bool
    openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    QDirIterator it(sessionsDir, QDir::Readable|QDir::Files);

    while (it.hasNext())
    {
        QFileInfo fi(it.next());
        restoreSession(fi.filePath(), openTabsInsideCurrentWindow, parent);
    }
}

static const QList<KConfigGroup> windowConfigGroups(const KConfig& config)
{
    QList<KConfigGroup> groups;
    KConfigGroup generalGroup(&config, "General");
    const int size = generalGroup.readEntry("Number of Windows", 0);
    for(int i = 0; i < size; i++) {
        groups << KConfigGroup(&config, "Window" + QString::number(i));
    }
    return groups;
}

void KonqSessionManager::restoreSession(const QString &sessionFilePath, bool
    openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    if (!QFile::exists(sessionFilePath))
        return;

    const KConfig config(sessionFilePath, KConfig::SimpleConfig);
    const QList<KConfigGroup> groups = windowConfigGroups(config);
    Q_FOREACH(const KConfigGroup& configGroup, groups) {
        if(!openTabsInsideCurrentWindow)
            KonqViewManager::openSavedWindow(configGroup)->show();
        else
            parent->viewManager()->openSavedWindow(configGroup, true);
    }
}


bool KonqSessionManager::askUserToRestoreAutosavedAbandonedSessions()
{
    const QStringList sessionFilePaths = takeSessionsOwnership();
    if(sessionFilePaths.isEmpty())
        return false;

    QStringList detailsList;
    // Collect info from the sessions to restore
    Q_FOREACH(const QString& sessionFile, sessionFilePaths) {
        kDebug() << sessionFile;
        const KConfig config(sessionFile, KConfig::SimpleConfig);
        const QList<KConfigGroup> groups = windowConfigGroups(config);
        Q_FOREACH(const KConfigGroup& group, groups) {
            // To avoid a recursive search, let's do linear search on Foo_CurrentHistoryItem=1
            Q_FOREACH(const QString& key, group.keyList()) {
                if (key.endsWith("_CurrentHistoryItem")) {
                    const QString viewId = key.left(key.length() - strlen("_CurrentHistoryItem"));
                    const QString historyIndex = group.readEntry(key, QString());
                    const QString prefix = "HistoryItem" + viewId + '_' + historyIndex;
                    // Ignore the sidebar views
                    if (group.readEntry(prefix + "StrServiceName", QString()).startsWith("konq_sidebar"))
                        continue;
                    const QString url = group.readEntry(prefix + "Url", QString());
                    const QString title = group.readEntry(prefix + "Title", QString());
                    kDebug() << viewId << url << title;
                    if (title.trimmed().isEmpty()) {
                        detailsList << url;
                    } else {
                        detailsList << title;
                    }
                }
            }
        }
    }

    disableAutosave();

    switch(KMessageBox::warningYesNoCancelList(0, // there is no questionYesNoCancelList
        i18n("Konqueror did not close correctly. Would you like to restore the previous session?"),
        detailsList,
        i18nc("@title:window", "Restore Session?"),
        KGuiItem(i18n("Restore Session"), "window-new"),
        KGuiItem(i18n("Do Not Restore"), "dialog-close"),
        KGuiItem(i18n("Ask Me Later"), "chronometer"),
        "Restore session when konqueror didn't close correctly"
    ))
    {
        case KMessageBox::Yes:
            restoreSessions(sessionFilePaths);
            enableAutosave();
            return true;
        case KMessageBox::No:
            deleteOwnedSessions();
            enableAutosave();
            return false;
        default:
            // Remove the ownership of the currently owned files
            QDirIterator it(dirForMyOwnedSessionFiles(),
                QDir::Writable|QDir::Files);

            while (it.hasNext())
            {
                it.next();
                // remove ownership of the abandoned file
                QFile::rename(it.filePath(), m_autosaveDir + '/' + it.fileName());
            }
            // Remove the owned_by directory
            KTempDir::removeDir(dirForMyOwnedSessionFiles());
            enableAutosave();
            return false;
    }
}

#include "konqsessionmanager.moc"
