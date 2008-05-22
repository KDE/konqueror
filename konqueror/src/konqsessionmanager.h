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

#ifndef KONQSESSIONMANAGER_H
#define KONQSESSIONMANAGER_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QString>

#include <kconfig.h>

class QDBusMessage;

/**
 * This class is a singleton. It does some session related tasks:
 *  - Autosave current session every X seconds
 *  - Restore a saved session if konqueror crashed
 *  - Restore a given session manually
 */
class KonqSessionManager : public QObject {
    Q_OBJECT
public:
    friend class KonqSessionManagerPrivate;
    
    static KonqSessionManager *self();

    /**
     * Find out if there is any session that hasn't been cleanly closed.
     */
    bool hasAutosavedDirtySessions();
    
    /**
     * Restore saved session(s).
     */
    void restoreSessions(const QStringList &sessionFileNamesList);
    
    /**
     * Restore saved session(s) given a directory.
     */
    void restoreSessions(const QString &sessionsDir);
    
    /**
     * Restore saved session.
     */
    void restoreSession(const QString &sessionFileName);

public Q_SLOTS:
    /**
     * Ask the user with a KPassivePopup ballon if session should be restored
     */
    void askUserToRestoreAutosavedDirtySessions();
    
    /**
     * Saves current session.
     * This is function is called by the autosave timer.
     */
    void autoSaveSession();
    
    /**
     * Restore saved sessions
     */
    void restoreSessions();
    
    /**
     * Do not restore sessions and remove the restorable saved sessions.
     */
    void doNotRestoreSessions();
    
    /**
     * Save current session in a custom KConfig
     */
    void saveCurrentSession(KConfig* sessionConfig);
    
    /**
     * Save current sessions of all konqueror instances (propagated via a
     * dbus signal).
     */
    void saveCurrentSessions(const QString & path);
private:
    KonqSessionManager();
    
    ~KonqSessionManager();

private:
    QTimer m_autoSaveTimer;
    QStringList m_DirtyAutosavedSessions;
    KConfig *m_autoSavedSessionConfig;
Q_SIGNALS: // DBUS signals
    /**
     * Save current session of all konqueror running instances in a given
     * directory
     */
    void saveCurrentSession( const QString& path );
private Q_SLOTS:// connected to DBUS signals
    void slotSaveCurrentSession( const QString& path );
};

/**
 * These are some helper functions to encode/decode session filenames. The 
 * problem here is that windows doesn't like files with ':' inside.
 */

QString encodeFilename(QString filename);

QString decodeFilename(QString filename);

#endif /* KONQSESSIONMANAGER_H */
