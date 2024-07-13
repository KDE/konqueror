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

class KONQ_TESTS_EXPORT KonqViewManager : public KParts::PartManager
{
    Q_OBJECT
public:
    explicit KonqViewManager(KonqMainWindow *mainWindow);
    ~KonqViewManager() override;

    KonqView *createFirstView(const QString &mimeType, const QString &serviceName);

    /**
     * Splits the view, depending on orientation, either horizontally or
     * vertically. The first view in the splitter will contain the initial
     * view, the other will be a new one, constructed from the same service
     * (part) as the first view.
     * Returns the newly created view or 0 if the view couldn't be created.
     *
     * @param newOneFirst if true, move the new view as the first one (left or top)
     */
    KonqView *splitView(KonqView *view,
                        Qt::Orientation orientation,
                        bool newOneFirst = false, bool forceAutoEmbed = false);

    /**
     * Does basically the same as splitView() but inserts the new view inside the
     * specified container (usually used with the main container, to insert
     * the new view at the top of the view tree).
     * Returns the newly created view or 0 if the view couldn't be created.
     *
     * @param newOneFirst if true, move the new view as the first one (left or top)
     */
    KonqView *splitMainContainer(KonqView *currentView,
                                 Qt::Orientation orientation,
                                 const Konq::ViewType &type = QString(),
                                 const QString &serviceName = QString(),
                                 bool newOneFirst = false);

    /**
     * Adds a tab to m_tabContainer
     */
    KonqView *addTab(const Konq::ViewType &type,
                     const QString &serviceName = QString(),
                     bool passiveMode = false, bool openAfterCurrentPage = false, int pos = -1);

    /**
     * Duplicates the specified tab
     */
    void duplicateTab(int tabIndex, bool openAfterCurrentPage = false);

    /**
     * creates a new tab from a history entry
     * used for MMB on back/forward
     */
    KonqView *addTabFromHistory(KonqView *currentView, int steps, bool openAfterCurrentPage);

    /**
     * Break the specified tab off into a new window
     */
    KonqMainWindow *breakOffTab(int tab, const QSize &windowSize);

    /**
     * Guess!:-)
     * Also takes care of setting another view as active if @p view was the active view
     */
    void removeView(KonqView *view);

    /**
     * Removes specified tab
     * Also takes care of setting another view as active if the active view was in this tab
     */
    void removeTab(KonqFrameBase *tab, bool emitAboutToRemoveSignal = true);

    /**
     * Removes all, but the specified tab.
     * Also takes care of setting the specified tab as active if the active view was not in this tab
     * @param tabIndex the index of the tab
     */
    void removeOtherTabs(int tabIndex);

    /**
     * Locates and activates the next tab
     *
     */
    void activateNextTab();

    /**
     * Locates and activates the previous tab
     *
     */
    void activatePrevTab();

    /**
     * Activate given tab
     *
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

    void moveTabBackward();
    void moveTabForward();

    void reloadAllTabs();

    /**
     * Creates the tabwidget on demand and returns it.
     */
    KonqFrameTabs *tabContainer();

    /**
     * Returns true if the tabwidget exists and the tabbar is visible
     */
    bool isTabBarVisible() const;
    
    /**
     * Forces hiding the tab bar, regardless of settings or number of tabs
     * 
     * This is used to provide movie full screen
     */
    void forceHideTabBar(bool force);

    // Apply configuration that applies to us, like alwaysTabbedMode.
    void applyConfiguration();

    /**
     * Brings the tab specified by @p tabIndex to the front of the stack
     */
    void showTab(int tabIndex);

    /**
     * Brings the tab specified by @p view to the front of the stack
     * Deprecated, used the other one; this one breaks too easily with split views
     * (if passing the current view to @p view)
     */
    void showTab(KonqView *view);

    /**
     * Updates favicon pixmaps used in tabs
     *
     */
    void updatePixmaps();

