//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef KONQINTERFACES_WINDOW_H
#define KONQINTERFACES_WINDOW_H

#include "libkonq_export.h"

#include <QObject>
#include <QMenu>

class QWidget;
class QAction;

namespace KonqInterfaces {

/**
 * @brief Interface for the menu displayed when the user right-clicks on the tab bar
 *
 * Using this interface allows other parts of Konqueror to display the same menu without
 * having to reimplement it.
 *
 * The menu assumes that it was activated on a tab (usually, by right-clicking on it):
 * this tab is called the **working tab** and doesn't need to be the current tab (the tab
 * currently visible in the main window). Actions acting on a single tab will act on the
 * working tab, while actions acting on "other tabs" will act on any tab except the working
 * tab.
 *
 * @warning You should never call any of the `QMenu::exec()` overloads in this class. You should
 * use execWithWorkingTab() instead, which correctly sets the working tab and ensures the correct
 * actions are enabled and the list of tabs is up to date.
 */
class LIBKONQ_EXPORT TabBarContextMenu : public QMenu {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent the parent widget
     */
    TabBarContextMenu(QWidget *parent = nullptr);
    /**
     * @brief Replacement for `QMenu::exec()` which allows to specify the working tab
     *
     * @warning You should always call this method instead of using any of the overloads of `QMenu::exec()`
     *
     * @param pt the global position where the menu should be displayed
     * @param workingTab the working tab. Pass `std::nullopt` if, for any reason, there's no working tab
     * @return action chosen in the menu
     */
    virtual QAction* execWithWorkingTab(const QPoint &pt, std::optional<int> workingTab = std::nullopt) = 0;
};

/**
 * @brief Interface for interacting with the main window
 */
class LIBKONQ_EXPORT Window : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    Window(QObject* parent = nullptr);

    ~Window() override; //!< Destructor

    /**
     * @brief Casts the given object or one of its children to a Window
     *
     * This is similar to
     * @code
     * obj->findChild<Window*>();
     * @endcode
     * except that if @p obj derives from Window, it will be returned, regardless of whether any
     * of its children also derive from it.
     * @param obj the object to cast to a Window
     * @return @p obj or one of its children as a Window* or `nullptr` if neither @p obj nor its children derive from Window
     */
    static Window* window(QObject* obj);

    /**
     * @brief The widget representing the window
     * @return The widget representing the window
     */
    virtual QWidget *widget() = 0;

    /**
     * @brief The number of tabs in the window
     * @return The number of tabs in the window
     */
    virtual int tabsCount() const = 0;
    /**
     * @brief The favicon for a given tab
     * @param idx the index of the tab
     * @return the favicon for the tab at position @p idx
     */
    virtual QIcon tabFavicon(int idx) = 0;
    /**
     * @brief The title of a given tab
     * @param idx the index of the tab
     * @return the title of the tab at position @p idx
     */
    virtual QString tabTitle(int idx) const = 0;
    /**
     * @brief The URL of a given tab
     * @param idx the index of the tab
     * @return the URL of the tab at position @p idx
     */
    virtual QUrl tabUrl(int idx) const = 0;
    /**
     * @brief The index of the active tab
     * @return the index of the active tab
     */
    virtual int activeTab() const = 0;
    /**
     * @brief A menu behaving in the same way as the one shown when the user right-clicks on the tab bar
     * @return a menu behaving in the same way as the one shown when the user right-clicks on the tab bar
     * @note This should not be the _same_ object as the menu shown when the user right-clicks on the tab bar
     * but it should behave the same way. Most likely, it will be another instance of the same class
     * @param parent the menu parent widget
     * @return the menu
     */
    virtual TabBarContextMenu* tabBarContextMenu(QWidget *parent = nullptr) const = 0;

public Q_SLOTS:
    /**
     * @brief Activates a tab
     * @param idx the index of the tab to activate
     */
    virtual void activateTab(int idx) = 0;
    /**
     * @brief Moves a tab in the tab bar
     * @param fromIdx the index of the tab to move
     * @param toIdx the index of the position where the tab should be moved
     */
    virtual void moveTab(int fromIdx, int toIdx) = 0;

Q_SIGNALS:
    /**
     * @brief Signal emitted when a new tab is added to the tab bar
     * @param idx the index of the new tab
     */
    void tabAdded(int idx);
    /**
     * @brief Signal emitted when a tab is closed
     * @param idx the index of the closed tab
     */
    void tabRemoved(int idx);
    /**
     * @brief Signal emitted when a tab is moved
     * @param oldIdx the original index of the tab
     * @param newIdx the new index of the tab
     */
    void tabMoved(int oldIdx, int newIdx);
    /**
     * @brief Signal emitted when the a different tab becomes active
     * @param idx the index of the new active tab
     */
    void currentTabChanged(int idx);
    /**
     * @brief Signal emitted when the title of a tab changes
     * @param idx the index of the tab
     * @param title the new title of the tab
     */
    void tabTitleChanged(int idx, const QString &title);
    /**
     * @brief Signal emitted when the url of a tab changes
     * @param idx the index of the tab
     * @param title the new url of the tab
     */
    void tabUrlChanged(int idx, const QUrl &url);

};

}

#endif // KONQINTERFACES_WINDOW_H
