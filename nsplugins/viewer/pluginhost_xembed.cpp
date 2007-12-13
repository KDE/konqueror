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

PluginHostXEmbed::PluginHostXEmbed(NSPluginInstance* plugin, QX11EmbedWidget* outside):
    _plugin(plugin), _outside(outside), _container(0)
{
    // Nothing here yet --- will do once embedded, inside setupWindow
}

PluginHostXEmbed::~PluginHostXEmbed()
{
/*    delete _inside;
    _inside = 0;*/
}


void PluginHostXEmbed::resizePlugin(int width, int height)
{
//    return;
    kDebug(1431) << _container->geometry() << _outside->geometry();
/*    _inside->setGeometry(0, 0, width, height);*/
}

void PluginHostXEmbed::setupWindow(int width, int height)
{
    if (!_container) {
        QVBoxLayout* outsideLayout = new QVBoxLayout(_outside);
        outsideLayout->setMargin(0);
        _outside->setLayout(outsideLayout);
        
        _container = new QX11EmbedContainer(_outside);
        _container->setWindowTitle("_inner");
        outsideLayout->addWidget(_container);
        
//         QPalette p = _container->palette();
//         p.setColor(_container->backgroundRole(), Qt::green);
//         _container->setPalette(p);
//         _container->setAutoFillBackground(true);

        _container->show();
        outsideLayout->update();
    }
    kDebug() << _outside->geometry() << _container->geometry() << 
        _outside->isVisible() << _container->isVisible();
    
    setupPluginWindow(_plugin, (void*)_container->winId(), width, height);
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
