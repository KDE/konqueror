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

#include "glibevents.h"

#ifdef HAVE_GLIB2

#include <qapplication.h>

GlibEvents::GlibEvents()
    {
    g_main_context_ref( g_main_context_default());
    connect( &timer, SIGNAL( timeout()), SLOT( process()));
    // TODO Poll for now
    timer.start( 10 );
    }

GlibEvents::~GlibEvents()
    {
    g_main_context_unref( g_main_context_default());
    }

void GlibEvents::process()
    {
    if( g_main_depth() > 0 )
        return; // avoid reentrancy when Qt's Glib integration is used
    while( g_main_context_pending( g_main_context_default()))
        g_main_context_iteration( g_main_context_default(), false );
    }

#include "glibevents.moc"

#endif
