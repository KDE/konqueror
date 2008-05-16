/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers <konq-e@kde.org>
    Copyright (C) 2002-2003 Douglas Hanley <douglash@caltech.edu>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/

#include "konqtabs.h"

#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QPushButton>
#include <QtGui/QKeyEvent>

#include <kapplication.h>
#include <kcolorscheme.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstringhandler.h>

#include "konqview.h"
#include "konqviewmanager.h"
#include "konqmisc.h"
#include "konqsettingsxt.h"
#include "konqframevisitor.h"
#include "konqproxystyle.h"

#include <kacceleratormanager.h>
#include <konqpixmapprovider.h>
#include <kstandardshortcut.h>
#include <QtGui/QTabBar>

#include <QtGui/QStyle>

#define DUPLICATE_ID 3
#define RELOAD_ID 4
#define BREAKOFF_ID 5
#define CLOSETAB_ID 6
#define OTHERTABS_ID 7

//###################################################################

class KonqTabsStyle : public KonqProxyStyle
{
public:
  KonqTabsStyle(QWidget *parent) : KonqProxyStyle(parent) {}
 
  void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
  QRect subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const;
};

void KonqTabsStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                  QPainter *painter, const QWidget *widget) const
{
  if (element == PE_FrameTabWidget)
  {
    const KonqFrameTabs *tw = static_cast<const KonqFrameTabs*>(widget);
    const QTabBar *tb = tw->tabBar();

    QStyleOptionTabV2 tab;
    tab.initFrom(tb);
    tab.shape = tb->shape();
    int overlap = style()->pixelMetric(PM_TabBarBaseOverlap, &tab, tb);

    if (overlap <= 0 || tw->isTabBarHidden())
      return;

    QStyleOptionTabBarBase opt;
    opt.initFrom(tb);

    opt.selectedTabRect = tb->tabRect(tb->currentIndex());
    opt.selectedTabRect = QRect(tb->mapToParent(opt.selectedTabRect.topLeft()), opt.selectedTabRect.size());
    opt.tabBarRect = QRect(tb->mapToParent(tb->rect().topLeft()), tb->size());

    if (tw->tabPosition() == QTabWidget::North)
      opt.rect = QRect(option->rect.left(), option->rect.top(), option->rect.width(), overlap);
    else
      opt.rect = QRect(option->rect.left(), option->rect.bottom() - overlap - 1, option->rect.width(), overlap);

    style()->drawPrimitive(PE_FrameTabBarBase, &opt, painter, tb);
    return;
  }

  KonqProxyStyle::drawPrimitive(element, option, painter, widget);
}

QRect KonqTabsStyle::subElementRect(SubElement element, const QStyleOption *option, const QWidget *widget) const
{
  if (element == SE_TabWidgetTabContents)
  {
    QRect rect = style()->subElementRect(SE_TabWidgetTabPane, option, widget);

    const KonqFrameTabs *tw = static_cast<const KonqFrameTabs*>(widget);
    const QTabBar *tb = tw->tabBar();

    QStyleOptionTabV2 tab;
    tab.initFrom(tb);
    tab.shape = tb->shape();
    int overlap = style()->pixelMetric(PM_TabBarBaseOverlap, &tab, tw->tabBar());

    if (overlap <= 0 || tw->isTabBarHidden())
      return rect;

    return tw->tabPosition() == QTabWidget::North ?
        rect.adjusted(0, overlap, 0, 0) : rect.adjusted(0, 0, 0, -overlap);
  }

  return KonqProxyStyle::subElementRect(element, option, widget);
}


//###################################################################


