/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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

#include <QtCore/QFileInfo>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>

#include <kaccelgen.h>
#include <kactionmenu.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <ktemporaryfile.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kmenu.h>

#include <assert.h>

#include "konqview.h"
#include "konqframe.h"
#include "konqframestatusbar.h"
#include "konqtabs.h"
#include "konqprofiledlg.h"
#include <konq_events.h>
#include "konqsettingsxt.h"
#include "konqframevisitor.h"

//#define DEBUG_VIEWMGR

KonqViewManager::KonqViewManager( KonqMainWindow *mainWindow )
 : KParts::PartManager( mainWindow )
{
  m_pMainWindow = mainWindow;

  m_pamProfiles = 0L;
  m_bProfileListDirty = true;
  m_bLoadingProfile = false;

  m_activePartChangedTimer = new QTimer(this);
  m_activePartChangedTimer->setSingleShot(true);

  m_tabContainer = 0;

  connect(m_activePartChangedTimer, SIGNAL(timeout()), this, SLOT( emitActivePartChanged()));
  connect( this, SIGNAL( activePartChanged ( KParts::Part * ) ),
           this, SLOT( slotActivePartChanged ( KParts::Part * ) ) );
}

KonqView* KonqViewManager::createFirstView( const QString &serviceType, const QString &serviceName )
{
    //kDebug(1202) << "KonqViewManager::createFirstView() " << serviceName << endl;
  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;
  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers, true /*forceAutoEmbed*/ );
  if ( newViewFactory.isNull() )
  {
    kDebug(1202) << "KonqViewManager::createFirstView() No suitable factory found." << endl;
    return 0;
  }

  KonqView* childView = setupView( tabContainer(), newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, false );

  setActivePart( childView->part() );

  m_tabContainer->asQWidget()->show();
  return childView;
}

KonqViewManager::~KonqViewManager()
{
  //kDebug(1202) << "KonqViewManager::~KonqViewManager()" << endl;
  clear();
}

KonqView* KonqViewManager::splitView( KonqView* currentView,
                                      Qt::Orientation orientation,
                                      bool newOneFirst, bool forceAutoEmbed )
{
#ifdef DEBUG_VIEWMGR
  kDebug(1202) << "KonqViewManager::splitView" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
#endif

  KonqFrame* splitFrame = currentView->frame();

  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;
  const QString serviceType = currentView->serviceType();

  KonqViewFactory newViewFactory = createView( serviceType, currentView->service()->desktopEntryName(), service, partServiceOffers, appServiceOffers, forceAutoEmbed );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  assert( splitFrame );

  KonqFrameContainerBase* parentContainer = splitFrame->parentContainer();

  bool moveNewContainer = false;
  QList<int> splitterSizes;
  int index= -1;

  if (parentContainer->frameType()=="Container") {
    moveNewContainer = static_cast<KonqFrameContainer*>(parentContainer)->hasWidgetAfter( splitFrame->asQWidget() );
    splitterSizes = static_cast<KonqFrameContainer*>(parentContainer)->sizes();
  }
  else if (parentContainer->frameType()=="Tabs")
    index = static_cast<KonqFrameTabs*>(parentContainer)->indexOf( splitFrame->asQWidget() );

#ifndef NDEBUG
  //printSizeInfo( splitFrame, parentContainer, "before split");
#endif

  parentContainer->asQWidget()->setUpdatesEnabled( false );

  //kDebug(1202) << "Move out child" << endl;
  QPoint pos = splitFrame->asQWidget()->pos();
  parentContainer->removeChildFrame( splitFrame );
  splitFrame->asQWidget()->setParent( parentContainer->asQWidget()->topLevelWidget() );
  splitFrame->asQWidget()->move( pos );

  //kDebug(1202) << "Create new Container" << endl;
  KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, parentContainer->asQWidget(), parentContainer );
  connect(newContainer, SIGNAL(ctrlTabPressed()), m_pMainWindow, SLOT(slotCtrlTabPressed()));

  parentContainer->insertChildFrame( newContainer, index );
  if ( moveNewContainer ) {
    static_cast<KonqFrameContainer*>(parentContainer)->insertWidget( 0, newContainer );
    static_cast<KonqFrameContainer*>(parentContainer)->swapChildren();
  }

  //kDebug(1202) << "Move in child" << endl;
  splitFrame->asQWidget()->setParent( newContainer );
  splitFrame->asQWidget()->move( pos );
  newContainer->insertChildFrame( splitFrame );

#ifndef NDEBUG
  //printSizeInfo( splitFrame, parentContainer, "after reparent" );
#endif

  //kDebug(1202) << "Create new child" << endl;
  KonqView *newView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, false );

#ifndef DEBUG
  //printSizeInfo( splitFrame, parentContainer, "after child insert" );
#endif

  if ( newOneFirst )
  {
    newContainer->addWidget( splitFrame->asQWidget() );
    newContainer->swapChildren();
  }

  QList<int> newSplitterSizes;
  newSplitterSizes << 50 << 50;
  newContainer->setSizes( newSplitterSizes );

  if (parentContainer->frameType()=="Container") {
    static_cast<KonqFrameContainer*>(parentContainer)->setSizes( splitterSizes );
  }
  else if (parentContainer->frameType()=="Tabs") {
    KonqFrameTabs* pc = static_cast<KonqFrameTabs*>(parentContainer);
    pc->setCurrentIndex( pc->indexOf( newContainer ) );
  }

  splitFrame->show();
  //newView->frame()->show();
  newContainer->show();

  parentContainer->asQWidget()->setUpdatesEnabled( true );

  assert( newView->frame() );
  assert( newView->part() );
  newContainer->setActiveChild( newView->frame() );
  setActivePart( newView->part(), false );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kDebug(1202) << "KonqViewManager::splitView(ServiceType) done" << endl;
#endif

  return newView;
}

