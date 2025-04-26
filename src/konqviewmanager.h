/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_viewmanager_h__
#define __konq_viewmanager_h__

#include "konqprivate_export.h"
#include "konqfactory.h"
#include "konqframe.h"
#include "konqopenurlrequest.h"

#include <QMap>
#include <QPointer>
#include <QUrl>

#include <KService>
#include <KParts/PartManager>
#include <KSharedConfig>
#include <KPluginMetaData>

class KonqFrameTabs;
class QString;
class KConfig;
class KConfigGroup;
class KonqMainWindow;
class KonqFrameBase;
class KonqFrameContainer;
class KonqFrameContainerBase;
class KonqView;
class KonqClosedTabItem;
class KonqClosedWindowItem;

namespace KParts
{
class ReadOnlyPart;
}

/**
 * @brief Class which handles the views in a main window
 *
 * Among other things, this class provides functions to create views, add and remove
 * tabs, split views, move a view from the current window to a new one, load a view
 * hierarchy from a configuration file, move tabs, change the active tab and remove tabs.
 */
class KONQ_TESTS_EXPORT KonqViewManager : public KParts::PartManager
{
    Q_OBJECT
public:

    /**
     * @brief Constructor
     *
     * @param mainWindow the main window whose views should be managed
     */
    explicit KonqViewManager(KonqMainWindow *mainWindow);

    ~KonqViewManager() override; //!< Destructor

    /**
     * @brief Creates the first view in the window
     *
     * @note This doesn't check that there are no other views in the window: it's
     * up to the caller to make sure of it.
     *
     * @param mimeType the mimetype which will be shown in the window
     * @param serviceName the plugin id of the part to use in the view. Leave empty
     * to use the default part for @p mimeType
     */
    KonqView *createFirstView(const QString &mimeType, const QString &serviceName);

    /**
     * @brief Splits the view either horizontally or vertically
     *
     * The first view in the splitter will contain the original view, the other
     * will be a new one , constructed from the same part as the original view.
     *
     * The width or height (depending on @p orientation) of the original view is
     * split in half between the original and the new view.
     *
     * @param currentView the view to split
     * @param orientation the direction the view should be split: if `Qt::Horizontal`,
     * the view will be split in a left and a right view; if `Qt::Vertical` it will
     * be split in a top and a bottom view
     * @param newOneFirst `true` if the new view should become the first one (left or top)
     * and `false` if it should become the second one (right or bottom)
     * @param forceAutoEmbed whether or not to always prefer embedding rather than
     * opening in an external application
     * @return the newly created view or `nullptr` if the view couldn't be created
     */
    KonqView *splitView(KonqView *currentView,
                        Qt::Orientation orientation,
                        bool newOneFirst = false, bool forceAutoEmbed = false);

    /**
     * @brief Splits the main container either horizontally or vertically
     *
     * The main container is the only KonqFrameBase which is a direct child of the
     * main window.
     *
     * This is mainly used when creating views for the sidebar or the terminal emulator.
     *
     * @param currentView the view to split
     * @param orientation the direction the view should be split: if `Qt::Horizontal`,
     * the view will be split in a left and a right view; if `Qt::Vertical` it will
     * be split in a top and a bottom view
     * @param type the view type of the new view
     * @param serviceName the plugin id of the part to use in the new view. If empty,
     * the preferred part for @p type will be used
     * @param newOneFirst `true` if the new view should become the first one (left or top)
     * and `false` if it should become the second one (right or bottom)
     * @return the new view or `nullptr` if the view couldn't be created
     */
    KonqView *splitMainContainer(KonqView *currentView,
                                 Qt::Orientation orientation,
                                 const Konq::ViewType &type = QString(),
                                 const QString &serviceName = QString(),
                                 bool newOneFirst = false);

    /**
     * @brief Adds a new tab to the tab container
     *
     * A new view is created for the new tab.
     *
     * @param type the type of the new view
     * @param serviceName the plugin id of the part to use for the new view. Leave
     * empty to use the preferred part for @p type
     * @param passiveMode whether the new view should be a passive one
     * @param openAfterCurrentPage `true` if the new tab should be created after the
     * current one or `false` if it should be created at another position
     * @param pos the position in the tab container where the new tab should be
     * inserted. If negative, the new tab will be inserted at the end of the tab
     * container . This is ignored if @p openAfterCurrentPage is `true`
     * @return the view in the new tab
     */
    KonqView *addTab(const Konq::ViewType &type,
                     const QString &serviceName = QString(),
                     bool passiveMode = false, bool openAfterCurrentPage = false, int pos = -1);