KonqFrameTabs::KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer,
                             KonqViewManager* viewManager)
  : KTabWidget(parent),
    m_pPopupMenu(0),
    m_pSubPopupMenuTab(0),
    m_rightWidget(0), m_leftWidget(0), m_alwaysTabBar(false),
    m_closeOtherTabsId(0)
{
  // Set an object name so the widget style can identify this widget.
  setObjectName("kde_konq_tabwidget");

  KAcceleratorManager::setNoAccel(this);

  // Set the widget style to a forwarding proxy style that removes the tabwidget frame,
  // and draws a tabbar base underneath the tabbar.
  setStyle(new KonqTabsStyle(this));

  // The base will be drawn on the frame instead of on the tabbar, so it extends across
  // the whole widget.
  tabBar()->setDrawBase(false);

  tabBar()->setWhatsThis(i18n( "This bar contains the list of currently open tabs. Click on a tab to make it "
			  "active. The option to show a close button instead of the website icon in the left "
			  "corner of the tab is configurable. You can also use keyboard shortcuts to "
			  "navigate through tabs. The text on the tab is the title of the website "
			  "currently open in it, put your mouse over the tab too see the full title in "
			  "case it was truncated to fit the tab size." ) );
  //kDebug(1202) << "KonqFrameTabs::KonqFrameTabs()";

  m_pParentContainer = parentContainer;
  m_pActiveChild = 0L;
  m_pViewManager = viewManager;

  connect( this, SIGNAL( currentChanged ( QWidget * ) ),
           this, SLOT( slotCurrentChanged( QWidget* ) ) );

  m_MouseMiddleClickClosesTab = KonqSettings::mouseMiddleClickClosesTab();

  m_permanentCloseButtons = KonqSettings::permanentCloseButton();
  if (m_permanentCloseButtons) {
    setHoverCloseButton( true );
    setHoverCloseButtonDelayed( false );
  }
  else
    setHoverCloseButton( KonqSettings::hoverCloseButton() );
  setTabCloseActivatePrevious( KonqSettings::tabCloseActivatePrevious() );
  if (KonqSettings::tabPosition()=="Bottom")
    setTabPosition(QTabWidget::Bottom);
  connect( this, SIGNAL( closeRequest( QWidget * )), SLOT(slotCloseRequest( QWidget * )));
  connect( this, SIGNAL( removeTabPopup() ),
           m_pViewManager->mainWindow(), SLOT( slotRemoveTabPopup() ) );

  if ( KonqSettings::addTabButton() ) {
    m_leftWidget = new QPushButton( this );
    m_leftWidget->setFlat(true);
    connect( m_leftWidget, SIGNAL( clicked() ),
             m_pViewManager->mainWindow(), SLOT( slotAddTab() ) );
    m_leftWidget->setIcon( KIcon( "tab-new" ) );
    m_leftWidget->adjustSize();
    m_leftWidget->setToolTip( i18n("Open a new tab"));
    setCornerWidget( m_leftWidget, Qt::TopLeftCorner );
  }
  if ( KonqSettings::closeTabButton() ) {
    m_rightWidget = new QPushButton( this );
    m_rightWidget->setFlat(true);
    connect( m_rightWidget, SIGNAL( clicked() ),
             m_pViewManager->mainWindow(), SLOT( slotRemoveTab() ) );
    m_rightWidget->setIcon( KIcon( "tab-close" ) );
    m_rightWidget->adjustSize();
    m_rightWidget->setToolTip( i18n("Close the current tab"));
    setCornerWidget( m_rightWidget, Qt::TopRightCorner );
  }

  setAutomaticResizeTabs( true );
  setTabReorderingEnabled( true );
  connect( this, SIGNAL( movedTab( int, int ) ),
           SLOT( slotMovedTab( int, int ) ) );
  connect( this, SIGNAL( mouseMiddleClick() ),
           SLOT( slotMouseMiddleClick() ) );
  connect( this, SIGNAL( mouseMiddleClick( QWidget * ) ),
           SLOT( slotMouseMiddleClick( QWidget * ) ) );
  connect( this, SIGNAL( mouseDoubleClick() ),
           m_pViewManager->mainWindow(), SLOT( slotAddTab() ) );

  connect( this, SIGNAL( testCanDecode(const QDragMoveEvent *, bool & )),
           SLOT( slotTestCanDecode(const QDragMoveEvent *, bool & ) ) );
  connect( this, SIGNAL( receivedDropEvent( QDropEvent * )),
           SLOT( slotReceivedDropEvent( QDropEvent * ) ) );
  connect( this, SIGNAL( receivedDropEvent( QWidget *, QDropEvent * )),
           SLOT( slotReceivedDropEvent( QWidget *, QDropEvent * ) ) );
  connect( this, SIGNAL( initiateDrag( QWidget * )),
           SLOT( slotInitiateDrag( QWidget * ) ) );


  initPopupMenu();
}

KonqFrameTabs::~KonqFrameTabs()
{
  //kDebug(1202) << "KonqFrameTabs::~KonqFrameTabs() " << this << " - " << className();
  qDeleteAll( m_childFrameList );
  m_childFrameList.clear();
}

