/*
    SPDX-FileCopyrightText: 2009 John Tapsell <tapsell@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSORTFILTERPROXYMODEL_H
#define KSORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class KSortFilterProxyModelPrivate;

/**
 * @brief Subclass of `QSortFilterProxyModel` which offers more flexibility in the
 * handling of index hierarchies
 *
 * This class changes the behaviour of `QSortFilterProxyModel` in how it handles
 * children indexes:
 * - with `QSortFilterProxyModel`, if an index doesn't match the filter, then all
 * its children automatically don't match the filter. This class inverts this behaviour:
 * if at least one child of an index matches the filter, then its parent also is
 * considered matching
 * - it allows to consider children of matching indexes to always match, regardless
 * of their content. This behaviour is disabled by default and should be enabled
 * calling setShowAllChildren() with `true`.
 *
 * @author John Tapsell <tapsell@kde.org>
 * @since 4.4
 */
class KSortFilterProxyModel
    : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    KSortFilterProxyModel(QObject *parent = nullptr);

    ~KSortFilterProxyModel() override; //!< Destructor

    /**
     * @brief Whether or not all children of a parent which matches the children
     * automatically match the filter
     *
     * @return `true` if the children of a parent which matches the filter automatically
     * match the filter, too, regardless of their content and `false` if they should
     * match by themselves.
     */
    bool showAllChildren() const;

    /**
     * @brief Sets whether or not children of a parent which matches the filter should
     * automatically match, too, regardless of their content
     *
     * @param showAllChildren if `true`, when a parent index matches the filter, then
     * all of its children will automatically match the filter. If `false`, each
     * child will match or not according to its content
     */
    void setShowAllChildren(bool showAllChildren);

protected:

    /**
     * @brief Override of `QSortFilterProxyModel::filterAcceptsRow()`
     *
     * It works as the base class method except that it also accepts the row if
     * any of its children are accepted. If showAllChildren() is `true`, it also
     * accepts a row if its parent is accepted.
     *
     * @param source_row the number of the row in the source model
     * @param source_parent the parent index in the source model
     * @return `true` if the row should be accepted and `false` otherwise
     */
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    KSortFilterProxyModelPrivate *const d_ptr; //!< The d-pointer for this class

    Q_DISABLE_COPY(KSortFilterProxyModel)
};
#endif