    /**
     * @brief Duplicates the given tab
     *
     * @param tabIndex the tab to duplicate
     * @param openAfterCurrentPage `true` if the new tab should be created after the
     * current one or `false` if it should be created at the end of the tab container
     */
    void duplicateTab(int tabIndex, bool openAfterCurrentPage = false);

    /**
     * @brief Creates a new tab from a history entry in the given view
     *
     * This will create a new tab and a new view within it which will copy the
     * given history entry from @p currentView. The position of the new tab in the
     * tab container depends on @p openAfterCurrentPage.
     *
     * This is used when middle-clicking or ctrl-clicking on the back/forward history button.
     *
     * @param currentView the view whose history entry should be copied
     * @param steps the position of the history entry to copy relative to the
     * current history index in @p currentView: pass 0 to copy the current entry,
     * a positive number to copy an entry in the forward history and a negative
     * number to copy an entry in the back history
     * @param openAfterCurrentPage whether the new tab should be created after the
     * current one or at the end of the tab container
     * @return the new view
     */
    KonqView *addTabFromHistory(KonqView *currentView, int steps, bool openAfterCurrentPage);

    /**
     * @brief Breaks a tab from its window and inserts it into a new window
     *
     * This writes a configuration file with the contents of the tab, then creates
     * a new window and calls loadRootItem() on the view manager for that window
     * passing it the configuration file, then removes the tab from the original
     * window.
     *
     * @param tab the index of the tab to break off
     * @param windowSize the size of the window to create
     * @return the new window
     */
    KonqMainWindow *breakOffTab(int tab, const QSize &windowSize);

    /**
     * Guess!:-)
     * Also takes care of setting another view as active if @p view was the active view
     *
     * @brief Removes the given view from its container
     *
     * This deletes the view and, depending on its type, also the container.
     *
     * @param view the view to delete
     */
    void removeView(KonqView *view);

    /**
     * @brief Removes a tab from the tab container
     *
     * This deletes the tab and all views inside it. It also takes care to activate another view
     * if the tab contains the current view.
     *
     * @param tab the tab to delete
     * @param emitAboutToRemoveSignal whether to emit the emitAboutToRemoveTab() signal
     */
    void removeTab(KonqFrameBase *tab, bool emitAboutToRemoveSignal = true);

    /**
     * @brief Removes all tabs except the given one
     *
     * This also makes the given tab active, if it wasn't already
     * @param tabIndex the index of the tab to keep
     */
    void removeOtherTabs(int tabIndex);

    /**
     * @brief Activates the tab next to the active one
     *
     * If there's only one tab, nothing is done
     */
    void activateNextTab();

    /**
     * @brief Activates the tab before the active one
     *
     * If there's only one tab, nothing is done
     */
    void activatePrevTab();

    /**
     * @brief Activates the given tab
     *
     * @param position the index of the tab to activate. It must be a number from
     * 0 (included) to the number of existing tabs (excluded)
     */
    void activateTab(int position);

    /**
     * @brief The index of the current tab
     * @return the index of the current tab
     */
    int currentTabIndex() const;

    /**
     * @brief The number of tabs
     * @return the number of tabs
     */
    int tabsCount() const;

    /**
     * @brief Moves the current tab backwards by one
     */
    void moveTabBackward();

    /**
     * @brief Moves the current tab forwards by one
     */
    void moveTabForward();

    /**
     * @brief Reloads the active view in each tab
     */
    void reloadAllTabs();

    /**
     * @brief The tab container
     *
     * If the tab container doesn't exist, it is created.
     *
     * @return the tab container
     */
    KonqFrameTabs *tabContainer();

    /**
     * Returns true if the tabwidget exists and the tabbar is visible
     * @brief Whether the tab bar is visible
     *
     * @return `true` if the tab bar is visible and `false` if either the tab
     * container doesn't exist or its tab bar isn't visible
     */
    bool isTabBarVisible() const;
    
    /**
     * @brief Toggles forcing hiding the tabbar
     *
     * If forcing tabbar hiding is enabled, the tabbar will be hidden regardless
     * of the number of tabs it contains and of user setting. This is necessary,
     * for example, to implement complete full screen. If forcing tabbar hiding
     * is disabled, the tabbar will be shown or hidden depending on the number of
     * tabs and the user settings.
     *
     * @param force `true` to enable forcing tabbar hiding and `false` to disable it
     *
     * @see FullScreenManager
     * @see KonqTabs::forceHideTabBar()
     */
    void forceHideTabBar(bool force);

