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

#include "konqviewmanager.h"

#include "konqcloseditem.h"
#include "konqundomanager.h"
#include "konqmisc.h"
#include "konqview.h"
#include "konqframestatusbar.h"
#include "konqtabs.h"
#include "konqprofiledlg.h"
#include "konqsettingsxt.h"
#include "konqframevisitor.h"
#include <konq_events.h>

#include <QtCore/QFileInfo>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>

#include <kaccelgen.h>
#include <kactionmenu.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <ktemporaryfile.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktoolbarpopupaction.h>
#include <kmenu.h>

//#define DEBUG_VIEWMGR

KonqViewManager::KonqViewManager( KonqMainWindow *mainWindow )
 : KParts::PartManager( mainWindow )
{
  m_pMainWindow = mainWindow;

  m_pamProfiles = 0L;
  m_bProfileListDirty = true;
  m_bLoadingProfile = false;
  m_tabContainer = 0;

  connect( this, SIGNAL(activePartChanged(KParts::Part*)),
           this, SLOT(slotActivePartChanged(KParts::Part*)) );
}

KonqView* KonqViewManager::createFirstView( const QString &mimeType, const QString &serviceName )
{
    //kDebug() << serviceName;
  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;
  KonqViewFactory newViewFactory = createView( mimeType, serviceName, service, partServiceOffers, appServiceOffers, true /*forceAutoEmbed*/ );
  if ( newViewFactory.isNull() )
  {
    kDebug() << "No suitable factory found.";
    return 0;
  }

  KonqView* childView = setupView( tabContainer(), newViewFactory, service, partServiceOffers, appServiceOffers, mimeType, false );

  setActivePart( childView->part() );

  m_tabContainer->asQWidget()->show();
  return childView;
}

KonqViewManager::~KonqViewManager()
{
  clear();
}

KonqView* KonqViewManager::splitView( KonqView* currentView,
                                      Qt::Orientation orientation,
                                      bool newOneFirst, bool forceAutoEmbed )
{
#ifdef DEBUG_VIEWMGR
  kDebug();
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  KonqFrame* splitFrame = currentView->frame();
  const QString serviceType = currentView->serviceType();

  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;

  KonqViewFactory newViewFactory = createView( serviceType, currentView->service()->desktopEntryName(), service, partServiceOffers, appServiceOffers, forceAutoEmbed );

  if( newViewFactory.isNull() )
    return 0; //do not split at all if we can't create the new view

  Q_ASSERT( splitFrame );

  KonqFrameContainerBase* parentContainer = splitFrame->parentContainer();

  // We need the sizes of the views in the parentContainer to restore these after the new container is inserted.
  // To access the sizes via QSplitter::sizes(), a pointer to a KonqFrameContainerBase is not sufficient.
  // We need a pointer to a KonqFrameContainer which is derived from QSplitter.
  KonqFrameContainer* parentKonqFrameContainer = dynamic_cast<KonqFrameContainer*> ( parentContainer );
  QList<int> parentSplitterSizes;
  if ( parentKonqFrameContainer ) {
    parentSplitterSizes = parentKonqFrameContainer->sizes();
  }

  KonqFrameContainer* newContainer = parentContainer->splitChildFrame(splitFrame, orientation);

  //kDebug() << "Create new child";
  KonqView *newView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, false );

#ifndef DEBUG
  //printSizeInfo( splitFrame, parentContainer, "after child insert" );
#endif

  newContainer->insertWidget(newOneFirst ? 0 : 1, newView->frame());
  if ( newOneFirst ) {
    newContainer->swapChildren();
  }

  Q_ASSERT(newContainer->count() == 2);
  QList<int> newSplitterSizes;
  newSplitterSizes << 50 << 50;
  newContainer->setSizes( newSplitterSizes );
  splitFrame->show();
  newContainer->show();

  if ( parentKonqFrameContainer ) {
    parentKonqFrameContainer->setSizes( parentSplitterSizes );
  }

  Q_ASSERT( newView->frame() );
  Q_ASSERT( newView->part() );
  newContainer->setActiveChild( newView->frame() );
  setActivePart( newView->part() );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
  kDebug() << "done";
#endif

  return newView;
}

KonqView* KonqViewManager::splitMainContainer( KonqView* currentView,
                                               Qt::Orientation orientation,
                                               const QString &serviceType, // This can be Browser/View, not necessarily a mimetype
                                               const QString &serviceName,
                                               bool newOneFirst )
{
    //kDebug();

    KService::Ptr service;
    KService::List partServiceOffers, appServiceOffers;

    KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers );

    if( newViewFactory.isNull() )
      return 0; //do not split at all if we can't create the new view

    // Get main frame. Note: this is NOT necessarily m_tabContainer!
    // When having tabs plus a konsole, the main frame is a splitter (KonqFrameContainer).
    KonqFrameBase* mainFrame = m_pMainWindow->childFrame();

    KonqFrameContainer* newContainer = m_pMainWindow->splitChildFrame(mainFrame, orientation);

    KonqView* childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, true );

    newContainer->insertWidget(newOneFirst ? 0 : 1, childView->frame());
    if( newOneFirst ) {
        newContainer->swapChildren();
    }

    newContainer->show();
    newContainer->setActiveChild( mainFrame );

    childView->openUrl( currentView->url(), currentView->locationBarURL() );

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
    kDebug() << "done";
#endif

    return childView;
}

KonqView* KonqViewManager::addTab(const QString &serviceType, const QString &serviceName, bool passiveMode, bool openAfterCurrentPage, int pos  )
{
#ifdef DEBUG_VIEWMGR
  kDebug() << "------------- KonqViewManager::addTab starting -------------";
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;

  Q_ASSERT( !serviceType.isEmpty() );

  QString actualServiceName = serviceName;
  if (actualServiceName.isEmpty()) {
      // Use same part as the current view (e.g. khtml/webkit).
      // This is down here in this central method because it should work for
      // MMB-opens-tab, window.open (createNewWindow), and more.
      KonqView* currentView = m_pMainWindow->currentView();
      // Don't use supportsMimeType("text/html"), it's true for katepart too.
      // (Testcase: view text file, ctrl+shift+n, was showing about page in katepart)
      if (currentView) {
          KMimeType::Ptr mime = currentView->mimeType();
          if (mime && mime->is(serviceType)) {
              actualServiceName = currentView->service()->desktopEntryName();
          }
      }
  }

  KonqViewFactory newViewFactory = createView( serviceType, actualServiceName, service, partServiceOffers, appServiceOffers, true /*forceAutoEmbed*/ );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  KonqView* childView = setupView( tabContainer(), newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode, openAfterCurrentPage, pos );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
  kDebug() << "------------- KonqViewManager::addTab done -------------";
#endif

  return childView;
}

