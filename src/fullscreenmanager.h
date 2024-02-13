// This file is part of the KDE project
// SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef FULLSCREENMANAGER_H
#define FULLSCREENMANAGER_H

#include <QObject>
#include <QDebug>

class KonqMainWindow;
class QAction;

/**
 * @brief Class which manages the full screen state of the main window
 *
 * There are three possible full screen states, corresponding to the values of the
 * FullScreenState enum:
 * - no full screen
 * - ordinary full screen: the window has no decoration and no menu bar. The content of
 *  the main window itself doesn't change
 * - complete full screen: it's only used on explicit request of one of the web pages,
 *  usually to play videos filling the whole screen. The main window has no decoration,
 *  menu, toolbars and togglable views are hidden and the current view takes all the space.
 *  Usually, the web page which requested this modality also changes its layout so that
 *  a single element (usually a video) fills all the available space
 */
class FullScreenManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param parent the main window
     */
    FullScreenManager(KonqMainWindow *parent);

    /**
     * Destructor
     */
    ~FullScreenManager();

    /**
     * @brief Reads the initial satus from the main window
     *
     * @warning The main window must call this function as soon as it is ready
     */
    void init();

    /**
     * @brief Enum describing the possible full screen states
     */
    enum class FullScreenState {
        NoFullScreen, //!<The window is not in a full screen state
        OrdinaryFullScreen, //!<The window is in ordinary full screen state
        CompleteFullScreen //!<The window is in complete full screen state
    };

    /**
     * @brief The current full screen state of the window
     * @return The current full screen state of the window
     */
    FullScreenState currentState() const {return m_currentState;}

public slots:
    /**
     * @brief Slot called when the "Toggle full screen" action is triggered
     *
     * It carries out all the operations needed to switch to the correct full screen
     * state.
     *
     * @note The same shortcut is used both for toggling the ordinary full screen and
     * for exiting the complete full screen (so that the user doesn't need to know the
     * difference between the two modalities). Because of this, this function is also
     * called when exiting complete full screen and may switch from complete full screen
     * to ordinary full screen
     * @param on whether the full screen state should be turned on or off. Note that if
     *  the window is currently in complete full screen, a value of `false` means only turning
     *  off complete full screen: if before switching to complete full screen the
     *  main window was in ordinary full screen, this function will restore ordinary
     *  full screen
     */
    void fullscreenToggleActionTriggered(bool on);

    /**
     * @brief Toggles on or off complete full screen
     *
     * When toggling complete full screen off, the window is restored to the state
     * it was in before activating complete full screen.
     * @param on whether to turn complete full screen on or off
     */
    void toggleCompleteFullScreen(bool on);

private:

    /**
     * @brief The main window associated with this object
     * @return The main window associated with this object
     */
    KonqMainWindow* mainWindow() const;
    //TODO KF6: after dropping compatibility with KF6, remove this and use mainWindow()->action() instead
    /**
     * @brief Retrieves an action with the given name from the main window
     * @param name the name of the action
     * @return the action with name @p name or `nullptr` if no such action exists
     */
    QAction* action(const QString &name);

    /**
     * @brief Switches to the given state
     *
     * If the new state is the same as the current state, nothing is done
     * @param newState the state to switch to
     */
    void switchToState(FullScreenState newState);

    /**
     * @brief Toggles on or off the menu bar as appropriate for switching to a given state
     * @param newState the state to switch to
     */
    void toggleMenuBar(FullScreenState newState);

    /**
     * @brief Toggles on or off a tool bar containing only the "Toggle full screen action"
     *
     * @param on whether the tool bar should be turned on or off
     */
    void toggleFullScreenToolbar(bool on);

    /**
     * @brief Toggles on or off toggle views appropriate for switching to a given state
     * @param newState the state to switch to
     */
    void toggleToggableViews(FullScreenState newState);

    /**
     * @brief Toggles on or off the tool bars as appropriate for switching to a given state
     * @param newState the state to switch to
     */
    void toggleToolbars(FullScreenState newState);

    /**
     * @brief Toggles on or off the tool bars as appropriate for switching to a given state
     * @param newState the state to switch to
     */
    void toggleStatusBar(FullScreenState newState);

    /**
     * @brief Hides or shows all frames in current tab except for the current one
     *
     * This is needed when switching to complete full screen because it requires one view
     * to fill the whole screen, and switching away from complete full screen to restore the
     * original state.
     *
     * @note When switching away from complete full screens, all frames in the current tab
     * are made visible: this assumes that there are no other reasons for a frame to be hidden
     * except for complete full screen; if any other reason is introduced, then it becomes
     * necessary to keep a list of views to make visible when exiting complete full screen
     * @param  newState the state to switch to
     */
    void hideOtherViews(FullScreenState newState);

    FullScreenState m_previousState; //!< The previous full screen state
    FullScreenState m_currentState; //!< The current full screen state
    bool m_wasMenuBarVisible = true; //!< Whether the menu bar was visible before switching to full screen state
    bool m_wasStatusBarVisible = true; //!< Whether the status bar was visible before switching to complete full screen state
    QStringList m_visibleToolBars; //!< Which toolbars were visible before switching to complete full screen state
    QStringList m_visibleViewGUIClients; //!< Which toggle views were visible before switching to complete full screen state
};

QDebug& operator<< (QDebug &dbg, FullScreenManager::FullScreenState state);

#endif // FULLSCREENMANAGER_H
