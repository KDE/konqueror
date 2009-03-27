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

#ifndef KONQ_HISTORYPROXYMODEL_H
#define KONQ_HISTORYPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

class KonqHistorySettings;

class KonqHistoryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit KonqHistoryProxyModel(KonqHistorySettings *settings, QObject *parent = 0);
    ~KonqHistoryProxyModel();

    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

protected:
    virtual bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private Q_SLOTS:
    void slotSettingsChanged();

private:
    KonqHistorySettings *m_settings;
};

#endif // KONQ_HISTORYPROXYMODEL_H