KonqView* KonqViewManager::addTabFromHistory( KonqView* currentView, int steps, bool openAfterCurrentPage )
{
  int oldPos = currentView->historyIndex();
  int newPos = oldPos + steps;

  const HistoryEntry * he = currentView->historyAt(newPos);
  if(!he)
      return 0L;

  KonqView* newView = 0L;
  newView  = addTab( he->strServiceType, he->strServiceName, false, openAfterCurrentPage );

  if(!newView)
      return 0;

  newView->copyHistory(currentView);
  newView->setHistoryIndex(newPos);
  newView->restoreHistory();

  return newView;
}


void KonqViewManager::duplicateTab(int tabIndex, bool openAfterCurrentPage)
{
#ifdef DEBUG_VIEWMGR
  kDebug() << tabIndex;
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  KTemporaryFile tempFile;
  tempFile.open();
  KConfig config( tempFile.fileName() );
  KConfigGroup profileGroup( &config, "Profile" );

  KonqFrameBase* tab = tabContainer()->tabAt(tabIndex);
  QString prefix = KonqFrameBase::frameTypeToString(tab->frameType()) + QString::number(0); // always T0
  profileGroup.writeEntry( "RootItem", prefix );
  prefix.append( QLatin1Char( '_' ) );
  KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
  tab->saveConfig( profileGroup, prefix, flags, 0L, 0, 1);

  loadRootItem( profileGroup, tabContainer(), KUrl(), true, KUrl(), QString(), openAfterCurrentPage );

  if (openAfterCurrentPage)
    m_tabContainer->setCurrentIndex( m_tabContainer->currentIndex () + 1 );
  else
    m_tabContainer->setCurrentIndex( m_tabContainer->count() - 1 );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif
}

KonqMainWindow* KonqViewManager::breakOffTab(int tab, const QSize& windowSize)
{
#ifdef DEBUG_VIEWMGR
    kDebug() << "tab=" << tab;
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  KTemporaryFile tempFile;
  tempFile.open();
  KSharedConfigPtr config = KSharedConfig::openConfig( tempFile.fileName() );
  KConfigGroup profileGroup( config, "Profile" );

  KonqFrameBase* tabFrame = tabContainer()->tabAt(tab);
  QString prefix = KonqFrameBase::frameTypeToString(tabFrame->frameType()) + QString::number(0); // always T0
  profileGroup.writeEntry( "RootItem", prefix );
  prefix.append( QLatin1Char( '_' ) );
  KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
  tabFrame->saveConfig( profileGroup, prefix, flags, 0L, 0, 1);

  KonqMainWindow *mainWindow = new KonqMainWindow(KUrl(), m_pMainWindow->xmlFile());

  KonqFrameTabs* newTabContainer = mainWindow->viewManager()->tabContainer();
  mainWindow->viewManager()->loadRootItem( profileGroup, newTabContainer, KUrl(), true, KUrl() );
  mainWindow->viewManager()->setCurrentProfile( currentProfile() );

  removeTab( tabFrame, false );

  mainWindow->enableAllActions( true );
  mainWindow->resize( windowSize );
  mainWindow->activateChild();
  mainWindow->show();

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  return mainWindow;
}

void KonqViewManager::openClosedWindow(const KonqClosedWindowItem& closedWindowItem)
{
    openSavedWindow(closedWindowItem.configGroup())->show();
}

KonqMainWindow *KonqViewManager::openSavedWindow(const KConfigGroup& configGroup)
{
    const QString xmluiFile =
        configGroup.readEntry("XMLUIFile","konqueror.rc");

    // TODO factorize to avoid code duplication with loadViewProfileFromGroup
    KonqMainWindow *mainWindow = new KonqMainWindow(KUrl(), xmluiFile);

    if (configGroup.readEntry( "FullScreen", false ))
    {
        // Full screen on
        mainWindow->showFullScreen();
    } else {
        // Full screen off
        if( mainWindow->isFullScreen())
            mainWindow->showNormal();
        // Window size comes from the applyMainWindowSettings call below
    }

    mainWindow->viewManager()->loadRootItem( configGroup, mainWindow, KUrl(), true, KUrl() );
    mainWindow->applyMainWindowSettings( configGroup, true );
    mainWindow->activateChild();

#ifdef DEBUG_VIEWMGR
    mainWindow->viewManager()->printFullHierarchy();
#endif
    return mainWindow;
}

KonqMainWindow *KonqViewManager::openSavedWindow(const KConfigGroup& configGroup,
    bool openTabsInsideCurrentWindow)
{
    if(!openTabsInsideCurrentWindow)
    {
        return KonqViewManager::openSavedWindow(configGroup);
    } else {
        loadRootItem( configGroup, tabContainer(), KUrl(), true, KUrl() );
#ifndef NDEBUG
        printFullHierarchy();
#endif
        return m_pMainWindow;
    }
}


void KonqViewManager::removeTab( KonqFrameBase* currentFrame, bool emitAboutToRemoveSignal )
{
    Q_ASSERT(currentFrame);
#ifdef DEBUG_VIEWMGR
  kDebug() << currentFrame;
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  if ( m_tabContainer->count() == 1 )
    m_pMainWindow->slotAddTab(); // #214378

  if(emitAboutToRemoveSignal)
    emit aboutToRemoveTab(currentFrame);

  if (currentFrame->asQWidget() == m_tabContainer->currentWidget())
    setActivePart(0);

  const QList<KonqView*> viewList = KonqViewCollector::collect(currentFrame);
  foreach ( KonqView* view, viewList )
  {
    if (view == m_pMainWindow->currentView())
      setActivePart(0);
    m_pMainWindow->removeChildView( view );
    delete view;
  }

  m_tabContainer->childFrameRemoved(currentFrame);

  delete currentFrame;

  m_tabContainer->slotCurrentChanged(m_tabContainer->currentIndex());

  m_pMainWindow->viewCountChanged();

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif
}

void KonqViewManager::reloadAllTabs( )
{
  foreach ( KonqFrameBase* frame, tabContainer()->childFrameList() )
  {
      if ( frame && frame->activeChildView())
      {
          if( !frame->activeChildView()->locationBarURL().isEmpty())
              frame->activeChildView()->openUrl( frame->activeChildView()->url(), frame->activeChildView()->locationBarURL());
      }
  }
}

void KonqViewManager::removeOtherTabs(int tabIndex)
{
    QList<KonqFrameBase*> tabs = m_tabContainer->childFrameList();
    for (int i = 0; i < tabs.count(); ++i) {
        if (i != tabIndex) {
            removeTab(tabs.at(i));
        }
    }
}

void KonqViewManager::moveTabBackward()
{
    if( m_tabContainer->count() == 1 ) return;

    int iTab = m_tabContainer->currentIndex();
    m_tabContainer->moveTabBackward(iTab);
}

void KonqViewManager::moveTabForward()
{
    if( m_tabContainer->count() == 1 ) return;

    int iTab = m_tabContainer->currentIndex();
    m_tabContainer->moveTabForward(iTab);
}

void KonqViewManager::activateNextTab()
{
  if( m_tabContainer->count() == 1 ) return;

  int iTab = m_tabContainer->currentIndex();

  iTab++;

  if( iTab == m_tabContainer->count() )
    iTab = 0;

  m_tabContainer->setCurrentIndex( iTab );
}

