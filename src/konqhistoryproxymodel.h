/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORYPROXYMODEL_H
#define KONQ_HISTORYPROXYMODEL_H

#include "ksortfilterproxymodel.h"

class KonqHistorySettings;

/**
 * Proxy model used for sorting and filtering the history model.
 *
 * It uses KSortFilterProxyModel instead of QSortFilterProxyModel so that one can
 * search in the history by typing parts of a page title (KSortFilterProxyModel keeps
 * parents of filtered items).
 */
class KonqHistoryProxyModel : public KSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit KonqHistoryProxyModel(KonqHistorySettings *settings, QObject *parent = nullptr);
    ~KonqHistoryProxyModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private Q_SLOTS:
    void slotSettingsChanged();

private:
    KonqHistorySettings *m_settings;
};

#endif // KONQ_HISTORYPROXYMODEL_H
