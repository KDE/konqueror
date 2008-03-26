/*
   This file is part of the KDE project
   Copyright (C) 1998-2005 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/


#include "konqview.h"
#include "KonqViewAdaptor.h"
#include "konqsettingsxt.h"
#include "konqframestatusbar.h"
#include "konqrun.h"
#include <konq_events.h>
#include "konqviewmanager.h"
#include "konqtabs.h"
#include "konqbrowseriface.h"
#include <kparts/statusbarextension.h>
#include <kparts/browserextension.h>

#include "konqhistorymanager.h"
#include "konqpixmapprovider.h"

#include <assert.h>
#include <kdebug.h>
#include <kcursor.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <fixx11h.h>
#include <krandom.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <ktoggleaction.h>

#include <QtGui/QApplication>
#include <QtCore/QArgument>
#include <QtCore/QObject>
#include <QtCore/QByteRef>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtCore/QFile>
#include <QtGui/QScrollArea>
#include <kjobuidelegate.h>

//#define DEBUG_HISTORY

KonqView::KonqView( KonqViewFactory &viewFactory,
                    KonqFrame* viewFrame,
                    KonqMainWindow *mainWindow,
                    const KService::Ptr &service,
                    const KService::List &partServiceOffers,
                    const KService::List &appServiceOffers,
                    const QString &serviceType,
                    bool passiveMode
                    )
{
  m_pKonqFrame = viewFrame;
  m_pKonqFrame->setView( this );

  m_sLocationBarURL = "";
  m_pageSecurity = KonqMainWindow::NotCrypted;
  m_bLockHistory = false;
  m_doPost = false;
  m_pMainWindow = mainWindow;
  m_pRun = NULL;
  m_pPart = NULL;

  m_randID = KRandom::random();

  m_service = service;
  m_partServiceOffers = partServiceOffers;
  m_appServiceOffers = appServiceOffers;
  m_serviceType = serviceType;

  m_bAllowHTML = m_pMainWindow->isHTMLAllowed();
  m_lstHistoryIndex = -1;
  m_bLoading = false;
  m_bPendingRedirection = false;
  m_bPassiveMode = passiveMode;
  m_bLockedLocation = false;
  m_bLinkedView = false;
  m_bAborted = false;
  m_bToggleView = false;
  m_bHierarchicalView = false;
  m_bDisableScrolling = false;
  m_bGotIconURL = false;
  m_bPopupMenuEnabled = true;
  m_browserIface = new KonqBrowserInterface( this );
  m_bBackRightClick = KonqSettings::backRightClick();
  m_bFollowActive = false;
  m_bBuiltinView = false;
  m_bURLDropHandling = false;

  switchView( viewFactory );
}

KonqView::~KonqView()
{
  //kDebug(1202) << "KonqView::~KonqView : part = " << m_pPart;

  if (KonqMainWindow::s_crashlog_file) {
     QString part_url;
     if (m_pPart)
        part_url = m_pPart->url().url();
     if (part_url.isNull())
        part_url = "";
     QByteArray line;
     line = ( QString("close(%1):%2\n").arg(m_randID,0,16).arg(part_url) ).toUtf8();
     KonqMainWindow::s_crashlog_file->write(line, line.length());
     KonqMainWindow::s_crashlog_file->flush();
  }

  // We did so ourselves for passive views
  if (m_pPart != 0L)
  {
    finishedWithCurrentURL();
    if ( isPassiveMode() )
      disconnect( m_pPart, SIGNAL( destroyed() ), m_pMainWindow->viewManager(), SLOT( slotObjectDestroyed() ) );

    delete m_pPart;
  }

  setRun( 0L );
  //kDebug(1202) << "KonqView::~KonqView " << this << " done";
}

void KonqView::openUrl( const KUrl &url, const QString & locationBarURL,
                        const QString & nameFilter, bool tempFile )
{
  kDebug(1202) << "KonqView::openUrl url=" << url << " locationBarURL=" << locationBarURL;
  setPartMimeType();

  if (KonqMainWindow::s_crashlog_file) {
     QString part_url;
     if (m_pPart)
        part_url = m_pPart->url().url();
     if (part_url.isNull())
        part_url = "";

     QString url_url = url.url();
     if (url_url.isNull())
        url_url = QString("");

     QByteArray line;

     line = ( QString("closed(%1):%2\n").arg(m_randID,0,16).arg(part_url) ).toUtf8();
     KonqMainWindow::s_crashlog_file->write(line,line.length());
     line = ( QString("opened(%3):%4\n").arg(m_randID,0,16).arg(url_url)  ).toUtf8();
     KonqMainWindow::s_crashlog_file->write(line,line.length());
     KonqMainWindow::s_crashlog_file->flush();
  }

  KParts::OpenUrlArguments args = m_pPart->arguments();
  KParts::BrowserExtension *ext = browserExtension();
  KParts::BrowserArguments browserArgs;
  if ( ext )
    browserArgs = ext->browserArguments();

  // Typing "Enter" again after the URL of an aborted view, triggers a reload.
  if ( m_bAborted && m_pPart && m_pPart->url() == url && !browserArgs.doPost())
  {
    if ( !prepareReload( args, browserArgs, false /* not softReload */ ) )
      return;
    m_pPart->setArguments( args );
  }

#ifdef DEBUG_HISTORY
  kDebug(1202) << "m_bLockedLocation=" << m_bLockedLocation << " browserArgs.lockHistory()=" << browserArgs.lockHistory();
#endif
  if ( browserArgs.lockHistory() )
    lockHistory();

  if ( !m_bLockHistory )
  {
    // Store this new URL in the history, removing any existing forward history.
    // We do this first so that everything is ready if a part calls completed().
    createHistoryEntry();
  } else
    m_bLockHistory = false;

  callExtensionStringMethod( "setNameFilter", nameFilter );
  if ( m_bDisableScrolling )
    callExtensionMethod( "disableScrolling" );

  setLocationBarURL( locationBarURL );
  setPageSecurity(KonqMainWindow::NotCrypted);

  if ( !args.reload() )
  {
    // Save the POST data that is necessary to open this URL
    // (so that reload can re-post it)
    m_doPost = browserArgs.doPost();
    m_postContentType = browserArgs.contentType();
    m_postData = browserArgs.postData;
    // Save the referrer
    m_pageReferrer = args.metaData()["referrer"];
  }

  if ( tempFile ) {
      // Store the path to the tempfile. Yes, we could store a bool only,
      // but this would be more dangerous. If anything goes wrong in the code,
      // we might end up deleting a real file.
      if ( url.isLocalFile() )
          m_tempFile = url.path();
      else
          kWarning(1202) << "Tempfile option is set, but URL is remote: " << url ;
  }

  aboutToOpenURL( url, args );

  m_pPart->openUrl( url );

  updateHistoryEntry(false /* don't save location bar URL yet */);
  // add pending history entry
  KonqHistoryManager::kself()->addPending( url, locationBarURL, QString());