    /**
     * @brief Applies settings from the configuration file
     *
     * It reads the configuration file and applies the relevant settings to the view
     * manager.
     */
    void applyConfiguration();

    /**
     * @brief Brings the given tab to the front
     *
     * @param tabIndex the index of the tab to move to the front. It must be between 0 (included)
     * and the number of tab (excluded)
     */
    void showTab(int tabIndex);

    /**
     * @brief Brings the tab containing the given view to the front
     *
     * @param view the view whose tab should be moved to the front
     * @warning Deprecated, used the other one; this one breaks too easily with split views
     * (if passing the current view to @p view).
     */
    void showTab(KonqView *view);

    /**
     * @brief Updates the tabs icon so that they match the favicons for the views
     */
    void updatePixmaps();

    /**
     * @brief Saves the current view layout to a group in a configuration object
     *
     * @param cfg the config file
     * @param options whether to save nothing, the URLs or the complete history of each view in the profile
     */
    void saveViewConfigToGroup(KConfigGroup &cfg, KonqFrameBase::Options options);

    /**
     * @brief Loads a view layout from a configuration object
     *
     * Removes all views before loading.
     * @param cfg the configuration object
     * @param filename unused (used to be the profile name, but profile don't exist anymore)
     * @param forcedUrl if set, the URL to open, whatever the profile says
     * @param req attributes related to @p forcedUrl settings, they will be reset to the defaults
     * @param openUrl whether the URL associated with the view should be opened or not
     */
    void loadViewConfigFromGroup(const KConfigGroup &cfg, const QString &filename,
                                  const QUrl &forcedUrl = QUrl(),
                                  const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                                  bool openUrl = true);
    /**
     * @brief Whether we are currently loading a view hierarchy from a configuration object
     *
     * @return `true` if this object is creating a view hierarchy according to the contents
     * of a configuration object and false otherwise
     */
    bool isLoadingProfile() const
    {
        return m_bLoadingProfile;
    }

    /**
     * @brief Removes and deletes all the views, their containers and the tab container
     *
     * This leaves an empty window.
     */
    void clear();

    /**
     * @brief The next non-passive view after the given one
     *
     * @param view the view to find the next view from
     * @return the next non-passive view after @p view. If no passive view is found
     * after @p view, the first non-passive view _before_ @p view is returned. If
     * no non-passive view is found, `nullptr` is returned.
     */
    KonqView *chooseNextView(KonqView *view);

    /**
     * @brief Updates the UI when the number of views or of passive views changes
     *
     * Currently, this updates the statusbar of each view, toggling
     * the widgets showing the active window and the linked status depending on
     * whether or not there is more than one window.
     *
     * This method should be called whenever the number of views changes or when
     * the passive status of a view changes.
     */
    void viewCountChanged();

    /**
     * @brief The main window whose views are managed by this object
     * @return the main window whose views are managed by this object
     */
    KonqMainWindow *mainWindow() const
    {
        return m_pMainWindow;
    }

    /**
     * Reimplemented from PartManager
     *
     * @brief Override of `KParts::PartManager::removePart()`
     *
     * Besides calling the base class implementation, it also handles closing the
     * view associated with the part. It also closes the main window if the view
     * associated with the part is the last one.
     *
     * @param part the part to remove
     */
    void removePart(KParts::Part *part) override;

    /**
     * @brief Override of `KParts::PartManager::setActivePart()`
     *
     * It calls doSetActivePart().
     * @param part the part to activate
     * @param widget the widget which was responsible for the activation of the
     * part. Unused
     */
    void setActivePart(KParts::Part *part, QWidget *widget = nullptr) override;

    /**
     * @brief Does the necessary operation to make a part active
     *
     * It updates the location bar URL of the current view in the main window so that
     * it matches that shown in the location bar (this assumes that the part to activate
     * is in the current view) and gives focus to the part's widget, except when
     * it displays an error URL. It also calls KonqMainWindow::slotPartActivated().
     *
     * @param part the part to activate
     */
    void doSetActivePart(KParts::ReadOnlyPart *part);

    /**
     * @brief Applies to the main window the window size settings read from a configuration object
     *
     * @param profileGroup the configuration object containing the size settings
     */
    void applyWindowSize(const KConfigGroup &profileGroup);

#ifndef NDEBUG
    /**
     * @brief Debug function which prints the view hierarchy
     */
    void printFullHierarchy();
#endif

