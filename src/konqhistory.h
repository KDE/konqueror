/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORY_H
#define KONQ_HISTORY_H

#include <QtGlobal>

namespace KonqHistory
{

enum ExtraData {
    TypeRole = Qt::UserRole + 0xaaff00,
    DetailedToolTipRole,
    UrlRole,
    LastVisitedRole
};

enum EntryType {
    HistoryType = 1,
    GroupType = 2
};

}

#endif // KONQ_HISTORY_H
