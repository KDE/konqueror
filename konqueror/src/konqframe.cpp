/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

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

// Own
#include "konqframe.h"

// Local
#include "konqtabs.h"
#include "konqview.h"
#include "konqviewmanager.h"
#include "konqframevisitor.h"
#include "konqframestatusbar.h"

// std
#include <assert.h>

// Qt
#include <QtGui/QPainter>
#include <QtGui/QToolButton>
#include <QtGui/QTabBar>
#include <QtGui/QMenu>
#include <QtGui/QKeySequence>
#include <QtGui/QProgressBar>
#include <QtGui/QPixmap>
#include <QtGui/QKeyEvent>
#include <QtCore/QEvent>
#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>

// KDE
#include <konq_pixmapprovider.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kstandardshortcut.h>
#include <konq_events.h>

KonqFrame::KonqFrame( QWidget* parent, KonqFrameContainerBase *parentContainer )
    : QWidget ( parent )
{
   //kDebug(1202) << "KonqFrame::KonqFrame()" << endl;

   m_pLayout = 0L;
   m_pView = 0L;

   // the frame statusbar
   m_pStatusBar = new KonqFrameStatusBar( this);
   m_pStatusBar->setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL( linkedViewClicked( bool ) ), this, SLOT( slotLinkedViewClicked( bool ) ) );
   m_separator = 0;
   m_pParentContainer = parentContainer;
}

KonqFrame::~KonqFrame()
{
   //kDebug(1202) << "KonqFrame::~KonqFrame() " << this << endl;
}

bool KonqFrame::isActivePart()
{
  return ( m_pView &&
           static_cast<KonqView*>(m_pView) == m_pView->mainWindow()->currentView() );
}

void KonqFrame::listViews( ChildViewList *viewList )
{
  viewList->append( childView() );
}

void KonqFrame::saveConfig( KConfigGroup& config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int /*id*/, int /*depth*/ )
{
  if (saveURLs)
    config.writePathEntry( QString::fromLatin1( "URL" ).prepend( prefix ),
                        childView()->url().url() );
  config.writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config.writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->desktopEntryName() );
  config.writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->isPassiveMode() );
  config.writeEntry( QString::fromLatin1( "LinkedView" ).prepend( prefix ), childView()->isLinkedView() );
  config.writeEntry( QString::fromLatin1( "ToggleView" ).prepend( prefix ), childView()->isToggleView() );
  config.writeEntry( QString::fromLatin1( "LockedLocation" ).prepend( prefix ), childView()->isLockedLocation() );
  //config.writeEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), statusbar()->isVisible() );
  if (this == docContainer) config.writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );

  KonqConfigEvent ev( config.config(), prefix+'_', true/*save*/);
  QApplication::sendEvent( childView()->part(), &ev );
}

void KonqFrame::copyHistory( KonqFrameBase *other )
{
    assert( other->frameType() == "View" );
    childView()->copyHistory( static_cast<KonqFrame *>( other )->childView() );
}

 KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   KonqViewFactory factory( viewFactory );

   // Note that we set the parent to 0.
   // We don't want that deleting the widget deletes the part automatically
   // because we already have that taken care of in KParts...

   m_pPart = factory.create( this, 0 );

   assert( m_pPart->widget() );

   attachWidget(m_pPart->widget());

   m_pStatusBar->slotConnectToNewView(0, 0, m_pPart);

   return m_pPart;
}

void KonqFrame::attachWidget(QWidget* widget)
{
   //kDebug(1202) << "KonqFrame::attachInternal()" << endl;
   delete m_pLayout;

   m_pLayout = new QVBoxLayout( this );
   m_pLayout->setObjectName( "KonqFrame's QVBoxLayout" );
   m_pLayout->setMargin( 0 );
   m_pLayout->setSpacing( 0 );

   m_pLayout->addWidget( widget, 1 );
   m_pLayout->addWidget( m_pStatusBar, 0 );
   widget->show();

   m_pLayout->activate();

   widget->installEventFilter(this);
}

bool KonqFrame::eventFilter(QObject* /*obj*/, QEvent *ev)
{
   if (ev->type()==QEvent::KeyPress)
   {
      QKeyEvent * keyEv = static_cast<QKeyEvent*>(ev);
      if ((keyEv->key()==Qt::Key_Tab) && (keyEv->modifiers()==Qt::ControlModifier))
      {
         emit ((KonqFrameContainer*)parent())->ctrlTabPressed();
         return true;
      }
   }
   return false;
}

void KonqFrame::insertTopWidget( QWidget * widget )
{
    assert(m_pLayout);
    assert(widget);
    m_pLayout->insertWidget( 0, widget );
    widget->installEventFilter(this);
}

void KonqFrame::setView( KonqView* child )
{
   m_pView = child;
   if (m_pView)
   {
     connect(m_pView,SIGNAL(sigPartChanged(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)),
             m_pStatusBar,SLOT(slotConnectToNewView(KonqView *, KParts::ReadOnlyPart *,KParts::ReadOnlyPart *)));
   }
}

void KonqFrame::setTitle( const QString &title , QWidget* /*sender*/)
{
  //kDebug(1202) << "KonqFrame::setTitle( " << title << " )" << endl;
  m_title = title;
  if (m_pParentContainer) m_pParentContainer->setTitle( title , this);
}

void KonqFrame::setTabIcon( const KUrl &url, QWidget* /*sender*/ )
{
  //kDebug(1202) << "KonqFrame::setTabIcon( " << url << " )" << endl;
  if (m_pParentContainer) m_pParentContainer->setTabIcon( url, this );
}

void KonqFrame::slotStatusBarClicked()
{
  if ( !isActivePart() && m_pView && !m_pView->isPassiveMode() )
    m_pView->mainWindow()->viewManager()->setActivePart( part() );
}

void KonqFrame::slotLinkedViewClicked( bool mode )
{
  if ( m_pView->mainWindow()->linkableViewsCount() == 2 )
    m_pView->mainWindow()->slotLinkView();
  else
    m_pView->setLinkedView( mode );
}

void
KonqFrame::paintEvent( QPaintEvent* )
{
#ifdef __GNUC__
    #warning "m_pStatusBar->repaint() leads to endless recursion; does anyone know why it's needed?"
#endif
//   m_pStatusBar->repaint();
}

void KonqFrame::slotRemoveView()
{
   m_pView->mainWindow()->viewManager()->removeView( m_pView );
}

void KonqFrame::activateChild()
{
  if (m_pView && !m_pView->isPassiveMode() )
    m_pView->mainWindow()->viewManager()->setActivePart( part() );
}

KonqView* KonqFrame::childView() const
{
  return m_pView;
}

KonqView* KonqFrame::activeChildView() const
{
  return m_pView;
}

bool KonqFrame::accept( KonqFrameVisitor* visitor )
{
    return visitor->visit( this );
}

#include "konqframe.moc"
