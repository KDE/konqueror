/*  This file is part of the KDE project

    SPDX-FileCopyrightText: 2002-2003 Konqueror Developers <konq-e@kde.org>
    SPDX-FileCopyrightText: 2002-2003 Douglas Hanley <douglash@caltech.edu>
    SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQTABS_H
#define KONQTABS_H

#include "konqframe.h"
#include "konqframecontainer.h"
#include "interfaces/window.h"

#include "ktabwidget.h"

#include <QKeyEvent>
#include <QList>
#include <QMenu>

class KonqView;
class KonqViewManager;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KConfig;
class QToolButton;

class NewTabToolButton;
class KonqFrameTabs;
class KonqMainWindow;

namespace KonqInterfaces {
    class TabBarContextMenu;
}

/**
 * @brief The context menu displayed when the user right-clicks on the tab bar
 *
 * It contains actions to manipulate tabs (create new tabs, duplicate the current tab,
 * close other tabs, ...).
 *
 * It implements the KonqInterfaces::TabBarContextMenu interface, so that the same menu
 * can be displayed by any element which implements a tabbar-like behavior.
 *
 * This menu makes use of the concept of *working tab* as described in KonqInterfaces::TabBarContextMenu.
 *
 * The menu is structured in two parts: the main menu contains actions acting only on the
 * working tab, while the submenu "other tabs" contains action acting on multiple tabs or
 * on tabs other than the working tab. The sub menu also contains a list of all open tabs.
 *
 * @warning You should never call any of the `QMenu::exec()` overloads in this class. You should
 * use execWithWorkingTab() instead, which correctly sets the working tab and ensures the correct
 * actions are enabled and the list of tabs is up to date.
 */
class KONQ_TESTS_EXPORT TabBarContextMenu : public KonqInterfaces::TabBarContextMenu
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param tabsContainer the window's tab container
     * @param parent the menu parent widget
     */
    TabBarContextMenu(KonqFrameTabs *tabsContainer, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~TabBarContextMenu() = default;

    /**
     * @brief Replacement for `QMenu::exec()` which also set the working tab and ensures the menu contents are up to date
     *
     * @warning You must use this method instead of any of the `QMenu::exec()` overloads.
     *
     * @param pt the global position where the menu should be displayed
     * @param workingTab the working tab. Pass `std::nullopt` if, for
     * any reason, there's no working tab
     * @return action chosen in the menu
     */
    QAction* execWithWorkingTab(const QPoint &pt, std::optional<int> workingTab = std::nullopt) override;

Q_SIGNALS:
    /**
     * @brief Signal emitted when the user chooses one of the actions to switch to a different tab
     *
     * @param idx the tab to switch to
     */
    void switchToTabTriggered(int idx);

private Q_SLOTS:

    /**
     * @brief Slot called when one of the actions in the All Tabs submenu is triggered
     *
     * If @p action is an action to switch to another tab, it emits the switchToTabTriggered() signal,
     * otherwise it does nothing.
     *
     * @param action the action which was triggered
     */
    void allTabsSubMenuTriggered(QAction *action);

private:
    /**
     * @brief Prepares the menu to be displayed
     *
     * It fills the list of tabs in the submenu and appropriately enables and disables actions
     * @param hasWorkingTab whether or not there is a working tab
     */
    void prepare(bool hasWorkingTab);

    /**
     * @brief Fills the list of tabs in the submenu
     *
     * The actions for tabs currently in the menu will be deleted.
     *
     * @param actions the list of actions corresponding to tabs to be put in the submenu
     */
    void setOtherTabsActions(const QList<QAction*> &actions);

private:
    /**
     * @brief Enum describing the actions contained in the menu
     */
    enum class TabAction {
        New, //!< Create a new tab
        Duplicate, //!< Duplicate the working tab
        Reload, //!< Reload the working tab
        AllTabs, //!< The "all tabs" submenu
        BreakOff, //!< Remove the working tab from the current window and insert it into a new window
        Remove, //!< Close the working tab
        ReloadAll, //!< Reload all tabs
        CloseOthers //!< Close all tabs except the working tab
    };

    QPointer<KonqFrameTabs> m_tabsContainer; //!< The tabs container
    QMap<TabAction, QAction *> m_actions; //!< All the actions in the menu (except those representing tabs)
    QMenu *m_subMenu; //!< The "other tabs" submenu
};

