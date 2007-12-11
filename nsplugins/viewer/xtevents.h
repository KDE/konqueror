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

#ifndef XTEVENTS_H
#define XTEVENTS_H

#include <qwidget.h>
#include <qtimer.h>

#include <X11/Intrinsic.h>

class XtEvents
    : public QWidget
    {
    Q_OBJECT
    public:
        XtEvents();
        virtual ~XtEvents();
        virtual bool x11Event( XEvent* );
    private slots:
        void idleProcess();
    private:
        XtAppContext context;
        QTimer timer;
    };

#endif
