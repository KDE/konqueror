/*

  XEmbed plugin embedding support

  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
                     Stefan Schimanski <1Stein@gmx.de>
                2003-2005 George Staikos <staikos@kde.org>
                2007 Maksim orlovich     <maksim@kde.org>

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

#include "pluginhost_xembed.h"
#include <QX11Info>
#include <QVBoxLayout>
#include <QLabel>

PluginHostXEmbed::PluginHostXEmbed(NSPluginInstance* plugin):
    _plugin(plugin)
{}

PluginHostXEmbed::~PluginHostXEmbed()
{}


void PluginHostXEmbed::setupWindow(int winId, int width, int height)
{
    kDebug() << winId << width << height;
    setupPluginWindow(_plugin, (void*)winId, width, height);
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