/**
 * @brief A tab widget which acts as a KonqFrameContainerBase
 *
 * Each tab contains exactly one child frame (which in turn may contain other frames); conversely,
 * each child frame (but not the child's children) corresponds to a tab.
 */
class KONQ_TESTS_EXPORT KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     * @param parentContainer the frame container which contains this widget
     * @param viewManager the view manager managing the views which this widget will contain
     */
    KonqFrameTabs(QWidget *parent, KonqFrameContainerBase *parentContainer,
                  KonqViewManager *viewManager);
    ~KonqFrameTabs() override; //!< Destructor

    /**
     * @brief Override of KonqFrameContainerBase::accept()
     *
     * It makes @p visitor visit itself and either all the child frames or just
     * the current current tab depending on what KonqFrameVisitor::visitAllTabs()
     * returns.
     *
     * @param visitor the visitor
     * @return `true` if all the visits succeed and `false` otherwise
     */
    bool accept(KonqFrameVisitor *visitor) override;

    /**
     * @brief Override of KonqFrameBase::saveConfig()
     *
     * It saves the state of each tab, the number of tabs and the index of the current
     * tab. Each tab is saved using the prefix `T`_n_, where _n_ is its index
     *
     * @param config the group where to write the information
     * @param prefix a string to add to each key to make it unique in the group
     * @param options which information to include in the config group
     * @param docContainer the doc container
     * @param id an identifier to use
     * @param depth the level inside the frame hierarchy
     */
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options,
                            KonqFrameBase *docContainer, int id = 0, int depth = 0) override;

    /**
     * @brief Override of KonqFrame::copyHistory()
     *
     * It copies the history of each tab. It does nothing if @p other is not of
     * type KonqFrameBase::Tabs.
     *
     * @param other the frame to copy history from
     */
    void copyHistory(KonqFrameBase *other) override;

    /**
     * @brief A list of all tabs as FrameBase objects
     * @return a list of all tabs as FrameBase objects
     */
    const QList<KonqFrameBase *> &childFrameList() const
    {
        return m_childFrameList;
    }

    /**
     * @brief Override of KonqFrameBase::setTitle()
     *
     * Sets the text of the tab containing to @p sender.
     *
     * @param title the new title
     * @param sender the widget which requested to change the title
     */
    void setTitle(const QString &title, QWidget *sender) override;

    /**
     * @brief Override of KonqFrameBase::setTabIcon()
     *
     * @param url the url of the tab
     * @param sender the widget which requested to change the icon
     */
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    /**
     * @brief Override of KonqFrameBase::asQWidget()
     *
     * @return `this`
     */
    QWidget *asQWidget() override
    {
        return this;
    }

    /**
     * @brief Override of KonqFrameBase::frameType()
     *
     * @return KonqFrameBase::Tabs
     */
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::Tabs;
    }

    /**
     * @brief Override of KonqFrameContainerBase::activateChild()
     *
     * It shows the tab containing the active child (if any) and calls KonqFrameBase::activateChild()
     * on it
     */
    void activateChild() override;

    /**
     * @brief Override of KonqFrameContainerBase::insertChildFrame()
     *
     * It creates a new tab containing @p frame at the given position.
     *
     * @param frame the frame to insert
     * @param index an index representing where the new frame should be inserted.
     * A value of -1 means to insert the frame at the end
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;

    /**
     * @brief Override of KonqFrameContainerBase::childFrameRemoved()
     *
     * It removes the tab containing @p frame
     *
     * @param frame the frame which will be deleted
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    /**
     * @brief Override of KonqFrameContainerBase::replaceChildFrame()
     *
     * It replaces @p oldFrame with @p newFrame in the tab which contained @p oldFrame
     * (after calling childFrameRemoved()), then makes that tab current.
     *
     * @param oldFrame the frame to replace
     * @param newFrame the frame to replace @p oldFrame with
     */
    void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame) override;

    /**
     * @brief The frame in the tab with the given index
     *
     * @param index the index of the tab
     * @return the child in the tab @p index or `nullptr` if there aren't tabs
     * corresponding to @p index
     */
    KonqFrameBase *tabAt(int index) const;

    /**
     * @brief The frame corresponding to the current tab
     * @return the frame corresponding to the current tab
     */
    KonqFrameBase *currentTab() const;

    /**
     * @brief Moves the tab at the given index towards the left by one position
     *
     * It does nothing if the given tab already is the left-most tab, that is if
     * @p index is 0.
     *
     * @param index the index of the tab to move
     */
    void moveTabBackward(int index);

    /**
     * @brief Moves the tab at the given index towards the right by one position
     *
     * It does nothing if the given tab already is the right-most tab, that is if
     * @p index equals `count()-1`.
     *
     * @param index the index of the tab to move
     */
    void moveTabForward(int index);

    /**
     * @brief Visually marks a tab to show the loading status of the given frame
     *
     * If @p frame is the active frame in one of the tabs, it changes the color of
     * the tab label text. Special colors are used to show that:
     * - the active frame in the tab is loading
     * - the active frame in a tab except the current one has finished loading.
     *
     * @param frame the frame whose loading status has changed. If it isn't the
     * active frame in its tab, nothing is done
     * @param loading whether @p frame is loading or has finished loading
     */
    void setLoading(KonqFrameBase *frame, bool loading);

    /**
     * @brief The index of the tab which contains the given frame
     *
     * @param frame the frame to retrieve the tab index for. It doesn't have to
     * be a direct child of the KonqFrameTabs (that is, it doesn't have to represent
     * a tab)
     * @return the index of the tab which contains @p frame
     */
    int tabIndexContaining(KonqFrameBase *frame) const;

    /**
     * @brief Override of `QObject::eventFilter()`
     *
     * It intercepts middle click press and release events on the tab bar if the
     * user chose to use a middle mouse click on a tab to close it. In this case,
     * it closes the tab on middle mouse click without first activating it.
     *
     * @param watched the object which received the event
     * @param event the received event
     * @return `true` if @p watched is the tab bar and event is a middle mouse click
     * or release event and the user has turned on closing tabs with a middle mouse
     * click. In all other cases, it returns `false`.
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * @brief The main window containing this widget
     * @return the main window containing this widget
     */
    KonqMainWindow *mainWindow() const;

    /**
     * @brief The number of tabs in the widget
     * @return the number of tabs in the widget
     */
    int tabCount() const;

    /**
     * @brief A list of actions corresponding to each tab
     *
     * @return a list of actions corresponding to each tab
     */
    QList<QAction*> otherTabsActions() const;

