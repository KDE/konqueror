/*
  Copyright (c) 2007 Lubos Lunak <l.lunak@suse.cz>
 
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

#include "xtevents.h"

#include <fixx11h.h>

#include <kapplication.h>
#include <QX11Info>

XtEvents::XtEvents()
    {
    XtToolkitInitialize();
    context = XtCreateApplicationContext();
    int argc = qApp->argc();
    XtDisplayInitialize( context, QX11Info::display(), qAppName().toLatin1(), QX11Info::appClass(),
        NULL, 0, &argc, qApp->argv());
    connect( &timer, SIGNAL( timeout()), SLOT( idleProcess()));
    kapp->installX11EventFilter( this );
    // No way to find out when to process Xt events, so poll :(
    timer.start( 10 );
    }

XtEvents::~XtEvents()
    {
    }

bool XtEvents::x11Event( XEvent* e )
    {
    return XtDispatchEvent( e );
    }

void XtEvents::idleProcess()
    {
    for(;;)
        {
        XtInputMask mask = XtAppPending( context );
        mask &= ~XtIMXEvent; // these are processed in x11Event()
        if( mask == 0 )
            break;
        // TODO protect from zero timers starving everything
        XtAppProcessEvent( context, mask );
        }
    }

#include "xtevents.moc"