void KonqViewManager::activatePrevTab()
{
  if( m_tabContainer->count() == 1 ) return;

  int iTab = m_tabContainer->currentIndex();

  iTab--;

  if( iTab == -1 )
    iTab = m_tabContainer->count() - 1;

  m_tabContainer->setCurrentIndex( iTab );
}

void KonqViewManager::activateTab(int position)
{
  if (position<0 || m_tabContainer->count() == 1 || position>=m_tabContainer->count()) return;

  m_tabContainer->setCurrentIndex( position );
}

void KonqViewManager::showTab( KonqView *view )
{
  if (m_tabContainer->currentWidget() != view->frame()) {
    m_tabContainer->setCurrentIndex( m_tabContainer->indexOf( view->frame() ) );
  }
}

void KonqViewManager::showTab(int tabIndex)
{
    if (m_tabContainer->currentIndex() != tabIndex) {
        m_tabContainer->setCurrentIndex(tabIndex);
    }
}

void KonqViewManager::updatePixmaps()
{
    const QList<KonqView*> viewList = KonqViewCollector::collect(tabContainer());
    foreach ( KonqView* view, viewList )
        view->setTabIcon( KUrl( view->locationBarURL() ) );
}

void KonqViewManager::openClosedTab(const KonqClosedTabItem& closedTab)
{
    kDebug();
    loadRootItem( closedTab.configGroup(), m_tabContainer, KUrl(), true, KUrl(), QString(), false, closedTab.pos() );

    int pos = ( closedTab.pos() < m_tabContainer->count() ) ? closedTab.pos() : m_tabContainer->count()-1;
    kDebug() << "pos, m_tabContainer->count():" << pos << m_tabContainer->count()-1;

    m_tabContainer->setCurrentIndex( pos );
}

void KonqViewManager::removeView( KonqView *view )
{
#ifdef DEBUG_VIEWMGR
  kDebug() << view;
  m_pMainWindow->dumpViewList();
  printFullHierarchy();
#endif

  if (!view)
    return;

  KonqFrame* frame = view->frame();
  KonqFrameContainerBase* parentContainer = frame->parentContainer();

  kDebug() << "view=" << view << "frame=" << frame << "parentContainer=" << parentContainer;

  if (parentContainer->frameType() == KonqFrameBase::Container)
  {
    setActivePart(0);

    kDebug() << "parentContainer is a KonqFrameContainer";

    KonqFrameContainerBase* grandParentContainer = parentContainer->parentContainer();
    kDebug() << "grandParentContainer=" << grandParentContainer;

    KonqFrameBase* otherFrame = static_cast<KonqFrameContainer*>(parentContainer)->otherChild( frame );
    if( !otherFrame ) {
        kWarning() << "This shouldn't happen!" ;
        return;
    }

    static_cast<KonqFrameContainer*>(parentContainer)->setAboutToBeDeleted();

    // If the grand parent is a KonqFrameContainer, we need the sizes of the views inside it to restore these after
    // the parent is replaced. To access the sizes via QSplitter::sizes(), a pointer to a KonqFrameContainerBase
    //  is not sufficient. We need a pointer to a KonqFrameContainer which is derived from QSplitter.
    KonqFrameContainer* grandParentKonqFrameContainer = dynamic_cast<KonqFrameContainer*> ( grandParentContainer );
    QList<int> grandParentSplitterSizes;
    if ( grandParentKonqFrameContainer ) {
      grandParentSplitterSizes = grandParentKonqFrameContainer->sizes();
    }

    m_pMainWindow->removeChildView(view);

    //kDebug() << "--- Deleting view" << view;
    grandParentContainer->replaceChildFrame(parentContainer, otherFrame);

    //kDebug() << "--- Removing otherFrame from parentContainer";
    parentContainer->childFrameRemoved( otherFrame );

    delete view; // This deletes the view, which deletes the part, which deletes its widget

    delete parentContainer;

    if ( grandParentKonqFrameContainer ) {
      grandParentKonqFrameContainer->setSizes( grandParentSplitterSizes );
    }

    grandParentContainer->setActiveChild( otherFrame );
    grandParentContainer->activateChild();
    m_pMainWindow->viewCountChanged();
  }
  else if (parentContainer->frameType() == KonqFrameBase::Tabs) {
    kDebug() << "parentContainer" << parentContainer << "is a KonqFrameTabs";

    removeTab( frame );
  }
  else if (parentContainer->frameType() == KonqFrameBase::MainWindow)
    kDebug() << "parentContainer is a KonqMainWindow.  This shouldn't be removeable, not removing.";
  else
    kDebug() << "Unrecognized frame type, not removing.";

#ifdef DEBUG_VIEWMGR
  printFullHierarchy();
  m_pMainWindow->dumpViewList();

  kDebug() << "done";
#endif
}

// reimplemented from PartManager
void KonqViewManager::removePart( KParts::Part * part )
{
    //kDebug() << part;
  // This is called when a part auto-deletes itself (case 1), or when
  // the "delete view" above deletes, in turn, the part (case 2)

  KParts::PartManager::removePart( part );

  // If we were called by PartManager::slotObjectDestroyed, then the inheritance has
  // been deleted already... Can't use inherits().

  KonqView * view = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(part) );
  if ( view ) // the child view still exists, so we are in case 1
  {
      kDebug() << "Found a child view";

      // Make sure that deleting the frame won't delete the part's widget;
      // that's already taken care of by the part.
      view->part()->widget()->hide();
      view->part()->widget()->setParent(0);

      view->partDeleted(); // tell the child view that the part auto-deletes itself

      if (m_pMainWindow->mainViewsCount() == 1)
      {
        kDebug() << "Deleting last view -> closing the window";
        clear();
        kDebug() << "Closing m_pMainWindow" << m_pMainWindow;
        m_pMainWindow->close(); // will delete it
        return;
      } else { // normal case
        removeView( view );
      }
  }

  //kDebug() << part << "done";
}

void KonqViewManager::slotPassiveModePartDeleted()
{
  // Passive mode parts aren't registered to the part manager,
  // so we have to handle suicidal ones ourselves
  KParts::ReadOnlyPart * part = const_cast<KParts::ReadOnlyPart *>( static_cast<const KParts::ReadOnlyPart *>( sender() ) );
  disconnect( part, SIGNAL(destroyed()), this, SLOT(slotPassiveModePartDeleted()) );
  kDebug() << "part=" << part;
  KonqView * view = m_pMainWindow->childView( part );
  kDebug() << "view=" << view;
  if ( view != 0L) // the child view still exists, so the part suicided
  {
    view->partDeleted(); // tell the child view that the part deleted itself
    removeView( view );
  }
}