#ifdef DEBUG_HISTORY
  kDebug(1202) << "Current position : " << historyIndex();
#endif
}

void KonqView::switchView( KonqViewFactory &viewFactory )
{
    //kDebug(1202) << "KonqView::switchView";
  if ( m_pPart )
    m_pPart->widget()->hide();

  KParts::ReadOnlyPart *oldPart = m_pPart;
  m_pPart = m_pKonqFrame->attach( viewFactory ); // creates the part

  // Set the statusbar in the BE asap to avoid a KMainWindow statusbar being created.
  KParts::StatusBarExtension* sbext = statusBarExtension();
  if ( sbext )
    sbext->setStatusBar( frame()->statusbar() );

  // Activate the new part
  if ( oldPart )
  {
    m_pPart->setObjectName( oldPart->objectName() );
    emit sigPartChanged( this, oldPart, m_pPart );
    delete oldPart;
  }

  connectPart();

  QVariant prop;

  prop = m_service->property( "X-KDE-BrowserView-FollowActive");
  if (prop.isValid() && prop.toBool())
  {
    //kDebug(1202) << "KonqView::switchView X-KDE-BrowserView-FollowActive -> setFollowActive";
    setFollowActive(true);
  }

  prop = m_service->property( "X-KDE-BrowserView-Built-Into" );
  m_bBuiltinView = (prop.isValid() && prop.toString() == "konqueror");

  if ( !m_pMainWindow->viewManager()->isLoadingProfile() )
  {
    // Honor "non-removeable passive mode" (like the dirtree)
    prop = m_service->property( "X-KDE-BrowserView-PassiveMode");
    if ( prop.isValid() && prop.toBool() )
    {
      kDebug(1202) << "KonqView::switchView X-KDE-BrowserView-PassiveMode -> setPassiveMode";
      setPassiveMode( true ); // set as passive
    }

    // Honor "linked view"
    prop = m_service->property( "X-KDE-BrowserView-LinkedView");
    if ( prop.isValid() && prop.toBool() )
    {
      setLinkedView( true ); // set as linked
      // Two views : link both
      if (m_pMainWindow->viewCount() <= 2) // '1' can happen if this view is not yet in the map
      {
        KonqView * otherView = m_pMainWindow->otherView( this );
        if (otherView)
          otherView->setLinkedView( true );
      }
    }
  }

  prop = m_service->property( "X-KDE-BrowserView-HierarchicalView");
  if ( prop.isValid() && prop.toBool() )
  {
    kDebug() << "KonqView::switchView X-KDE-BrowserView-HierarchicalView -> setHierarchicalView";
    setHierarchicalView( true );  // set as hierarchial
  }
  else
  {
    setHierarchicalView( false );
  }
}

bool KonqView::ensureViewSupports( const QString &mimeType,
                                   bool forceAutoEmbed )
{
    if (supportsMimeType(mimeType))
        return true;
    return changePart(mimeType, QString(), forceAutoEmbed);
}

bool KonqView::changePart(const QString &mimeType,
                          const QString &serviceName,
                          bool forceAutoEmbed)
{
    // Caller should call stop first.
    assert( !m_bLoading );

    //kDebug(1202) << "mimeType=" << mimeType
    //             << "requested serviceName=" << serviceName
    //             << "current service name=" << m_service->desktopEntryName();

    if (serviceName == m_service->desktopEntryName()) {
        m_serviceType = mimeType;
        return true;
    }

    if (isLockedViewMode()) {
        //kDebug(1202) << "This view's mode is locked - can't change";
        return false; // we can't do that if our view mode is locked
    }

    KService::List partServiceOffers, appServiceOffers;
    KService::Ptr service;
    KonqFactory konqFactory;
    KonqViewFactory viewFactory = konqFactory.createView( mimeType, serviceName, &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed );

  if ( viewFactory.isNull() )
  {
    // Revert location bar's URL to the working one
    if(currentHistoryEntry())
      setLocationBarURL( currentHistoryEntry()->locationBarURL );
    return false;
  }

  m_serviceType = mimeType;
  m_partServiceOffers = partServiceOffers;
  m_appServiceOffers = appServiceOffers;

  // Check if that's already the kind of part we have -> no need to recreate it
  // Note: we should have an operator== for KService...
  if ( m_service && m_service->entryPath() == service->entryPath() )
  {
    kDebug( 1202 ) << "KonqView::changeViewMode. Reusing service. Service type set to " << m_serviceType;
    if (  m_pMainWindow->currentView() == this )
      m_pMainWindow->updateViewModeActions();
  }
  else
  {
    m_service = service;

    switchView( viewFactory );
  }

  if ( m_pMainWindow->viewManager()->activePart() != m_pPart )
  {
    // Make the new part active. Note that we don't do it each time we
    // open a URL (becomes awful in view-follows-view mode), but we do
    // each time we change the view mode.
    // We don't do it in switchView either because it's called from the constructor too,
    // where the location bar url isn't set yet.
    //kDebug(1202) << "Giving focus to new part " << m_pPart;
    m_pMainWindow->viewManager()->setActivePart( m_pPart );
  }
  return true;
}