KonqView* KonqViewManager::splitMainContainer( KonqView* currentView,
                                               Qt::Orientation orientation,
                                               const QString &serviceType,
                                               const QString &serviceName,
                                               bool newOneFirst )
{
  kDebug(1202) << "KonqViewManager::splitMainContainer()" << endl;

  KUrl url = currentView->url();
  QString locationBarURL = currentView->locationBarURL();

  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;

  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  // Get main frame. Note: this is NOT necessarily m_tabContainer!
  // When having tabs plus a konsole, the main frame is a splitter (KonqFrameContainer).
  KonqFrameBase* mainFrame = m_pMainWindow->childFrame();

  mainFrame->asQWidget()->setUpdatesEnabled( false );

  //kDebug(1202) << "Move out child" << endl;
  QPoint pos = mainFrame->asQWidget()->pos();
  m_pMainWindow->removeChildFrame( mainFrame );

  KonqFrameContainer *newContainer = new KonqFrameContainer( orientation, m_pMainWindow, 0L);
  connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));

  m_pMainWindow->insertChildFrame( newContainer );

  newContainer->insertChildFrame( mainFrame );
  mainFrame->asQWidget()->setParent( newContainer );
  mainFrame->asQWidget()->move( pos );

  KonqView* childView = setupView( newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, true );

  if( newOneFirst ) {
    newContainer->insertWidget( 0, childView->frame() );
    newContainer->swapChildren();
  }

  newContainer->show();
  mainFrame->asQWidget()->show();

  mainFrame->asQWidget()->setUpdatesEnabled( true );

  childView->openUrl( url, locationBarURL );

  newContainer->setActiveChild( mainFrame );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kDebug(1202) << "KonqViewManager::splitMainContainer() done" << endl;
#endif

  return childView;
}

KonqView* KonqViewManager::addTab(const QString &serviceType, const QString &serviceName, bool passiveMode, bool openAfterCurrentPage  )
{
#ifdef DEBUG_VIEWMGR
  kDebug(1202) << "------------- KonqViewManager::addTab starting -------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
#endif

  KService::Ptr service;
  KService::List partServiceOffers, appServiceOffers;

  Q_ASSERT( !serviceType.isEmpty() );

  KonqViewFactory newViewFactory = createView( serviceType, serviceName, service, partServiceOffers, appServiceOffers, true /*forceAutoEmbed*/ );

  if( newViewFactory.isNull() )
    return 0L; //do not split at all if we can't create the new view

  KonqView* childView = setupView( tabContainer(), newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode, openAfterCurrentPage );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kDebug(1202) << "------------- KonqViewManager::addTab done -------------" << endl;
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


void KonqViewManager::duplicateTab( KonqFrameBase* currentFrame, bool openAfterCurrentPage )
{
#ifdef DEBUG_VIEWMGR
  kDebug(1202) << "---------------- KonqViewManager::duplicateTab( " << currentFrame << " ) --------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
#endif

  KTemporaryFile tempFile;
  tempFile.open();
  KConfig config( tempFile.fileName() );
  KConfigGroup profileGroup( &config, "Profile" );

  QString prefix = QString::fromLatin1( currentFrame->frameType() ) + QString::number(0);
  profileGroup.writeEntry( "RootItem", prefix );
  prefix.append( QLatin1Char( '_' ) );
  currentFrame->saveConfig( profileGroup, prefix, true, 0L, 0, 1);

  QString rootItem = profileGroup.readEntry( "RootItem", "empty" );

  if (rootItem.isNull() || rootItem == "empty") return;

  // This flag is used by KonqView, to distinguish manual view creation
  // from profile loading (e.g. in switchView)
  m_bLoadingProfile = true;

  loadItem( profileGroup, tabContainer(), rootItem, KUrl(""), true, openAfterCurrentPage );

  m_bLoadingProfile = false;

  m_pMainWindow->enableAllActions(true);

  // This flag disables calls to viewCountChanged while creating the views,
  // so we do it once at the end :
  m_pMainWindow->viewCountChanged();

  if (openAfterCurrentPage)
    m_tabContainer->setCurrentIndex( m_tabContainer->currentIndex () + 1 );
  else
    m_tabContainer->setCurrentIndex( m_tabContainer->count() - 1 );

  KonqFrameBase* duplicatedFrame = dynamic_cast<KonqFrameBase*>(m_tabContainer->currentWidget());
  if (duplicatedFrame)
    duplicatedFrame->copyHistory( currentFrame );

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kDebug(1202) << "------------- KonqViewManager::duplicateTab done --------------" << endl;
#endif
}

void KonqViewManager::breakOffTab( KonqFrameBase* currentFrame, const QSize& windowSize )
{
#ifdef DEBUG_VIEWMGR
  kDebug(1202) << "---------------- KonqViewManager::breakOffTab( " << currentFrame << " ) --------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
#endif

  KTemporaryFile tempFile;
  tempFile.open();
  KConfig config( tempFile.fileName() );
  KConfigGroup profileGroup( &config, "Profile" );

  QString prefix = QString::fromLatin1( currentFrame->frameType() ) + QString::number(0);
  profileGroup.writeEntry( "RootItem", prefix );
  prefix.append( QLatin1Char( '_' ) );
  currentFrame->saveConfig( profileGroup, prefix, true, 0L, 0, 1);

  KonqMainWindow *mainWindow = new KonqMainWindow;

  mainWindow->viewManager()->loadViewProfile( config, "" );

  KonqFrameTabs * kft = mainWindow->viewManager()->tabContainer();
  KonqFrameBase *newFrame = dynamic_cast<KonqFrameBase*>(kft->currentWidget());
  if(newFrame)
    newFrame->copyHistory( currentFrame );

  removeTab( currentFrame );

  mainWindow->enableAllActions( true );
  mainWindow->resize( windowSize );
  mainWindow->activateChild();
  mainWindow->show();

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );

  mainWindow->dumpViewList();
  mainWindow->viewManager()->printFullHierarchy( mainWindow );

  kDebug(1202) << "------------- KonqViewManager::breakOffTab done --------------" << endl;
#endif
}