public Q_SLOTS:
    /**
     * @brief Slot called when the current tab changes
     *
     * It ensures the text of the tab has the current color (see setLoading()),
     * activates the active child in the new current tab and informs the main window
     * that the number of linkable views may have changed.
     *
     * @param index the new current tab
     */
    void slotCurrentChanged(int index);

    /**
     * @brief Toggles always displaying the tab bar
     *
     * If @p on is `true` the tab bar is shown even if there's only one tab, while
     * if it's `false` the tab bar is only shown when there are at least two tabs.
     *
     * @param on whether the tab bar should be shown even when there's only one tab
     */
    void setAlwaysTabbedMode(bool on);

    /**
     * @brief Toggles forcing hiding the tab bar
     *
     * If @p force is `true`, the tab bar will be hidden regardless of the number of
     * tabs it contains. If it's `false`, the tab bar will be made visible or hidden
     * as described in setAlwaysTabbedMode().
     *
     * @param force whether to force hiding the tab bar or not
     */
    void forceHideTabBar(bool force);

    /**
     * @brief Applies configuration options
     */
    void reparseConfiguration();

    /**
     * @brief Moves a tab in the tab bar
     *
     * @param oldIdx the index of the tab to move
     * @param newIdx the index of the position to move the tab to
     */
    void moveTab(int oldIdx, int newIdx);

