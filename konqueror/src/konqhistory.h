/* This file is part of the KDE project
   Copyright 2009 Pino Toscano <pino@kde.org>

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

#ifndef KONQ_HISTORY_H
#define KONQ_HISTORY_H

#include <QtCore/qglobal.h>

namespace KonqHistory
{

enum ExtraData
{
    TypeRole = Qt::UserRole + 0xaaff00,
    DetailedToolTipRole,
    UrlRole,
    LastVisitedRole
};

enum EntryType
{
    HistoryType = 1,
    GroupType = 2
};

}

#endif // KONQ_HISTORY_H
