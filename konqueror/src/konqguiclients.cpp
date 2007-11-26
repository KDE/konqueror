/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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
#include "konqguiclients.h"

// KDE
#include <ktoggleaction.h>
#include <kdebug.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kactioncollection.h>
#include <kservicetypetrader.h>

// Local
#include "konqview.h"
#include "konqsettingsxt.h"
#include "konqframe.h"
#include "konqframevisitor.h"
#include "konqframestatusbar.h"
#include "konqviewmanager.h"

PopupMenuGUIClient::PopupMenuGUIClient( const KService::List &embeddingServices,
                                        KParts::BrowserExtension::ActionGroupMap& actionGroups,
                                        QAction* showMenuBar, QAction* stopFullScreen )
    : m_actionCollection(this),
      m_embeddingServices(embeddingServices)
{
    QList<QAction *> topActions;
    if (showMenuBar) {
        topActions.append(showMenuBar);
        QAction* separator = new QAction(&m_actionCollection);
        separator->setSeparator(true);
        topActions.append(separator);
    }

    if (stopFullScreen) {
        topActions.append(stopFullScreen);
        QAction* separator = new QAction(&m_actionCollection);
        separator->setSeparator(true);
        topActions.append(separator);
    }

    if (!embeddingServices.isEmpty()) {
        QList<QAction *> previewActions;
        if (embeddingServices.count() == 1) {
            KService::Ptr service = embeddingServices.first();
            QAction* act = addEmbeddingService( 0, i18n( "Preview in %1", service->name() ), service );
            previewActions.append(act);
        } else if (embeddingServices.count() > 1) {
            KService::List::ConstIterator it = embeddingServices.begin();
            const KService::List::ConstIterator end = embeddingServices.end();
            int idx = 0;
            for (; it != end; ++it, ++idx ) {
                QAction* act = addEmbeddingService( idx, (*it)->name(), *it );
                previewActions.append(act);
            }
        }
        actionGroups.insert("preview", previewActions);
    }
    actionGroups.insert("topactions", topActions);
}

PopupMenuGUIClient::~PopupMenuGUIClient()
{
}

QAction* PopupMenuGUIClient::addEmbeddingService( int idx, const QString &name, const KService::Ptr &service )
{
    QAction *act = m_actionCollection.addAction( QByteArray::number( idx ) );
    act->setText( name );
    act->setIcon( KIcon(service->icon()) );
    QObject::connect(act, SIGNAL(triggered(bool)), this, SLOT( slotOpenEmbedded() ));
    return act;
}

void PopupMenuGUIClient::slotOpenEmbedded()
{
    int idx = sender()->objectName().toInt();
    // This calls KonqMainWindow::slotOpenEmbedded(service) (delayed so that the menu is closed first)
    emit openEmbedded(m_embeddingServices.at(idx));
}

////

ToggleViewGUIClient::ToggleViewGUIClient( KonqMainWindow *mainWindow )
: QObject( mainWindow )
{
  m_mainWindow = mainWindow;

  KService::List offers = KServiceTypeTrader::self()->query( "Browser/View" );
  KService::List::Iterator it = offers.begin();
  while ( it != offers.end() )
  {
    QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
    QVariant orientation = (*it)->property( "X-KDE-BrowserView-ToggableView-Orientation" );

    if ( !prop.isValid() || !prop.toBool() ||
         !orientation.isValid() || orientation.toString().isEmpty() )
    {
      offers.erase( it );
      it = offers.begin();
    }
    else
      ++it;
  }

  m_empty = ( offers.count() == 0 );

  if ( m_empty )
    return;

  KService::List::ConstIterator cIt = offers.begin();
  KService::List::ConstIterator cEnd = offers.end();
  for (; cIt != cEnd; ++cIt )
  {
    QString description = i18n( "Show %1" ,  (*cIt)->name() );
    QString name = (*cIt)->desktopEntryName();
    //kDebug(1202) << "ToggleViewGUIClient: name=" << name;
    KToggleAction *action = new KToggleAction( description, this );
    mainWindow->actionCollection()->addAction( name.toLatin1(), action );
    action->setCheckedState( KGuiItem(i18n( "Hide %1" ,  (*cIt)->name() )) );

    // HACK
    if ( (*cIt)->icon() != "unknown" )
      action->setIcon( KIcon((*cIt)->icon()) );

    connect( action, SIGNAL( toggled( bool ) ),
             this, SLOT( slotToggleView( bool ) ) );

    m_actions.insert( name, action );

    QVariant orientation = (*cIt)->property( "X-KDE-BrowserView-ToggableView-Orientation" );
    bool horizontal = orientation.toString().toLower() == "horizontal";
    m_mapOrientation.insert( name, horizontal );
  }

  connect( m_mainWindow, SIGNAL( viewAdded( KonqView * ) ),
           this, SLOT( slotViewAdded( KonqView * ) ) );
  connect( m_mainWindow, SIGNAL( viewRemoved( KonqView * ) ),
           this, SLOT( slotViewRemoved( KonqView * ) ) );
}