Q_SIGNALS:

    /**
     * @brief Signal emitted when the user has requested to close a tab
     *
     * This signal should be emitted after calling KonqMainWindow::setWorkingTab() to
     * tell the main window which tab should be closed.
     */
    void removeTabPopup();

    /**
     * @brief Signal emitted when a drop operation requires a view to open an URL
     *
     * @param view the view which should open the URL
     * @param url the URL to open
     */
    void openUrl(KonqView *view, const QUrl &url);
    //NOTE: we can't use the standard tabAdded and tabRemoved names as tabRemoved is already defined
    //by QTabWidget
    /**
     * @brief Signal emitted when a new tab has been added
     * @param idx the index of the new tab
     */
    void tabHasBeenAdded(int idx);

    /**
     * @brief Signal emitted when a new tab has been removed
     * @param idx the index of the removed tab
     */
    void tabHasBeenRemoved(int idx);

protected:
    /**
     * @brief Override of KTabWidget::tabInserted() which emits the tabHasBeenAdded() signal
     */
    void tabInserted(int idx) override;

    /**
     * @brief Override of KTabWidget::tabRemoved() which emits the tabHasBeenRemoved() signal
     */
    void tabRemoved(int idx) override;

private:
    /**
     * @brief Updates the visibility of the tab bar
     *
     * By default, this shows the tab bar if there are at least two tabs and hides it otherwise.
     * If something forces the tab bar to be always visible or always hidden, then this will
     * show or hide it regardless of how many tabs it contains.
     *
     * @see m_alwaysTabBar
     * @see m_forceHideTabBar
     */
    void updateTabBarVisibility();

    /**
     * @brief Creates the context menu for the tab bar
     *
     * This stores a new TabBarContextMenu in #m_pPopupMenu and connects signal related to it
     */
    void initPopupMenu();

    /**
     * @brief Finds the tab whose active frame is the given frame
     *
     * @param frame the frame to look for in the tabs
     * @return the index of the tab containing @p frame if @p frame is the active frame in the tab
     * and -1 if @p frame isn't the active frame in the tab containing it or if no tab contains it
     */
    int tabWhereActive(KonqFrameBase *frame) const;

    /**
     * @brief Applies the user choice regarding the tab bar position
     *
     * If the option contains an invalid value, `North` will be used
     */
    void applyTabBarPositionOption();

private Q_SLOTS:

    /**
     * @brief Shows the context menu tab at the given point
     *
     * This assumes that the user requested the context menu from the current tab
     *
     * @param p the point where to show the context menu
     */
    void slotContextMenu(const QPoint &p);

    /**
     * @brief Override of slotContextMenu(const QPoint &) which allows to specify which tab the user requested the context menu for
     *
     * It shows the context menu for the given tab
     *
     * @param w the tab the user requested the context menu for. It must be one of the widgets
     * representing a tab
     * @param p the point where to show the menu
     */
    void slotContextMenu(QWidget *w, const QPoint &p);

    /**
     * @brief Slot called when the user requests to close a tab
     *
     * It calls KonqMainWindow::setWorkingTab() then emits the removeTabPopup() signal.
     *
     * @param idx the index of the tab to close
     */
    void slotCloseRequest(int idx);

    /**
     * @brief Slot called when a tab is moved
     *
     * It updates the list of child frames to reflect the new tab order and activates
     * the current frame.
     *
     * @param from the old position of the tab
     * @param to the new position of the tab
     */
    void slotMovedTab(int from, int to);

    /**
     * @brief Slot called when the user middle-clicks on an empty space in the tab bar
     *
     * If the clipboard selection contains a valid, non-error URL, it creates a new tab
     * and opens the URL in it.
     */
    void slotMouseMiddleClick();

    /**
     * @brief Slot called when the user middle-clicks on a tab
     *
     * If the clipboard selection contains a valid, non-error URL, it opens the URL in
     * the active view of the tab.
     *
     * @param w the tab where the user middle-clicked
     */
    void slotMouseMiddleClick(QWidget *w);

    /**
     * @brief Checks whether the given `QDragMoveEvent` can be accepted
     *
     * The event can be accepted if it contains URLs.
     *
     * @param e the event to check
     * @param [out] accept the variable where the result will be stored. This will
     * be `true` if the event can be accepted and `false` otherwise.
     */
    void slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */);

    /**
     * @brief Slot called in response to a drop event on an empty space
     *
     * If the event contains one or more URLs, it opens the first URL in a new tab
     *
     * @param e the drop event
     */
    void slotReceivedDropEvent(QDropEvent *e);

    /**
     * @brief Starts a drag & drop operation
     *
     * It starts a drag & drop operation with the URL of the active view in the dragged tab
     * @param w the tab which is being dragged
     */
    void slotInitiateDrag(QWidget *w);

    /**
     * @brief Slot called in response to a drop event on a given tab
     *
     * If the event contains one or more URLs, it opens the first URL in the tab's active view
     *
     * @param e the drop event
     */
    void slotReceivedDropEvent(QWidget *w, QDropEvent *e);

    /**
     * @brief Slot called when an action in the "All Tabs" submenu of the context menu is triggered
     *
     * @param action the action which was triggered
     */
    // void slotSubPopupMenuTabActivated(QAction *action);