void KonqView::connectPart()
{
  //kDebug(1202) << "KonqView::connectPart";
  connect( m_pPart, SIGNAL( started( KIO::Job * ) ),
           this, SLOT( slotStarted( KIO::Job * ) ) );
  connect( m_pPart, SIGNAL( completed() ),
           this, SLOT( slotCompleted() ) );
  connect( m_pPart, SIGNAL( completed(bool) ),
           this, SLOT( slotCompleted(bool) ) );
  connect( m_pPart, SIGNAL( canceled( const QString & ) ),
           this, SLOT( slotCanceled( const QString & ) ) );
  connect( m_pPart, SIGNAL( setWindowCaption( const QString & ) ),
           this, SLOT( setCaption( const QString & ) ) );
  if (!internalViewMode().isEmpty()) {
      // Update checked action in "View Mode" menu when switching view mode in dolphin
      connect(m_pPart, SIGNAL(viewModeChanged()),
              m_pMainWindow, SLOT(slotInternalViewModeChanged()));
  }

  KParts::BrowserExtension *ext = browserExtension();

  if ( ext )
  {
      ext->setBrowserInterface( m_browserIface );

      connect( ext, SIGNAL( openUrlRequestDelayed(const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &) ),
               m_pMainWindow, SLOT( slotOpenURLRequest( const KUrl &, const KParts::OpenUrlArguments&, const KParts::BrowserArguments & ) ) );

      if ( m_bPopupMenuEnabled )
      {
          m_bPopupMenuEnabled = false; // force
          enablePopupMenu( true );
      }

      connect( ext, SIGNAL( setLocationBarUrl( const QString & ) ),
               this, SLOT( setLocationBarURL( const QString & ) ) );

      connect( ext, SIGNAL( setIconUrl( const KUrl & ) ),
               this, SLOT( setIconURL( const KUrl & ) ) );

      connect( ext, SIGNAL( setPageSecurity( int ) ),
               this, SLOT( setPageSecurity( int ) ) );

      connect( ext, SIGNAL( createNewWindow(const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &, const KParts::WindowArgs &, KParts::ReadOnlyPart**) ),
               m_pMainWindow, SLOT( slotCreateNewWindow( const KUrl &, const KParts::OpenUrlArguments &, const KParts::BrowserArguments &, const KParts::WindowArgs &, KParts::ReadOnlyPart**) ) );

      connect( ext, SIGNAL( loadingProgress( int ) ),
               m_pKonqFrame->statusbar(), SLOT( slotLoadingProgress( int ) ) );

      connect( ext, SIGNAL( speedProgress( int ) ),
               m_pKonqFrame->statusbar(), SLOT( slotSpeedProgress( int ) ) );

      connect( ext, SIGNAL( selectionInfo( const KFileItemList& ) ),
               this, SLOT( slotSelectionInfo( const KFileItemList& ) ) );

      connect( ext, SIGNAL( mouseOverInfo( const KFileItem& ) ),
               this, SLOT( slotMouseOverInfo( const KFileItem& ) ) );

      connect( ext, SIGNAL( openUrlNotify() ),
               this, SLOT( slotOpenURLNotify() ) );

      connect( ext, SIGNAL( enableAction( const char *, bool ) ),
               this, SLOT( slotEnableAction( const char *, bool ) ) );

      connect( ext, SIGNAL( setActionText( const char *, const QString& ) ),
               this, SLOT( slotSetActionText( const char *, const QString& ) ) );

      connect( ext, SIGNAL( moveTopLevelWidget( int, int ) ),
               this, SLOT( slotMoveTopLevelWidget( int, int ) ) );

      connect( ext, SIGNAL( resizeTopLevelWidget( int, int ) ),
               this, SLOT( slotResizeTopLevelWidget( int, int ) ) );

      connect( ext, SIGNAL( requestFocus(KParts::ReadOnlyPart *) ),
               this, SLOT( slotRequestFocus(KParts::ReadOnlyPart *) ) );

      if (service()->desktopEntryName() != "konq_sidebartng") {
          connect( ext, SIGNAL( infoMessage( const QString & ) ),
               m_pKonqFrame->statusbar(), SLOT( message( const QString & ) ) );

          connect( ext,
                   SIGNAL( addWebSideBar(const KUrl&, const QString&) ),
                   m_pMainWindow,
                   SLOT( slotAddWebSideBar(const KUrl&, const QString&) ) );
      }
  }

  QVariant urlDropHandling;

  if ( ext )
      urlDropHandling = ext->property( "urlDropHandling" );
  else
      urlDropHandling = QVariant( true );

  // Handle url drops if
  //  a) either the property says "ok"
  //  or
  //  b) the part is a plain krop (no BE)
  m_bURLDropHandling = ( urlDropHandling.type() == QVariant::Bool &&
                         urlDropHandling.toBool() );

  m_pPart->widget()->installEventFilter( this );

  if (m_bBackRightClick) {
      QAbstractScrollArea* scrollArea = ::qobject_cast<QAbstractScrollArea *>( m_pPart->widget() );
      if ( scrollArea ) {
          scrollArea->viewport()->installEventFilter( this );
      }
  }

#if 0
  // KonqDirPart signal
  if ( ::qobject_cast<KonqDirPart *>(m_pPart) )
  {
      connect( m_pPart, SIGNAL( findOpen( KonqDirPart * ) ),
               m_pMainWindow, SLOT( slotFindOpen( KonqDirPart * ) ) );
  }
#endif
}

void KonqView::slotEnableAction( const char * name, bool enabled )
{
    if ( m_pMainWindow->currentView() == this )
        m_pMainWindow->enableAction( name, enabled );
    // Otherwise, we don't have to do anything, the state of the action is
    // stored inside the browser-extension.
}

void KonqView::slotSetActionText( const char* name, const QString& text )
{
    if ( m_pMainWindow->currentView() == this )
        m_pMainWindow->setActionText( name, text );
    // Otherwise, we don't have to do anything, the state of the action is
    // stored inside the browser-extension.
}

void KonqView::slotMoveTopLevelWidget( int x, int y )
{
  KonqFrameContainerBase* container = frame()->parentContainer();
  // If tabs are shown, only accept to move the whole window if there's only one tab.
  if ( container->frameType() != "Tabs" || static_cast<KonqFrameTabs*>(container)->count() == 1 )
    m_pMainWindow->move( x, y );
}

