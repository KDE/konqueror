/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_APPLICATION_H
#define KONQ_APPLICATION_H

#include "konqprivate_export.h"
#include <config-konqueror.h>

#include <QApplication>

#include <QCommandLineParser>

#ifdef KActivities_FOUND
namespace KActivities {
    class Consumer;
}
#endif

class KonqMainWindow;
class QDBusMessage;
class KonqBrowser;

namespace KParts {
    class Part;
}

/**
 * @brief Class representing an instance of Konqueror application
 *
 * It handles starting up (including command line arguments), session restoring,
 * setting up the activity manager and preloading windows.
 *
 * It also provides an object implementing the Konq::Browser interface.
 *
 * @warning Due to `QtWebEngine` requirements, Konqueror is a single-process, multiple-window program.
 * This means that if a second instance of Konqueror is launched, it must quickly exit and let the existing
 * instance handle things. An exception is made when Konqueror is run as root, when a separate process is
 * always used.
 *
 * @note If Konqueror was built with the `-DDEVELOPER_MODE=ON` cmake option, it will be possible to launch
 * multiple Konqueror processes by passing the `--force-new-process` command line switch. However, this is
 * meant only for use when developing Konqueror
 */
class KONQ_TESTS_EXPORT KonquerorApplication : public QApplication
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @param argc as in `QApplication` constructor
     * @param argv as in `QApplication` constructor
     */
    KonquerorApplication(int &argc, char **argv);

    /**
     * @brief Wrapper for `QApplication::exec()`
     *
     * This function takes care of:
     * - setting up the `KAboutData` for the application
     * - setting up the command line parser
     * - parsing command line options
     * - activating an existing Konqueror process if it exists, using a `KDBusService`
     *
     * If no other instance of Konqueror already exists (or if being run as root
     * or with the --force-new-process` command line option) it calls
     * startFirstInstance() to actually start a new Konqueror instance.
     */
    int start();

    /**
     * @brief Returns the id of the current activity
     * @return The id of the current activity or an empty string if activity support
     * is disabled
     */
    static QString currentActivity();

    /**
     * @brief Whether the activity service is running
     * @return `true` if the activity service is running and `false` otherwise
     * (including if activity support is disabled)
     */
    bool isActivityServiceRunning() const;

public slots:
    /**
     * @brief Read again the settings from configuration files and emits the configurationChanged() signal
     *
     * It also calls KonqMainWindow::reparseConfiguration for each window
     */
    void slotReparseConfiguration();

    /**
     * @brief Closes all windows, causing the application itself to be closed
     *
     * If there are multiple windows, the user is asked for confirmation before
     * closing Konqueror. He can choose to close only @p window.
     *
     * @param window the window which requested closing the application. It can
     * be `nullptr`
     */
    void quitKonqueror(KonqMainWindow *window);

signals:

    /**
     * @brief Signal emitted when the configuration changes
     *
     * Anything which reads the configuration files (directly or not) can connect to this
     * signal to be notified when the configuration has changed
     */
    void configurationChanged();

    /**
     * @brief Signal emitted just before the configuration dialog is shown by a window
     */
    void aboutToConfigure();
    void newWindowCreated(KonqMainWindow *mw);

private slots:
    /**
     * @brief Adds an URL to the location bar for all windows using DBus
     */
    void slotAddToCombo(const QString &url, const QDBusMessage &msg);
    /**
     * @brief Remove an URL to the location bar for all windows using DBus
     */
    void slotRemoveFromCombo(const QString &url, const QDBusMessage &msg);
    /**
     * @brief Clears the location bar history for all windows using DBus
     */
    void slotComboCleared(const QDBusMessage &msg);

