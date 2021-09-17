/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqhistoryproxymodel.h"

#include "konqhistory.h"
#include "konqhistorysettings.h"
#include <QDateTime>

KonqHistoryProxyModel::KonqHistoryProxyModel(KonqHistorySettings *settings, QObject *parent)
    : KSortFilterProxyModel(parent)
    , m_settings(settings)
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);

    connect(m_settings, &KonqHistorySettings::settingsChanged, this, &KonqHistoryProxyModel::slotSettingsChanged);
}

KonqHistoryProxyModel::~KonqHistoryProxyModel()
{
}

QVariant KonqHistoryProxyModel::data(const QModelIndex &index, int role) const
{
    if (!sourceModel()) {
        return QVariant();
    }

    const QModelIndex source_index = mapToSource(index);
    QVariant res;
    switch (source_index.data(KonqHistory::TypeRole).toInt()) {
    case KonqHistory::HistoryType:
        switch (role) {
        case Qt::ToolTipRole:
            if (m_settings->m_detailedTips) {
                res = sourceModel()->data(source_index, KonqHistory::DetailedToolTipRole);
            } else {
                res = sourceModel()->data(source_index, Qt::ToolTipRole);
            }
            break;
        case Qt::FontRole: {
            const QDateTime current = QDateTime::currentDateTime();
            const QDateTime entryDate = source_index.data(KonqHistory::LastVisitedRole).toDateTime();

            QDateTime dt;
            if (m_settings->m_metricYoungerThan == KonqHistorySettings::DAYS) {
                dt = current.addDays(-int(m_settings->m_valueYoungerThan));
            } else {
                dt = current.addSecs(-(int(m_settings->m_valueYoungerThan) * 60));
            }

            if (entryDate > dt) {
                res = QVariant::fromValue(m_settings->m_fontYoungerThan);
            } else {
                if (m_settings->m_metricOlderThan == KonqHistorySettings::DAYS) {
                    dt = current.addDays(-int(m_settings->m_valueOlderThan));
                } else {
                    dt = current.addSecs(-(int(m_settings->m_valueOlderThan) * 60));
                }
                if (entryDate < dt) {
                    res = QVariant::fromValue(m_settings->m_fontOlderThan);
                }
            }
            break;
        }
        }
        break;
    case KonqHistory::GroupType:
        break;
    }
    if (res.isNull()) {
        res = QSortFilterProxyModel::data(index, role);
    }
    return res;
}

bool KonqHistoryProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    switch (left.data(KonqHistory::TypeRole).toInt()) {
    case KonqHistory::HistoryType:
        Q_ASSERT(right.data(KonqHistory::TypeRole).toInt() == KonqHistory::HistoryType);
        if (m_settings->m_sortsByName) {
            return left.data().toString() < right.data().toString();
        } else {
            return left.data(KonqHistory::LastVisitedRole).toDateTime() > right.data(KonqHistory::LastVisitedRole).toDateTime();
        }
    case KonqHistory::GroupType:
        Q_ASSERT(right.data(KonqHistory::TypeRole).toInt() == KonqHistory::GroupType);
        if (m_settings->m_sortsByName) {
            return left.data().toString() < right.data().toString();
        } else {
            return left.data(KonqHistory::LastVisitedRole).toDateTime() > right.data(KonqHistory::LastVisitedRole).toDateTime();
        }
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

void KonqHistoryProxyModel::slotSettingsChanged()
{
    beginResetModel();
    endResetModel();
}