    /**
     * Saves the current view layout to a group in a config file.
     * This is shared between saveViewProfileToFile and saveProperties (session management)
     * Remove config file before saving, especially if SaveUrls is false.
     * @param cfg the config file
     * @param options whether to save nothing, the URLs or the complete history of each view in the profile
     */
    void saveViewConfigToGroup(KConfigGroup &cfg, KonqFrameBase::Options options);

    /**
     * Loads a view layout from a config file. Removes all views before loading.
     * @param cfg the config file
     * @param filename if set, remember the file name of the profile (for save settings)
     * It has to be under the profiles dir. Otherwise, set to QString()
     * @param forcedUrl if set, the URL to open, whatever the profile says
     * @param req attributes related to @p forcedUrl
     * settings, they will be reset to the defaults
     */
    void loadViewConfigFromGroup(const KConfigGroup &cfg, const QString &filename,
                                  const QUrl &forcedUrl = QUrl(),
                                  const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                                  bool openUrl = true);
    /**
     * Whether we are currently loading a profile
     */
    bool isLoadingProfile() const
    {
        return m_bLoadingProfile;
    }

    void clear();

    KonqView *chooseNextView(KonqView *view);

    /**
     * Called whenever
     * - the total number of views changed
     * - the number of views in passive mode changed
     * The implementation takes care of showing or hiding the statusbar indicators
     */
    void viewCountChanged();

    KonqMainWindow *mainWindow() const
    {
        return m_pMainWindow;
    }

    /**
     * Reimplemented from PartManager
     */
    void removePart(KParts::Part *part) override;

    /**
     * Reimplemented from PartManager
     */
    void setActivePart(KParts::Part *part, QWidget *widget = nullptr) override;

    void doSetActivePart(KParts::ReadOnlyPart *part);

    void applyWindowSize(const KConfigGroup &profileGroup);

#ifndef NDEBUG
    void printFullHierarchy();
#endif

    void setLoading(KonqView *view, bool loading);

    /**
     * Creates a copy of the current window
     */
    KonqMainWindow *duplicateWindow();

    /**
     * Open a saved window.
     *
     * @param openTabsInsideCurrentWindow if true, it will try to open the
     * tabs inside current window.
     */
    KonqMainWindow *openSavedWindow(const KConfigGroup &configGroup,
                                    bool openTabsInsideCurrentWindow);

    /**
     * Open a saved window in a new KonqMainWindow instance.
     * It doesn't have the openTabsInsideCurrentWindow because this is the
     * static version.
     */
    static KonqMainWindow *openSavedWindow(const KConfigGroup &configGroup);

public Q_SLOTS:
    /**
     * Opens a previously closed window in a new window
     */
    static void openClosedWindow(const KonqClosedWindowItem &closedTab);

    /**
     * Opens a previously closed tab in a new tab
     */
    void openClosedTab(const KonqClosedTabItem &closedTab);

    /**
     * @brief Applies any relevant configuration settings
     */
    void reparseConfiguration();

private Q_SLOTS:
    void emitActivePartChanged();

    void slotPassiveModePartDeleted();

    void slotActivePartChanged(KParts::Part *newPart);

    /**
     * @brief Apply delayed loading to a tab
     * @param idx the index of the tab
     * @see KonqView::delayedLoad
     */
    void delayedLoadTab(int idx);

signals:
// the signal is only emitted when the contents of the view represented by
// "tab" are going to be lost for good.
    void aboutToRemoveTab(KonqFrameBase *tab);

private:

    /**
     * @brief Struct containing the parameters to load a view from a configuration object
     */
    struct LoadViewUrlData {
        const QUrl& defaultUrl; //!<The default URL to load if none is specified
        const QUrl &forcedUrl; //!<The URL to load instead of the one specified in the configuration object
        const QString &forcedService; //!<The service (part) to use instead of the default one
        bool openUrl; //!<Whether to open URLs at all
    };

