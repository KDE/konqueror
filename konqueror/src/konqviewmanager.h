/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2007 Eduardo Robles Elvira <edulix@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __konq_viewmanager_h__
#define __konq_viewmanager_h__

#include "konqprivate_export.h"
#include "konqfactory.h"
#include "konqframe.h"

#include <QtCore/QMap>
#include <QtCore/QPointer>

#include <KService>

#include <kparts/partmanager.h>
#include "konqopenurlrequest.h"

class KMainWindow;
class KonqFrameTabs;
class QString;
class QTimer;
class KConfig;
class KConfigGroup;
class KonqMainWindow;
class KonqFrameBase;
class KonqFrameContainer;
class KonqFrameContainerBase;
class KonqView;
class KActionMenu;
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
    explicit KonqViewManager( KonqMainWindow *mainWindow );
    ~KonqViewManager();

    KonqView* createFirstView( const QString &mimeType, const QString &serviceName );

  /**
   * Splits the view, depending on orientation, either horizontally or
   * vertically. The first view in the splitter will contain the initial
   * view, the other will be a new one, constructed from the same service
   * (part) as the first view.
   * Returns the newly created view or 0 if the view couldn't be created.
   *
   * @param newOneFirst if true, move the new view as the first one (left or top)
   */
  KonqView* splitView( KonqView* view,
                       Qt::Orientation orientation,
                       bool newOneFirst = false, bool forceAutoEmbed = false );

  /**
   * Does basically the same as splitView() but inserts the new view inside the
   * specified container (usually used with the main container, to insert
   * the new view at the top of the view tree).
   * Returns the newly created view or 0 if the view couldn't be created.
   *
   * @param newOneFirst if true, move the new view as the first one (left or top)
   */
  KonqView* splitMainContainer( KonqView* currentView,
                            Qt::Orientation orientation,
                            const QString & serviceType = QString(),
                            const QString & serviceName = QString(),
                            bool newOneFirst = false );

  /**
   * Adds a tab to m_tabContainer
   */
  KonqView* addTab(const QString &serviceType,
                   const QString &serviceName = QString(),
                   bool passiveMode = false, bool openAfterCurrentPage = false, int pos = -1 );

  /**
   * Duplicates the specified tab, or else the current one if none is specified
   */
  void duplicateTab( KonqFrameBase* tab, bool openAfterCurrentPage = false );

  /**
   * creates a new tab from a history entry
   * used for MMB on back/forward
   */
  KonqView* addTabFromHistory( KonqView* currentView, int steps, bool openAfterCurrentPage );

  /**
   * Break the current tab off into a new window,
   * if none is specified, the current one is used
   */
  void breakOffTab( KonqFrameBase* tab, const QSize& windowSize );

  /**
   * Guess!:-)
   * Also takes care of setting another view as active if @p view was the active view
   */
  void removeView( KonqView *view );

  /**
   * Removes specified tab
   * Also takes care of setting another view as active if the active view was in this tab
   */
  void removeTab( KonqFrameBase* tab );

  /**
   * Removes all, but the specified tab.
   * Also takes care of setting the specified tab as active if the active view was not in this tab
   */
  void removeOtherTabs( KonqFrameBase* tab );

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

    void moveTabBackward();
    void moveTabForward();

    void reloadAllTabs();


    KonqFrameTabs *tabContainer();

    // Apply configuration that applies to us, like alwaysTabbedMode.
    void applyConfiguration();

  /**
   * Brings the tab specified by @p view to the front of the stack
   *
   */
  void showTab( KonqView *view );

  /**
   * Updates favicon pixmaps used in tabs
   *
   */
  void updatePixmaps();

  /**
   * Saves the current view layout to a config file, including menu/toolbar settings.
   * Remove config file before saving, especially if saveURLs is false.
   * @param fileName the name of the config file
   * @param profileName the name of the profile
   * @param saveURLs whether to save the URLs in the profile
   */
  void saveViewProfileToFile(const QString & fileName, const QString & profileName,
                             KonqFrameBase::Options options);

  /**
   * Saves the current view layout to a group in a config file.
   * This is shared between saveViewProfileToFile and saveProperties (session management)
   * Remove config file before saving, especially if saveURLs is false.
   * @param cfg the config file
   * @param options whether to save nothing, the URLs or the complete history of each view in the profile
   */
  void saveViewProfileToGroup(KConfigGroup & cfg, KonqFrameBase::Options options);


  /**
   * Loads a view layout from a config file. Removes all views before loading.
   * @param path the full path to the config file
   * @param filename if set, remember the file name of the profile (for save settings)
   * It has to be under the profiles dir. Otherwise, set to QString()
   * @param forcedUrl if set, the URL to open, whatever the profile says
   * @param req attributes related to @p forcedUrl
   * @param resetWindow if the profile doesn't have attributes like size or toolbar
   * settings, they will be reset to the defaults
   */
  void loadViewProfileFromFile( const QString & path, const QString & filename,
                                const KUrl & forcedUrl = KUrl(),
                                const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                                bool resetWindow = false, bool openUrl = true );
    // Overload for KonqMisc::createBrowserWindowFromProfile
  void loadViewProfileFromConfig( const KSharedConfigPtr& config,
                                  const QString& path,
                                  const QString & filename,
                                  const KUrl & forcedUrl = KUrl(),
                                  const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                                  bool resetWindow = false, bool openUrl = true );
  /**
   * Loads a view layout from a config file. Removes all views before loading.
   * @param cfg the config file
   * @param filename if set, remember the file name of the profile (for save settings)
   * It has to be under the profiles dir. Otherwise, set to QString()
   * @param forcedUrl if set, the URL to open, whatever the profile says
   * @param req attributes related to @p forcedUrl
   * @param resetWindow if the profile doesn't have attributes like size or toolbar
   * settings, they will be reset to the defaults
   */
  void loadViewProfileFromGroup( const KConfigGroup& cfg, const QString & filename,
                                 const KUrl & forcedUrl = KUrl(),
                                 const KonqOpenURLRequest &req = KonqOpenURLRequest(),
                                 bool openUrl = true );
  /**
   * Return the filename of the last profile that was loaded
   * by the view manager. For "save settings".
   */
  QString currentProfile() const { return m_currentProfile; }
  /**
   * Return the name (i18n'ed) of the last profile that was loaded
   * by the view manager. For "save settings".
   */
  QString currentProfileText() const { return m_currentProfileText; }

  /**
   * Whether we are currently loading a profile
   */
  bool isLoadingProfile() const { return m_bLoadingProfile; }

  void clear();

  KonqView *chooseNextView( KonqView *view );

  /**
   * Called whenever
   * - the total number of views changed
   * - the number of views in passive mode changed
   * The implementation takes care of showing or hiding the statusbar indicators
   */
  void viewCountChanged();

  void setProfiles( KActionMenu *profiles );

  void profileListDirty( bool broadcast = true );

