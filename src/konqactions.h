/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __konq_actions_h__
#define __konq_actions_h__

#include <ktoolbarpopupaction.h>
#include <konqhistorymanager.h>
#include <kactionmenu.h>
#include <QList>

struct HistoryEntry;
class QMenu;

namespace KonqActions
{

/**
* @brief Fills a popup menu to navigate backwards or forwards in history
*
* It creates actions describing each history entry from the current one (going
* forwards or backwards depending on the values of @p onlyForward and @p onlyBack),
* sets its icon, its text and adds data describing its index ine the history list
* relative to the current index (@p historyIndex). The text is elided to a maximum
* of 30 characters and any & is escaped. Each action is then inserted in the
* popup menu.
*
* @param history the history entries to navigate in
* @param historyIndex the current position in @p history
* @param popup the popup menu to fill. It must not be `nullptr`
* @param onlyBack whether or not the menu should only allow to go back in history
* @param onlyForward whether or not the menu should only allow to go forward in history
* @note according to comments, at least one of @p onlyBack and @p onlyForward must always be true
* @todo Replace @p onlyBack and @p onlyForward with either a single boolean (`navigateBackwards`) or
* an enum `NavigationMode {Backwards, Forwards}.
*/
void fillHistoryPopup(const QList<HistoryEntry *> &history, int historyIndex,
                      QMenu *popup,
                      bool onlyBack,
                      bool onlyForward);
}

#if 0
/**
 * Plug this action into a menu to get a bidirectional history
 * (both back and forward, including current location)
 */
class KonqBidiHistoryAction : public KToolBarPopupAction
{
    Q_OBJECT
public:
    KonqBidiHistoryAction(const QString &text, QObject *parent);
    virtual ~KonqBidiHistoryAction();

    void fillGoMenu(const QList<HistoryEntry *> &history, int historyIndex);

protected Q_SLOTS:
    void slotTriggered(QAction *action);

Q_SIGNALS:
    void menuAboutToShow();
    // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
    void step(int);
private:
    int m_startPos;
    int m_currentPos; // == history.at()
};

#endif

/////

/**
 * @brief Action menu which contains actions to go to the most often visited URLs
 */
class KonqMostOftenURLSAction : public KActionMenu
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     * @param text the text of the action
     * @param parent the parent object
     */
    KonqMostOftenURLSAction(const QString &text, QObject *parent);

    /**
     * @brief Destructor
     */
    ~KonqMostOftenURLSAction() override;

    /**
     * @brief Compares the number of times two KonqHistoryEntry have been visited
     * @param lhs the first history entry
     * @param rhs the second history entry
     * @return `true` if @p lhs has been visited less times than @p rhs and false otherwise
     */
    static bool numberOfVisitOrder(const KonqHistoryEntry &lhs, const KonqHistoryEntry &rhs)
    {
        return lhs.numberOfTimesVisited < rhs.numberOfTimesVisited;
    }

Q_SIGNALS:

    /**
     * @brief Signal emitted when an action in the menu is activated
     * @param url the URL corresponding to the activated action
     */
    void activated(const QUrl &url);

private Q_SLOTS:

    /**
     * @brief Slot called when history is cleared
     *
     * It clears the list of most visited URLs and disables the menu itself
     */
    void slotHistoryCleared();

    /**
     * @brief Updates the menu when an entry is added to history
     *
     * The menu is only updated if the @p entry is, or has become, one of the most often
     * visited URLs. If needed, this can cause one action to be removed from the menu.
     *
     * @param entry the new history entry
     */
    void slotEntryAdded(const KonqHistoryEntry &entry);

    /**
     * @brief Updates the menu when an entry is removed from history
     *
     * If @p entry was one of the most often used URLs, it's removed from the menu
     * @param entry the removed history entry
     * @note Unlike slotEntryAdded, this doesn't add a less used action to the menu if
     * @p entry was removed
     */
    void slotEntryRemoved(const KonqHistoryEntry &entry);

    /**
     * @brief Fills the menu creating actions according to history contents
     *
     * This slot is connected to the `aboutToShow()` signal, so that the menu can be
     * updated before showing it to the user
     */
    void slotFillMenu();

    /**
     * @brief Emits the activated() signal
     *
     * This slot is called in response to the `QMenu::triggered(QAction*)` signal and
     * emits the activated() signal passing the `QUrl` stored inside the action as parameter.
     * @param action the triggered action
     */
    void slotActivated(QAction *action);

private:

    /**
     * @brief Initializes the state of the object
     *
     * @note Can't this just go in the constructor?
     */
    void init();

    /**
     * @brief Reads the contents of history and fills the list of most often used URLs
     *
     * It also connects the slotEntryAdded(), slotEntryRemoved() and slotHistoryCleared()
     * to the appropriate signals of the KonqHistoryManager.
     *
     * @note This function is only called once, the first time slotFillMenu() is called: after
     * that, the list of visited URLs is kept up to date by slotEntryAdded(),
     * slotEntryRemoved() and slotHistoryCleared().
     */
    void parseHistory();

    /**
     * @brief Inserts the given history entry in the appropriate place in the list of
     * visited URLs
     *
     * @param entry the entry to place in the list. If it already is in the list, it won't be
     * inserted again: the existing item will be moved to the correct place
     */
    static void inSort(const KonqHistoryEntry &entry);

    bool m_parsingDone; //!< Whether or not parseHistory() has already been called
};

/////

/**
 * @brief Action menu which contains actions to go to the most recently visited URLs
 */
class KonqHistoryAction : public KActionMenu
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     * @param text the text of the action
     * @param parent the parent object
     */
    KonqHistoryAction(const QString &text, QObject *parent);

    /**
     * @brief Destructor
     */
    ~KonqHistoryAction() override;

Q_SIGNALS:

    /**
     * @brief Signal emitted when an action in the menu is activated
     * @param url the URL corresponding to the activated action
     */
    void activated(const QUrl &url);

private Q_SLOTS:

    /**
     * @brief Fills the menu creating actions according to history contents
     *
     * This slot is connected to the `aboutToShow()` signal, so that the menu can be
     * updated before showing it to the user
     */
    void slotFillMenu();

    /**
     * @brief Emits the activated() signal
     *
     * This slot is called in response to the `QMenu::triggered(QAction*)` signal and
     * emits the activated() signal passing the `QUrl` stored inside the action as parameter.
     * @param action the triggered action
     */
    void slotActivated(QAction *action);
};

#endif