void KonqFrameTabs::saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options,
                                KonqFrameBase* docContainer, int id, int depth )
{
  //write children
  QStringList strlst;
  int i = 0;
  QString newPrefix;
  foreach (KonqFrameBase* frame, m_childFrameList)
    {
      newPrefix = QString::fromLatin1( frame->frameType() ) + 'T' + QString::number(i);
      strlst.append( newPrefix );
      newPrefix.append( QLatin1Char( '_' ) );
      frame->saveConfig( config, newPrefix, options, docContainer, id, depth + i );
      i++;
    }

  config.writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  config.writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ),
                      currentIndex() );
}

void KonqFrameTabs::copyHistory( KonqFrameBase *other )
{

  if( !other ) {
    kDebug(1202) << "The Frame does not exist";
    return;
  }

  if( other->frameType() != "Tabs" ) {
    kDebug(1202) << "Frame types are not the same";
    return;
  }

  for (int i = 0; i < m_childFrameList.count(); i++ )
    {
      m_childFrameList.at(i)->copyHistory( static_cast<KonqFrameTabs *>( other )->m_childFrameList.at(i) );
    }
}

void KonqFrameTabs::setTitle( const QString &title , QWidget* sender)
{
  // kDebug(1202) << "KonqFrameTabs::setTitle( " << title << " , " << sender << " )";
  setTabText( indexOf( sender ), title );
}

void KonqFrameTabs::setTabIcon( const KUrl &url, QWidget* sender )
{
  //kDebug(1202) << "KonqFrameTabs::setTabIcon( " << url << " , " << sender << " )";
  KIcon iconSet;
  if (m_permanentCloseButtons)
    iconSet = KIcon( "window-close" );
  else
    iconSet = KIcon( KonqPixmapProvider::self()->iconNameFor( url ) );
  const int pos = indexOf(sender);
  if (tabIcon(pos).pixmap().serialNumber() != iconSet.pixmap().serialNumber())
    KTabWidget::setTabIcon( pos, iconSet );
}

void KonqFrameTabs::activateChild()
{
    if (m_pActiveChild) {
        setCurrentIndex( indexOf( m_pActiveChild->asQWidget() ) );
        m_pActiveChild->activateChild();
    }
}

void KonqFrameTabs::insertChildFrame( KonqFrameBase* frame, int index )
{
    //kDebug(1202) << "KonqFrameTabs " << this << ": insertChildFrame " << frame;

    if (!frame) {
        kWarning(1202) << "KonqFrameTabs " << this << ": insertChildFrame(0) !" ;
        return;
    }

    //kDebug(1202) << "Adding frame";

    //QTabWidget docs say that inserting tabs while already shown causes
    //flicker...
    setUpdatesEnabled(false);

    insertTab(index, frame->asQWidget(), "");

    frame->setParentContainer(this);
    if (index == -1) {
        m_childFrameList.append(frame);
    } else {
        m_childFrameList.insert(index, frame);
    }

    if (m_rightWidget) {
      m_rightWidget->setEnabled( m_childFrameList.count() > 1 );
    }

    if (KonqView* activeChildView = frame->activeChildView()) {
      activeChildView->setCaption( activeChildView->caption() );
      activeChildView->setTabIcon( activeChildView->url() );
    }

    updateTabBarVisibility();
    setUpdatesEnabled(true);
}

void KonqFrameTabs::childFrameRemoved( KonqFrameBase * frame )
{
  //kDebug(1202) << "KonqFrameTabs::RemoveChildFrame " << this << ". Child " << frame << " removed";
  if (frame) {
    removeTab(indexOf(frame->asQWidget()));
    m_childFrameList.removeAll(frame);
    if (m_rightWidget)
      m_rightWidget->setEnabled( m_childFrameList.count()>1 );
    if (count() == 1)
      updateTabBarVisibility();
  }
  else
    kWarning(1202) << "KonqFrameTabs " << this << ": childFrameRemoved(0L) !" ;

  //kDebug(1202) << "KonqFrameTabs::RemoveChildFrame finished";
}

void KonqFrameTabs::moveTabBackward( int index )
{
  if ( index == 0 )
    return;
  moveTab( index, index-1 );
}

void KonqFrameTabs::moveTabForward( int index )
{
  if ( index == count()-1 )
    return;
  moveTab( index, index+1 );
}