private:
    QList<KonqFrameBase *> m_childFrameList; //!< A list of all frame children

    KonqViewManager *m_pViewManager; //!< The view manager
    TabBarContextMenu *m_pPopupMenu; //!< The popup menu used when the user right-clicks on the tab bar
    QToolButton *m_rightWidget; //!< The tool button on the right of the tab bar which closes the current tab
    NewTabToolButton *m_leftWidget; //!< The tool button on the left of the tab bar which creates a new tab
    bool m_permanentCloseButtons; //!< Whether to always show a close button on tabs
    bool m_alwaysTabBar; //!< Whether to show the tab bar even when there are less than 2 tabs
    bool m_forceHideTabBar; //!< Whether to always hide the tab bar
    QMap<QString, QAction *> m_popupActions; //!< UNUSED
};

#include <QToolButton>

/**
 * @brief Subclass of `QToolButton` with drag & drop support
 *
 * This class has no knowledge on whether to accept a drag event or what to do on a
 * drop event: it emits signals allowing other objects to decide how to react to those
 * events.
 *
 * @see testCanDecode()
 * @see receivedDropEvent()
 */
class NewTabToolButton : public QToolButton
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     *
     * @param parent the parent widget
     */
    NewTabToolButton(QWidget *parent)
        : QToolButton(parent)
    {
        setAcceptDrops(true);
    }

Q_SIGNALS:

    /**
     * @brief Signal emitted to determine whether to accept a drag enter event
     *
     * This signal is emitted in response to a drag enter event. Objects connecting
     * to this signals should decide whether the event should be accepted or not and
     * set @p accept accordingly.
     *
     * @param event the event
     * @param [out] accept whether or not the event should be accepted. By default,
     * this is `false`
     */
    void testCanDecode(const QDragMoveEvent *event, bool &accept);

    /**
     * @brief Signal emitted what to do in response to a drop event
     *
     * Objects connecting to this signal should handle the event.
     *
     * @param event the drop event
     */
    void receivedDropEvent(QDropEvent *event);

protected:

    /**
     * @brief Override of `QWidget::dragEnterEvent()`
     *
     * It emits the testCanDecode() signal and accepts the proposed action depending
     * on the value of its second argument
     * @param event the event
     */
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        bool accept = false;
        emit testCanDecode(event, accept);
        if (accept) {
            event->acceptProposedAction();
        }
    }

    /**
     * @brief Override of `QWidget::dropEvent()`
     *
     * It emits the receivedDropEvent() signal with argument @p event and accepts the proposed action
     * @param event the event
     */
    void dropEvent(QDropEvent *event) override
    {
        emit receivedDropEvent(event);
        event->acceptProposedAction();
    }
};

#endif // KONQTABS_H