void KonqViewManager::viewCountChanged()
{
  bool bShowActiveViewIndicator = ( m_pMainWindow->viewCount() > 1 );
  bool bShowLinkedViewIndicator = ( m_pMainWindow->linkableViewsCount() > 1 );

  const KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();
  KonqMainWindow::MapViews::ConstIterator it = mapViews.begin();
  KonqMainWindow::MapViews::ConstIterator end = mapViews.end();
  for (  ; it != end ; ++it )
  {
    KonqFrameStatusBar* sb = it.value()->frame()->statusbar();
    sb->showActiveViewIndicator( bShowActiveViewIndicator && !it.value()->isPassiveMode() );
    sb->showLinkedViewIndicator( bShowLinkedViewIndicator && !it.value()->isFollowActive() );
  }
}

void KonqViewManager::clear()
{
    //kDebug();
    setActivePart(0);

    if (m_pMainWindow->childFrame() == 0) return;

    const QList<KonqView*> viewList = KonqViewCollector::collect(m_pMainWindow);
    if ( !viewList.isEmpty() ) {
        //kDebug() << viewList.count() << "items";

        foreach ( KonqView* view, viewList ) {
            m_pMainWindow->removeChildView( view );
            //kDebug() << "Deleting" << view;
            delete view;
        }
    }

    KonqFrameBase* frame = m_pMainWindow->childFrame();
    Q_ASSERT( frame );
    //kDebug() << "deleting mainFrame ";
    m_pMainWindow->childFrameRemoved( frame ); // will set childFrame() to NULL
    delete frame;
    // tab container was deleted by the above
    m_tabContainer = 0;
    m_pMainWindow->viewCountChanged();
}

KonqView *KonqViewManager::chooseNextView(KonqView *view)
{
    //kDebug() << view;

    int it = 0;
    const QList<KonqView*> viewList = KonqViewCollector::collect(m_pMainWindow);
    if (viewList.isEmpty()) {
        return 0; // We have no view at all - this used to happen with totally-empty-profiles
    }

    if (view) { // find it in the list
        it = viewList.indexOf(view);
    }

    // the view should always be in the list
    if (it == -1) {
        kWarning() << "View" << view << "is not in list!" ;
        it = 0;
    }

    bool rewinded = false;
    const int startIndex = it;
    const int end = viewList.count();

    //kDebug() << "count=" << end;
    while (true) {
        //kDebug() << "going next";
        if (++it == end) { // move to next
            // end reached: restart from begin (but only once)
            if (!rewinded) {
                it = 0;
                rewinded = true;
            } else {
                break; // nothing found, probably buggy profile
            }
        }

        if (it == startIndex && view)
            break; // no next view found

        KonqView *nextView = viewList.at(it);;
        if (nextView && !nextView->isPassiveMode())
            return nextView;
        //kDebug() << "nextView=" << nextView << "passive=" << nextView->isPassiveMode();
    }

    //kDebug() << "returning 0";
    return 0; // no next view found
}

KonqViewFactory KonqViewManager::createView( const QString &serviceType,
                                             const QString &serviceName,
                                             KService::Ptr &service,
                                             KService::List &partServiceOffers,
                                             KService::List &appServiceOffers,
                                             bool forceAutoEmbed )
{
  KonqViewFactory viewFactory;

  if( serviceType.isEmpty() && m_pMainWindow->currentView() ) {
    //clone current view
    KonqView *cv = m_pMainWindow->currentView();
    QString _serviceType, _serviceName;
    if ( cv->service()->desktopEntryName() == "konq_sidebartng" ) {
      _serviceType = "text/html";
    }
    else {
      _serviceType = cv->serviceType();
      _serviceName = cv->service()->desktopEntryName();
    }

    KonqFactory konqFactory;
    viewFactory = konqFactory.createView( _serviceType, _serviceName,
                                           &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed );
  }
  else {
    //create view with the given servicetype
    KonqFactory konqFactory;
    viewFactory = konqFactory.createView( serviceType, serviceName,
                                           &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed );
  }

  return viewFactory;
}

KonqView *KonqViewManager::setupView( KonqFrameContainerBase *parentContainer,
                                      KonqViewFactory &viewFactory,
                                      const KService::Ptr &service,
                                      const KService::List &partServiceOffers,
                                      const KService::List &appServiceOffers,
                                      const QString &serviceType,
                                      bool passiveMode,
                                      bool openAfterCurrentPage,
                                      int pos )
{
    //kDebug() << "passiveMode=" << passiveMode;

  QString sType = serviceType;

  if ( sType.isEmpty() ) // TODO remove this -- after checking all callers; splitMainContainer seems to need this logic
    sType = m_pMainWindow->currentView()->serviceType();

  //kDebug() << "creating KonqFrame with parent=" << parentContainer;
  KonqFrame* newViewFrame = new KonqFrame( parentContainer->asQWidget(), parentContainer );
  newViewFrame->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );

  //kDebug() << "Creating KonqView";
  KonqView *v = new KonqView( viewFactory, newViewFrame,
                              m_pMainWindow, service, partServiceOffers, appServiceOffers, sType, passiveMode );
  //kDebug() << "KonqView created - v=" << v << "v->part()=" << v->part();

  QObject::connect( v, SIGNAL(sigPartChanged(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)),
                    m_pMainWindow, SLOT(slotPartChanged(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)) );

  m_pMainWindow->insertChildView( v );


  int index = -1;
  if ( openAfterCurrentPage )
    index = m_tabContainer->currentIndex() + 1;
  else if(pos > -1)
    index = pos;

  parentContainer->insertChildFrame( newViewFrame, index );

  if (parentContainer->frameType() != KonqFrameBase::Tabs) {
      newViewFrame->show();
  }

  // Don't register passive views to the part manager
  if ( !v->isPassiveMode() ) // note that KonqView's constructor could set this to true even if passiveMode is false
    addPart( v->part(), false );
  else
  {
    // Passive views aren't registered, but we still want to detect the suicidal ones
    connect( v->part(), SIGNAL(destroyed()), this, SLOT(slotPassiveModePartDeleted()) );
  }

  if (!m_bLoadingProfile)
      m_pMainWindow->viewCountChanged();

  //kDebug() << "done";
  return v;
}

///////////////// Profile stuff ////////////////

void KonqViewManager::saveViewProfileToFile(const QString & fileName, const QString & profileName, KonqFrameBase::Options options)
{
    const QString path = KStandardDirs::locateLocal("data", QString::fromLatin1("konqueror/profiles/") +
                                                    fileName);
    QFile::remove(path); // in case it exists already

    KConfig _cfg(path, KConfig::SimpleConfig);
    KConfigGroup profileGroup(&_cfg, "Profile");
    if  (!profileName.isEmpty())
        profileGroup.writePathEntry("Name", profileName);

    saveViewProfileToGroup(profileGroup, options);

    _cfg.sync();
}

void KonqViewManager::saveViewProfileToGroup(KConfigGroup & profileGroup, KonqFrameBase::Options options)
{
    if( m_pMainWindow->childFrame() ) {
        QString prefix = KonqFrameBase::frameTypeToString(m_pMainWindow->childFrame()->frameType())
                         + QString::number(0);
        profileGroup.writeEntry( "RootItem", prefix );
        prefix.append( QLatin1Char( '_' ) );
        m_pMainWindow->saveConfig( profileGroup, prefix, options, tabContainer(), 0, 1);
    }

    profileGroup.writeEntry( "FullScreen", m_pMainWindow->fullScreenMode());
    profileGroup.writeEntry("XMLUIFile", m_pMainWindow->xmlFile());

    m_pMainWindow->saveMainWindowSettings(profileGroup);
}