void KonqFrameTabs::slotMovedTab( int from, int to )
{
  KonqFrameBase* fromFrame = m_childFrameList.at( from );
  m_childFrameList.removeAll( fromFrame );
  m_childFrameList.insert( to, fromFrame );

  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>( currentWidget() );
  if ( currentFrame && !m_pViewManager->isLoadingProfile() ) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::slotContextMenu( const QPoint &p )
{
  refreshSubPopupMenuTab();
  m_pPopupMenu->setItemEnabled( RELOAD_ID, false );
  m_pPopupMenu->setItemEnabled( DUPLICATE_ID, false );
  m_pPopupMenu->setItemEnabled( BREAKOFF_ID, false );
  m_pPopupMenu->setItemEnabled( CLOSETAB_ID, false );
  m_pPopupMenu->setItemEnabled( OTHERTABS_ID, true );
  m_pSubPopupMenuTab->setItemEnabled( m_closeOtherTabsId, false );

  m_pPopupMenu->exec( p );
}

void KonqFrameTabs::slotContextMenu( QWidget *w, const QPoint &p )
{
  refreshSubPopupMenuTab();
  uint tabCount = m_childFrameList.count();
  m_pPopupMenu->setItemEnabled( RELOAD_ID, true );
  m_pPopupMenu->setItemEnabled( DUPLICATE_ID, true );
  m_pPopupMenu->setItemEnabled( BREAKOFF_ID, tabCount>1 );
  m_pPopupMenu->setItemEnabled( CLOSETAB_ID, tabCount>1 );
  m_pPopupMenu->setItemEnabled( OTHERTABS_ID, tabCount>1 );
  m_pSubPopupMenuTab->setItemEnabled( m_closeOtherTabsId, true );

  // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a
  // class hierarchy and it could crash one day, but I haven't checked
  // setWorkingTab so I don't know if it can handle nulls.

  m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
  m_pPopupMenu->exec( p );
}

void KonqFrameTabs::refreshSubPopupMenuTab()
{
    m_pSubPopupMenuTab->clear();
    int i=0;
    m_pSubPopupMenuTab->insertItem( KIcon( "reload_all_tabs" ),
                                    i18n( "&Reload All Tabs" ),
                                    m_pViewManager->mainWindow(),
                                    SLOT( slotReloadAllTabs() ),
                                    m_pViewManager->mainWindow()->action("reload_all_tabs")->shortcut() );
    m_pSubPopupMenuTab->addSeparator();
    foreach (KonqFrameBase* frameBase, m_childFrameList)
    {
        KonqFrame* frame = dynamic_cast<KonqFrame *>(frameBase);
        if ( frame && frame->activeChildView() )
        {
            QString title = frame->title().trimmed();
            const QString url = frame->activeChildView()->url().url();
            if ( title.isEmpty() )
                title = url;
            title = KStringHandler::csqueeze( title, 50 );
            m_pSubPopupMenuTab->insertItem( QIcon( KonqPixmapProvider::self()->pixmapFor( url ) ), title, i );
        }
        i++;
    }
    m_pSubPopupMenuTab->addSeparator();
    m_closeOtherTabsId =
      m_pSubPopupMenuTab->insertItem( KIcon( "tab-close-other" ),
				      i18n( "Close &Other Tabs" ),
				      m_pViewManager->mainWindow(),
				      SLOT( slotRemoveOtherTabsPopup() ),
				      m_pViewManager->mainWindow()->action("removeothertabs")->shortcut() );
}

void KonqFrameTabs::slotCloseRequest( QWidget *w )
{
  if ( m_childFrameList.count() > 1 ) {
    // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a class hierarchy and it could crash one day, but I haven't checked setWorkingTab so I don't know if it can handle nulls.
    m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
    emit removeTabPopup();
  }
}

void KonqFrameTabs::slotSubPopupMenuTabActivated( int _id)
{
    setCurrentIndex( _id );
}

void KonqFrameTabs::slotMouseMiddleClick()
{
  KUrl filteredURL ( KonqMisc::konqFilteredURL( this, QApplication::clipboard()->text(QClipboard::Selection) ) );
  if ( !filteredURL.isEmpty() ) {
    KonqView* newView = m_pViewManager->addTab("text/html", QString(), false, false);
    if (newView == 0L) return;
    m_pViewManager->mainWindow()->openUrl( newView, filteredURL, QString() );
    m_pViewManager->showTab( newView );
    m_pViewManager->mainWindow()->focusLocationBar();
  }
}

void KonqFrameTabs::slotMouseMiddleClick( QWidget *w )
{
  if ( m_MouseMiddleClickClosesTab ) {
    if ( m_childFrameList.count() > 1 ) {
      // Yes, I know this is an unchecked dynamic_cast - I'm casting sideways in a class hierarchy and it could crash one day, but I haven't checked setWorkingTab so I don't know if it can handle nulls.
      m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
      emit removeTabPopup();
    }
  }
  else {
  KUrl filteredURL ( KonqMisc::konqFilteredURL( this, QApplication::clipboard()->text(QClipboard::Selection ) ) );
  if ( !filteredURL.isEmpty() ) {
    KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(w);
    if (frame) {
      m_pViewManager->mainWindow()->openUrl( frame->activeChildView(), filteredURL );
    }
  }
  }
}

void KonqFrameTabs::slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */)
{
  accept = KUrl::List::canDecode( e->mimeData() );
}

