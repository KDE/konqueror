/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORYPROXYMODEL_H
#define KONQ_HISTORYPROXYMODEL_H

#include "ksortfilterproxymodel.h"

class KonqHistorySettings;

/**
 * @brief Proxy model used for sorting and filtering the history model
 *
 * It sorts the entries by name or date according to the user settings. Tooltips
 * and fonts are also dependent on the user settings.
 *
 * It uses KSortFilterProxyModel instead of QSortFilterProxyModel so that one can
 * search in the history by typing parts of a page title, since KSortFilterProxyModel
 * shows item which match the filter even when their parent doesn't, while QSortFilterProxyModel
 * hides them.
 */
class KonqHistoryProxyModel : public KSortFilterProxyModel
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param settings the object containing history settings
     * @param parent the parent object
     */
    explicit KonqHistoryProxyModel(KonqHistorySettings *settings, QObject *parent = nullptr);
    ~KonqHistoryProxyModel() override; //!< Destructor

    /**
     * @brief Override of `KSortFilterProxyModel::data()`
     *
     * @param index the index to retrieve data for
     * @param role the display role
     * @return a value depending on the user settings for the `Qt::ToolTipRole`
     * role and for the `Qt::FontRole` and the same as `QSortFilterProxyModel::data()`
     * for all other roles
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    /**
     * @brief Override of `QSortFilterProxyModel::lessThan()`
     *
     * It sorts the item by name or by date depending on the user settings
     * @param left the first item to compare
     * @param right the second item to compare
     * @return `true` if @p left should be sorted before @p right and `false` otherwise.
     * The comparison is made using the items' text or their date depending on the
     * user's settings
     * @note This function assumes that both @p left and @p right are either groups
     * or history entries, since sorting always happens among siblings
     */
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private Q_SLOTS:
    /**
     * @brief Slot called when settings change
     *
     * It resets the model
     */
    void slotSettingsChanged();

private:
    KonqHistorySettings *m_settings; //!< The settings object
};

#endif // KONQ_HISTORYPROXYMODEL_H