void KonqViewManager::removeTab( KonqFrameBase* currentFrame )
{
#ifdef DEBUG_VIEWMGR
  kDebug(1202) << "---------------- KonqViewManager::removeTab( " << currentFrame << " ) --------------" << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
#endif

  if ( m_tabContainer->count() == 1 )
    return;

  if (currentFrame->asQWidget() == m_tabContainer->currentWidget())
    setActivePart( 0L, true );

  m_tabContainer->removeChildFrame(currentFrame);

  QList<KonqView*> viewList;
  currentFrame->listViews( &viewList );

  foreach ( KonqView* view, viewList )
  {
    if (view == m_pMainWindow->currentView())
      setActivePart( 0L, true );
    m_pMainWindow->removeChildView( view );
    delete view;
  }

  delete currentFrame;

  m_tabContainer->slotCurrentChanged(m_tabContainer->currentWidget());

#ifdef DEBUG_VIEWMGR
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
  kDebug(1202) << "------------- KonqViewManager::removeTab done --------------" << endl;
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

void KonqViewManager::removeOtherTabs( KonqFrameBase* currentFrame )
{
  foreach ( KonqFrameBase* frame, m_tabContainer->childFrameList() )
  {
    if ( frame && frame != currentFrame )
      removeTab(frame);
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
  if (m_tabContainer->currentWidget() != view->frame())
  {
    m_tabContainer->setCurrentIndex( m_tabContainer->indexOf( view->frame() ) );
    emitActivePartChanged();
  }
}

void KonqViewManager::updatePixmaps()
{
  QList<KonqView*> viewList;
  tabContainer()->listViews( &viewList );
  foreach ( KonqView* view, viewList )
    view->setTabIcon( KUrl( view->locationBarURL() ) );
}

void KonqViewManager::removeView( KonqView *view )
{
#ifdef DEBUG_VIEWMGR
  kDebug(1202) << "---------------- removeView --------------" << view << endl;
  m_pMainWindow->dumpViewList();
  printFullHierarchy( m_pMainWindow );
#endif

  if (!view)
    return;

  KonqFrame* frame = view->frame();
  KonqFrameContainerBase* parentContainer = frame->parentContainer();

  kDebug(1202) << "view=" << view << " frame=" << frame << " parentContainer=" << parentContainer << endl;

  if (parentContainer->frameType()=="Container")
  {
    kDebug(1202) << "parentContainer is a KonqFrameContainer" << endl;

    KonqFrameContainerBase* grandParentContainer = parentContainer->parentContainer();
    kDebug(1202) << "grandParentContainer=" << grandParentContainer << endl;

    setActivePart( 0L, true );

    int index = -1;
    QList<int> splitterSizes;
    bool moveOtherChild = false;

    if (grandParentContainer->frameType()=="Tabs")
      index = static_cast<KonqFrameTabs*>(grandParentContainer)->indexOf( parentContainer->asQWidget() );
    else if (grandParentContainer->frameType()=="Container")
    {
      moveOtherChild = static_cast<KonqFrameContainer*>(grandParentContainer)->hasWidgetAfter( parentContainer->asQWidget() );
      splitterSizes = static_cast<KonqFrameContainer*>(grandParentContainer)->sizes();
    }

    KonqFrameBase* otherFrame = static_cast<KonqFrameContainer*>(parentContainer)->otherChild( frame );
    kDebug(1202) << "otherFrame=" << otherFrame << endl;

    if( otherFrame == 0L )
    {
        kWarning(1202) << "KonqViewManager::removeView: This shouldn't happen!" << endl;
        return;
    }

    grandParentContainer->asQWidget()->setUpdatesEnabled( false );
    static_cast<KonqFrameContainer*>(parentContainer)->setAboutToBeDeleted();

    QWidget* mainWindow = otherFrame->asQWidget()->topLevelWidget();
    //kDebug(1202) << "--- Reparenting otherFrame to mainWindow " << mainWindow << endl;
    QPoint pos = otherFrame->asQWidget()->pos();
    otherFrame->reparentFrame( mainWindow, pos );

    //kDebug(1202) << "--- Removing otherFrame from parentContainer" << endl;
    parentContainer->removeChildFrame( otherFrame );

    //kDebug(1202) << "--- Removing parentContainer from grandParentContainer" << endl;
    grandParentContainer->removeChildFrame( parentContainer );

    //kDebug(1202) << "--- Removing view from view list" << endl;
    m_pMainWindow->removeChildView(view);
    //kDebug(1202) << "--- Deleting view " << view << endl;
    delete view; // This deletes the view, which deletes the part, which deletes its widget

    //kDebug(1202) << "--- Deleting parentContainer " << parentContainer
    //              << ". Its parent is " << parentContainer->asQWidget()->parent() << endl;
    delete parentContainer;

    //kDebug(1202) << "--- Reparenting otherFrame to grandParentContainer" << grandParentContainer << endl;
    otherFrame->reparentFrame( grandParentContainer->asQWidget(), pos );

    //kDebug(1202) << "--- Inserting otherFrame into grandParentContainer" << grandParentContainer << endl;
    grandParentContainer->insertChildFrame( otherFrame, index );

    if( moveOtherChild )
    {
      static_cast<KonqFrameContainer*>(grandParentContainer)->insertWidget( 0, otherFrame->asQWidget() );
      static_cast<KonqFrameContainer*>(grandParentContainer)->swapChildren();
    }

    if (grandParentContainer->frameType()=="Container")
      static_cast<KonqFrameContainer*>(grandParentContainer)->setSizes( splitterSizes );

    otherFrame->asQWidget()->show();

    grandParentContainer->setActiveChild( otherFrame );
    grandParentContainer->activateChild();

    grandParentContainer->asQWidget()->setUpdatesEnabled( true );
  }
  else if (parentContainer->frameType()=="Tabs") {
    kDebug(1202) << "parentContainer " << parentContainer << " is a KonqFrameTabs" << endl;

    removeTab( frame );
  }
  else if (parentContainer->frameType()=="MainWindow")
    kDebug(1202) << "parentContainer is a KonqMainWindow.  This shouldn't be removeable, not removing." << endl;
  else
    kDebug(1202) << "Unrecognized frame type, not removing." << endl;

#ifdef DEBUG_VIEWMGR
  printFullHierarchy( m_pMainWindow );
  m_pMainWindow->dumpViewList();

  kDebug(1202) << "------------- removeView done --------------" << endl;
#endif
}

// reimplemented from PartManager
void KonqViewManager::removePart( KParts::Part * part )
{
  kDebug(1202) << "KonqViewManager::removePart ( " << part << " )" << endl;
  // This is called when a part auto-deletes itself (case 1), or when
  // the "delete view" above deletes, in turn, the part (case 2)

  kDebug(1202) << "Calling KParts::PartManager::removePart " << part << endl;
  KParts::PartManager::removePart( part );

  // If we were called by PartManager::slotObjectDestroyed, then the inheritance has
  // been deleted already... Can't use inherits().

  KonqView * view = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(part) );
  if ( view ) // the child view still exists, so we are in case 1
  {
      kDebug(1202) << "Found a child view" << endl;
      view->partDeleted(); // tell the child view that the part auto-deletes itself
      if (m_pMainWindow->mainViewsCount() == 1)
      {
        kDebug(1202) << "Deleting last view -> closing the window" << endl;
        clear();
        kDebug(1202) << "Closing m_pMainWindow " << m_pMainWindow << endl;
        m_pMainWindow->close(); // will delete it
        return;
      } else { // normal case
        removeView( view );
      }
  }

  kDebug(1202) << "KonqViewManager::removePart ( " << part << " ) done" << endl;
}

void KonqViewManager::slotPassiveModePartDeleted()
{
  // Passive mode parts aren't registered to the part manager,
  // so we have to handle suicidal ones ourselves
  KParts::ReadOnlyPart * part = const_cast<KParts::ReadOnlyPart *>( static_cast<const KParts::ReadOnlyPart *>( sender() ) );
  disconnect( part, SIGNAL( destroyed() ), this, SLOT( slotPassiveModePartDeleted() ) );
  kDebug(1202) << "KonqViewManager::slotPassiveModePartDeleted part=" << part << endl;
  KonqView * view = m_pMainWindow->childView( part );
  kDebug(1202) << "view=" << view << endl;
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

  KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();
  KonqMainWindow::MapViews::Iterator it = mapViews.begin();
  KonqMainWindow::MapViews::Iterator end = mapViews.end();
  for (  ; it != end ; ++it )
  {
    KonqFrameStatusBar* sb = it.value()->frame()->statusbar();
    sb->showActiveViewIndicator( bShowActiveViewIndicator && !it.value()->isPassiveMode() );
    sb->showLinkedViewIndicator( bShowLinkedViewIndicator && !it.value()->isFollowActive() );
  }
}

void KonqViewManager::clear()
{
    kDebug(1202) << "KonqViewManager::clear" << endl;
    setActivePart( 0L, true /* immediate */ );

    if (m_pMainWindow->childFrame() == 0) return;

    QList<KonqView*> viewList;
    m_pMainWindow->listViews( &viewList );
    if ( !viewList.isEmpty() ) {
        kDebug(1202) << viewList.count() << " items" << endl;

        foreach ( KonqView* view, viewList ) {
            m_pMainWindow->removeChildView( view );
            kDebug(1202) << "Deleting " << view << endl;
            delete view;
        }
    }

    KonqFrameBase* frame = m_pMainWindow->childFrame();
    Q_ASSERT( frame );
    kDebug(1202) << "deleting mainFrame " << endl;
    m_pMainWindow->removeChildFrame( frame ); // will set childFrame() to NULL
    delete frame;
    // tab container was deleted by the above
    m_tabContainer = 0;
}

// TODO move to KonqMainWindow
KonqView *KonqViewManager::chooseNextView( KonqView *view )
{
  //kDebug(1202) << "KonqViewManager(" << this << ")::chooseNextView(" << view << ")" << endl;
  KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();

  KonqMainWindow::MapViews::Iterator it = mapViews.begin();
  KonqMainWindow::MapViews::Iterator end = mapViews.end();
  if ( view ) // find it in the map - can't use the key since view->part() might be 0L
      while ( it != end && it.value() != view )
          ++it;

  // the view should always be in the list
   if ( it == end ) {
     if ( view )
       kWarning() << "View " << view << " is not in list !" << endl;
     it = mapViews.begin();
     if ( it == end )
       return 0L; // We have no view at all - this used to happen with totally-empty-profiles
   }

  KonqMainWindow::MapViews::Iterator startIt = it;

  //kDebug(1202) << "KonqViewManager::chooseNextView: count=" << mapViews.count() << endl;
  while ( true )
  {
    //kDebug(1202) << "*KonqViewManager::chooseNextView going next" << endl;
    if ( ++it == end ) // move to next
      it = mapViews.begin(); // rewind on end

    if ( it == startIt && view )
      break; // no next view found

    KonqView *nextView = it.value();
    if ( nextView && !nextView->isPassiveMode() )
      return nextView;
    //kDebug(1202) << "KonqViewManager::chooseNextView nextView=" << nextView << " passive=" << nextView->isPassiveMode() << endl;
  }

  //kDebug(1202) << "KonqViewManager::chooseNextView: returning 0L" << endl;
  return 0L; // no next view found
}

KonqViewFactory KonqViewManager::createView( const QString &serviceType,
                                             const QString &serviceName,
                                             KService::Ptr &service,
                                             KService::List &partServiceOffers,
                                             KService::List &appServiceOffers,
                                             bool forceAutoEmbed )
{
  kDebug(1202) << "KonqViewManager::createView " << serviceName << endl;
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
                                      bool openAfterCurrentPage )
{
  kDebug(1202) << "KonqViewManager::setupView passiveMode=" << passiveMode << endl;

  QString sType = serviceType;

  if ( sType.isEmpty() ) // TODO remove this -- after checking all callers; splitMainContainer seems to need this logic
    sType = m_pMainWindow->currentView()->serviceType();

  //kDebug(1202) << "KonqViewManager::setupView creating KonqFrame with parent=" << parentContainer << endl;
  KonqFrame* newViewFrame = new KonqFrame( parentContainer->asQWidget(), parentContainer );
  newViewFrame->setGeometry( 0, 0, m_pMainWindow->width(), m_pMainWindow->height() );

  //kDebug(1202) << "Creating KonqView" << endl;
  KonqView *v = new KonqView( viewFactory, newViewFrame,
                              m_pMainWindow, service, partServiceOffers, appServiceOffers, sType, passiveMode );
  //kDebug(1202) << "KonqView created - v=" << v << " v->part()=" << v->part() << endl;

  QObject::connect( v, SIGNAL( sigPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ),
                    m_pMainWindow, SLOT( slotPartChanged( KonqView *, KParts::ReadOnlyPart *, KParts::ReadOnlyPart * ) ) );

  m_pMainWindow->insertChildView( v );


  int index = -1;
  if ( openAfterCurrentPage )
    index = m_tabContainer->currentIndex() +1 ;

  parentContainer->insertChildFrame( newViewFrame, index );

  if (parentContainer->frameType() != "Tabs") newViewFrame->show();

  // Don't register passive views to the part manager
  if ( !v->isPassiveMode() ) // note that KonqView's constructor could set this to true even if passiveMode is false
    addPart( v->part(), false );
  else
  {
    // Passive views aren't registered, but we still want to detect the suicidal ones
    connect( v->part(), SIGNAL( destroyed() ), this, SLOT( slotPassiveModePartDeleted() ) );
  }

  //kDebug(1202) << "KonqViewManager::setupView done" << endl;
  return v;
}

///////////////// Profile stuff ////////////////

void KonqViewManager::saveViewProfile( const QString & fileName, const QString & profileName, bool saveURLs, bool saveWindowSize )
{

  QString path = KStandardDirs::locateLocal( "data", QString::fromLatin1( "konqueror/profiles/" ) +
                                          fileName, KGlobal::mainComponent() );

  if ( QFile::exists( path ) )
    QFile::remove( path );

  KConfig _cfg( path, KConfig::OnlyLocal );
  KConfigGroup cfg(&_cfg, "Profile" );
  if ( !profileName.isEmpty() )
      cfg.writePathEntry( "Name", profileName );

  saveViewProfile( _cfg, saveURLs, saveWindowSize );

}

void KonqViewManager::saveViewProfile( KConfig & cfg, bool saveURLs, bool saveWindowSize )
{
  //kDebug(1202) << "KonqViewManager::saveViewProfile" << endl;
  KConfigGroup profileGroup(&cfg, "Profile");
  if( m_pMainWindow->childFrame() != 0L ) {
    QString prefix = QString::fromLatin1( m_pMainWindow->childFrame()->frameType() )
                     + QString::number(0);
    profileGroup.writeEntry( "RootItem", prefix );
    prefix.append( QLatin1Char( '_' ) );
    m_pMainWindow->saveConfig( profileGroup, prefix, saveURLs, tabContainer(), 0, 1);
  }

  profileGroup.writeEntry( "FullScreen", m_pMainWindow->fullScreenMode());
  profileGroup.writeEntry("XMLUIFile", m_pMainWindow->xmlFile());
  if ( saveWindowSize )
  {
    profileGroup.writeEntry( "Width", m_pMainWindow->width() );
    profileGroup.writeEntry( "Height", m_pMainWindow->height() );
  }

  // Save menu/toolbar settings in profile. Relys on konq_mainwindow calling
  // setAutoSaveSetting( "KonqMainWindow", false ). The false is important,
  // we do not want this call save size settings in the profile, because we
  // do it ourselves. Save in a separate group than the rest of the profile.
  KConfigGroup cg = cfg.group( "Main Window Settings" );
  m_pMainWindow->saveMainWindowSettings( cg );

  cfg.sync();
}

void KonqViewManager::loadViewProfile( const QString & path, const QString & filename,
                                       const KUrl & forcedURL, const KonqOpenURLRequest &req,
                                       bool resetWindow, bool openUrl )
{
  KConfig cfg( path );
  loadViewProfile( cfg, filename, forcedURL, req, resetWindow, openUrl );
}

void KonqViewManager::loadViewProfile( KConfig &cfg, const QString & filename,
                                       const KUrl & forcedURL, const KonqOpenURLRequest &req,
                                       bool resetWindow, bool openUrl )
{
  cfg.setDollarExpansion( true );
  KConfigGroup profileGroup( &cfg, "Profile" );

  if ( tabContainer()->count() > 1 )
  {
      if ( KMessageBox::warningContinueCancel( 0,
                                               i18n("You have multiple tabs open in this window.\n"
                                                    "Loading a view profile will close them."),
                                               i18n("Confirmation"),
                                               KGuiItem(i18n("Load View Profile")),
                                               KStandardGuiItem::cancel(),
                                               "LoadProfileTabsConfirm" ) == KMessageBox::Cancel )
          return;
  }

  KonqView *originalView = m_pMainWindow->currentView();
  bool showTabCalled = false;
  foreach ( KonqFrameBase* frame, m_tabContainer->childFrameList() )
  {
      KonqView *view = frame->activeChildView();
      if (view && view->part() && (view->part()->metaObject()->indexOfProperty("modified") != -1)) {
          QVariant prop = view->part()->property("modified");
          if (prop.isValid() && prop.toBool()) {
              showTab( view );
              showTabCalled = true;
              if ( KMessageBox::warningContinueCancel( 0,
                                                       i18n("This tab contains changes that have not been submitted.\nLoading a profile will discard these changes."),
                                                       i18n("Discard Changes?"), KGuiItem(i18n("&Discard Changes")), KStandardGuiItem::cancel(), "discardchangesloadprofile") != KMessageBox::Continue )
                  /* WE: maybe KStandardGuiItem(Discard) here? */
              {
                  showTab( originalView );
                  return;
              }
          }
      }
  }
  if ( showTabCalled && originalView )
      showTab( originalView );

  m_currentProfile = filename;
  m_currentProfileText = profileGroup.readPathEntry("Name",filename);
  m_profileHomeURL = profileGroup.readEntry("HomeURL", QString());

  m_pMainWindow->currentProfileChanged();
  KUrl defaultURL;
  if ( m_pMainWindow->currentView() )
    defaultURL = m_pMainWindow->currentView()->url();

  clear();

  QString rootItem = profileGroup.readEntry( "RootItem", "empty" );

  //kDebug(1202) << "KonqViewManager::loadViewProfile : loading RootItem " << rootItem <<
  //" forcedURL " << forcedURL.url() << endl;

  if ( forcedURL.url() != "about:blank" )
  {
    // This flag is used by KonqView, to distinguish manual view creation
    // from profile loading (e.g. in switchView)
    m_bLoadingProfile = true;

    loadItem( profileGroup, m_pMainWindow, rootItem, defaultURL, openUrl && forcedURL.isEmpty() );

    m_bLoadingProfile = false;

    m_pMainWindow->enableAllActions(true);

    // This flag disables calls to viewCountChanged while creating the views,
    // so we do it once at the end :
    m_pMainWindow->viewCountChanged();
  }
  else
  {
    m_pMainWindow->disableActionsNoView();
    m_pMainWindow->action( "clear_location" )->trigger();
  }

  //kDebug(1202) << "KonqViewManager::loadViewProfile : after loadItem " << endl;

  // Set an active part first so that we open the URL in the current view
  // (to set the location bar correctly and asap)
  KonqView *nextChildView = 0L;
  nextChildView = m_pMainWindow->activeChildView();
  if (nextChildView == 0L) nextChildView = chooseNextView( 0L );
  setActivePart( nextChildView ? nextChildView->part() : 0L, true /* immediate */);

  // #71164
  if ( !req.args.frameName.isEmpty() && nextChildView ) {
      nextChildView->setViewName( req.args.frameName );
  }

  if ( openUrl && !forcedURL.isEmpty())
  {
      KonqOpenURLRequest _req(req);
      _req.openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
      _req.forceAutoEmbed = true; // it's a new window, let's use it

      m_pMainWindow->openUrl( nextChildView /* can be 0 for an empty profile */,
                              forcedURL, _req.args.serviceType, _req, _req.args.trustedSource );

      // TODO choose a linked view if any (instead of just the first one),
      // then open the same URL in any non-linked one
  }
  else
  {
      if ( m_pMainWindow->locationBarURL().isEmpty() ) // No URL -> the user will want to type one
          m_pMainWindow->focusLocationBar();
  }

  // Window size
  if ( !m_pMainWindow->initialGeometrySet() )
  {
     if (profileGroup.readEntry( "FullScreen",false ))
     {
         // Full screen on
         m_pMainWindow->showFullScreen();
     }
     else
     {
         // Full screen off
         if( m_pMainWindow->isFullScreen())
             m_pMainWindow->showNormal();

         QSize size = readConfigSize( profileGroup, m_pMainWindow );
         if ( size.isValid() )
             m_pMainWindow->resize( size );
         else  // no size in the profile; use last known size
             m_pMainWindow->restoreWindowSize();
     }
  }

  if( resetWindow )
  { // force default settings for the GUI
     m_pMainWindow->applyMainWindowSettings( KConfigGroup( KGlobal::config(), "KonqMainWindow" ), true );
  }

  // Apply menu/toolbar settings saved in profile. Read from a separate group
  // so that the window doesn't try to change the size stored in the Profile group.
  // (If applyMainWindowSettings finds a "Width" or "Height" entry, it
  // sets them to 0,0)
  m_pMainWindow->applyMainWindowSettings( KConfigGroup(&cfg, "Main Window Settings") );

#ifdef DEBUG_VIEWMGR
  printFullHierarchy( m_pMainWindow );
#endif

  //kDebug(1202) << "KonqViewManager::loadViewProfile done" << endl;
}

void KonqViewManager::setActivePart( KParts::Part *part, QWidget * )
{
    setActivePart( part, false );
}

void KonqViewManager::setActivePart( KParts::Part *part, bool immediate )
{
    //kDebug(1202) << "KonqViewManager::setActivePart " << part << endl;
    //if ( part )
    //    kDebug(1202) << "    " << part->className() << " " << part->name() << endl;

    // Due to the single-shot timer below, we need to also make sure that
    // the mainwindow also has the right part active already
    KParts::Part* mainWindowActivePart = (m_pMainWindow && m_pMainWindow->currentView())
                                         ? m_pMainWindow->currentView()->part() : 0;
    if (part == activePart() && (!immediate || mainWindowActivePart == part))
    {
      if ( part )
        kDebug(1202) << "Part is already active!" << endl;
      return;
    }

    // Don't activate when part changed in non-active tab
    KonqView* partView = m_pMainWindow->childView((KParts::ReadOnlyPart*)part);
    if (partView)
    {
      KonqFrameContainerBase* parentContainer = partView->frame()->parentContainer();
      if (parentContainer->frameType()=="Tabs")
      {
        KonqFrameTabs* parentFrameTabs = static_cast<KonqFrameTabs*>(parentContainer);
        if (partView->frame() != parentFrameTabs->currentWidget())
           return;
      }
    }

    if (m_pMainWindow && m_pMainWindow->currentView())
      m_pMainWindow->currentView()->setLocationBarURL(m_pMainWindow->locationBarURL());

    KParts::PartManager::setActivePart( part );

    if (part && part->widget())
        part->widget()->setFocus();

    if (!immediate && reason() != ReasonRightClick) {
        // We use a 0s single shot timer so that when left-clicking on a part,
        // we process the mouse event before rebuilding the GUI.
        // Otherwise, when e.g. dragging icons, the mouse pointer can already
        // be very far from where it was...
        // TODO: use a QTimer member var, so that if two conflicting calls to
        // setActivePart(part,immediate=false) happen, the 1st one gets canceled.
        m_activePartChangedTimer->start( 0 );
        // This is not done with right-clicking so that the part is activated before the
        // popup appears (#75201)
    } else {
        emitActivePartChanged();
    }
}

void KonqViewManager::slotActivePartChanged ( KParts::Part *newPart )
{
    //kDebug(1202) << "KonqViewManager::slotActivePartChanged " << newPart << endl;
    if (newPart == 0L) {
      //kDebug(1202) << "newPart = 0L , returning" << endl;
      return;
    }
    KonqView * view = m_pMainWindow->childView( static_cast<KParts::ReadOnlyPart *>(newPart) );
    if (view == 0L) {
      kDebug(1202) << k_funcinfo << "No view associated with this part" << endl;
      return;
    }
    if (view->frame()->parentContainer() == 0L) return;
    if (!m_bLoadingProfile)  {
        view->frame()->statusbar()->updateActiveStatus();
        view->frame()->parentContainer()->setActiveChild( view->frame() );
    }
    //kDebug(1202) << "KonqViewManager::slotActivePartChanged done" << endl;
}

void KonqViewManager::emitActivePartChanged()
{
    m_pMainWindow->slotPartActivated( activePart() );
}


QSize KonqViewManager::readConfigSize( KConfigGroup &cfg, QWidget *widget )
{
    bool ok;

    QString widthStr = cfg.readEntry( "Width" );
    QString heightStr = cfg.readEntry( "Height" );

    int width = -1;
    int height = -1;

    QRect geom = KGlobalSettings::desktopGeometry(widget);

    if ( widthStr.endsWith( '%' ) )
    {
        widthStr.truncate( widthStr.length() - 1 );
        int relativeWidth = widthStr.toInt( &ok );
        if ( ok ) {
            width = relativeWidth * geom.width() / 100;
        }
    }
    else
    {
        width = widthStr.toInt( &ok );
        if ( !ok )
            width = -1;
    }

    if ( heightStr.endsWith( '%' ) )
    {
        heightStr.truncate( heightStr.length() - 1 );
        int relativeHeight = heightStr.toInt( &ok );
        if ( ok ) {
            height = relativeHeight * geom.height() / 100;
        }
    }
    else
    {
        height = heightStr.toInt( &ok );
        if ( !ok )
            height = -1;
    }

    return QSize( width, height );
}

void KonqViewManager::loadItem( KConfigGroup &cfg, KonqFrameContainerBase *parent,
                                const QString &name, const KUrl & defaultURL, bool openUrl, bool openAfterCurrentPage )
{
  QString prefix;
  if( name != "InitialView" )
    prefix = name + QLatin1Char( '_' );

  //kDebug(1202) << "KonqViewManager::loadItem: begin name " << name << " openUrl " << openUrl << endl;

  if( name.startsWith("View") || name == "empty" ) {
    //load view config
    QString serviceType;
    QString serviceName;
    if ( name == "empty" ) {
        // An empty profile is an empty KHTML part. Makes all KHTML actions available, avoids crashes,
        // makes it easy to DND a URL onto it, and makes it fast to load a website from there.
        serviceType = "text/html";
        serviceName = "html";
    } else {
        serviceType = cfg.readEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), QString("inode/directory"));
        serviceName = cfg.readEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), QString() );
    }
    kDebug(1202) << "KonqViewManager::loadItem: ServiceType " << serviceType << " " << serviceName << endl;

    KService::Ptr service;
    KService::List partServiceOffers, appServiceOffers;

    KonqFactory konqFactory;
    KonqViewFactory viewFactory = konqFactory.createView( serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers, true /*forceAutoEmbed*/ );
    if ( viewFactory.isNull() )
    {
      kWarning(1202) << "Profile Loading Error: View creation failed" << endl;
      return; //ugh..
    }

    bool passiveMode = cfg.readEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), false );

    //kDebug(1202) << "KonqViewManager::loadItem: Creating View Stuff; parent=" << parent << endl;
    if ( parent == m_pMainWindow )
        parent = tabContainer();
    KonqView *childView = setupView( parent, viewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode, openAfterCurrentPage );

    if (!childView->isFollowActive()) childView->setLinkedView( cfg.readEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), false ) );
    childView->setToggleView( cfg.readEntry( QString::fromLatin1( "ToggleView" ).prepend( prefix ), false ) );
    if( !cfg.readEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), true ) )
      childView->frame()->statusbar()->hide();