void KonqView::slotResizeTopLevelWidget( int w, int h )
{
  KonqFrameContainerBase* container = frame()->parentContainer();
  // If tabs are shown, only accept to resize the whole window if there's only one tab.
  // ### Maybe we could store the size requested by each tab and resize the window to the biggest one.
  if ( container->frameType() != "Tabs" || static_cast<KonqFrameTabs*>(container)->count() == 1 )
    m_pMainWindow->resize( w, h );
}

void KonqView::slotStarted( KIO::Job * job )
{
  //kDebug(1202) << "KonqView::slotStarted"  << job;
  setLoading( true );

  if (job)
  {
      // Manage passwords properly...
      kDebug(7035) << "slotStarted: Window ID = " << m_pMainWindow->topLevelWidget()->winId();
      job->ui()->setWindow (m_pMainWindow->topLevelWidget ());

      connect( job, SIGNAL( percent( KJob *, unsigned long ) ), this, SLOT( slotPercent( KJob *, unsigned long ) ) );
      connect( job, SIGNAL( speed( KJob *, unsigned long ) ), this, SLOT( slotSpeed( KJob *, unsigned long ) ) );
      connect( job, SIGNAL( infoMessage( KJob *, const QString &, const QString & ) ), this, SLOT( slotInfoMessage( KJob *, const QString & ) ) );
  }
}

void KonqView::slotRequestFocus( KParts::ReadOnlyPart * )
{
  m_pMainWindow->viewManager()->showTab(this);
}

void KonqView::setLoading( bool loading, bool hasPending /*= false*/)
{
    //kDebug(1202) << "KonqView::setLoading loading=" << loading << " hasPending=" << hasPending;
    m_bLoading = loading;
    m_bPendingRedirection = hasPending;
    if ( m_pMainWindow->currentView() == this )
        m_pMainWindow->updateToolBarActions( hasPending );

    m_pMainWindow->viewManager()->setLoading( this, loading || hasPending );
}

void KonqView::slotPercent( KJob *, unsigned long percent )
{
  m_pKonqFrame->statusbar()->slotLoadingProgress( percent );
}

void KonqView::slotSpeed( KJob *, unsigned long bytesPerSecond )
{
  m_pKonqFrame->statusbar()->slotSpeedProgress( bytesPerSecond );
}

void KonqView::slotInfoMessage( KJob *, const QString &msg )
{
  m_pKonqFrame->statusbar()->message( msg );
}

void KonqView::slotCompleted()
{
   slotCompleted( false );
}

void KonqView::slotCompleted( bool hasPending )
{
  //kDebug(1202) << "KonqView::slotCompleted hasPending=" << hasPending;
  m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );

  if ( ! m_bLockHistory )
  {
      // Success... update history entry, including location bar URL
      updateHistoryEntry( true );

      if ( m_bAborted ) // remove the pending entry on error
          KonqHistoryManager::kself()->removePending( url() );
      else if ( currentHistoryEntry() ) // register as proper history entry
          KonqHistoryManager::kself()->confirmPending(url(), typedUrl(),
						      currentHistoryEntry()->title);

      emit viewCompleted( this );
  }
  setLoading( false, hasPending );

  if (!m_bGotIconURL && !m_bAborted)
  {
    if ( KonqSettings::enableFavicon() == true )
    {
      // Try to get /favicon.ico
      if ( supportsMimeType( "text/html" ) && url().protocol().startsWith( "http" ) )
          KonqPixmapProvider::self()->downloadHostIcon( url().url() );
    }
  }
}

void KonqView::slotCanceled( const QString & errorMsg )
{
  kDebug(1202) << "KonqView::slotCanceled";
  // The errorMsg comes from the ReadOnlyPart's job.
  // It should probably be used in a KMessageBox
  // Let's use the statusbar for now
  m_pKonqFrame->statusbar()->message( errorMsg );
  m_bAborted = true;
  slotCompleted();
}

void KonqView::slotSelectionInfo( const KFileItemList &items )
{
  KonqFileSelectionEvent ev( items, m_pPart );
  QApplication::sendEvent( m_pMainWindow, &ev );
}

void KonqView::slotMouseOverInfo( const KFileItem& item )
{
  KonqFileMouseOverEvent ev( item, m_pPart );
  QApplication::sendEvent( m_pMainWindow, &ev );
}

void KonqView::setLocationBarURL( const KUrl& locationBarURL )
{
  setLocationBarURL( locationBarURL.pathOrUrl() );
}

void KonqView::setLocationBarURL( const QString & locationBarURL )
{
  //kDebug(1202) << "KonqView::setLocationBarURL " << locationBarURL << " this=" << this;

  m_sLocationBarURL = locationBarURL;
  if ( m_pMainWindow->currentView() == this )
  {
    //kDebug(1202) << "is current view " << this;
    m_pMainWindow->setLocationBarURL( m_sLocationBarURL );
    m_pMainWindow->setPageSecurity( m_pageSecurity );
  }
  if (!m_bPassiveMode) setTabIcon( KUrl( m_sLocationBarURL ) );
}

void KonqView::setIconURL( const KUrl & iconURL )
// This function sets the favIcon in konqui's window if enabled,
// thus it is responsible for the icon in the taskbar.
// It does not set the tab's favIcon.
{
  if ( KonqSettings::enableFavicon() )
  {
    KonqPixmapProvider::self()->setIconForUrl( m_sLocationBarURL, iconURL.url() );
    m_bGotIconURL = true;
  }
}

void KonqView::setPageSecurity( int pageSecurity )
{
  m_pageSecurity = (KonqMainWindow::PageSecurity)pageSecurity;

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->setPageSecurity( m_pageSecurity );
}

void KonqView::setTabIcon( const KUrl &url )
{
  if (!m_bPassiveMode) frame()->setTabIcon( url, 0L );
}

void KonqView::setCaption( const QString & caption )
{
  if (caption.isEmpty()) return;

  QString adjustedCaption = caption;
  // For local URLs we prefer to use only the directory name
  if (url().isLocalFile())
  {
     // Is the caption a URL?  If so, is it local?  If so, only display the filename!
     KUrl url(caption);
     if (url.isValid() && url.isLocalFile() && url.fileName() == this->url().fileName())
        adjustedCaption = url.fileName();
  }

  m_caption = adjustedCaption;
  if (!m_bPassiveMode) frame()->setTitle( adjustedCaption , 0L );
}