//  KonqFrameBase *docContainer() const { return m_pDocContainer; }
//  void setDocContainer( KonqFrameBase* docContainer ) { m_pDocContainer = docContainer; }

  KonqMainWindow *mainWindow() const { return m_pMainWindow; }

  /**
   * Reimplemented from PartManager
   */
  virtual void removePart( KParts::Part * part );

  /**
   * Reimplemented from PartManager
   */
  virtual void setActivePart( KParts::Part *part, QWidget *widget = 0 );

  void setActivePart( KParts::Part *part, bool immediate );

  void showProfileDlg( const QString & preselectProfile );

    /**
     * Read default size from profile (e.g. Width=80%)
     */
    static QSize readDefaultSize(const KConfigGroup& cfg, QWidget* window);

#ifndef NDEBUG
  void printFullHierarchy( KonqFrameContainerBase * container );
#endif

  void setLoading( KonqView *view, bool loading );

  void showHTML(bool b);

public Q_SLOTS:
    /**
     * Opens a previously closed tab in a new tab
     */
    void openClosedTab(const KonqClosedTabItem& closedTab);
    /**
     * Opens a previously closed window in a new window
     */
    void openClosedWindow(const KonqClosedWindowItem& closedTab);

private Q_SLOTS:
  void emitActivePartChanged();

  void slotProfileDlg();

  void slotProfileActivated(QAction* action);

  void slotProfileListAboutToShow();

  void slotPassiveModePartDeleted();

  void slotActivePartChanged ( KParts::Part *newPart );

private:

  /**
   * Load the config entries for a view.
   * @param cfg the config file
   * ...
   * @param defaultURL the URL to use if the profile doesn't contain urls
   * @param openUrl whether to open urls at all (from the profile or using @p defaultURL).
   *  (this is set to false when we have a forcedUrl to open)
   */
  void loadItem( const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                 const QString &name, const KUrl & defaultURL, bool openUrl,
                 const KUrl& forcedUrl,
                 bool openAfterCurrentPage = false, int pos = -1 );

    void loadRootItem( const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                       const KUrl & defaultURL, bool openUrl,
                       const KUrl& forcedUrl,
                       bool openAfterCurrentPage = false,
                       int pos = -1 );

    void createTabContainer(QWidget* parent, KonqFrameContainerBase* parentContainer);

    // Disabled - we do it ourselves
    virtual void setActiveComponent(const KComponentData &) {}

    void setCurrentProfile(const QString& profileFileName);

signals:
  void aboutToRemoveTab( KonqFrameBase* tab );

private:
  /**
   * Creates a new View based on the given ServiceType. If serviceType is empty
   * it clones the current view.
   * Returns the newly created view.
   */
    KonqViewFactory createView( const QString &serviceType, /* can be servicetype or mimetype */
                              const QString &serviceName,
                              KService::Ptr &service,
                              KService::List &partServiceOffers,
                              KService::List &appServiceOffers,
			      bool forceAutoEmbed = false );

  /**
   * Mainly creates the backend structure(KonqView) for a view and
   * connects it
   */
  KonqView *setupView( KonqFrameContainerBase *parentContainer,
                       KonqViewFactory &viewFactory,
                       const KService::Ptr &service,
                       const KService::List &partServiceOffers,
                       const KService::List &appServiceOffers,
                       const QString &serviceType,
                       bool passiveMode, bool openAfterCurrentPage = false, int pos = -1);

#ifndef NDEBUG
  //just for debugging
  void printSizeInfo( KonqFrameBase* frame,
                      KonqFrameContainerBase* parent,
                      const char* msg );
#endif

  KonqMainWindow *m_pMainWindow;

  KonqFrameTabs *m_tabContainer;

  QPointer<KActionMenu> m_pamProfiles;
  bool m_bProfileListDirty;
  bool m_bLoadingProfile;
  QString m_currentProfile;
  QString m_currentProfileText;

    QMap<QString /*display name*/, QString /*path to file*/> m_mapProfileNames;

  QTimer *m_activePartChangedTimer;
};

#endif