void KonqFrameTabs::slotReceivedDropEvent( QDropEvent *e )
{
  const KUrl::List lstDragURLs = KUrl::List::fromMimeData( e->mimeData() );
  if ( !lstDragURLs.isEmpty() ) {
    KonqView* newView = m_pViewManager->addTab("text/html", QString(), false, false);
    if (newView == 0L) return;
    m_pViewManager->mainWindow()->openUrl( newView, lstDragURLs.first(), QString() );
    m_pViewManager->showTab( newView );
    m_pViewManager->mainWindow()->focusLocationBar();
  }
}

void KonqFrameTabs::slotReceivedDropEvent( QWidget *w, QDropEvent *e )
{
  KUrl::List lstDragURLs = KUrl::List::fromMimeData( e->mimeData() );
  KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>(w);
  if ( lstDragURLs.count() && frame ) {
    KUrl lstDragURL = lstDragURLs.first();
    if ( lstDragURL != frame->activeChildView()->url() )
      m_pViewManager->mainWindow()->openUrl( frame->activeChildView(), lstDragURL );
  }
}

void KonqFrameTabs::slotInitiateDrag( QWidget *w )
{
  KonqFrameBase* frame = dynamic_cast<KonqFrameBase*>( w );
  if (frame) {
    QDrag *d = new QDrag( this );
    QMimeData* md = new QMimeData();
    frame->activeChildView()->url().populateMimeData(md);
    d->setMimeData( md );
    QString iconName = KMimeType::iconNameForUrl(frame->activeChildView()->url());
    d->setPixmap(KIconLoader::global()->loadIcon(iconName, KIconLoader::Small, 0));
    //d->setPixmap( KIconLoader::global()->loadMimeTypeIcon(KMimeType::pixmapForURL( frame->activeChildView()->url(), 0), KIconLoader::Small ) );
    d->start();
  }
}

void KonqFrameTabs::updateTabBarVisibility()
{
    if ( m_alwaysTabBar ) {
        setTabBarHidden(false);
    } else {
        setTabBarHidden(count() <= 1);
    }
}

void KonqFrameTabs::setAlwaysTabbedMode( bool enable )
{
    const bool update = ( enable != m_alwaysTabBar );
    m_alwaysTabBar = enable;
    if ( update ) {
        updateTabBarVisibility();
    }
}

