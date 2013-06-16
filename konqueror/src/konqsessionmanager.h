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
#include <kdialog.h>
#include <konqprivate_export.h>

class QDBusMessage;
class KonqMainWindow;
class QTreeWidgetItem;

class SessionRestoreDialog : public KDialog
{
    Q_OBJECT
public:
    explicit SessionRestoreDialog(const QStringList& sessionFilePaths, QWidget* parent=0);
    virtual ~SessionRestoreDialog();

    /**
     * Returns the list of session discarded/unselected by the user.
     */
    QStringList discardedSessionList() const;

    /**
     * Returns true if the don't show checkbox is checked.
     */
    bool isDontShowChecked() const;

    /**
     * Returns true if the corresponding session restore dialog should be shown.
     *
     * @param dontShowAgainName the name that identify the session restore dialog box.
     * @param result if not null, it will be set to the result that was chosen the last
     * time the dialog box was shown. This is only useful if the restore dialog box should
     * be shown.
     */
    static bool shouldBeShown(const QString &dontShowAgainName, int* result);

    /**
     * Save the fact that the session restore dialog should not be shown again.
     *
     * @param dontShowAgainName the name that identify the session restore dialog. If
     * empty, this method does nothing.
     * @param result the value (Yes or No) that should be used as the result
     * for the message box.
     */
    static void saveDontShow(const QString &dontShowAgainName, int result);

private Q_SLOTS:
    void slotClicked(bool);
    void slotItemChanged(QTreeWidgetItem*, int);

private:
    QStringList m_discardedSessionList;
    QHash<QTreeWidgetItem*, int> m_checkedSessionItems;
    int m_sessionItemsCount;
    bool m_dontShowChecked;
};


/**
 * This class is a singleton. It does some session related tasks:
 *  - Autosave current session every X seconds
 *  - Restore a saved session if konqueror crashed
 *  - Restore a given session manually
 */
class KONQ_TESTS_EXPORT KonqSessionManager : public QObject {
    Q_OBJECT
public:
    friend class KonqSessionManagerPrivate;

    static KonqSessionManager *self();

    /**
     * Restore saved session(s).
     *
     * @param sessionFilePathsList list of session files to restore.
     * @param openTabsInsideCurrentWindow indicates if you want to open the tabs
     * in current window or not. False by default.
     * @param parent indicates in which window the tabs will be opened if
     * openTabsInsideCurrentWindow is set to true. Otherwise it won't be used.
     */
    void restoreSessions(const QStringList &sessionFilePathsList, bool
        openTabsInsideCurrentWindow = false, KonqMainWindow *parent = 0L);

    /**
     * Restore saved session(s).
     *
     * @param sessionsDir directory containing the session files to
     * restore.
     * @param openTabsInsideCurrentWindow indicates if you want to open the tabs
     * in current window or not. False by default.
     * @param parent indicates in which window the tabs will be opened if
     * openTabsInsideCurrentWindow is set to true. Otherwise it won't be used.
     */
    void restoreSessions(const QString &sessionsDir, bool
        openTabsInsideCurrentWindow = false, KonqMainWindow *parent = 0L);

    /**
     * Restore saved session.
     * @param sessionFilePath session file to restore.
     * @param openTabsInsideCurrentWindow indicates if you want to open the tabs
     * in current window or not. False by default.
     * @param parent indicates in which window the tabs will be opened if
     * openTabsInsideCurrentWindow is set to true. Otherwise it won't be used.
     */
    void restoreSession(const QString &sessionFilePath, bool
        openTabsInsideCurrentWindow = false, KonqMainWindow *parent = 0L);

    /**
     * Disable the autosave feature. It's called when a konqueror instance is
     * being preloaded
     */
    void disableAutosave();

    /**
     * Enable the autosave feature. It's called when a konqueror instance stops
     * being preloaded and starts having a window showed to the user.
     */
    void enableAutosave();

    /**
     * Removes the owned_by directory and all its files inside (which were
     * referencing the owned sessions).
     */
    void deleteOwnedSessions();

    /**
     * Save current session in a given path (absolute path to a file)
     */
    void saveCurrentSessionToFile(const QString& sessionConfig);
    
    /**
     * Returns the autosave directory
     */
    QString autosaveDirectory() const;

public Q_SLOTS:
    /**
     * Ask the user with a KPassivePopup ballon if session should be restored
     */
    bool askUserToRestoreAutosavedAbandonedSessions();

    /**
     * Saves current session.
     * This is function is called by the autosave timer, but you can call it too
     * if you want. It won't do anything if m_autosaveEnabled is false.
     */
    void autoSaveSession();

    /**
     * Restore owned sessions
     */
    //void restoreSessions();

    /**
     * Save current sessions of all konqueror instances (propagated via a
     * dbus signal).
     */
    void saveCurrentSessions(const QString & path);
private:
    KonqSessionManager();

    ~KonqSessionManager();

    /**
     * Creates the owned_by directory with files inside referencing the owned
     * sessions and returns the list of filepaths with sessions to restore.
     * Returns an empty list if there is nothing to restore.
     */
    QStringList takeSessionsOwnership();

    QString dirForMyOwnedSessionFiles() const {
        return m_autosaveDir + "/owned_by" + m_baseService;
    }

    void saveCurrentSessionToFile(KConfig*);
private:
    QTimer m_autoSaveTimer;
    QString m_autosaveDir;
    QString m_baseService;
    bool m_autosaveEnabled;
    bool m_createdOwnedByDir;
    KConfig* m_sessionConfig;

Q_SIGNALS: // DBUS signals
    /**
     * Save current session of all konqueror running instances in a given
     * directory
     */
    void saveCurrentSession( const QString& path );
private Q_SLOTS:// connected to DBUS signals
    void slotSaveCurrentSession( const QString& path );
};

#endif /* KONQSESSIONMANAGER_H */
