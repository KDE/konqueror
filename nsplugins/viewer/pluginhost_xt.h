/*

  Xt plugin embedding support

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
                2003-2005 George Staikos <staikos@kde.org>
                2007 Maksim Orlovich <maksim@kde.org>

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

#ifndef PLUGIN_HOST_XT_H
#define PLUGIN_HOST_XT_H

#include "pluginhost.h"
#include <QX11EmbedContainer>

#include <X11/Intrinsic.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <fixx11h.h>

class PluginHostXt : public PluginHost
{
public:
    PluginHostXt(NSPluginInstance* plugin, QX11EmbedWidget* outside, int w, int h);
    virtual void setupWindow(int width, int height);
    virtual void resizePlugin(int width, int height);
    virtual ~PluginHostXt();
private:
    static void forwarder(Widget w, XtPointer cl_data, XEvent * event, Boolean * cont);
    NSPluginInstance*   _plugin;
    QX11EmbedWidget*    _outside;
    Widget              _toplevel;
    Widget              _form;
};


#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