    /**
     * @brief Informs the tab manager that a given view started or finished loading an URL
     *
     * @param view the view
     * @param loading whether @p view has started or finished loading and URL
     */
    void setLoading(KonqView *view, bool loading);

    /**
     * @brief Creates a copy of the current window
     *
     * @return the new main window
     */
    KonqMainWindow *duplicateWindow();

    /**
     * @brief Opens the view hierarchy saved in the given configuration object
     *
     * Depending on the value of @p openTabsInsideCurrentWindow, either a new window
     * will be created or the view hierarchy saved in the configuration object will
     * be opened inside the current window, together with the views already there.
     *
     * @param configGroup the configuration object from which to read the view hierarchy
     * @param openTabsInsideCurrentWindow whether to open the view hierarchy inside the
     * current window or in a new window
     *
     * @return the window where the view hierarchy has been opened
     */
    KonqMainWindow *openSavedWindow(const KConfigGroup &configGroup,
                                    bool openTabsInsideCurrentWindow);

    /**
     * @brief Override of openSavedWindow(const KConfigGroup, bool)
     *
     * It always opens the view hierarchy in a new window. It's static because the
     * result is the same whatever window it's called on and can be called even
     * if no window exists yet.
     *
     * @param configGroup the configuration object from which to read the view hierarchy
     * @return the new window
     */
    static KonqMainWindow *openSavedWindow(const KConfigGroup &configGroup);

public Q_SLOTS:
    /**
     * @brief Restores a previously closed window
     *
     * This creates a new window with the contents of the closed window
     *
     * @param closedWindowItem the object representing the closed window
     */
    static void openClosedWindow(const KonqClosedWindowItem &closedWindowItem);

    /**
     * @brief Restores a previously closed tab
     *
     * This creates a new tab with the contents of the closed tab, then makes it
     * the current tab.
     *
     * @param closedTab the object representing the closed tab
     */
    void openClosedTab(const KonqClosedTabItem &closedTab);

    /**
     * @brief Applies any relevant configuration settings
     */
    void reparseConfiguration();

private Q_SLOTS:

    /**
     * @brief Calls KonqMainWindow::slotPartActivated()
     */
    void emitActivePartChanged();

    /**
     * @brief Slot called when the part in a passive view is deleted
     *
     * It calls KonqView::partDeleted() on the view containing the part and removes
     * the view. This is necessary because parts in passive views aren't registered
     * with the part manager.
     *
     * @warning This slot relies on `QObject::sender()` to determine the deleted part,
     * so it must only be called in response to a signal.
     */
    void slotPassiveModePartDeleted();

    /**
     * @brief Slot called when the active part changes
     *
     * It sends a `KParts::PartActivateEvent` and, if appropriate, it updates
     * the statusbar and the part's frame container's active child.
     *
     * This slot is called in response to the `KParts::PartManager::activePartChanged()`
     * signal emitted by the view manager itself.
     *
     * @param newPart the part which was activated
     */
    void slotActivePartChanged(KParts::Part *newPart);

    /**
     * @brief Apply delayed loading to a tab
     *
     * This loads each view in the tab from the delayed loading data it contains.
     *
     * @param idx the index of the tab
     * @see KonqView::delayedLoad
     */
    void delayedLoadTab(int idx);

signals:
// the signal is only emitted when the contents of the view represented by
// "tab" are going to be lost for good.

    /**
     * @brief Signal emitted when a tab is about to be removed and its contents are going to be lost
     *
     * This signal is not emitted, for example, when breaking it off from the main window, as its
     * contents aren't lost: the tab is removed from the main window only to be added to another
     * window.
     *
     * @param tab the tab which is being removed
     */
    void aboutToRemoveTab(KonqFrameBase *tab);

    /**
     * @brief Signal emitted when the tab container changes
     *
     * @param container the new tab container. It can be `nullptr`
     */
    void tabContainerChanged(KonqFrameTabs *container);

    /**
     * @brief Signal emitted when a new view has been created
     */
    void viewCreated(KonqView *newView);

private:

    /**
     * @brief Struct containing the parameters to load a view from a configuration object
     */
    struct LoadViewUrlData {
        const QUrl& defaultUrl; //!<The default URL to load if none is specified
        const QUrl &forcedUrl; //!<The URL to load instead of the one specified in the configuration object
        const QString &forcedService; //!<The plugin id of the part to use instead of the default one
        bool openUrl; //!<Whether to open URLs at all
    };