#if 0 // dead code
    if (!m_pDocContainer)
    {
      if (parent->frameType() == "MainWindow")
        m_pDocContainer = childView->frame(); // Child view of mainWindow

      else if (parent->frameType() == "Container")
      {
        KonqFrameContainer* parentContainer = static_cast<KonqFrameContainer*>(parent);
        KonqFrameBase* otherFrame = parentContainer->otherChild( childView->frame() );
        if (otherFrame)
        {
          if (childView->isPassiveMode())
          {
            if (otherFrame->frameType() == "View")
            {
              KonqFrame* viewFrame = static_cast<KonqFrame*>(otherFrame);
              if (viewFrame->childView()->isPassiveMode())
                m_pDocContainer = parentContainer; // Both views are passive, shouldn't happen
              else
                m_pDocContainer = viewFrame; // This one is passive, the other is active
            }
          }
          else
          {
            if (otherFrame->frameType() == "View")
            {
              KonqFrame* viewFrame = static_cast<KonqFrame*>(otherFrame);
              if (viewFrame->childView()->isPassiveMode())
                m_pDocContainer = childView->frame(); // This one is active, the other is passive
              else
                m_pDocContainer = parentContainer; // Both views are active
            }
            else
              m_pDocContainer = parentContainer; // This one is active, the other is a Container
          }
        }
      }
    }