void KonqViewManager::loadViewProfileFromFile( const QString & path, const QString & filename,
                                               const KUrl & forcedUrl, const KonqOpenURLRequest &req,
                                               bool resetWindow, bool openUrl )
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(path, KConfig::SimpleConfig);
    loadViewProfileFromConfig(config, path, filename, forcedUrl, req, resetWindow, openUrl );
}

void KonqViewManager::setCurrentProfile(const QString& profileFileName)
{
    m_currentProfile = profileFileName;

    // We'll use the profile for saving window settings - so ensure we can save to it
    const QString localPath = KStandardDirs::locateLocal("data", QString::fromLatin1("konqueror/profiles/") +
                                                         profileFileName, KGlobal::mainComponent());
    kDebug() << profileFileName << "localPath=" << localPath;
    KSharedConfigPtr cfg = KSharedConfig::openConfig(localPath, KConfig::SimpleConfig);
    if (!QFile::exists(localPath)) {
        const QString globalFile = KStandardDirs::locate("data", QString::fromLatin1("konqueror/profiles/") +
                                                         profileFileName, KGlobal::mainComponent());
        kDebug() << "globalFile=" << globalFile;
        if (!globalFile.isEmpty()) {
            KSharedConfigPtr globalCfg = KSharedConfig::openConfig(globalFile, KConfig::SimpleConfig);
            globalCfg->copyTo(localPath, cfg.data());
        }
    }

    KConfigGroup profileGroup(cfg, "Profile");
    m_currentProfileText = profileGroup.readPathEntry("Name", m_currentProfile);

    // setProfileConfig must be done after setting m_currentProfile/m_currentProfileText
    // We also do it after loadViewProfileFromGroup so that we can override the default size (Width=80%)
    m_pMainWindow->setProfileConfig(profileGroup);
}

void KonqViewManager::loadViewProfileFromConfig(const KSharedConfigPtr& _cfg,
                                                const QString& path,
                                                const QString & filename,
                                                const KUrl & forcedUrl,
                                                const KonqOpenURLRequest &req,
                                                bool resetWindow, bool openUrl)
{
    Q_UNUSED(path); // _cfg and path could be passed to setCurrentProfile for optimization
    // resetWindow was used to resize the window to a default size,
    // not needed anymore, since the size is in the profile (## what about about:blank?)
    Q_UNUSED(resetWindow);

    KConfigGroup profileGroup(_cfg, "Profile");


    // Repair profiles without tabs (#203166)
    const QString rootItem = profileGroup.readEntry("RootItem", "empty");
    const QString childrenKey = rootItem + "_Children";
    if (profileGroup.readEntry(childrenKey, QStringList()) == (QStringList() << "View1" << "View2")) {
        kDebug() << "Activating special tabwidget-insertion-hack";
        profileGroup.writeEntry(childrenKey, QStringList() << "View1" << "Tabs1");
        profileGroup.writeEntry("Tabs1_Children", "View2");
    }

    loadViewProfileFromGroup(profileGroup, filename, forcedUrl, req, openUrl);

    setCurrentProfile(filename);

#ifdef DEBUG_VIEWMGR
    printFullHierarchy();
#endif
}

void KonqViewManager::loadViewProfileFromGroup( const KConfigGroup &profileGroup, const QString & filename,
                                                const KUrl & forcedUrl, const KonqOpenURLRequest &req,
                                                bool openUrl )
{
    Q_UNUSED(filename); // could be useful in case of error messages

    KUrl defaultURL;
    if (m_pMainWindow->currentView())
        defaultURL = m_pMainWindow->currentView()->url();

    clear();

    if (forcedUrl.url() != "about:blank") {
        loadRootItem( profileGroup, m_pMainWindow, defaultURL, openUrl && forcedUrl.isEmpty(), forcedUrl, req.serviceName );
    } else {
        // ## in this case we won't resize the window, so bool resetWindow could be useful after all?
        m_pMainWindow->disableActionsNoView();
        m_pMainWindow->action("clear_location")->trigger();
    }

    //kDebug() << "after loadRootItem";

    // Set an active part first so that we open the URL in the current view
    // (to set the location bar correctly and asap)
    KonqView *nextChildView = 0;
    nextChildView = m_pMainWindow->activeChildView();
    if (nextChildView == 0) nextChildView = chooseNextView(0);
    setActivePart(nextChildView ? nextChildView->part() : 0);

    // #71164
    if (!req.browserArgs.frameName.isEmpty() && nextChildView) {
      nextChildView->setViewName(req.browserArgs.frameName);
    }

    if (openUrl && !forcedUrl.isEmpty()) {
      KonqOpenURLRequest _req(req);
      _req.openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
      _req.forceAutoEmbed = true; // it's a new window, let's use it

      m_pMainWindow->openUrl( nextChildView /* can be 0 for an empty profile */,
                              forcedUrl, _req.args.mimeType(), _req, _req.browserArgs.trustedSource );

      // TODO choose a linked view if any (instead of just the first one),
      // then open the same URL in any non-linked one
    } else {
        if (forcedUrl.isEmpty() && m_pMainWindow->locationBarURL().isEmpty()) {
            // No URL -> the user will want to type one
            m_pMainWindow->focusLocationBar();
        }
    }

    // Window size
    if (!m_pMainWindow->initialGeometrySet()) {
        if (profileGroup.readEntry("FullScreen", false)) {
            // Full screen on
            m_pMainWindow->setWindowState(m_pMainWindow->windowState() | Qt::WindowFullScreen);
        } else {
            // Full screen off
            m_pMainWindow->setWindowState(m_pMainWindow->windowState() & ~Qt::WindowFullScreen);
            m_pMainWindow->applyWindowSizeFromProfile(profileGroup);
        }
    }

    //kDebug() << "done";
}

void KonqViewManager::setActivePart(KParts::Part *part, QWidget *)
{
    doSetActivePart( static_cast<KParts::ReadOnlyPart*>(part) );
}

void KonqViewManager::doSetActivePart( KParts::ReadOnlyPart *part )
{
    if (part)
        kDebug() << part << part->url();

    KParts::Part* mainWindowActivePart = m_pMainWindow->currentView()
                                         ? m_pMainWindow->currentView()->part() : 0;
    if (part == activePart() && mainWindowActivePart == part)
    {
      //kDebug() << "Part is already active!";
      return;
    }

    // ## is this the right currentView() already?
    if (m_pMainWindow->currentView())
      m_pMainWindow->currentView()->setLocationBarURL(m_pMainWindow->locationBarURL());

    KParts::PartManager::setActivePart( part );

    // Giving focus to the part widget will trigger PartManager which will call KonqViewManager::setActivePart
    if (part && part->widget()) {
        part->widget()->setFocus();

        // However in case of an error URL we want to make it possible for the user to fix it
        KonqView* view = m_pMainWindow->viewMap().value(part);
        if (view && view->isErrorUrl()) {
            m_pMainWindow->focusLocationBar();
        }
    }

    emitActivePartChanged(); // This is what triggers KonqMainWindow::slotPartActivated
}