void KonqView::slotOpenURLNotify()
{
#ifdef DEBUG_HISTORY
  kDebug(1202) << "KonqView::slotOpenURLNotify";
#endif
  updateHistoryEntry(true);
  createHistoryEntry();
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();
}

void KonqView::createHistoryEntry()
{
    // First, remove any forward history
    HistoryEntry * current = currentHistoryEntry();
    if (current)
    {
#ifdef DEBUG_HISTORY
        kDebug(1202) << "Truncating history";
#endif
        while ( current != m_lstHistory.last() )
            delete m_lstHistory.takeLast();
    }
    // Append a new entry
#ifdef DEBUG_HISTORY
    kDebug(1202) << "Append a new entry";
#endif
    m_lstHistory.append( new HistoryEntry );
    setHistoryIndex( m_lstHistory.count()-1 ); // made current
#ifdef DEBUG_HISTORY
    kDebug(1202) << "at=" << historyIndex() << " count=" << m_lstHistory.count();
#endif
}

void KonqView::updateHistoryEntry( bool saveLocationBarURL )
{
  Q_ASSERT( !m_bLockHistory ); // should never happen

  HistoryEntry * current = currentHistoryEntry();
  if ( !current )
    return;

  if ( browserExtension() )
  {
    current->buffer = QByteArray(); // Start with empty buffer.
    QDataStream stream( &current->buffer, QIODevice::WriteOnly );

    browserExtension()->saveState( stream );
  }

#ifdef DEBUG_HISTORY
  kDebug(1202) << "Saving part URL : " << m_pPart->url() << " in history position " << historyIndex();
#endif
  current->url = m_pPart->url();

  if (saveLocationBarURL)
  {
#ifdef DEBUG_HISTORY
    kDebug(1202) << "Saving location bar URL : " << m_sLocationBarURL << " in history position " << historyIndex();
#endif
    current->locationBarURL = m_sLocationBarURL;
    current->pageSecurity = m_pageSecurity;
  }
#ifdef DEBUG_HISTORY
  kDebug(1202) << "Saving title : " << m_caption << " in history position " << historyIndex();
#endif
  current->title = m_caption;
  current->strServiceType = m_serviceType;
  current->strServiceName = m_service->desktopEntryName();

  current->doPost = m_doPost;
  current->postData = m_doPost ? m_postData : QByteArray();
  current->postContentType = m_doPost ? m_postContentType : QString();
  current->pageReferrer = m_pageReferrer;
}

void KonqView::goHistory( int steps )
{
  // This is called by KonqBrowserInterface
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->viewManager()->setActivePart( part() );

  // Delay the go() call (we need to return to the caller first)
  m_pMainWindow->slotGoHistoryActivated( steps );
}

void KonqView::go( int steps )
{
  if ( !steps ) // [WildFox] i bet there are sites on the net with stupid devs who do that :)
  {
#ifdef DEBUG_HISTORY
      kDebug(1202) << "KonqView::go(0)";
#endif
      // [David] and you're right. And they expect that it reloads, apparently.
      // [George] I'm going to make nspluginviewer rely on this too. :-)
      m_pMainWindow->slotReload();
      return;
  }

  int newPos = historyIndex() + steps;
#ifdef DEBUG_HISTORY
  kDebug(1202) << "go : steps=" << steps
               << " newPos=" << newPos
               << " m_lstHistory.count()=" << m_lstHistory.count();
#endif
  if( newPos < 0 || newPos >= m_lstHistory.count() )
    return;

  stop();

  setHistoryIndex( newPos ); // sets current item

#ifdef DEBUG_HISTORY
  kDebug(1202) << "New position " << historyIndex();
#endif

  restoreHistory();
}

void KonqView::restoreHistory()
{
  HistoryEntry h( *currentHistoryEntry() ); // make a copy of the current history entry, as the data
                                          // the pointer points to will change with the following calls

#ifdef DEBUG_HISTORY
  kDebug(1202) << "Restoring servicetype/name, and location bar URL from history : " << h.locationBarURL;
#endif
  setLocationBarURL( h.locationBarURL );
  setPageSecurity( h.pageSecurity );
  m_sTypedURL.clear();

    if (!changePart(h.strServiceType, h.strServiceName)) {
        kWarning(1202) << "Couldn't change view mode to" << h.strServiceType << h.strServiceName;
        return /*false*/;
    }

  setPartMimeType();

  aboutToOpenURL( h.url );

  if ( browserExtension() )
  {
    //kDebug(1202) << "Restoring view from stream";
    QDataStream stream( h.buffer );

    browserExtension()->restoreState( stream );

    m_doPost = h.doPost;
    m_postContentType = h.postContentType;
    m_postData = h.postData;
    m_pageReferrer = h.pageReferrer;
  }
  else
    m_pPart->openUrl( h.url );

  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->updateToolBarActions();

#ifdef DEBUG_HISTORY
  kDebug(1202) << "New position (2) " << historyIndex();
#endif
}

const HistoryEntry * KonqView::historyAt(int pos)
{
    return m_lstHistory.value(pos);
}

void KonqView::copyHistory( KonqView *other )
{
    qDeleteAll( m_lstHistory );
    m_lstHistory.clear();

    foreach ( HistoryEntry* he, other->m_lstHistory )
        m_lstHistory.append( new HistoryEntry( *he ) );
    setHistoryIndex(other->historyIndex());
}

KUrl KonqView::url() const
{
  assert( m_pPart );
  return m_pPart->url();
}

KUrl KonqView::upUrl() const
{
    KUrl currentURL;
    if ( m_pRun )
	currentURL = m_pRun->url();
    else
	currentURL = m_sLocationBarURL;
    return currentURL.upUrl();
}

void KonqView::setRun( KonqRun * run )
{
  if ( m_pRun )
  {
    // Tell the KonqRun to abort, but don't delete it ourselves.
    // It could be showing a message box right now. It will delete itself anyway.
    m_pRun->abort();
    // finish() will be emitted later (when back to event loop)
    // and we don't want it to call slotRunFinished (which stops the animation and stop button).
    m_pRun->disconnect( m_pMainWindow );
    if ( !run )
        frame()->unsetCursor();
  }
  else if ( run )
      frame()->setCursor( Qt::BusyCursor );
  m_pRun = run;
}

