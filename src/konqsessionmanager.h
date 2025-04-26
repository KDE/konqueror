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
 * @brief Class which manages Konqueror sessions
 *
 * A Konqueror session is a list of windows and views within it. Each session
 * is stored as a directory, containing one or more file which describe the windows
 * and their content.
 *
 * This class:
 * - autosaves the current session at fixed intervals
 * - restores automatically saved sessions after a crash
 * - restores sessions saved manually
 * - automatically saves the current session at logout and restores it
 * -if configured to do so, automatically saves the last window state when closing the application and
 * restores it when it's next started
 *
 * There can only be a single instance of this class, and it can be accessed using
 * self().
 *
 * When restoring sessions after a crash, there could be the risk of having multiple
 * Konqueror instances trying to restore the same session. To avoid this, before
 * starting the recovery, a Konqueror instance would move all sessions to recover
 * into a directory whose name depended on the instance d-bus name. Other Konqueror
 * instances would then ignore sessions in directories corresponding to running
 * Konqueror instances (see takeSessionsOwnership() and deleteOwnedSessions()).
 * All of this shouldn't be needed anymore, as Konqueror is now a single-instance
 * application.
 *
 * @todo Remove everything related to session ownership
 *
 * @subsection Format Format of a session file
 * A file containing a session is a `KConfig` file with the following groups:
 * - `General`: it contains the singole `Number of Windows` entry which records
 * the number of non-preloaded windows described in the session file
 * - Window<i>n</i>, where _n_ is an integer: contains information about the window number
 * _n_ as written by KonqMainWindow::saveProperties(). _n_ goes from 1 up to the
 * value of the `Number of Windows` entry.
 *
 */
class KONQ_TESTS_EXPORT KonqSessionManager : public QObject
{
    Q_OBJECT
public:
    friend class KonqSessionManagerPrivate;

    /**
     * @brief The single instance of this class
     *
     * @return the single instance of KonqSessionManager. If no instance exists,
     * a new one is created
     */
    static KonqSessionManager *self();

    /**
     * @brief Restores all windows automatically saved at logout
     *
     * If there are autosaved abandoned sessions, the user is also asked whether to
     * restore them.
     *
     * @internal
     * We need to manually take care of preloaded windows, since they should never
     * be restored (otherwise they would be shown as empty normal windows). Since
     * there's no way to tell
     * @endinternal
     */
    void restoreSessionSavedAtLogout();

    /**
     * @brief Restores the application state saved when the last window was closed
     *
     * @note This function does nothing if the corresponding option has been disabled,
     * according to Konq::Settings::restoreLastState().
     *
     * @note Preloaded windows are never restored
     *
     * @return `true` if at least one `KonqMainWindow` has been restore and `false` otherwise
     */
    bool restoreSessionSavedAtExit();

    /**
     * @brief Restores the sessions saved in the given files
     *
     * @param sessionFilePathsList the list of files containing the sessions to restore
     * @param openTabsInsideCurrentWindow whether to open the tabs
     * in the current window or in a new window
     * @param parent in which window the tabs will be opened if
     * @p openTabsInsideCurrentWindow is `true`. It's unused if @p openTabsInsideCurrentWindow
     * is `false`
     */
    void restoreSessions(const QStringList &sessionFilePathsList, bool
                         openTabsInsideCurrentWindow = false, KonqMainWindow *parent = nullptr);

    /**
     * @brief Restores the sessions saved in the given directory
     *
     * @param sessionsDir directory containing the session files to
     * restore
     * @param openTabsInsideCurrentWindow whether to open the tabs
     * in the current window or in a new window
     * @param parent in which window the tabs will be opened if
     * @p openTabsInsideCurrentWindow is `true`. It's unused if @p openTabsInsideCurrentWindow
     * is `false`
     */
    void restoreSessions(const QString &sessionsDir, bool
                         openTabsInsideCurrentWindow = false, KonqMainWindow *parent = nullptr);

    /**
     * @brief Restores the session saved in the given file
     * @param sessionFilePath the path of the file containing the session to restore
     * @param openTabsInsideCurrentWindow whether to open the tabs
     * in the current window or in a new window
     * @param parent in which window the tabs will be opened if
     * @p openTabsInsideCurrentWindow is `true`. It's unused if @p openTabsInsideCurrentWindow
     * is `false`
     */
    void restoreSession(const QString &sessionFilePath, bool
                        openTabsInsideCurrentWindow = false, KonqMainWindow *parent = nullptr);

    /**
     * @brief Disables the session autosave feature
     */
    void disableAutosave();

