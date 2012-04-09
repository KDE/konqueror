// -*- c++ -*-

/*
  Copyright 2003 by Richard J. Moore, rich@kde.org

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 */

#include <kparts/part.h>
#include <kdebug.h>
#include "autorefresh.h"
#include <kaction.h>
#include <kcomponentdata.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qtimer.h>
#include <kselectaction.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>

#include <KIcon>

AutoRefresh::AutoRefresh( QObject* parent, const QVariantList & /*args*/ )
    : Plugin( parent )
{
    timer = new QTimer( this );
   connect( timer, SIGNAL(timeout()), this, SLOT(slotRefresh()) );

    refresher = actionCollection()->add<KSelectAction>("autorefresh");
    refresher->setText(i18n("&Auto Refresh"));
    refresher->setIcon(KIcon("view-refresh"));
    connect(refresher, SIGNAL(triggered(QAction*)),this, SLOT(slotIntervalChanged()));
    QStringList sl;
    sl << i18n("None");
    sl << i18n("Every 15 Seconds");
    sl << i18n("Every 30 Seconds");
    sl << i18n("Every Minute");
    sl << i18n("Every 5 Minutes");
    sl << i18n("Every 10 Minutes");
    sl << i18n("Every 15 Minutes");
    sl << i18n("Every 30 Minutes");
    sl << i18n("Every 60 Minutes");
    sl << i18n("Every 2 Hours");
    sl << i18n("Every 6 Hours");

    refresher->setItems( sl );
    refresher->setCurrentItem( 0 );
}

AutoRefresh::~AutoRefresh()
{
}

void AutoRefresh::slotIntervalChanged()
{
   int idx = refresher->currentItem();
   int timeout = 0;
   switch (idx) {
     case 1:
         timeout = ( 15*1000 );
         break;
     case 2:
         timeout = ( 30*1000 );
         break;
     case 3:
         timeout = ( 60*1000 );
         break;
     case 4:
         timeout = ( 5*60*1000 );
         break;
     case 5:
         timeout = ( 10*60*1000 );
         break;
     case 6:
         timeout = ( 15*60*1000 );
         break;
     case 7:
         timeout = ( 30*60*1000 );
         break;
     case 8:
         timeout = ( 60*60*1000 );
         break;
     case 9:
         timeout = ( 2*60*60*1000 );
         break;
     case 10:
         timeout = ( 6*60*60*1000 );
         break;
     default:
         break;
   }
   timer->stop();
   if ( timeout )
      timer->start( timeout );
}

void AutoRefresh::slotRefresh()
{
    KParts::ReadOnlyPart *part = qobject_cast< KParts::ReadOnlyPart * >( parent() );
    if ( !part ) {
        QString title = i18nc( "@title:window", "Cannot Refresh Source" );
        QString text = i18n( "<qt>This plugin cannot auto-refresh the current part.</qt>" );

        KMessageBox::error( 0, text, title );
    }
    else
    {
        // Get URL
        KUrl url = part->url();
        part->openUrl( url );
    }
}

K_PLUGIN_FACTORY( AutoRefreshFactory, registerPlugin< AutoRefresh >(); )
K_EXPORT_PLUGIN( AutoRefreshFactory( "autorefresh" ) )

#include "autorefresh.moc"

