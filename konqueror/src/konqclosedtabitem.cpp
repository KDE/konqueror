/* This file is part of the KDE project
   Copyright 2007 David Faure <faure@kde.org>
   Copyright 2007 Eduardo Robles Elvira <edulix@gmail.com>

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

#include "konqclosedtabitem.h"
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>

K_GLOBAL_STATIC_WITH_ARGS(KConfig, s_config, ("konqueror_closedtabs", KConfig::NoGlobals) );

KonqClosedTabItem::KonqClosedTabItem(const QString& url, const QString& title, int pos, quint64 serialNumber)
      :  m_url(url), m_title(title), m_pos(pos),
         m_configGroup(s_config, "Closed_Tab" + QString::number((qint64)this)),
         m_serialNumber(serialNumber)
{
    kDebug(1202) << m_configGroup.name();
}

KonqClosedTabItem::~KonqClosedTabItem()
{
    m_configGroup.deleteGroup();
    kDebug(1202) << "deleted group" << m_configGroup.name();
}
