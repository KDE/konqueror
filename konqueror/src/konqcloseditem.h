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

#ifndef KONQCLOSEDITEM_H
#define KONQCLOSEDITEM_H

#include "konqprivate_export.h"
#include <kconfiggroup.h>
#include <QString>
#include <QImage>

class KONQ_TESTS_EXPORT KonqClosedItem : public QObject {
public:
    virtual ~KonqClosedItem();
    const KConfigGroup& configGroup() const { return m_configGroup; }
    KConfigGroup& configGroup() { return m_configGroup; }
    quint64 serialNumber() const { return m_serialNumber; }
    QString title() const { return m_title; }
    virtual QPixmap icon() = 0;

protected:
    KonqClosedItem(const QString& title, const QString& group, quint64 serialNumber);
    QString m_title;
    KConfigGroup m_configGroup;
    quint64 m_serialNumber;
};

/**
 * This class stores all the needed information about a closed tab
 * in order to be able to reopen it if requested
 */
class KONQ_TESTS_EXPORT KonqClosedTabItem : public KonqClosedItem {
public:
    KonqClosedTabItem(const QString& url, const QString& title, int index, quint64 serialNumber);
    virtual ~KonqClosedTabItem();
    virtual QPixmap icon();
    QString url() const { return m_url; }
    /// The position inside the tabbar that the tab had when it was  closed
    int pos() const { return m_pos; }

protected:
    QString m_url;
    int m_pos;
};

/**
 * This class stores all the needed information about a closed tab
 * in order to be able to reopen it if requested
 */
class KONQ_TESTS_EXPORT KonqClosedWindowItem : public KonqClosedItem {
public:
    KonqClosedWindowItem(const QString& title, quint64 serialNumber, int numTabs);
    virtual ~KonqClosedWindowItem();
    virtual QPixmap icon();
    
protected:
    int m_numTabs;
};

#endif /* KONQCLOSEDITEM_H */