private:

    /**
     * @brief Type used to describe the result of creating a window
     *
     * The first element is a pointer to the created window, while the second is
     * `0` if no error occurred and `1` if the window couldn't be created because
     * of errors. If the second element is 1, the first is always `nullptr`.
     *
     * @note It's possible that the second element is `0` but the first element
     * is `nullptr` because no window was created. This happens if the function
     * creating the window determines there's no _need_ to create a window.
     */
    using WindowCreationResult = QPair<KonqMainWindow*, int>;

    /**
     * @brief Fills the `KAboutData` object describing the application
     */
    void setupAboutData();

    /**
     * @brief Sets up the command line parser
     */
    void setupParser();

    /**
     * @brief Executes the application
     *
     * This assumes that there are no other instances of Konqueror running (except
     * in the case when it's run as root or using the `--force-new-process` switch).
     *
     * Besides calling `QApplication::exec()`, this function ensure sessions are restored,
     * handles command line arguments and ensures that a new instance is run *after* this
     * one has exited, if the user chose to always keep an instance preloaded.
     *
     * @return The value returned by `QApplication::exec()`
     */
    int startFirstInstance();

    /**
     * @brief Opens any window requested by the command line arguments
     *
     * If the `--preload` command line argument is given, a preloaded window is created.
     * If the `--preload` command line argument is not given and no command line arguments
     * require the creation of a window, a new empty window is created.
     *
     * This function also handles the restoring of the previous Konqueror session,
     * either crashed sessions or the last session if the user chose to restore it.
     *
     * @param workingDirectory the directory Konqueror should consider as _current_
     * @param firstInstance whether this function was called when creating the first Konqueror
     * process or when activating an existing Konqueror when a second one was requested.
     * @note This function is called whenever the user asks for a new Konqueror instance,
     * even if it will use an existing process
     * @note This function doesn't restore X session, which are handled by startFirstInstance()
     */
    int performStart(const QString &workingDirectory, bool firstInstance = false);

    /**
     * @brief Restores the session which was saved when logging out
     */
    void restoreSession();

    /**
     * @brief Writes to `stdout` a list of the available sessions saved by the user
     */
    void listSessions();

    /**
     * @brief Loads the session with the given name
     *
     * Each session is a directory in `$XDG_DATA_HOME/konqueror/sessions` and contains a file for each window
     * @return 0 if at least one window was created and 1 otherwise
     */
    int openSession(const QString &session);
    //TODO: remove the second argument when workaround with activities won't be necessary anymore
    //See the comment in the body of createEmptyWindow()
    /**
     * @brief Attempts to create an empty window, if no other window exists and
     * no command line options preventing its creation where given
     *
     * @param firstInstance `true` if this is the first instance of the application and
     * `false` if an already existing instance is being activated
     * @param calledFromPerformStart `true` if this function is called by performStart() and `false` otherwise
     * @return a WindowCreationResult describing the result of the window creation
     */
    WindowCreationResult createEmptyWindow(bool firstInstance, bool calledFromPerformStart = false);

    /**
     * @brief Handles the `--preload` command line switch
     *
     * It creates a preloaded window, displaying a message on `stderr` if URLs were
     * given on the command line.
     *
     * @param args the URLs passed on the command line
     */
    void preloadWindow(const QStringList &args);

    /**
     * @brief Opens the URLs given on the command line in a window, creating it if necessary
     *
     * @param args the list of URLs passed on the command line
     * @param workingDirectory the _current_ directory
     * @param mainwin the window where to open @p args. If `nullptr`, the URLs
     * will be opened in a new window
     * @return a WindowCreationResult having as first element the window where the URLs were
     * opened
     */
    WindowCreationResult createWindowsForUrlArguments(const QStringList &args, const QString &workingDirectory, KonqMainWindow *mainwin= nullptr);

    /**
     * @brief What to do when running Konqueror as root
     */
    enum KonquerorAsRootBehavior {
        NotRoot, //!< Konqueror is not being run as root
        PreventRunningAsRoot, //!< The user tried to run Konqueror as root but answered to exit when asked
        RunInDangerousMode //!< The user decided to run Konqueror as user enabling the `--no-sandbox` chromium flag
    };

    /**
     * @brief Checks whether Konqueror is being run as root and, if so, asks the user what to do
     * @return NotRoot if Konqueror isn't being run as root or PreventRunningAsRoot or RunInDangerousMode, according
     * to the user's choice, otherwise
     */
    static KonquerorAsRootBehavior checkRootBehavior();

private:
    QCommandLineParser m_parser; //!< The command line parser
    bool m_sessionRecoveryAttempted = false; //!< Whether a session recovery has been attempted after starting the application
    KonquerorAsRootBehavior m_runningAsRootBehavior = NotRoot; //!< What to do when the user attempts to run as root
    KonqBrowser *m_browser; //!< The object implementing the KonqInterfaces::Browser interface
    bool m_forceNewProcess = false; //!< Whether the `--force-new-process` switch has been given on the command line

#ifdef KActivities_FOUND
    KActivities::Consumer* m_activityConsumer; //!< The object used to communicate with the activity service
#endif

};

#endif