void KonqView::stop()
{
  //kDebug(1202) << "KonqView::stop()";
  m_bAborted = false;
  finishedWithCurrentURL();
  if ( m_bLoading || m_bPendingRedirection )
  {
    // aborted -> confirm the pending url. We might as well remove it, but
    // we decided to keep it :)
    KonqHistoryManager::kself()->confirmPending( url(), m_sTypedURL );

    //kDebug(1202) << "m_pPart->closeUrl()";
    m_pPart->closeUrl();
    m_bAborted = true;
    m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );
    setLoading( false, false );
  }
  if ( m_pRun )
  {
    // Revert to working URL - unless the URL was typed manually
    // This is duplicated with KonqMainWindow::slotRunFinished, but we can't call it
    //   since it relies on sender()...
    if ( currentHistoryEntry() && m_pRun->typedUrl().isEmpty() ) { // not typed
      setLocationBarURL( currentHistoryEntry()->locationBarURL );
      setPageSecurity( currentHistoryEntry()->pageSecurity );
    }

    setRun( 0L );
    m_pKonqFrame->statusbar()->slotLoadingProgress( -1 );
  }
  if ( !m_bLockHistory && m_lstHistory.count() > 0 )
    updateHistoryEntry(true);
}

void KonqView::finishedWithCurrentURL()
{
  if ( !m_tempFile.isEmpty() )
  {
    kDebug(1202) << "######### Deleting tempfile after use:" << m_tempFile;
    QFile::remove( m_tempFile );
    m_tempFile.clear();
  }
}

void KonqView::setPassiveMode( bool mode )
{
  // In theory, if m_bPassiveMode is true and mode is false,
  // the part should be removed from the part manager,
  // and if the other way round, it should be readded to the part manager...
  m_bPassiveMode = mode;

  if ( mode && m_pMainWindow->viewCount() > 1 && m_pMainWindow->currentView() == this )
  {
   KParts::Part * part = m_pMainWindow->viewManager()->chooseNextView( this )->part(); // switch active part
   m_pMainWindow->viewManager()->setActivePart( part );
  }

  // Update statusbar stuff
  m_pMainWindow->viewManager()->viewCountChanged();
}

void KonqView::setHierarchicalView( bool mode )
{
 m_bHierarchicalView=mode;
}



void KonqView::setLinkedView( bool mode )
{
  m_bLinkedView = mode;
  if ( m_pMainWindow->currentView() == this )
    m_pMainWindow->linkViewAction()->setChecked( mode );
  frame()->statusbar()->setLinkedView( mode );
}

void KonqView::setLockedLocation( bool b )
{
  m_bLockedLocation = b;
}

void KonqView::aboutToOpenURL( const KUrl &url, const KParts::OpenUrlArguments &args )
{
  KParts::OpenUrlEvent ev( m_pPart, url, args );
  QApplication::sendEvent( m_pMainWindow, &ev );

  m_bGotIconURL = false;
  m_bAborted = false;
}

void KonqView::setPartMimeType()
{
  KParts::OpenUrlArguments args( m_pPart->arguments() );
  args.setMimeType( m_serviceType );
  m_pPart->setArguments( args );
}

QStringList KonqView::frameNames() const
{
  return childFrameNames( m_pPart );
}

QStringList KonqView::childFrameNames( KParts::ReadOnlyPart *part )
{
  QStringList res;

  KParts::BrowserHostExtension *hostExtension = KParts::BrowserHostExtension::childObject( part );

  if ( !hostExtension )
    return res;

  res += hostExtension->frameNames();

  const QList<KParts::ReadOnlyPart*> children = hostExtension->frames();
  QListIterator<KParts::ReadOnlyPart *> i(children);
  while (i.hasNext())
		res += childFrameNames( i.next() );

  return res;
}

KParts::BrowserHostExtension* KonqView::hostExtension( KParts::ReadOnlyPart *part, const QString &name )
{
    KParts::BrowserHostExtension *ext = KParts::BrowserHostExtension::childObject( part );

  if ( !ext )
    return 0;

  if ( ext->frameNames().contains( name ) )
    return ext;

  const QList<KParts::ReadOnlyPart*> children = ext->frames();
  QListIterator<KParts::ReadOnlyPart *> i(children);
  while (i.hasNext())
  {
     KParts::BrowserHostExtension *childHost = hostExtension( i.next(), name);
    if ( childHost )
      return childHost;
  }


  return 0;
}

bool KonqView::callExtensionMethod( const char *methodName )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  return QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection);
}

bool KonqView::callExtensionBoolMethod( const char *methodName, bool value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  return QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection, Q_ARG(bool,value));
}

bool KonqView::callExtensionStringMethod( const char *methodName, const QString &value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  return QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection, Q_ARG(QString, value));
}

bool KonqView::callExtensionURLMethod( const char *methodName, const KUrl& value )
{
  QObject *obj = KParts::BrowserExtension::childObject( m_pPart );
  if ( !obj ) // not all views have a browser extension !
    return false;

  return QMetaObject::invokeMethod( obj, methodName,  Qt::DirectConnection, Q_ARG(KUrl, value));
}

void KonqView::setViewName( const QString &name )
{
    //kDebug() << "KonqView::setViewName this=" << this << " name=" << name;
    if ( m_pPart )
        m_pPart->setObjectName( name );
}

QString KonqView::viewName() const
{
    return m_pPart ? m_pPart->objectName() : QString();
}