ToggleViewGUIClient::~ToggleViewGUIClient()
{
}

QList<QAction*> ToggleViewGUIClient::actions() const
{
  return m_actions.values();
}

void ToggleViewGUIClient::slotToggleView( bool toggle )
{
  QString serviceName = sender()->objectName();

  bool horizontal = m_mapOrientation[ serviceName ];

  KonqViewManager *viewManager = m_mainWindow->viewManager();

  if ( toggle )
  {
    // Don't crash when doing things too quickly.
    if ( !m_mainWindow->currentView() )
        return;
    KonqView *childView = viewManager->splitMainContainer( m_mainWindow->currentView(),
                                                    horizontal ? Qt::Vertical : Qt::Horizontal,
                                                    QLatin1String( "Browser/View" ),
                                                    serviceName,
                                                    !horizontal /* vertical = make it first */);

    QList<int> newSplitterSizes;

    if ( horizontal )
      newSplitterSizes << 100 << 30;
    else
      newSplitterSizes << 30 << 100;

    if (!childView || !childView->frame())
        return;

    // Toggleviews don't need their statusbar
    childView->frame()->statusbar()->hide();

    KonqFrameContainerBase *newContainer = childView->frame()->parentContainer();

    if (newContainer->frameType()=="Container")
      static_cast<KonqFrameContainer*>(newContainer)->setSizes( newSplitterSizes );

#if 0 // already done by splitWindow
    if ( m_mainWindow->currentView() )
    {
        QString locBarURL = m_mainWindow->currentView()->url().prettyUrl(); // default one in case it doesn't set it
        childView->openUrl( m_mainWindow->currentView()->url(), locBarURL );
    }
#endif

    // If not passive, set as active :)
    if (!childView->isPassiveMode())
      viewManager->setActivePart( childView->part() );

    kDebug() << "ToggleViewGUIClient::slotToggleView setToggleView(true) on " << childView;
    childView->setToggleView( true );

    m_mainWindow->viewCountChanged();

  }
  else
  {
    const QList<KonqView*> viewList = KonqViewCollector::collect(m_mainWindow);
    foreach ( KonqView* view, viewList ) {
      if ( view->service() && view->service()->desktopEntryName() == serviceName )
        // takes care of choosing the new active view, and also calls slotViewRemoved
        viewManager->removeView( view );
    }
  }

}


void ToggleViewGUIClient::saveConfig( bool add, const QString &serviceName )
{
  // This is used on konqueror's startup....... so it's never used, since
  // the K menu only contains calls to kfmclient......
  // Well, it could be useful again in the future.
  // Currently, the profiles save this info.
  QStringList toggableViewsShown = KonqSettings::toggableViewsShown();
  if (add)
  {
      if ( !toggableViewsShown.contains( serviceName ) )
          toggableViewsShown.append(serviceName);
  }
  else
      toggableViewsShown.removeAll(serviceName);
  KonqSettings::setToggableViewsShown( toggableViewsShown );
}

void ToggleViewGUIClient::slotViewAdded( KonqView *view )
{
  QString name = view->service()->desktopEntryName();

  QAction *action = m_actions.value( name );

  if ( action )
  {
    disconnect( action, SIGNAL( toggled( bool ) ),
                this, SLOT( slotToggleView( bool ) ) );
    static_cast<KToggleAction *>( action )->setChecked( true );
    connect( action, SIGNAL( toggled( bool ) ),
             this, SLOT( slotToggleView( bool ) ) );

    saveConfig( true, name );

    // KonqView::isToggleView() is not set yet.. so just check for the orientation

#if 0
    QVariant vert = view->service()->property( "X-KDE-BrowserView-ToggableView-Orientation");
    bool vertical = vert.toString().toLower() == "vertical";
    QVariant nohead = view->service()->property( "X-KDE-BrowserView-ToggableView-NoHeader");
    bool noheader = nohead.isValid() ? nohead.toBool() : false;
    // if it is a vertical toggle part, turn on the header.
    // this works even when konq loads the view from a profile.
    if ( vertical && (!noheader))
    {
        view->frame()->header()->setText(view->service()->name());
        view->frame()->header()->setAction(action);
    }
#endif
  }
}

void ToggleViewGUIClient::slotViewRemoved( KonqView *view )
{
  QString name = view->service()->desktopEntryName();

  QAction *action = m_actions.value( name );

  if ( action )
  {
    disconnect( action, SIGNAL( toggled( bool ) ),
             this, SLOT( slotToggleView( bool ) ) );
    static_cast<KToggleAction *>( action )->setChecked( false );
    connect( action, SIGNAL( toggled( bool ) ),
             this, SLOT( slotToggleView( bool ) ) );
    saveConfig( false, name );
  }
}

#include "konqguiclients.moc"
