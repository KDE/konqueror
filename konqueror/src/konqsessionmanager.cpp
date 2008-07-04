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
#include "konqsessionmanager_interface.h"
#include "konqsessionmanageradaptor.h"
#include "konqmainwindow.h"
#include "konqviewmanager.h"
#include "konqsettingsxt.h"

#include <kglobal.h>
#include <kdebug.h>
#include <kurl.h>
#include <kio/deletejob.h>
#include <kstandarddirs.h>
#include <kvbox.h>
#include <khbox.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kio/netaccess.h>

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
    KonqSessionManager instance;
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
    m_baseService = encodeFilename(dbus.baseService());
    dbus.connect(QString(), dbusPath, dbusInterface, "saveCurrentSession", this, SLOT(slotSaveCurrentSession(QString)));

    // Initialize the timer
    int interval = KonqSettings::autoSaveInterval();
    if(interval > 0)
    {
        m_autoSaveTimer.setInterval(interval*1000);
        connect( &m_autoSaveTimer, SIGNAL( timeout() ), this,
            SLOT( autoSaveSession() ) );
    }
    m_autosaveEnabled = false; // so that enableAutosave works
    enableAutosave();
}

KonqSessionManager::~KonqSessionManager()
{
    disableAutosave();
    deleteOwnedSessions(); // we closed normally
}

void KonqSessionManager::disableAutosave()
{
    if(!m_autosaveEnabled)
        return;

    m_autosaveEnabled = false;
    m_autoSaveTimer.stop();
    QString file = KStandardDirs::locateLocal("appdata", 
        m_autoSavedSessionConfig->name());
    QFile::remove(file);
    delete m_autoSavedSessionConfig;
}

void KonqSessionManager::enableAutosave()
{
    if(m_autosaveEnabled)
        return;

    // Create the config file for autosaving current session
    QString filename = "autosave/" + m_baseService;
    QString file = KStandardDirs::locateLocal("appdata", filename);
    QFile::remove(file);
    
    m_autoSavedSessionConfig = new KConfig(filename, KConfig::SimpleConfig,
        "appdata");
    m_autosaveEnabled = true;
    m_autoSaveTimer.start();
}

KonqSessionManager* KonqSessionManager::self()
{
    return &myKonqSessionManagerPrivate->instance;
}

void KonqSessionManager::deleteOwnedSessions()
{
    // Not dealing with the sessions about to remove anymore
    QDir dir(dirForMyOwnedSessionFiles());
    
    if(dir.exists())
        KIO::NetAccess::del(KUrl(dir.path()), NULL);
}


void KonqSessionManager::autoSaveSession()
{
    if(!m_autosaveEnabled)
        return;
    
    bool isActive = m_autoSaveTimer.isActive();
    if(isActive)
        m_autoSaveTimer.stop();
    
    saveCurrentSession(m_autoSavedSessionConfig);
    
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
    QString filename = path + "/" + m_baseService;
    
    KConfig sessionConfig(filename, KConfig::SimpleConfig, "appdata");
    saveCurrentSession(&sessionConfig);
}

void KonqSessionManager::saveCurrentSession(KConfig* sessionConfig)
{
    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    unsigned int counter = 0;
    
    if(!mainWindows || mainWindows->isEmpty())
        return;
    
    foreach ( KonqMainWindow* window, *mainWindows )
    {
        KConfigGroup configGroup(sessionConfig, "Window" +
            QString::number(counter));
        window->saveProperties(configGroup);
        counter++;
    }
    KConfigGroup configGroup(sessionConfig, "General");
    configGroup.writeEntry("Number of Windows", counter);
    sessionConfig->sync();
}

bool KonqSessionManager::takeSessionsOwnership()
{
    bool found = false;
    // Tell to other konqueror instances that we are the one dealing with
    // these sessions
    QDir dir(dirForMyOwnedSessionFiles());
    QDir parentDir(m_autosaveDir);
    
    if(!dir.exists())
        parentDir.mkdir("owned_by" + m_baseService);

    QDirIterator it(m_autosaveDir, QDir::Writable|QDir::Files|QDir::Dirs|
        QDir::NoDotAndDotDot);
    
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
                decodeFilename(it.fileName().remove(0, 8))))
            {
                QDirIterator it2(it.filePath(), QDir::Writable|QDir::Files);
                while (it2.hasNext())
                {
                    found = true;
                    it2.next();
                    // take ownership of the abandoned file
                    QFile::rename(it2.filePath(), dirForMyOwnedSessionFiles() +
                        "/" + it2.fileName());
                }
                // Remove the old directory
                KIO::NetAccess::del(KUrl(it.filePath()), NULL);
            }
        } else { // it's a file
            if(!idbus->isServiceRegistered(decodeFilename(it.fileName())))
            {
                // and it's abandoned: take its ownership
                QFile::rename(it.filePath(), dirForMyOwnedSessionFiles() + "/" + 
                    it.fileName());
                found = true;
            }
        }
    }
    
    return found;
}


void KonqSessionManager::restoreSessions()
{
    QStringList ownedSessions;
    QDirIterator it(dirForMyOwnedSessionFiles(), QDir::Writable|QDir::Files);
    
    while (it.hasNext())
    {
        it.next();
        ownedSessions.append(it.filePath());
    }
    restoreSessions(ownedSessions);
}

void KonqSessionManager::restoreSessions(const QStringList &sessionFilePathsList)
{
    foreach ( const QString& sessionFilePath, sessionFilePathsList )
    {
        restoreSession(sessionFilePath);
    }
}

void KonqSessionManager::restoreSessions(const QString &sessionsDir)
{
    QDirIterator it(sessionsDir, QDir::Readable|QDir::Files);
    
    while (it.hasNext())
    {
        QFileInfo fi(it.next());
        restoreSession(fi.filePath());
    }
}
void KonqSessionManager::restoreSession(const QString &sessionFilePath)
{
    QString file(sessionFilePath);
    if(!QFile::exists(file))
        return;
    
    KConfig config(sessionFilePath, KConfig::SimpleConfig);
    
    KConfigGroup generalGroup(&config, "General");
    int size = generalGroup.readEntry("Number of Windows", 0);
    
    for(int i = 0; i < size; i++)
    {
        KConfigGroup configGroup(&config, "Window" + QString::number(i));
        KonqViewManager::openSavedWindow(configGroup);
    }
}


bool KonqSessionManager::askUserToRestoreAutosavedAbandonedSessions()
{
    if(!takeSessionsOwnership())
        return false;

    disableAutosave();
    
    switch(KMessageBox::questionYesNoCancel(0,
        i18n("Konqueror didn't close correctly. Would you like to restore session?"),
        i18n("Restore session?"),
        KGuiItem(i18n("Restore session"), "window-new"),
        KGuiItem(i18n("Do not restore"), "dialog-close"),
        KGuiItem(i18n("Ask me later"), "chronometer"),
        "Restore session when konqueror didn't close correctly"
    ))
    {
        case KMessageBox::Yes:
            restoreSessions();
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
                QFile::rename(it.filePath(), m_autosaveDir + "/" + it.fileName());
            }
            // Remove the owned_by directory
            KIO::NetAccess::del(KUrl(dirForMyOwnedSessionFiles()), NULL);            
            enableAutosave();
            return false;
    }
}

QString encodeFilename(QString filename)
{
    return filename.replace(':', "_");
}

QString decodeFilename(QString filename)
{
    return filename.replace('_', ":");
}

#include "konqsessionmanager.moc"
