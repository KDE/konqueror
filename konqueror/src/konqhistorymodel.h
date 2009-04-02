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

#ifndef KONQ_HISTORYMODEL_H
#define KONQ_HISTORYMODEL_H

#include <QtCore/QAbstractItemModel>

#include "konq_historyentry.h"

class KonqHistoryManager;
namespace KHM
{
struct Entry;
struct GroupEntry;
struct RootEntry;
struct HistoryEntry;
}

class KonqHistoryModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit KonqHistoryModel(QObject *parent = 0);
    ~KonqHistoryModel();

    // reimplementations from QAbstractItemModel
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    void deleteItem(const QModelIndex &index);

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    void slotEntryAdded(const KonqHistoryEntry &);
    void slotEntryRemoved(const KonqHistoryEntry &);

private:
    KHM::Entry* entryFromIndex(const QModelIndex &index, bool returnRootIfNull = false) const;
    KHM::GroupEntry* getGroupItem(const KUrl &url);
    QModelIndex indexFor(KHM::HistoryEntry *entry) const;
    QModelIndex indexFor(KHM::GroupEntry *entry) const;

    KHM::RootEntry *m_root;
};

#endif // KONQ_HISTORYMODEL_H
