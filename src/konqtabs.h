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
     * @brief Signal emitted when the user chooses an action in the "other tabs" submenu
     */
    void subMenuTriggered(QAction *action);

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

class KONQ_TESTS_EXPORT KonqFrameTabs : public KTabWidget, public KonqFrameContainerBase
{
    Q_OBJECT

public:
    KonqFrameTabs(QWidget *parent, KonqFrameContainerBase *parentContainer,
                  KonqViewManager *viewManager);
    ~KonqFrameTabs() override;

    bool accept(KonqFrameVisitor *visitor) override;

    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options,
                            KonqFrameBase *docContainer, int id = 0, int depth = 0) override;
    void copyHistory(KonqFrameBase *other) override;

    const QList<KonqFrameBase *> &childFrameList() const
    {
        return m_childFrameList;
    }

    void setTitle(const QString &title, QWidget *sender) override;
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    QWidget *asQWidget() override
    {
        return this;
    }
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::Tabs;
    }

    void activateChild() override;

    /**
     * Insert a new frame into the container.
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;

    /**
     * Call this before deleting one of our children.
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame) override;

    /**
     * Returns the tab at a given index
     * (same as widget(index), but casted to a KonqFrameBase)
     */
    KonqFrameBase *tabAt(int index) const;

    /**
     * Returns the current tab
     * (same as currentWidget(), but casted to a KonqFrameBase)
     */
    KonqFrameBase *currentTab() const;

    void moveTabBackward(int index);
    void moveTabForward(int index);

    void setLoading(KonqFrameBase *frame, bool loading);

    /**
     * Returns the tab index that contains (directly or indirectly) the frame @p frame,
     * or -1 if the frame is not in the tab widget.
     */
    int tabIndexContaining(KonqFrameBase *frame) const;

    /**
     * Implemented to catch MMB click when Konq::Settings::mouseMiddleClickClosesTab()
     * returns true so that the tab can be properly closed without being activated
     * first.
     */
    bool eventFilter(QObject *, QEvent *) override;

    KonqMainWindow *mainWindow() const;

    int tabCount() const;

    QList<QAction*> otherTabsActions() const;

public Q_SLOTS:
    void slotCurrentChanged(int index);
    void setAlwaysTabbedMode(bool);
    void forceHideTabBar(bool force);
    void reparseConfiguration();
    void moveTab(int oldIdx, int newIdx);

Q_SIGNALS:
    void removeTabPopup();
    void openUrl(KonqView *view, const QUrl &url);
    //NOTE: we can't use the standard tabAdded and tabRemoved names as tabRemoved is already defined
    //by QTabWidget
    void tabHasBeenAdded(int idx);
    void tabHasBeenRemoved(int idx);

protected:
    void tabInserted(int idx) override;
    void tabRemoved(int idx) override;

private:
    void updateTabBarVisibility();
    void initPopupMenu();
    /**
     * Returns the index position of the tab where the frame @p frame is, assuming that
     * it's the active frame in that tab,
     * or -1 if the frame is not in the tab widget or it's not active.
     */
    int tabWhereActive(KonqFrameBase *frame) const;

    /**
     * @brief Applies the user choice regarding the tab bar position
     *
     * If the option contains an invalid value, `North` will be used
     */
    void applyTabBarPositionOption();

private Q_SLOTS:
    void slotContextMenu(const QPoint &);
    void slotContextMenu(QWidget *, const QPoint &);
    void slotCloseRequest(int);
    void slotMovedTab(int, int);
    void slotMouseMiddleClick();
    void slotMouseMiddleClick(QWidget *);

    void slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */);
    void slotReceivedDropEvent(QDropEvent *);
    void slotInitiateDrag(QWidget *);
    void slotReceivedDropEvent(QWidget *, QDropEvent *);
    void slotSubPopupMenuTabActivated(QAction *);

private:
    QList<KonqFrameBase *> m_childFrameList;

    KonqViewManager *m_pViewManager;
    TabBarContextMenu *m_pPopupMenu;
    QToolButton *m_rightWidget;
    NewTabToolButton *m_leftWidget;
    bool m_permanentCloseButtons;
    bool m_alwaysTabBar;
    bool m_forceHideTabBar;
    QMap<QString, QAction *> m_popupActions;
};

#include <QToolButton>

class NewTabToolButton : public QToolButton // subclass with drag'n'drop functionality for links
{
    Q_OBJECT
public:
    NewTabToolButton(QWidget *parent)
        : QToolButton(parent)
    {
        setAcceptDrops(true);
    }

Q_SIGNALS:
    void testCanDecode(const QDragMoveEvent *event, bool &accept);
    void receivedDropEvent(QDropEvent *event);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        bool accept = false;
        emit testCanDecode(event, accept);
        if (accept) {
            event->acceptProposedAction();
        }
    }

    void dropEvent(QDropEvent *event) override
    {
        emit receivedDropEvent(event);
        event->acceptProposedAction();
    }
};

#endif // KONQTABS_H
