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

#include "konqcloseditem.h"
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <konqpixmapprovider.h>

K_GLOBAL_STATIC_WITH_ARGS(KConfig, s_config, ("konqueror_closeditems", KConfig::NoGlobals) )

KonqClosedItem::KonqClosedItem(const QString& title, const QString& group, quint64 serialNumber)
    : m_title(title), m_configGroup(s_config, group), m_serialNumber(serialNumber)
{
}

KonqClosedItem::~KonqClosedItem()
{
    m_configGroup.deleteGroup();
    kDebug(1202) << "deleted group" << m_configGroup.name();
}

KonqClosedTabItem::KonqClosedTabItem(const QString& url, const QString& title, int pos, quint64 serialNumber)
      :  KonqClosedItem(title, "Closed_Tab" + QString::number((qint64)this), serialNumber),  m_url(url), m_pos(pos)
{
    kDebug(1202) << m_configGroup.name();
}

KonqClosedTabItem::~KonqClosedTabItem()
{
    m_configGroup.deleteGroup();
    kDebug(1202) << "deleted group" << m_configGroup.name();
}

QPixmap KonqClosedTabItem::icon() {
    return KonqPixmapProvider::self()->pixmapFor(m_url);
}

KonqClosedWindowItem::KonqClosedWindowItem(const QString& title, quint64 serialNumber)
      :  KonqClosedItem(title, "Closed_Window" + QString::number((qint64)this), serialNumber)
{
    kDebug(1202) << m_configGroup.name();
}

KonqClosedWindowItem::~KonqClosedWindowItem()
{
    m_configGroup.deleteGroup();
    kDebug(1202) << "deleted group" << m_configGroup.name();
}

QPixmap KonqClosedWindowItem::icon() {
    return KonqPixmapProvider::self()->pixmapFor("about:blank");
}
