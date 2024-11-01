/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef VERTICAL_TABBAR_H
#define VERTICAL_TABBAR_H

#include <konqsidebarplugin.h>

#include <QPointer>

class VerticalTabBarModel;

class QListView;
class QStandardItemModel;
class QStandardItem;
class QItemSelection;

namespace KonqInterfaces {
    class Window;
    class TabBarContextMenu;
}

/**
 * Sidebar module acting like a tab bar
 *
 * This widget displays a list of all tabs in the current window and allows the user to interact with
 * them as he would using the usual tab bar. This includes:
 * - activating them by clicking on their title
 * - moving them by drag and drop
 * - displaying the same menu as when the user right-clicks on the tab bar
 *
 * As this module uses a `QListView` as widget, each tab s represented by a `QModelIndex`
 *
 * @todo
 * - Add ability to indent/outdent tabs (thereby effectively creating child tabs), through mouse dragging and/or keyboard shortcuts
 * - Add ability to have new tabs automatically created as children of current tabs if certain conditions are fulfilled (e.g. 'open new tab within some domain / server'.
 * - Add ability to display the views contained in a tab (as children items?)
 * @see https://bugs.kde.org/show_bug.cgi?id=205276
 */
class KonqVerticalTabBar : public KonqSidebarModule
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent the parent widget
     * @param configGroup the config group used for this sidebar widget
     */
    KonqVerticalTabBar(QWidget *parent, const KConfigGroup &configGroup);

    ~KonqVerticalTabBar() override; //!< Destructor

    /**
     * @brief The widget associated with the module
     * @return the widget associated with the module
     */
    QWidget *getWidget() override;

private Q_SLOTS:
    /**
     * @brief activates the tab corresponding to the given index
     * @param idx the index corresponding to the tab to activate
     */
    void activateItem(const QModelIndex &idx);

    /**
     * @brief Selects the item associated with the given tab
     * @param tabIdx the number of the tab to select
     */
    void selectTab(int tabIdx);

    /**
     * @brief Displays the context menu at the given position
     * @param pt the position to display the menu at, in view coordinates
     */
    void displayContextMenu(const QPoint &pt);

private:
    KonqInterfaces::Window* m_window; //!< The main window
    QListView *m_view; //!< The view used by the module
    VerticalTabBarModel *m_model; //!< The model containing information about the tabs
    QPointer<KonqInterfaces::TabBarContextMenu> m_contextMenu; //!< The tab contex menu
};

#endif