void KonqFrameTabs::initPopupMenu()
{
  m_pPopupMenu = new QMenu( this );
  m_pPopupMenu->insertItem( KIcon( "tab-new" ),
                            i18n("&New Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotAddTab() ),
                            m_pViewManager->mainWindow()->action("newtab")->shortcut() );
  m_pPopupMenu->insertItem( KIcon( "tab-duplicate" ),
                            i18n("&Duplicate Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotDuplicateTabPopup() ),
                            m_pViewManager->mainWindow()->action("duplicatecurrenttab")->shortcut(),
                            DUPLICATE_ID );
  m_pPopupMenu->insertItem( KIcon( "view-refresh" ),
                            i18n( "&Reload Tab" ),
                            m_pViewManager->mainWindow(),
                            SLOT( slotReloadPopup() ),
                            m_pViewManager->mainWindow()->action("reload")->shortcut(), RELOAD_ID );
  m_pPopupMenu->addSeparator();
  m_pSubPopupMenuTab = new QMenu( this );
  m_pPopupMenu->insertItem( i18n("Other Tabs" ), m_pSubPopupMenuTab, OTHERTABS_ID );
  connect( m_pSubPopupMenuTab, SIGNAL( activated ( int ) ),
           this, SLOT( slotSubPopupMenuTabActivated( int ) ) );
  m_pPopupMenu->addSeparator();
  m_pPopupMenu->insertItem( KIcon( "tab-detach" ),
                            i18n("D&etach Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotBreakOffTabPopup() ),
                            m_pViewManager->mainWindow()->action("breakoffcurrenttab")->shortcut(),
                            BREAKOFF_ID );
  m_pPopupMenu->addSeparator();
  m_pPopupMenu->insertItem( KIcon( "tab-close" ),
                            i18n("&Close Tab"),
                            m_pViewManager->mainWindow(),
                            SLOT( slotRemoveTabPopup() ),
                            m_pViewManager->mainWindow()->action("removecurrenttab")->shortcut(),
                            CLOSETAB_ID );
  connect( this, SIGNAL( contextMenu( QWidget *, const QPoint & ) ),
           SLOT(slotContextMenu( QWidget *, const QPoint & ) ) );
  connect( this, SIGNAL( contextMenu( const QPoint & ) ),
           SLOT(slotContextMenu( const QPoint & ) ) );

}

bool KonqFrameTabs::accept( KonqFrameVisitor* visitor )
{
    if ( !visitor->visit( this ) )
        return false;
    foreach( KonqFrameBase* frame, m_childFrameList ) {
        Q_ASSERT( frame );
        if ( !frame->accept( visitor ) )
            return false;
    }
    if ( !visitor->endVisit( this ) )
        return false;
    return true;
}

void KonqFrameTabs::slotCurrentChanged( QWidget* newPage )
{
    const KColorScheme colorScheme(QPalette::Active, KColorScheme::Window);
    setTabTextColor(indexOf(newPage), colorScheme.foreground(KColorScheme::NormalText));

    KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>(newPage);
    if (currentFrame && !m_pViewManager->isLoadingProfile()) {
        m_pActiveChild = currentFrame;
        currentFrame->activateChild();
    }
}

#if 0
/**
 * Returns the index position of the tab that contains (directly or indirectly) the frame @p frame,
 * or -1 if the frame is not in the tab widget.
 */
int KonqFrameTabs::tabContaining(KonqFrameBase* frame) const
{
    KonqFrameBase* frameBase = frame;
    while (frameBase && frameBase->parentContainer() != this)
        frameBase = frameBase->parentContainer();
    if (frameBase)
        return indexOf(frameBase->asQWidget());
    else
        return -1;
}
#endif

int KonqFrameTabs::tabWhereActive(KonqFrameBase* frame) const
{
    for (int i = 0; i < m_childFrameList.count(); i++ ) {
        KonqFrameBase* f = m_childFrameList.at(i);
        while (f && f != frame) {
            f = f->isContainer() ? static_cast<KonqFrameContainerBase *>(f)->activeChild() : 0;
        }
        if (f == frame)
            return i;
    }
    return -1;
}

void KonqFrameTabs::setLoading(KonqFrameBase* frame, bool loading)
{
    const int pos = tabWhereActive(frame);
    if (pos == -1)
        return;

    const KColorScheme colorScheme(QPalette::Active, KColorScheme::Window);
    QColor color;
    if (loading) {
        color = colorScheme.foreground(KColorScheme::NeutralText); // a tab is currently loading
    } else {
        if (currentIndex() != pos) {
            // another tab has newly loaded contents. Use "link" because you can click on it to read it.
            color = colorScheme.foreground(KColorScheme::LinkText);
        } else {
            // the current tab has finished loading.
            color = colorScheme.foreground(KColorScheme::NormalText);
        }
    }
    setTabTextColor(pos, color);
}

void KonqFrameTabs::replaceChildFrame(KonqFrameBase* oldFrame, KonqFrameBase* newFrame)
{
    const int index = indexOf(oldFrame->asQWidget());
    childFrameRemoved(oldFrame);
    insertChildFrame(newFrame, index);
    setCurrentIndex(index);
}

#include "konqtabs.moc"
