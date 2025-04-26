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

/**
 * @brief Item model representing history
 *
 * History is represented as a tree where history entries are grouped according to
 * their host. Entries corresponding to a local file are all grouped together in
 * a `Local` group. All groups are child of a single root item.
 */
class KonqHistoryModel : public QAbstractItemModel
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    explicit KonqHistoryModel(QObject *parent = nullptr);

    ~KonqHistoryModel() override; //!< Destructor

    // reimplementations from QAbstractItemModel
    /**
     * @brief Override of `QAbstractItemModel::columnCount()`
     *
     * @param parent the parent index
     * @return 1 if @p parent is a root or group entry and 0 if it's a history entry
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Override of `QAbstractItemModel::data()`
     *
     * @param index the index to retrieve data for
     * @param role the kind of data to retrieve
     * @return the value returned by the @link KHM::Entry::data() data()@endlink of
     * the KHM::Entry corresponding to @p index or an invalid `QVariant` if there's
     * not such entry
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /**
     * @brief Override of `QAbstractItemModel::index()`
     *
     * @param row the row of the index
     * @param column the column of the index
     * @param parent the parent of the index
     * @return an index containing a pointer to the KHM::Entry corresponding to the
     * child at row @p row of @p parent or an invalid index if no such child exists.
     */
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Override of `QAbstractItemModel::parent()`
     *
     * @param index the index whose parent should be found
     * @return an index corresponding to the KHM::Group @p index belongs to if
     * @p index is an KHM::HistoryEntry and an invalid index if it's a KHM::GroupEntry
     * or KHM::RootEntry
     */
    QModelIndex parent(const QModelIndex &index) const override;

    /**
     * @brief Override of `QAbstractItemModel::rowCount()`
     *
     * @param parent the index whose rows should be returned
     * @return the number of groups if @p parent corresponds to a KHM::RootEntry
     * entry, the number of child history entries if it corresponds to a KHM::GroupEntry
     * entry and 0 if it corresponds to a KHM::HistoryEntry entry
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Deletes the history item corresponding to the given index from history
     *
     * If the index corresponds to a KHM::HistoryEntry, that entry is deleted;
     * if it corresponds to KHM::GroupEntry, all the group's children are deleted.
     * If it corresponds to a KHM::RootEntry, nothing is done.
     *
     * @note This doesn't only remove the item from the model, but from history
     * itself, using KonqHistoryProvider::emitRemoveFromHistory() or
     * KonqHistoryProvider::emitRemoveListFromHistory().
     *
     * @param index the index corresponding to the item to remove
     */
    void deleteItem(const QModelIndex &index);

public Q_SLOTS:

    /**
     * @brief Removes all the entries from the model
     *
     * @note Unlike deleteItem(), this doesn't affect the history but only the
     * model
     */
    void clear();

private Q_SLOTS:

    /**
     * @brief Slot called when an entry is added to history
     *
     * If the model doesn't have an item corresponding to @p entry, a new one is
     * created, creating a new group if needed.
     *
     * If an item already exists, it's updated only if it doesn't yet have a valid
     * last-visited date, to avoid rearranging the items when ordered by date
     * ([BUG 61450](https://bugs.kde.org/show_bug.cgi?id=61450)).
     *
     * @param entry the history entry which has been added
     */
    void slotEntryAdded(const KonqHistoryEntry &entry);

    /**
     * @brief Slot called when an entry is removed from history
     *
     * It deletes the entry from the model and, if the parent group only had that
     * entry, it deletes the group, too.
     *
     * @param entry the entry which has been remvoed
     */
    void slotEntryRemoved(const KonqHistoryEntry &entry);

private:
    /**
     * @brief Enum describing how getGroupItem() should behave when creating a new group
     */
    enum SignalEmission {
        EmitSignals, //!< Call `beginInsertRows()` and `endInsertRows()`
        DontEmitSignals //!< Don't call `beginInsertRows()` and `endInsertRows()`
    };
    /**
     * @brief Retrieves the entry corresponding to a given index
     *
     * @param index the index to retrieve the entry for
     * @param returnRootIfNull what to return if @p index doesn't correspond to an
     * entry
     * @return the internal pointer contained in the index and cast to a KHM::Entry.
     * If this is `nullptr, the returned value depends on @p returnRootIfNull:
     * if `true`, it returns the root entry; if `false` it returns `nullptr`
     */
    KHM::Entry *entryFromIndex(const QModelIndex &index, bool returnRootIfNull = false) const;

    /**
     * @brief Retrieves the group containing the given url, creating it if needed
     *
     * The name of the group depends on @p url:
     * - if it's a local URL, the group name is `Local`
     * - if it's a remote URL with a non-empty host, the group name is the host name
     * - if it's a remote URL with an empty host, the group name is `Miscellaneous`
     *
     * If the group for @p url doesn't exist, it's created and, if @p se is EmitSignals,
     * the `rowsAboutToBeInserted()` signal is emitted.
     *
     * @param url the URL for which the group should be retrieved
     * @param se whether or not to emit the `rowsAboutToBeInserted()` signal
     * when a new group is created. This should be DontEmitSignals only when this
     * function is called in the constructor.
     * @return the KHM::GroupEntry which @p url should be child of
     */
    KHM::GroupEntry *getGroupItem(const QUrl &url, SignalEmission se);

    /**
     * @brief The index for the given history entry
     *
     * @param entry the entry to create the index for
     * @return an index corresponding to @p entry
     */
    QModelIndex indexFor(KHM::HistoryEntry *entry) const;

    /**
     * @brief The index for the given group
     *
     * @param entry the group entry to create the index for
     * @return an index corresponding to @p entry
     */
    QModelIndex indexFor(KHM::GroupEntry *entry) const;

    KHM::RootEntry *m_root; //!< The root entry
};

#endif // KONQ_HISTORYMODEL_H