    /**
     * @brief Enable the autosave feature
     */
    void enableAutosave();

    /**
     * @brief Removes the directory with the sessions this object has restored
     *
     * It removes the directory returned by dirForMyOwnedSessionFiles() and all its files inside it
     */
    void deleteOwnedSessions();

    /**
     * @brief Saves a session to a file
     *
     * The session will include the contents of either a single window or of all
     * existing windows (except preloaded windows).
     *
     * @param sessionConfigPath the path of the file where to save the session
     * @param mainWindow the window whose contents should be saved in the session
     * or `nullptr` to save the contents of all windows
     */
    void saveCurrentSessionToFile(const QString &sessionConfigPath, KonqMainWindow *mainWindow = nullptr);

    /**
     * @brief The directory where sessions will be autosaved
     *
     * This is the `autosave` subdirectory in the `QStandardPaths::AppDataLocation` directory
     * (usually, this is `$HOME/.local/share/konqueror/autosave`).
     *
     * @return the directory where sessions will be autosaved
     */
    QString autosaveDirectory() const;

    void registerMainWindow(KonqMainWindow *window);

    /**
     */
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
     * @brief Saves the current session
     *
     * The current session comprises the list of all main windows (excluding preloaded ones)
     * and their tabs and views.
     *
     * This is function is called by the autosave timer, but it can also called
     * manually. It won't do anything unless autosave is enabled (even if called manually).
     */
    void autoSaveSession();

    /**
     * @brief Emits a DBus signal telling all Konqueror instances to save the current
     * session
     *
     * @param path the directory in which sessions should be saved. Instances will
     * create the sessions directory under this directory
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

    /**
     * @brief Slot called in response to the application's `commitDataRequest()` signal
     *
     * It ensures that Konqueror isn't restarted if there are only preloaded windows.
     *
     * @param sm the `QSessionManager` to use
     */
    void slotCommitData(QSessionManager &sm);

private:
    KonqSessionManager(); //!< Default constructor

    ~KonqSessionManager() override; //!< Destructor

    /**
     * @brief Takes ownership of automatically saved sessions which aren't being restored by other instances
     *
     * It creates a directory under the path returned by dirForMyOwnedSessionFiles() and moves inside it
     * the files and directories corresponding to autosaved sessions whihc don't belong to other instances
     *
     * @return a list of paths corresponding to the sessions to restore and an empty list if there
     * are no sessions to restore
     *
     * @note Since Konqueror is now a single instance application, there shouldn't be need for _ownership_ anymore
     */
    QStringList takeSessionsOwnership();

    /**
     * @brief The directory where sessions owned by this instance should be moved to
     *
     * @return The directory where sessions owned by this instance should be moved to.
     * It's a directory located directly under #m_autosaveDir and its name starts with
     * `owned_by` followed by #m_baseService
     */
    QString dirForMyOwnedSessionFiles() const
    {
        return m_autosaveDir + "/owned_by" + m_baseService;
    }

    /**
     * @brief Writes the current session to a configuration object
     *
     * @param config the configuration object to write the session to
     * @param mainWindows a list of windows to save in the session file. If empty,
     * all the windows will be saved in the session
     */
    void saveCurrentSessionToFile(KConfig *config, const QList<KonqMainWindow *> &mainWindows = QList<KonqMainWindow *>());

private:
    QTimer m_autoSaveTimer; //!< Timer whose timeout will trigger autosaving of the current session

    /**
     * @brief The directory where sessions should be autosaved
     *
     * It's an `autosave` directory under the `QStandardPaths::AppDataLocation` directory
     * (usually `$HOME/.local/share/konqueror`).
     */
    QString m_autosaveDir;
    QString m_baseService; //!< DBus identifier of this Konqueror instance
    bool m_autosaveEnabled; //!< Whether session autosaving is enabled or not
    bool m_createdOwnedByDir; //!< Whether the directory to store owned sessions has already been created
    KConfig *m_sessionConfig; //!< The configuration object for autosaving sessions

#ifdef KActivities_FOUND
    ActivityManager *m_activityManager; //!< The activity manager
#endif

Q_SIGNALS: // DBUS signals
    /**
     * @brief DBus signal which instructs all Konqueror instances (including this one) to save the current session
     *
     * @param path the directory where sessions file should be saved
     */
    void saveCurrentSession(const QString &path);
private Q_SLOTS:// connected to DBUS signals

    /**
     * @brief Saves the current session to an owned directory in the given directory
     *
     * @param path the directory where to save the session in
     */
    void slotSaveCurrentSession(const QString &path);
};

#endif /* KONQSESSIONMANAGER_H */