    /**
     * @brief Loads an item of a view hierarchy from a configuration object
     *
     * The item can represent:
     * - a single view
     * - a tab
     * - view container.
     *
     * @param cfg the config object
     * @param parent the container where the new item should be put
     * @param name the name of the item
     * @param viewData information about how to load the hierarchy
     * @param openAfterCurrentPage whether the item should be put after the current tab or not
     * @param pos the position of the new item. A value of -1 means at the end
     *
     * @todo Refactor all the loading code
     */
    void loadItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                  const QString &name, const LoadViewUrlData &viewData,
                  bool openAfterCurrentPage = false, int pos = -1);

    /**
     * @brief Loads a view hierarchy from a configuration object
     *
     * This uses loadItem() to load the root item of the hierarchy and ensures
     * the current tab is loaded.
     *
     * @param cfg the config object
     * @param parent the container where the root item should be
     * @param defaultUrl the default URL to load in a view if none is specified
     * @param openUrl whether to open URLs in views
     * @param forcedUrl if not empty, an URL to load instead of that specified in the configuration object
     * @param forcedService the plugin id of the part to use instead of the default one
     * @param openAfterCurrentPage whether the view hierarchy should be put after the current tab or not
     * @param pos the position of the view hierarchy. A value of -1 means at the end
     */
    void loadRootItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                      const QUrl &defaultURL, bool openUrl,
                      const QUrl &forcedUrl, const QString &forcedService = QString(),
                      bool openAfterCurrentPage = false,
                      int pos = -1);

    /**
     * @brief Creates the tab container
     *
     * Note that usually @p parent and @p parentContainer will be the same object
     * which derives both from `QWidget` and from KonqFrameContainerBase (for example.
     * KonqMainWindow).
     *
     * @param parent the parent widget of the tab container
     * @param parentContainer the frame container which contains the tab container
     *
     * @warning This replaces the old tab container, if one exists, and doesn't
     * delete it
     */
    void createTabContainer(QWidget *parent, KonqFrameContainerBase *parentContainer);

    /**
     * @brief Creates a KonqViewFactory which can be used to create a view
     *
     * If @p type is empty, the type and plugin id of the current view (if any)
     * will be used, except if the current view is the sidebar. In that case, or
     * if the current view is the sidebar, default values will be used.
     *
     * @param type the mimetype or the part capability of the view to create
     * @param serviceName the plugin id of the part the view should display
     * @param [out] service a variable to store information about the plugin in
     * @param [out] partServiceOffers a variable to store a list of the parts able
     * to display @p type
     * @param [out] appServiceOffers a variable to store a list of the applications
     * able to open @p type. It will only be used if @p type is a mimetype and not
     * a part capability
     * @param forceAutoEmbed whether or not we should ignore the user preference of
     * displaying @p type and force embedding it
     * @return the view factory which will create the view with the requested
     * characteristics
     */
    KonqViewFactory createView(const Konq::ViewType &type,
                               const QString &serviceName,
                               KPluginMetaData &service,
                               QVector<KPluginMetaData> &partServiceOffers,
                               KService::List &appServiceOffers,
                               bool forceAutoEmbed = false);

    /**
     * @brief Creates a new view and inserts it in Konqueror
     *
     * The new view is inserted in a new frame which is, in turn, inserted in the
     * appropriate container (@p parentContainer). The view is also registered with
     * the main window and makes the necessary signal-slot connections.
     *
     * @param parentContainer the container where the view should be inserted
     * @param viewFactory the view factory to use when creating the view
     * @param service the metadata of the plugin providing the part to create
     * @param partServiceOffers a list of all available parts for the given view type
     * @param appServiceOffers a list of all available applications for the given view type.
     * It's ignored if the view type is  a part capability
     * @param type the view type of the view to create
     * @param passiveMode whether the view should be passive
     * @param openAfterCurrentPage whether the new view should be inserted in the
     * parent container right after the current view or in another position
     * @param pos the position in the parent container where the new view should
     * be inserted. A negative value means to insert it at the end of the container.
     * This is ignored if @p openAfterCurrentPage is `true`
     * @return the new view
     */
    KonqView *setupView(KonqFrameContainerBase *parentContainer,
                        KonqViewFactory &viewFactory,
                        const KPluginMetaData &service,
                        const QVector<KPluginMetaData> &partServiceOffers,
                        const KService::List &appServiceOffers,
                        const Konq::ViewType &type,
                        bool passiveMode, bool openAfterCurrentPage = false, int pos = -1);

    /**
     * @brief Overload of setupView() which creates a view with a PlaceholderPart
     *
     * This is an overload of setupView(KonqFrameContainerBase*, KonqViewFactory&, const KPluginMetaData&, const QVector<KPluginMetaData> &, const KService::List&, const Konq::ViewType&, bool, bool, int) which creates a view usinga PlaceholderPart instead of the part
     * appropriate for a given view type.
     *
     * @param parentContainer the container where the view should be inserted
     * @param passiveMode whether the view should be passive
     * @param openAfterCurrentPage whether the new view should be inserted in the
     * parent container right after the current view or in another position
     * @param pos the position in the parent container where the new view should
     * be inserted. A negative value means to insert it at the end of the container.
     * This is ignored if @p openAfterCurrentPage is `true`
     * @return the new view
     */
    KonqView* setupView(KonqFrameContainerBase *parentContainer, bool passiveMode, bool openAfterCurrentPage = false, int pos = -1);

    /**
     * @brief Loads a view from a `KConfigGroup`
     *
     * @param cfg the config group to load the view from
     * @param prefix the string to append to the entry names to obtain the keys in @p cfg
     * @param parent the frame container the view should be inserted into
     * @param name the name of the view. It's only meaningful if it's `"empty"`
     * @param data information about how to load the URL associated with the item
     * @param openAfterCurrentPage whether or not the view should be opened after the current page
     * @param pos the position of the view in @p parent. If -1, the view will be last.
     *  Ignored if @p openAfterCurrentPage is `true` and @p parent is a container of type \link KonqFrameBase::Tabs Tabs\endlink
     * @todo Refactor
     */
    void loadViewItem(const KConfigGroup &cfg, const QString &prefix, KonqFrameContainerBase *parent,
                               const QString &name, const LoadViewUrlData &data, bool openAfterCurrentPage, int pos);

    /**
     * @brief Loads a tab from a configuration object
     *
     * @param cfg the configuration object
     * @param prefix the string to append to the entry names to obtain the keys in @p cfg
     * @param parent the frame container the tab should be inserted into
     * @param viewData information about how to load the views in the tab. Passed to loadViewItem()
     */
    void loadTabsItem(const KConfigGroup &cfg, const QString &prefix, KonqFrameContainerBase *parent, const LoadViewUrlData &viewData);

    /**
     * @brief Loads a view container (split view) from a configuration object
     *
     * @param cfg the configuration object
     * @param prefix the string to append to the entry names to obtain the keys in @p cfg
     * @param parent the frame container the container should be inserted into
     * @param name the name of the container (only used for debugging)
     * @param openAfterCurrentPage whether or not the container should be opened after the current page
     * @param pos the position of the container in @p parent. If -1, the container will be last.
     *  Ignored if @p openAfterCurrentPage is `true` and @p parent is a container of type \link KonqFrameBase::Tabs Tabs\endlink
     * @param informationOnHow to load the views in the container. Passed to loadViewItem()
     */
    void loadContainerItem(const KConfigGroup &cfg, const QString &prefix, KonqFrameContainerBase *parent, const QString &name,
                           bool openAfterCurrentPage, int pos, const LoadViewUrlData &viewData);

    /**
     * @brief Restore the status of a view not contained in the tab widget from a configuration group
     *
     * This restores history and opens the correct URL in the view
     * @param view the view whose history should be restored
     * @param cfg the configuration group to read history from
     * @param prefix the prefix to append to keys to read entries in @p cfg
     * @param defaultURL a default URL to use
     * @param type the type of the view to display
     */
    void restoreViewOutsideTabContainer(KonqView *view, const KConfigGroup &cfg, const QString &prefix, const QUrl &defaultURL, const Konq::ViewType &type);

#ifndef NDEBUG
    //just for debugging
    void printSizeInfo(KonqFrameBase *frame,
                       KonqFrameContainerBase *parent,
                       const char *msg);
#endif

    KonqMainWindow *m_pMainWindow; //!< The main window

    /**
     * @brief The tab container
     *
     * This can be `nullptr`: when that happens, calling tabContainer() creates a new
     * tab container.
     */
    KonqFrameTabs *m_tabContainer;

    bool m_bLoadingProfile; //!< Whether we're in the process of reading a view hierarchy from a configuration file

    QMap<QString /*display name*/, QString /*path to file*/> m_mapProfileNames; //!< Unused
};

#endif
