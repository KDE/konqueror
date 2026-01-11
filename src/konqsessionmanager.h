/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQSESSIONMANAGER_H
#define KONQSESSIONMANAGER_H

#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QString>
#include <QTreeWidget>
#include <QDialog>
#include <QFontMetrics>

#include <kconfig.h>
#include <konqprivate_export.h>
#include <config-konqueror.h>
#include <KX11Extras>
#include <KConfigGroup>

class KonqMainWindow;
class QSessionManager;

#ifdef KActivities_FOUND
class ActivityManager;
#endif

/**
 * This class is a singleton. It does some session related tasks:
 *  - Autosave current session every X seconds
 *  - Restore a saved session if konqueror crashed
 *  - Restore a given session manually
 */
class KONQ_TESTS_EXPORT KonqSessionManager : public QObject
{
    Q_OBJECT
public:
    friend class KonqSessionManagerPrivate;

    static KonqSessionManager *self();

    void restoreSessionSavedAtLogout();

    /**
     * @brief Restores the application state saved by saveSessionBeforeClosingApplication if the corresponding option is enabled
     *
     * @return `true` if at least one `KonqMainWindow` exists and `false` if no `KonqMainWindow` exists
     */
    bool restoreSessionSavedAtExit();

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
                         openTabsInsideCurrentWindow = false, KonqMainWindow *parent = nullptr);

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
                         openTabsInsideCurrentWindow = false, KonqMainWindow *parent = nullptr);

    /**
     * Restore saved session.
     * @param sessionFilePath session file to restore.
     * @param openTabsInsideCurrentWindow indicates if you want to open the tabs
     * in current window or not. False by default.
     * @param parent indicates in which window the tabs will be opened if
     * openTabsInsideCurrentWindow is set to true. Otherwise it won't be used.
     */
    void restoreSession(const QString &sessionFilePath, bool
                        openTabsInsideCurrentWindow = false, KonqMainWindow *parent = nullptr);

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
     * @param mainWindow if 0, all windows will be saved, else only the given one
     */
    void saveCurrentSessionToFile(const QString &sessionConfigPath, KonqMainWindow *mainWindow = nullptr);

    /**
     * Returns the autosave directory
     */
    QString autosaveDirectory() const;

    void registerMainWindow(KonqMainWindow *window);

    static QString fullWindowId(const QString &sessionFile, const QString &windowId);
    static const QList<KConfigGroup> windowConfigGroups(/*NOT const, we'll use writeEntry*/ KConfig &config);

#ifdef KActivities_FOUND
    ActivityManager* activityManager();
#endif

public Q_SLOTS:
    /**
     * Ask the user with a dialog if session should be restored
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
    void saveCurrentSessions(const QString &path);

    /**
     * @brief Saves the session when the application is about to be closed and the user chose to restore last state
     *
     * This does nothing when saving session at logout (according to `QGuiApplication::savingSession`, as that situation
     * is handled separately.
     *
     * The session is saved in `last_state` in `QStandardPaths::ApplicationDataDir`
     */
    void saveSessionAtExit();

private Q_SLOTS:
    void slotCommitData(QSessionManager &sm);

private:
    KonqSessionManager();

    ~KonqSessionManager() override;

    /**
     * Creates the owned_by directory with files inside referencing the owned
     * sessions and returns the list of filepaths with sessions to restore.
     * Returns an empty list if there is nothing to restore.
     */
    QStringList takeSessionsOwnership();

    QString dirForMyOwnedSessionFiles() const
    {
        return m_autosaveDir + "/owned_by" + m_baseService;
    }

    void saveCurrentSessionToFile(KConfig *config, const QList<KonqMainWindow *> &mainWindows = QList<KonqMainWindow *>());

private:
    QTimer m_autoSaveTimer;
    QString m_autosaveDir;
    QString m_baseService;
    bool m_autosaveEnabled;
    bool m_createdOwnedByDir;
    KConfig *m_sessionConfig;

#ifdef KActivities_FOUND
    ActivityManager *m_activityManager;
#endif

Q_SIGNALS: // DBUS signals
    /**
     * Save current session of all konqueror running instances in a given
     * directory
     */
    void saveCurrentSession(const QString &path);
private Q_SLOTS:// connected to DBUS signals
    void slotSaveCurrentSession(const QString &path);
};

#endif /* KONQSESSIONMANAGER_H */
