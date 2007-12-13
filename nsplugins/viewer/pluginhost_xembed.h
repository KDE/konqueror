/*

  XEmbed plugin embedding support

  Copyright (c) 2007 Maksim Orlovich <maksim@kde.org>

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

#ifndef PLUGIN_HOST_XEMBED_H
#define PLUGIN_HOST_XEMBED_H

#include "pluginhost.h"
#include <QX11EmbedContainer>

class PluginHostXEmbed : public PluginHost
{
public:
    PluginHostXEmbed(NSPluginInstance* plugin, QX11EmbedWidget* outside);
    virtual void setupWindow(int width, int height);
    virtual void resizePlugin(int width, int height);
    virtual ~PluginHostXEmbed();
private:
    NSPluginInstance*   _plugin;
    QX11EmbedWidget*    _outside;
    QX11EmbedContainer* _container;
};


#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
