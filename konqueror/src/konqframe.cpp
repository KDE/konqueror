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

// Qt
#include <QKeyEvent>
#include <QApplication>
#include <QtCore/QEvent>
#include <QLabel>
#include <QBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kdebug.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <konq_events.h>
#include <kconfiggroup.h>

KonqFrameBase::KonqFrameBase()
    : m_pParentContainer(0)
{
}

QString KonqFrameBase::frameTypeToString( const KonqFrameBase::FrameType frameType )
{
    switch ( frameType ) {
        case View :
            return QString("View");
        case Tabs :
            return QString("Tabs");
        case ContainerBase :
            return QString("ContainerBase");
        case Container :
            return QString("Container");
        case MainWindow :
            return QString("MainWindow");
    }
    Q_ASSERT(0);
    return QString();
}

KonqFrameBase::FrameType frameTypeFromString( const QString& str )
{
    if ( str == "View" )
        return KonqFrameBase::View;
    if ( str == "Tabs" )
        return KonqFrameBase::Tabs;
    if ( str == "ContainerBase" )
        return KonqFrameBase::ContainerBase;
    if ( str == "Container" )
        return KonqFrameBase::Container;
    if ( str == "MainWindow" )
        return KonqFrameBase::MainWindow;
    Q_ASSERT(0);
    return KonqFrameBase::View;
}

KonqFrame::KonqFrame( QWidget* parent, KonqFrameContainerBase *parentContainer )
    : QWidget ( parent )
{
   //kDebug() << "KonqFrame::KonqFrame()";

   m_pLayout = 0L;
   m_pView = 0L;

   // the frame statusbar
   m_pStatusBar = new KonqFrameStatusBar( this);
   m_pStatusBar->setSizePolicy(QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
   connect(m_pStatusBar, SIGNAL(clicked()), this, SLOT(slotStatusBarClicked()));
   connect( m_pStatusBar, SIGNAL(linkedViewClicked(bool)), this, SLOT(slotLinkedViewClicked(bool)) );
   m_separator = 0;
   m_pParentContainer = parentContainer;
}

KonqFrame::~KonqFrame()
{
    //kDebug() << this;
}

bool KonqFrame::isActivePart()
{
  return ( m_pView &&
           static_cast<KonqView*>(m_pView) == m_pView->mainWindow()->currentView() );
}

void KonqFrame::saveConfig( KConfigGroup& config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase* docContainer, int /*id*/, int /*depth*/ )
{
  if (m_pView) m_pView->saveConfig(config, prefix, options);
  //config.writeEntry( QString::fromLatin1( "ShowStatusBar" ).prepend( prefix ), statusbar()->isVisible() );
  if (this == docContainer) config.writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );

#if 0 // currently unused
  KonqConfigEvent ev( config.config(), prefix+'_', true/*save*/);
  QApplication::sendEvent( childView()->part(), &ev );
#endif
}

void KonqFrame::copyHistory( KonqFrameBase *other )
{
    Q_ASSERT(other->frameType() == KonqFrameBase::View);
    if (m_pView)
        m_pView->copyHistory( static_cast<KonqFrame *>( other )->childView() );
}

 KParts::ReadOnlyPart *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
   KonqViewFactory factory( viewFactory );

   // Note that we set the parent to 0.
   // We don't want that deleting the widget deletes the part automatically
   // because we already have that taken care of in KParts...

   m_pPart = factory.create( this, 0 );

   Q_ASSERT( m_pPart->widget() );

   attachWidget(m_pPart->widget());

   m_pStatusBar->slotConnectToNewView(0, 0, m_pPart);

   return m_pPart;
}

void KonqFrame::attachWidget(QWidget* widget)
{
   //kDebug() << "KonqFrame::attachInternal()";
   delete m_pLayout;

   m_pLayout = new QVBoxLayout( this );
   m_pLayout->setObjectName( QLatin1String( "KonqFrame's QVBoxLayout" ) );
   m_pLayout->setMargin( 0 );
   m_pLayout->setSpacing( 0 );

   m_pLayout->addWidget( widget, 1 );
   m_pLayout->addWidget( m_pStatusBar, 0 );
   widget->show();

   m_pLayout->activate();

   installEventFilter(m_pView->mainWindow()); // for Ctrl+Tab
}

void KonqFrame::insertTopWidget( QWidget * widget )
{
    Q_ASSERT(m_pLayout);
    Q_ASSERT(widget);
    m_pLayout->insertWidget( 0, widget );
    installEventFilter(m_pView->mainWindow()); // for Ctrl+Tab
}

void KonqFrame::setView( KonqView* child )
{
   m_pView = child;
   if (m_pView)
   {
     connect(m_pView,SIGNAL(sigPartChanged(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)),
             m_pStatusBar,SLOT(slotConnectToNewView(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)));
   }
}

void KonqFrame::setTitle( const QString &title , QWidget* /*sender*/)
{
  //kDebug() << "KonqFrame::setTitle( " << title << " )";
  m_title = title;
  if (m_pParentContainer) m_pParentContainer->setTitle( title , this);
}

void KonqFrame::setTabIcon( const KUrl &url, QWidget* /*sender*/ )
{
  //kDebug() << "KonqFrame::setTabIcon( " << url << " )";
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

void KonqFrame::slotRemoveView()
{
   m_pView->mainWindow()->viewManager()->removeView( m_pView );
}

void KonqFrame::activateChild()
{
    if (m_pView && !m_pView->isPassiveMode() ) {
        m_pView->mainWindow()->viewManager()->setActivePart( part() );

        if (!m_pView->isLoading() && (m_pView->url().isEmpty() || m_pView->url() == "about:blank")) {
            //kDebug() << "SET FOCUS on the location bar";
            m_pView->mainWindow()->focusLocationBar(); // #84867 usability improvement
        }
    }
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