#endif // dead code

    KonqConfigEvent ev( cfg.config(), prefix+'_', false/*load*/);
    QApplication::sendEvent( childView->part(), &ev );

    childView->frame()->show();

    QString key = QString::fromLatin1( "URL" ).prepend( prefix );
    if ( openUrl )
    {
      KUrl url;

      //kDebug(1202) << "KonqViewManager::loadItem: key " << key << endl;
      if ( cfg.hasKey( key ) )
      {
        QString u = cfg.readPathEntry( key );
        if ( u.isEmpty() )
          u = QString::fromLatin1("about:blank");
        url = u;
      }
      else if(key == "empty_URL")
        url = QString::fromLatin1("about:blank");
      else
        url = defaultURL;

      if ( !url.isEmpty() )
      {
        //kDebug(1202) << "KonqViewManager::loadItem: calling openUrl " << url.prettyUrl() << endl;
        //childView->openUrl( url, url.prettyUrl() );
        // We need view-follows-view (for the dirtree, for instance)
        KonqOpenURLRequest req;
        if (url.protocol() != "about")
          req.typedUrl = url.prettyUrl();
        m_pMainWindow->openView( serviceType, url, childView, req );
      }
      //else kDebug(1202) << "KonqViewManager::loadItem: url is empty" << endl;
    }
    // Do this after opening the URL, so that it's actually possible to open it :)
    childView->setLockedLocation( cfg.readEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), false ) );
  }
  else if( name.startsWith("Container") ) {
    //kDebug(1202) << "KonqViewManager::loadItem Item is Container" << endl;

    //load container config
    QString ostr = cfg.readEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), QString() );
    //kDebug(1202) << "KonqViewManager::loadItem Orientation: " << ostr << endl;
    Qt::Orientation o;
    if( ostr == "Vertical" )
      o = Qt::Vertical;
    else if( ostr == "Horizontal" )
      o = Qt::Horizontal;
    else {
      kWarning() << "Profile Loading Error: No orientation specified in " << name << endl;
      o = Qt::Horizontal;
    }

    QList<int> sizes =
        cfg.readEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ),QList<int>());

    int index = cfg.readEntry( QString::fromLatin1( "activeChildIndex" ).prepend(prefix), -1 );

    QStringList childList = cfg.readEntry( QString::fromLatin1( "Children" ).prepend( prefix ),QStringList() );
    if( childList.count() < 2 )
    {
      kWarning() << "Profile Loading Error: Less than two children in " << name << endl;
      // fallback to defaults
      loadItem( cfg, parent, "InitialView", defaultURL, openUrl );
    }
    else
    {
      KonqFrameContainer *newContainer = new KonqFrameContainer( o, parent->asQWidget(), parent );
      connect(newContainer,SIGNAL(ctrlTabPressed()),m_pMainWindow,SLOT(slotCtrlTabPressed()));

      int tabindex = -1;
      if(openAfterCurrentPage && parent->frameType() == "Tabs") // Need to honor it, if possible
	tabindex = static_cast<KonqFrameTabs*>(parent)->currentIndex() + 1;
      parent->insertChildFrame( newContainer, tabindex );

      loadItem( cfg, newContainer, childList.at(0), defaultURL, openUrl );
      loadItem( cfg, newContainer, childList.at(1), defaultURL, openUrl );

      //kDebug(1202) << "loadItem: setSizes " << sizes << endl;
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
    //kDebug(1202) << "KonqViewManager::loadItem Item is a Tabs" << endl;

    int index = cfg.readEntry( QString::fromLatin1( "activeChildIndex" ).prepend(prefix), 0 );

    QStringList childList = cfg.readEntry( QString::fromLatin1( "Children" ).prepend( prefix ),QStringList() );
    for ( QStringList::Iterator it = childList.begin(); it != childList.end(); ++it )
    {
        loadItem( cfg, m_tabContainer, *it, defaultURL, openUrl );
        QWidget* currentPage = m_tabContainer->currentWidget();
        if (currentPage != 0L) {
          KonqView* activeChildView = dynamic_cast<KonqFrameBase*>(currentPage)->activeChildView();
          if (activeChildView != 0L) {
            activeChildView->setCaption( activeChildView->caption() );
            activeChildView->setTabIcon( activeChildView->url() );
          }
        }
    }

    m_tabContainer->setActiveChild( dynamic_cast<KonqFrameBase*>(m_tabContainer->widget(index)) );
    m_tabContainer->setCurrentIndex( index );
    m_tabContainer->show();
  }
  else
      kWarning() << "Profile Loading Error: Unknown item " << name;

  //kDebug(1202) << "KonqViewManager::loadItem: end " << name << endl;
}

