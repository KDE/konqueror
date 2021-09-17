/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORYMODEL_H
#define KONQ_HISTORYMODEL_H

#include <QAbstractItemModel>

#include "konq_historyentry.h"

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
    explicit KonqHistoryModel(QObject *parent = nullptr);
    ~KonqHistoryModel() override;

    // reimplementations from QAbstractItemModel
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    void deleteItem(const QModelIndex &index);

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    void slotEntryAdded(const KonqHistoryEntry &);
    void slotEntryRemoved(const KonqHistoryEntry &);

private:
    enum SignalEmission { EmitSignals, DontEmitSignals };
    KHM::Entry *entryFromIndex(const QModelIndex &index, bool returnRootIfNull = false) const;
    KHM::GroupEntry *getGroupItem(const QUrl &url, SignalEmission se);
    QModelIndex indexFor(KHM::HistoryEntry *entry) const;
    QModelIndex indexFor(KHM::GroupEntry *entry) const;

    KHM::RootEntry *m_root;
};

#endif // KONQ_HISTORYMODEL_H
