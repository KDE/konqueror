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

#ifndef KONQCLOSEDTABITEM_H
#define KONQCLOSEDTABITEM_H

#include "konqprivate_export.h"
#include <kconfiggroup.h>
#include <QString>

/**
 * This class stores all the needed information about a closed tab
 * in order to be able to reopen it if requested
 */
class KONQ_TESTS_EXPORT KonqClosedTabItem {
public:
    KonqClosedTabItem(const QString& url, const QString& title, int index, quint64 serialNumber);
    ~KonqClosedTabItem();
    const KConfigGroup& configGroup() const { return m_configGroup; }
    KConfigGroup& configGroup() { return m_configGroup; }
    quint64 serialNumber() const { return m_serialNumber; }
    QString url() const { return m_url; }
    QString title() const { return m_title; }
    /// The position inside the tabbar that the tab had when it was closed
    int pos() const { return m_pos; }

private:
    QString m_url;
    QString m_title;
    int m_pos;
    KConfigGroup m_configGroup;
    quint64 m_serialNumber;

    // Copying an item would delete the group in the config file (see destructor)!
    Q_DISABLE_COPY(KonqClosedTabItem)
};

#endif /* KONQCLOSEDTABITEM_H */

