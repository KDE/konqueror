/*

  Support for scripting of plugins using the npruntime interface

  Copyright (c) 2006, 2010 Maksim Orlovich <maksim@kde.org>

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

#ifndef NSPLUGIN_VIEWER_LC_TYPES_H
#define NSPLUGIN_VIEWER_LC_TYPES_H

#include <QDBusArgument>

/* Type used for transferring scripting results over qdbus */
struct NSLiveConnectResult
{
    NSLiveConnectResult() : success(false) {}
    bool success;
    int  type;
    quint32  objid;
    QString  value;
};


const QDBusArgument& operator<<(QDBusArgument& argument, const NSLiveConnectResult& res);
const QDBusArgument& operator>>(const QDBusArgument& argument, NSLiveConnectResult& res);

Q_DECLARE_METATYPE(NSLiveConnectResult)

namespace kdeNsPluginViewer {
void initDBusTypes();
}

#endif

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