    /**
     * Load the config entries for a view.
     * @param cfg the config object
     * @param parent the container where the new item should be put
     * @param name the name of the item
     * @param viewData how to load a view
     * @param openAfterCurrentPage whether the item should be put after the current tab or not
     * @param pos the position of the new item. -1 means at the end
     * @todo Refactor all the loading code
     */
    void loadItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                  const QString &name, const LoadViewUrlData &viewData,
                  bool openAfterCurrentPage = false, int pos = -1);

    void loadRootItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                      const QUrl &defaultURL, bool openUrl,
                      const QUrl &forcedUrl, const QString &forcedService = QString(),
                      bool openAfterCurrentPage = false,
                      int pos = -1);

    void createTabContainer(QWidget *parent, KonqFrameContainerBase *parentContainer);

    /**
     * Creates a new View based on the given ServiceType. If serviceType is empty
     * it clones the current view.
     * Returns the newly created view.
     */
    KonqViewFactory createView(const Konq::ViewType &type,
                               const QString &serviceName,
                               KPluginMetaData &service,
                               QVector<KPluginMetaData> &partServiceOffers,
                               KService::List &appServiceOffers,
                               bool forceAutoEmbed = false);

    /**
     * Mainly creates the backend structure(KonqView) for a view and
     * connects it
     */
    KonqView *setupView(KonqFrameContainerBase *parentContainer,
                        KonqViewFactory &viewFactory,
                        const KPluginMetaData &service,
                        const QVector<KPluginMetaData> &partServiceOffers,
                        const KService::List &appServiceOffers,
                        const Konq::ViewType &type,
                        bool passiveMode, bool openAfterCurrentPage = false, int pos = -1);

    KonqView* setupView(KonqFrameContainerBase *parentContainer, bool passiveMode, bool openAfterCurrentPage = false, int pos = -1);

    /**
     * @brief Loads a view item from a `KConfigGroup`
     * @param cfg the config group to load the view from
     * @param prefix the string to append to the entry names to obtain the keys in @p cfg
     * @param parent the frame container the view should belong to
     * @param name the name of the view
     * @param defaultURL the default URL to use if none is provided by @p cfg
     * @param openUrl whether to open an URL in the view
     * @param forcedService the plugin id of the plugin to use for the view
     * @param openAfterCurrentPage whether or not the view should be opened after the current page
     * @param pos the position of the view in @p parent. If -1, the view will be last.
     *  Ignored if @p openAfterCurrentPage is `true` and @p parent is a container of type \link KonqFrameBase::Tabs Tabs\endlink
     * @todo Refactor
     */
    void loadViewItem(const KConfigGroup &cfg, const QString &prefix, KonqFrameContainerBase *parent,
                               const QString &name, const LoadViewUrlData &data, bool openAfterCurrentPage, int pos);

    /**
     * @brief Load an item representing a tab from a configuration object
     * @param cfg the configuration object
     * @param prefix the string to append to the entry names to obtain the keys in @p cfg
     * @param parent the frame container the tab should belong to
     * @param viewData how to load the views in the tab
     */
    void loadTabsItem(const KConfigGroup &cfg, const QString &prefix, KonqFrameContainerBase *parent, const LoadViewUrlData &viewData);

    /**
     * @brief Load an item representing a container (split view) from a configuration object
     * @param cfg the configuration object
     * @param prefix the string to append to the entry names to obtain the keys in @p cfg
     * @param parent the frame container the container should belong to
     * @param name the name of the container
     * @param openAfterCurrentPage whether or not the container should be opened after the current page
     * @param pos the position of the container in @p parent. If -1, the container will be last.
     *  Ignored if @p openAfterCurrentPage is `true` and @p parent is a container of type \link KonqFrameBase::Tabs Tabs\endlink
     * @param viewData how to load the views in the container
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

    KonqMainWindow *m_pMainWindow;

    KonqFrameTabs *m_tabContainer;

    bool m_bLoadingProfile;

    QMap<QString /*display name*/, QString /*path to file*/> m_mapProfileNames;
};

#endif