void KonqView::enablePopupMenu( bool b )
{
  Q_ASSERT( m_pMainWindow );

  KParts::BrowserExtension *ext = browserExtension();

  if ( !ext )
    return;

  if ( m_bPopupMenuEnabled == b )
      return;

  // enable context popup
  if ( b ) {
    m_bPopupMenuEnabled = true;

    connect( ext, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_pMainWindow, SLOT(slotPopupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );

    connect( ext, SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_pMainWindow, SLOT(slotPopupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );
  }
  else // disable context popup
  {
    m_bPopupMenuEnabled = false;

    disconnect( ext, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_pMainWindow, SLOT(slotPopupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );

    disconnect( ext, SIGNAL(popupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
             m_pMainWindow, SLOT(slotPopupMenu(QPoint,KUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)) );
  }
  enableBackRightClick( m_bBackRightClick );
}

// caller should ensure that this is called only when b changed, or for new parts
void KonqView::enableBackRightClick( bool b )
{
    m_bBackRightClick = b;
    if ( b )
        connect( this, SIGNAL( backRightClick() ),
                 m_pMainWindow, SLOT( slotBack() ) );
    else
        disconnect( this, SIGNAL( backRightClick() ),
                    m_pMainWindow, SLOT( slotBack() ) );
}

void KonqView::reparseConfiguration()
{
    callExtensionMethod( "reparseConfiguration" );
    const bool b = KonqSettings::backRightClick();
    if ( m_bBackRightClick != b ) {
        QAbstractScrollArea* scrollArea = ::qobject_cast<QAbstractScrollArea *>( m_pPart->widget() );
        if (scrollArea) {
            if ( m_bBackRightClick ) {
                scrollArea->viewport()->installEventFilter( this );
            } else {
                scrollArea->viewport()->removeEventFilter( this );
            }
        }
        enableBackRightClick( b );
    }
}

void KonqView::disableScrolling()
{
    m_bDisableScrolling = true;
    callExtensionMethod( "disableScrolling" );
}

QString KonqView::dbusObjectPath()
{
    // TODO maybe this can be improved?
    // E.g. using the part's name, but we'd have to update the name in setViewName maybe?
    // And to make sure it's a valid dbus object path like in kmainwindow...
    static int s_viewNumber = 0;
    if ( m_dbusObjectPath.isEmpty() ) {
        m_dbusObjectPath = m_pMainWindow->dbusName() + '/' + QString::number( ++s_viewNumber );
        new KonqViewAdaptor( this );
        QDBusConnection::sessionBus().registerObject( m_dbusObjectPath, this );
    }
    return m_dbusObjectPath;
}

QString KonqView::partObjectPath()
{
    if ( !m_pPart )
        return QString();

    const QVariant dcopProperty = m_pPart->property( "dbusObjectPath" );
    return dcopProperty.toString();
}

bool KonqView::eventFilter( QObject *obj, QEvent *e )
{
    if ( !m_pPart )
        return false;
//  kDebug() << "--" << obj->className() << "--" << e->type() << "--" ;
    if ( e->type() == QEvent::DragEnter && m_bURLDropHandling && obj == m_pPart->widget() )
    {
        QDragEnterEvent *ev = static_cast<QDragEnterEvent *>( e );

        if ( KUrl::List::canDecode( ev->mimeData() ) )
        {
            KUrl::List lstDragURLs = KUrl::List::fromMimeData( ev->mimeData() );

            QList<QWidget *> children = qFindChildren<QWidget *>( m_pPart->widget() );

            if ( !lstDragURLs.isEmpty()
                 && !lstDragURLs.first().url().startsWith( "javascript:", Qt::CaseInsensitive ) && // ### this looks like a hack to me
                 ev->source() != m_pPart->widget() &&
                 !children.contains( ev->source() ) )
                ev->acceptProposedAction();
        }
    }
    else if ( e->type() == QEvent::Drop && m_bURLDropHandling && obj == m_pPart->widget() )
    {
        QDropEvent *ev = static_cast<QDropEvent *>( e );

        KUrl::List lstDragURLs = KUrl::List::fromMimeData( ev->mimeData() );
        KParts::BrowserExtension *ext = browserExtension();
        if ( !lstDragURLs.isEmpty() && ext && lstDragURLs.first().isValid() )
            emit ext->openUrlRequest( lstDragURLs.first() ); // this will call m_pMainWindow::slotOpenURLRequest delayed
    }

    if ( m_bBackRightClick )
    {
        if ( e->type() == QEvent::ContextMenu )
        {
            QContextMenuEvent *ev = static_cast<QContextMenuEvent *>( e );
            if ( ev->reason() == QContextMenuEvent::Mouse )
            {
                return true;
            }
        }
        else if ( e->type() == QEvent::MouseButtonPress )
        {
            QMouseEvent *ev = static_cast<QMouseEvent *>( e );
            if ( ev->button() == Qt::RightButton )
            {
                return true;
            }
        }
        else if ( e->type() == QEvent::MouseButtonRelease )
        {
            QMouseEvent *ev = static_cast<QMouseEvent *>( e );
            if ( ev->button() == Qt::RightButton )
            {
                emit backRightClick();
                return true;
            }
        }
        else if ( e->type() == QEvent::MouseMove )
        {
            QMouseEvent *ev = static_cast<QMouseEvent *>( e );
            if ( ev->button() == Qt::RightButton )
            {
                obj->removeEventFilter( this );
                QMouseEvent me( QEvent::MouseButtonPress, ev->pos(), Qt::RightButton, Qt::RightButton, Qt::NoModifier );
                QApplication::sendEvent( obj, &me );
                QContextMenuEvent ce( QContextMenuEvent::Mouse, ev->pos(), ev->globalPos() );
                QApplication::sendEvent( obj, &ce );
                obj->installEventFilter( this );
                return true;
            }
        }
    }

    if ( e->type() == QEvent::FocusIn )
    {
        setActiveComponent();
    }
    return false;
}

void KonqView::setActiveComponent()
{
  if ( m_bBuiltinView || !m_pPart->componentData().isValid() /*never!*/)
      KGlobal::setActiveComponent( KGlobal::mainComponent() );
  else
      KGlobal::setActiveComponent( m_pPart->componentData() );
}

bool KonqView::prepareReload( KParts::OpenUrlArguments& args, KParts::BrowserArguments& browserArgs, bool softReload )
{
    args.setReload( true );
    if ( softReload )
        browserArgs.softReload = true;

    // Repost form data if this URL is the result of a POST HTML form.
    if ( m_doPost && !browserArgs.redirectedRequest() )
    {
        if ( KMessageBox::warningContinueCancel( 0, i18n(
            "The page you are trying to view is the result of posted form data. "
            "If you resend the data, any action the form carried out (such as search or online purchase) will be repeated. "),
            i18n( "Warning" ), KGuiItem(i18n( "Resend" )) ) == KMessageBox::Continue )
        {
            browserArgs.setDoPost( true );
            browserArgs.setContentType( m_postContentType );
            browserArgs.postData = m_postData;
        }
        else
            return false;
    }
    // Re-set referrer
    args.metaData()["referrer"] = m_pageReferrer;

    return true;
}

KParts::BrowserExtension * KonqView::browserExtension() const
{
    return KParts::BrowserExtension::childObject( m_pPart );
}

KParts::StatusBarExtension * KonqView::statusBarExtension() const
{
    return KParts::StatusBarExtension::childObject( m_pPart );
}

bool KonqView::supportsMimeType( const QString &mimeType ) const
{
    KMimeType::Ptr mime = KMimeType::mimeType( mimeType );
    if (!mime)
        return false;
    const QStringList lst = serviceTypes();
    for( QStringList::ConstIterator it = lst.begin(); it != lst.end(); ++it ) {
        if ( mime->is(*it) ) // same as mime == *it, but also respect inheritance, mimeType can be a subclass
            return true;
    }
    return false;
}

void HistoryEntry::saveConfig( KConfigGroup& config, const QString &prefix)
{
    config.writeEntry( QString::fromLatin1( "Url" ).prepend( prefix ), url.url() );
    config.writeEntry( QString::fromLatin1( "LocationBarURL" ).prepend( prefix ), locationBarURL );
    config.writeEntry( QString::fromLatin1( "Title" ).prepend( prefix ), title );
    config.writeEntry( QString::fromLatin1( "Buffer" ).prepend( prefix ), buffer );
    config.writeEntry( QString::fromLatin1( "StrServiceType" ).prepend( prefix ), strServiceType );
    config.writeEntry( QString::fromLatin1( "StrServiceName" ).prepend( prefix ), strServiceName );
    config.writeEntry( QString::fromLatin1( "PostData" ).prepend( prefix ), postData );
    config.writeEntry( QString::fromLatin1( "PostContentType" ).prepend( prefix ), postContentType );
    config.writeEntry( QString::fromLatin1( "DoPost" ).prepend( prefix ), doPost );
    config.writeEntry( QString::fromLatin1( "PageReferrer" ).prepend( prefix ), pageReferrer );
    config.writeEntry( QString::fromLatin1( "PageSecurity" ).prepend( prefix ), (int)pageSecurity );
}

void HistoryEntry::loadItem( const KConfigGroup& config, const QString &prefix)
{
    url = KUrl(config.readEntry( QString::fromLatin1( "Url" ).prepend( prefix ), "" ));
    locationBarURL = config.readEntry( QString::fromLatin1( "LocationBarURL" ).prepend( prefix ), "" );
    title = config.readEntry( QString::fromLatin1( "Title" ).prepend( prefix ), "" );
    buffer = config.readEntry( QString::fromLatin1( "Buffer" ).prepend( prefix ), QByteArray() );
    strServiceType = config.readEntry( QString::fromLatin1( "StrServiceType" ).prepend( prefix ), "" );
    strServiceName = config.readEntry( QString::fromLatin1( "StrServiceName" ).prepend( prefix ), "" );
    postData = config.readEntry( QString::fromLatin1( "PostData" ).prepend( prefix ), QByteArray() );
    postContentType = config.readEntry( QString::fromLatin1( "PostContentType" ).prepend( prefix ), "" );
    doPost = config.readEntry( QString::fromLatin1( "DoPost" ).prepend( prefix ), false );
    pageReferrer = config.readEntry( QString::fromLatin1( "PageReferrer" ).prepend( prefix ), "" );
    pageSecurity = (KonqMainWindow::PageSecurity)config.readEntry( QString::fromLatin1( "PageSecurity" ).prepend( prefix ), 0 );
}

void KonqView::saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options)
{
    config.writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), serviceType() );
    config.writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), service()->desktopEntryName() );
    config.writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), isPassiveMode() );
    config.writeEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), isLinkedView() );
    config.writeEntry( QString::fromLatin1( "ToggleView" ).prepend( prefix ), isToggleView() );
    config.writeEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), isLockedLocation() );

    if (options & KonqFrameBase::saveURLs) {
        config.writePathEntry( QString::fromLatin1( "URL" ).prepend( prefix ), url().url() );
    } else if(options & KonqFrameBase::saveHistoryItems) {
        if (m_pPart)
            updateHistoryEntry(true);
        QList<HistoryEntry*>::Iterator it = m_lstHistory.begin();
        for ( uint i = 0; it != m_lstHistory.end(); ++it, ++i ) {
            (*it)->saveConfig(config, QString::fromLatin1( "HistoryItem" ) + QString::number(i).prepend( prefix ));
        }
        config.writeEntry( QString::fromLatin1( "CurrentHistoryItem" ).prepend( prefix ), m_lstHistoryIndex );
        config.writeEntry( QString::fromLatin1( "NumberOfHistoryItems" ).prepend( prefix ), historyLength() );
    }
}