void KonqViewManager::setProfiles( KActionMenu *profiles )
{
  m_pamProfiles = profiles;

  if ( m_pamProfiles )
  {
    connect( m_pamProfiles->menu(), SIGNAL( activated( int ) ),
             this, SLOT( slotProfileActivated( int ) ) ); connect( m_pamProfiles->menu(), SIGNAL( aboutToShow() ),
             this, SLOT( slotProfileListAboutToShow() ) );
  }
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
  //kDebug(1202) << "KonqViewManager::profileListDirty()" << endl;
  if ( !broadcast )
  {
    m_bProfileListDirty = true;
#if 0
  // There's always one profile at least, now...
  QStringList profiles = KonqFactory::componentData().dirs()->findAllResources( "data", "konqueror/profiles/*", false, true );
  if ( m_pamProfiles )
      m_pamProfiles->setEnabled( profiles.count() > 0 );
#endif
    return;
  }

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "updateAllProfileList");
    QDBusConnection::sessionBus().send(message);

}

void KonqViewManager::slotProfileActivated( int id )
{

  QMap<QString, QString>::ConstIterator iter = m_mapProfileNames.begin();
  QMap<QString, QString>::ConstIterator end = m_mapProfileNames.end();

  for(int i=0; iter != end; ++iter, ++i) {
    if( i == id ) {
      KUrl u;
      u.setPath( *iter );
      loadViewProfile( *iter, u.fileName() );
      break;
    }
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
  QStringList accel_strings;
  KAccelGen::generateFromKeys(m_mapProfileNames, accel_strings);

  // Store menu items
  QList<QString>::iterator iter = accel_strings.begin();
  for( int id = 0;
       iter != accel_strings.end();
       ++iter, ++id ) {
      popup->insertItem( *iter, id );
  }

  m_bProfileListDirty = false;
}

void KonqViewManager::setLoading( KonqView *view, bool loading )
{
  KonqFrameContainerBase* parentContainer = view->frame()->parentContainer();
  if ( parentContainer->frameType() == "Tabs" ) {
    QColor color;
    KonqFrameTabs* konqframetabs = static_cast<KonqFrameTabs*>( parentContainer );
    if ( loading )
      color = QColor( (KGlobalSettings::linkColor().red()  + KGlobalSettings::inactiveTextColor().red())/2,
                      (KGlobalSettings::linkColor().green()+ KGlobalSettings::inactiveTextColor().green())/2,
                      (KGlobalSettings::linkColor().blue() + KGlobalSettings::inactiveTextColor().blue())/2 );
    else
    {
      if ( konqframetabs->currentWidget() != view->frame() )
        color = KGlobalSettings::linkColor();
      else
        color = KGlobalSettings::textColor();
    }
    konqframetabs->setTabTextColor( konqframetabs->indexOf( view->frame() ), color );
  }
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

    if ( parent->frameType() == "Container" ) {
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
        kDebug(1202) << m_spaces << "KonqFrame " << frame
                     << " visible=" << frame->isVisible()
                     << " containing view " << frame->childView()
                     << " and part " << frame->part()
                     << " whose widget is a " << className << endl;
        return true;
    }
    virtual bool visit(KonqFrameContainer* container) {
        kDebug(1202) << m_spaces << "KonqFrameContainer " << container
                     << " visible=" << container->isVisible()
                     << " activeChild=" << container->activeChild() << endl;
        if (!container->activeChild())
            kDebug(1202) << "WARNING: " << container << " has a null active child!" << endl;

        m_spaces += "  ";
        return true;
    }
    virtual bool visit(KonqFrameTabs* tabs) {
        kDebug(1202) << m_spaces << "KonqFrameTabs " << tabs << " visible="
                     << tabs->isVisible()
                     << " activeChild=" << tabs->activeChild() << endl;
        if (!tabs->activeChild())
            kDebug(1202) << "WARNING: " << tabs << " has a null active child!" << endl;
        m_spaces += "  ";
        return true;
    }
    virtual bool endVisit(KonqFrameTabs*) {
        m_spaces.resize( m_spaces.size() - 2 );
        return true;
    }
    virtual bool endVisit(KonqFrameContainer*) {
        m_spaces.resize( m_spaces.size() - 2 );
        return true;
    }
private:
    QString m_spaces;
};

void KonqViewManager::printFullHierarchy( KonqFrameContainerBase * container )
{
    kDebug(1202) << "currentView=" << m_pMainWindow->currentView() << endl;

    KonqDebugFrameVisitor visitor;

    if (container)
        container->accept( &visitor );
    else
        m_pMainWindow->accept( &visitor );
}
#endif

KonqFrameTabs * KonqViewManager::tabContainer()
{
    if ( !m_tabContainer ) {
        m_tabContainer = new KonqFrameTabs( m_pMainWindow /*as widget*/, m_pMainWindow /*as container*/, this );

        connect( m_tabContainer, SIGNAL(ctrlTabPressed()), m_pMainWindow, SLOT(slotCtrlTabPressed()) );
        m_tabContainer->setAlwaysTabbedMode( KonqSettings::alwaysTabbedMode() );
        m_pMainWindow->insertChildFrame( m_tabContainer );
    }
    return m_tabContainer;
}

#include "konqviewmanager.moc"
