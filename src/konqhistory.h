/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORY_H
#define KONQ_HISTORY_H

#include <QtGlobal>

namespace KonqHistory
{

/**
* @brief Enum containing the roles used by models related to history
*/
enum ExtraData {
    TypeRole = Qt::UserRole + 0xaaff00, //!< Role corresponding to the type of history entry
    DetailedToolTipRole, //!< Role corresponding to the detailed tool tip for the index
    UrlRole, //!< Role corresponding to the URL of an history entry
    LastVisitedRole //!< Role corresponding to the last time an history entry has been visited
};

/**
 * @brief Enum describing history entry types
 */
enum EntryType {
    HistoryType = 1, //<! An history entry corresponding to an URL
    GroupType = 2 //!< A group of history entries
};

}

#endif // KONQ_HISTORY_H