void KonqView::loadHistoryConfig(const KConfigGroup& config, const QString &prefix)
{
    // First, remove any history
    qDeleteAll(m_lstHistory);
    m_lstHistory.clear();

    const int m_lstHistorySize = config.readEntry( QString::fromLatin1( "NumberOfHistoryItems" ).prepend( prefix ), 0 );

    // No history to restore..
    if (m_lstHistorySize == 0) {
        createHistoryEntry();
        return;
    }

    // restore history list
    for ( int i = 0; i < m_lstHistorySize; ++i )
    {
        HistoryEntry* historyEntry = new HistoryEntry;
        historyEntry->loadItem(config, QString::fromLatin1( "HistoryItem" ) + QString::number(i).prepend( prefix ));

        m_lstHistory.append( historyEntry );
    }

    // set and load the correct history index
    setHistoryIndex( config.readEntry( QString::fromLatin1( "CurrentHistoryItem" ).prepend( prefix ), historyLength()-1 ) );
    restoreHistory();
}


QString KonqView::internalViewMode() const
{
    const QVariant viewModeProperty = m_pPart->property("currentViewMode");
    return viewModeProperty.toString();
}

void KonqView::setInternalViewMode(const QString& viewMode)
{
    m_pPart->setProperty("currentViewMode", viewMode);
}

#include "konqview.moc"