void KonqViewManager::slotActivePartChanged ( KParts::Part *newPart )
{
    //kDebug() << newPart;
    if (newPart == 0L) {
      //kDebug() << "newPart = 0L , returning";
      return;
    }
    // Send event to mainwindow - this is useful for plugins (like searchbar)
    KParts::PartActivateEvent ev(true, newPart, newPart->widget());
    QApplication::sendEvent(m_pMainWindow, &ev);

    KonqView * view = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(newPart) );
    if (view == 0L) {
      kDebug() << "No view associated with this part";
      return;
    }
    if (view->frame()->parentContainer() == 0L) return;
    if (!m_bLoadingProfile)  {
        view->frame()->statusbar()->updateActiveStatus();
        view->frame()->parentContainer()->setActiveChild( view->frame() );
    }
    //kDebug() << "done";
}

void KonqViewManager::emitActivePartChanged()
{
    m_pMainWindow->slotPartActivated( activePart() );
}

QSize KonqViewManager::readDefaultSize(const KConfigGroup &cfg, QWidget *widget)
{
    QString widthStr = cfg.readEntry("Width");
    QString heightStr = cfg.readEntry("Height");
    int width = -1;
    int height = -1;
    const QRect geom = KGlobalSettings::desktopGeometry(widget);

    bool ok;
    if (widthStr.endsWith('%')) {
        widthStr.truncate(widthStr.length()-1);
        const int relativeWidth = widthStr.toInt(&ok);
        if (ok) {
            width = relativeWidth * geom.width() / 100;
        }
    } else {
        width = widthStr.toInt( &ok );
        if (!ok)
            width = -1;
    }

    if (heightStr.endsWith('%')) {
        heightStr.truncate(heightStr.length() - 1);
        int relativeHeight = heightStr.toInt(&ok);
        if (ok) {
            height = relativeHeight * geom.height() / 100;
        }
    } else {
        height = heightStr.toInt(&ok);
        if (!ok)
            height = -1;
    }

    return QSize(width, height);
}

void KonqViewManager::loadRootItem( const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                                    const KUrl & defaultURL, bool openUrl,
                                    const KUrl& forcedUrl, const QString& forcedService,
                                    bool openAfterCurrentPage,
                                    int pos )
{
    const QString rootItem = cfg.readEntry("RootItem", "empty");

    // This flag is used by KonqView, to distinguish manual view creation
    // from profile loading (e.g. in switchView)
    m_bLoadingProfile = true;

    loadItem( cfg, parent, rootItem, defaultURL, openUrl, forcedUrl, forcedService, openAfterCurrentPage, pos );

    m_bLoadingProfile = false;

    m_pMainWindow->enableAllActions(true);

    // This flag disables calls to viewCountChanged while creating the views,
    // so we do it once at the end :
    viewCountChanged();


}

void KonqViewManager::loadItem( const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                                const QString &name, const KUrl & defaultURL, bool openUrl,
                                const KUrl& forcedUrl,
                                const QString& forcedService,
                                bool openAfterCurrentPage, int pos )
{
    QString prefix;
    if (name != "InitialView") { // InitialView is old stuff, not in use anymore
        prefix = name + QLatin1Char( '_' );
    }

#ifdef DEBUG_VIEWMGR
    kDebug() << "begin name=" << name << "openUrl=" << openUrl;
#endif

    if (name.startsWith("View") || name == "empty") {
        // load view config

        QString serviceType;
        QString serviceName;
        if (name == "empty") {
            // An empty profile is an empty KHTML part. Makes all KHTML actions available, avoids crashes,
            // makes it easy to DND a URL onto it, and makes it fast to load a website from there.
            serviceType = "text/html";
            serviceName = forcedService; // coming e.g. from the cmdline, otherwise empty
        } else {
            serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), QString("inode/directory"));
            serviceName = cfg.readEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), QString() );
            if (serviceName == "konq_aboutpage") {
                if ( (!forcedUrl.isEmpty() && forcedUrl.protocol() != "about") ||
                     (forcedUrl.isEmpty() && openUrl == false)) // e.g. window.open
                {
                    // No point in loading the about page if we're going to replace it with a KHTML part right away
                    serviceType = "text/html";
                    serviceName = forcedService; // coming e.g. from the cmdline, otherwise empty
                }
            }
        }
        //kDebug() << "serviceType" << serviceType << serviceName;

        KService::Ptr service;
        KService::List partServiceOffers, appServiceOffers;

        KonqFactory konqFactory;
        KonqViewFactory viewFactory = konqFactory.createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers, true /*forceAutoEmbed*/ );
        if (viewFactory.isNull()) {
          kWarning() << "Profile Loading Error: View creation failed" ;
          return; //ugh..
        }

        bool passiveMode = cfg.readEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), false );

        //kDebug() << "Creating View Stuff; parent=" << parent;
        if ( parent == m_pMainWindow )
            parent = tabContainer();
        KonqView *childView = setupView( parent, viewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode, openAfterCurrentPage, pos );

        if (!childView->isFollowActive())
            childView->setLinkedView( cfg.readEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), false ) );
        const bool isToggleView = cfg.readEntry(QString::fromLatin1("ToggleView").prepend(prefix), false);
        childView->setToggleView( isToggleView );
        if (isToggleView /*100373*/ || !cfg.readEntry(QString::fromLatin1("ShowStatusBar").prepend(prefix), true)) {
            childView->frame()->statusbar()->hide();
        }

#if 0 // currently unused
        KonqConfigEvent ev( cfg.config(), prefix+'_', false/*load*/);
        QApplication::sendEvent( childView->part(), &ev );
