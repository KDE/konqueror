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
                                 const QString &serviceType = QString(),
                                 const QString &serviceName = QString(),
                                 bool newOneFirst = false);

    /**
     * Adds a tab to m_tabContainer
     */
    KonqView *addTab(const QString &serviceType,
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

private:

    /**
     * Load the config entries for a view.
     * @param cfg the config file
     * ...
     * @param defaultURL the URL to use if the profile doesn't contain urls
     * @param openUrl whether to open urls at all (from the profile or using @p defaultURL).
     *  (this is set to false when we have a forcedUrl to open)
     * @param forcedUrl open this URL instead of the one from the profile
     * @param forcedService use this service (part) instead of the one from the profile
     */
    void loadItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                  const QString &name, const QUrl &defaultURL, bool openUrl,
                  const QUrl &forcedUrl, const QString &forcedService,
                  bool openAfterCurrentPage = false, int pos = -1);

    void loadRootItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                      const QUrl &defaultURL, bool openUrl,
                      const QUrl &forcedUrl, const QString &forcedService = QString(),
                      bool openAfterCurrentPage = false,
                      int pos = -1);

    void createTabContainer(QWidget *parent, KonqFrameContainerBase *parentContainer);

signals:
// the signal is only emitted when the contents of the view represented by
// "tab" are going to be lost for good.
    void aboutToRemoveTab(KonqFrameBase *tab);

private:
    /**
     * Creates a new View based on the given ServiceType. If serviceType is empty
     * it clones the current view.
     * Returns the newly created view.
     */
    KonqViewFactory createView(const QString &serviceType,  /* can be servicetype or mimetype */
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
                        const QString &serviceType,
                        bool passiveMode, bool openAfterCurrentPage = false, int pos = -1);

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