#endif

        if (parent == m_tabContainer && m_tabContainer->count() == 1) {
            // First tab, make it the active one
            parent->setActiveChild(childView->frame());
        }

        if (openUrl) {
            const QString keyHistoryItems = QString::fromLatin1( "NumberOfHistoryItems" ).prepend( prefix );
            if( cfg.hasKey(keyHistoryItems) ) {
                childView->loadHistoryConfig(cfg, prefix);
                m_pMainWindow->updateHistoryActions();
            } else {
                // determine URL
                const QString urlKey = QString::fromLatin1("URL").prepend(prefix);
                KUrl url;
                if (cfg.hasKey(urlKey)) {
                    url = cfg.readPathEntry(urlKey, QString::fromLatin1("about:blank"));
                } else if (urlKey == "empty_URL") { // old stuff, not in use anymore
                    url = QString::fromLatin1("about:blank");
                } else {
                    url = defaultURL;
                }

                if ( !url.isEmpty() ) {
                    //kDebug() << "calling openUrl" << url;
                    //childView->openUrl( url, url.prettyUrl() );
                    // We need view-follows-view (for the dirtree, for instance)
                    KonqOpenURLRequest req;
                    if (url.protocol() != "about")
                        req.typedUrl = url.prettyUrl();
                    m_pMainWindow->openView( serviceType, url, childView, req );
                }
                //else kDebug() << "url is empty";
            }
        }
        // Do this after opening the URL, so that it's actually possible to open it :)
        childView->setLockedLocation( cfg.readEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), false ) );
    }
  else if( name.startsWith("Container") ) {
    //kDebug() << "Item is Container";

    //load container config
    QString ostr = cfg.readEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), QString() );
    //kDebug() << "Orientation:" << ostr;
    Qt::Orientation o;
    if( ostr == "Vertical" )
      o = Qt::Vertical;
    else if( ostr == "Horizontal" )
      o = Qt::Horizontal;
    else {
      kWarning() << "Profile Loading Error: No orientation specified in" << name ;
      o = Qt::Horizontal;
    }

    QList<int> sizes =
        cfg.readEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ),QList<int>());

    int index = cfg.readEntry( QString::fromLatin1( "activeChildIndex" ).prepend(prefix), -1 );

    QStringList childList = cfg.readEntry( QString::fromLatin1( "Children" ).prepend( prefix ),QStringList() );
    if( childList.count() < 2 )
    {
      kWarning() << "Profile Loading Error: Less than two children in" << name ;
      // fallback to defaults
      loadItem( cfg, parent, "InitialView", defaultURL, openUrl, forcedUrl, forcedService );
    }
    else
    {
      KonqFrameContainer *newContainer = new KonqFrameContainer( o, parent->asQWidget(), parent );

      int tabindex = pos;
      if(openAfterCurrentPage && parent->frameType() == KonqFrameBase::Tabs) // Need to honor it, if possible
	tabindex = static_cast<KonqFrameTabs*>(parent)->currentIndex() + 1;
      parent->insertChildFrame( newContainer, tabindex );

      loadItem( cfg, newContainer, childList.at(0), defaultURL, openUrl, forcedUrl, forcedService );
      loadItem( cfg, newContainer, childList.at(1), defaultURL, openUrl, forcedUrl, forcedService );

      //kDebug() << "setSizes" << sizes;
      newContainer->setSizes( sizes );

      if (index == 1)
        newContainer->setActiveChild( newContainer->secondChild() );
      else if (index == 0)
        newContainer->setActiveChild( newContainer->firstChild() );

      newContainer->show();
    }
  }
  else if( name.startsWith("Tabs") )
  {
    //kDebug() << "Item is a Tabs";

    int index = cfg.readEntry( QString::fromLatin1( "activeChildIndex" ).prepend(prefix), 0 );
    if ( !m_tabContainer ) {
        createTabContainer(parent->asQWidget(), parent);
        parent->insertChildFrame( m_tabContainer );
    }

    const QStringList childList = cfg.readEntry( QString::fromLatin1( "Children" ).prepend( prefix ),QStringList() );
    for ( QStringList::const_iterator it = childList.begin(); it != childList.end(); ++it )
    {
        loadItem( cfg, tabContainer(), *it, defaultURL, openUrl, forcedUrl, forcedService );
        QWidget* currentPage = m_tabContainer->currentWidget();
        if (currentPage != 0L) {
          KonqView* activeChildView = dynamic_cast<KonqFrameBase*>(currentPage)->activeChildView();
          if (activeChildView != 0L) {
            activeChildView->setCaption( activeChildView->caption() );
            activeChildView->setTabIcon( activeChildView->url() );
          }
        }
    }

    QWidget* w = m_tabContainer->widget(index);
    if(w)
    {
        m_tabContainer->setActiveChild( dynamic_cast<KonqFrameBase*>(w) );
        m_tabContainer->setCurrentIndex( index );
        m_tabContainer->show();
    } else
        kWarning() << "Profile Loading Error: Unknown current item index" << index;

  }
  else
      kWarning() << "Profile Loading Error: Unknown item" << name;

    //kDebug() << "end" << name;
}

void KonqViewManager::setProfiles( KActionMenu *profiles )
{
    m_pamProfiles = profiles;
    connect(m_pamProfiles->menu(), SIGNAL(triggered(QAction*)),
            this, SLOT(slotProfileActivated(QAction*)));
    connect(m_pamProfiles->menu(), SIGNAL(aboutToShow()),
            this, SLOT(slotProfileListAboutToShow()));
    //KonqMainWindow::enableAllActions will call it anyway
    //profileListDirty();
}

void KonqViewManager::showProfileDlg( const QString & preselectProfile )
{
  KonqProfileDlg dlg( this, preselectProfile, m_pMainWindow );
  dlg.exec();
  profileListDirty();
}

void KonqViewManager::slotProfileDlg()
{
  showProfileDlg( QString() );
}

void KonqViewManager::profileListDirty( bool broadcast )
{
    if (!broadcast) {
        m_bProfileListDirty = true;
        return;
    }

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "updateAllProfileList");
    QDBusConnection::sessionBus().send(message);
}

void KonqViewManager::slotProfileActivated(QAction* action)
{
    if ( tabContainer()->count() > 1 ) {
        if ( KMessageBox::warningContinueCancel( m_pMainWindow,
                                                 i18n("You have multiple tabs open in this window.\n"
                                                      "Loading a view profile will close them."),
                                                 i18nc("@title:window", "Confirmation"),
                                                 KGuiItem(i18n("Load View Profile")),
                                                 KStandardGuiItem::cancel(),
                                                 "LoadProfileTabsConfirm" ) == KMessageBox::Cancel )
            return;
    }
    const int originalTabIndex = m_tabContainer->currentIndex();
    foreach ( KonqFrameBase* frame, m_tabContainer->childFrameList() )
    {
        KonqView *view = frame->activeChildView();
        if (view && view->isModified()) {
                showTab( view );
                if ( KMessageBox::warningContinueCancel( 0,
                                                         i18n("This tab contains changes that have not been submitted.\nLoading a profile will discard these changes."),
                                                         i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes")), KStandardGuiItem::cancel(), "discardchangesloadprofile") != KMessageBox::Continue )
                    /* WE: maybe KStandardGuiItem(Discard) here? */
                {
                    showTab(originalTabIndex);
                    return;
                }
        }
    }
    showTab(originalTabIndex);

    const QString profilePath = action->data().toString();
    const QString fileName = KUrl(profilePath).fileName();
    KConfig cfg(profilePath);
    KConfigGroup profileGroup( &cfg, "Profile" );
    const QString xmluiFile = normalizedXMLFileName(profileGroup.readEntry("XMLUIFile","konqueror.rc"));

    //If the profile specifies an xmlgui file that differs from the currently
    //loaded one, then we have no choice but to recreate the window.  I've
    //experimented with trying to get xmlgui to recreate the gui, but it is
    //too brittle.  The only other choice is to ignore the profile which is
    //not very nice.
    if (xmluiFile != m_pMainWindow->xmlFile()) {
        m_pMainWindow->deleteLater();
        KonqMainWindow* mw = KonqMisc::createBrowserWindowFromProfile(profilePath, fileName, m_pMainWindow->currentView()->url());
        if (mw)
            mw->show();
    } else {
        loadViewProfileFromFile(profilePath, fileName);
    }
}

void KonqViewManager::slotProfileListAboutToShow()
{
  if ( !m_pamProfiles || !m_bProfileListDirty )
    return;

  KMenu *popup = m_pamProfiles->menu();
  popup->clear();

  // Fetch profiles
  m_mapProfileNames = KonqProfileDlg::readAllProfiles();

  // Generate accelerators
  const QStringList profileNames = m_mapProfileNames.keys();
  QStringList accel_strings;
  KAccelGen::generate(profileNames, accel_strings);

  // Store menu items
  const QStringList profilePaths = m_mapProfileNames.values();
  for (int i = 0; i < accel_strings.count(); ++i) {
      KAction* action = new KAction(accel_strings.at(i), popup);
      action->setData(profilePaths.at(i));
      popup->addAction(action);
  }

  m_bProfileListDirty = false;
}

void KonqViewManager::setLoading( KonqView *view, bool loading )
{
    tabContainer()->setLoading(view->frame(), loading);
}

void KonqViewManager::showHTML(bool b)
{
    foreach ( KonqFrameBase* frame, tabContainer()->childFrameList() ) {
        KonqView* view = frame->activeChildView();
        if ( view && view != m_pMainWindow->currentView() ) {
            view->setAllowHTML( b );
            if( !view->locationBarURL().isEmpty() ) {
                m_pMainWindow->showHTML( view, b, false );
            }
        }
    }
}



///////////////// Debug stuff ////////////////

#ifndef NDEBUG
void KonqViewManager::printSizeInfo( KonqFrameBase* frame,
                                     KonqFrameContainerBase* parent,
                                     const char* msg )
{
    const QRect r = frame->asQWidget()->geometry();
    qDebug("Child size %s : x: %d, y: %d, w: %d, h: %d", msg, r.x(),r.y(),r.width(),r.height() );

    if (parent->frameType() == KonqFrameBase::Container) {
        const QList<int> sizes = static_cast<KonqFrameContainer*>(parent)->sizes();
        printf( "Parent sizes %s :", msg );
        foreach( int i, sizes ) {
            printf( " %d", i );
        }
        printf("\n");
    }
}

class KonqDebugFrameVisitor : public KonqFrameVisitor
{
public:
    KonqDebugFrameVisitor() {}
    virtual bool visit(KonqFrame* frame) {
        QString className;
        if ( !frame->part() )
            className = "NoPart!";
        else if ( !frame->part()->widget() )
            className = "NoWidget!";
        else
            className = frame->part()->widget()->metaObject()->className();
        kDebug() << m_spaces << frame
                     << "parent=" << frame->parentContainer()
                     << (frame->isHidden() ? "hidden" : "shown")
                     << "containing view" << frame->childView()
                     << "and part" << frame->part()
                     << "whose widget is a" << className;
        return true;
    }
    virtual bool visit(KonqFrameContainer* container) {
        kDebug() << m_spaces << container
                     << (container->isHidden() ? "hidden" : "shown")
                     << (container->orientation() == Qt::Horizontal ? "horizontal" : "vertical")
                     << "sizes=" << container->sizes()
                     << "parent=" << container->parentContainer()
                     << "activeChild=" << container->activeChild();

        if (!container->activeChild())
            kDebug() << "WARNING:" << container << "has a null active child!";

        m_spaces += "  ";
        return true;
    }
    virtual bool visit(KonqFrameTabs* tabs) {
        kDebug() << m_spaces << "KonqFrameTabs" << tabs
                     << "visible=" << tabs->isVisible()
                     << "activeChild=" << tabs->activeChild();
        if (!tabs->activeChild())
            kDebug() << "WARNING:" << tabs << "has a null active child!";
        m_spaces += "  ";
        return true;
    }
    virtual bool visit(KonqMainWindow*) { return true; }
    virtual bool endVisit(KonqFrameTabs*) {
        m_spaces.resize( m_spaces.size() - 2 );
        return true;
    }
    virtual bool endVisit(KonqFrameContainer*) {
        m_spaces.resize( m_spaces.size() - 2 );
        return true;
    }
    virtual bool endVisit(KonqMainWindow*) { return true; }
private:
    QString m_spaces;
};

void KonqViewManager::printFullHierarchy()
{
    kDebug() << "currentView=" << m_pMainWindow->currentView();
    KonqDebugFrameVisitor visitor;
    m_pMainWindow->accept( &visitor );
}
#endif

KonqFrameTabs * KonqViewManager::tabContainer()
{
    if ( !m_tabContainer ) {
        createTabContainer(m_pMainWindow /*as widget*/, m_pMainWindow /*as container*/);
        m_pMainWindow->insertChildFrame( m_tabContainer );
    }
    return m_tabContainer;
}

bool KonqViewManager::isTabBarVisible() const
{
    if (!m_tabContainer)
        return false;
    return !m_tabContainer->isTabBarHidden();
}


void KonqViewManager::createTabContainer(QWidget* parent, KonqFrameContainerBase* parentContainer)
{
#ifdef DEBUG_VIEWMGR
    kDebug() << "createTabContainer" << parent << parentContainer;
#endif
    m_tabContainer = new KonqFrameTabs( parent, parentContainer, this );
    // Delay the opening of the URL for #106641
    bool ok = connect( m_tabContainer, SIGNAL(openUrl(KonqView*,KUrl)), m_pMainWindow, SLOT(openUrl(KonqView*,KUrl)), Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok);
    applyConfiguration();
}

void KonqViewManager::applyConfiguration()
{
    tabContainer()->setAlwaysTabbedMode( KonqSettings::alwaysTabbedMode() );
    tabContainer()->setTabsClosable( KonqSettings::permanentCloseButton() );
}

KonqMainWindow* KonqViewManager::duplicateWindow()
{
    KTemporaryFile tempFile;
    tempFile.open();
    KConfig config( tempFile.fileName() );
    KConfigGroup profileGroup( &config, "Profile" );
    KonqFrameBase::Options flags = KonqFrameBase::saveHistoryItems;
    saveViewProfileToGroup(profileGroup, flags);

    KonqMainWindow *mainWindow = openSavedWindow(profileGroup);
#ifndef NDEBUG
    mainWindow->viewManager()->printFullHierarchy();
#endif
    return mainWindow;
}

QString KonqViewManager::normalizedXMLFileName(const QString& xmluiFile)
{
    // Compatibility with pre-kde-4.2 times where there were 2 forks of konqueror.rc
    // Those have been merged back again, so convert to "konqueror.rc".
    if (xmluiFile == "konq-filemanagement.rc" || xmluiFile == "konq-webbrowsing.rc")
        return "konqueror.rc";
    return xmluiFile;
}

#include "konqviewmanager.moc"
