/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000, 2001 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2007-2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KONQ_HISTORYPROVIDER_H
#define KONQ_HISTORYPROVIDER_H

#include <historyprovider.h>
#include <QUrl>
#include "libkonq_export.h"
#include "konq_historyentry.h"

class KonqHistoryEntry;
class KonqHistoryList;
class KonqHistoryProviderPrivate;

/**
 * @brief Class which maintains and manages a history of all URLs visited by Konqueror.
 *
 * It synchronizes the history with other KonqHistoryProvider instances in
 * other processes (konqueror, history list, krunner etc.) via D-Bus to keep
 * one global and persistent history.
 *
 * This is a singleton class. To use it, you first need to create the instance using
 * the constructor and then you can access it using self().
 *
 * @note Currently (2025-07-26) the only other user of this class seem to be the
 * sweeper utility ([](https://apps.kde.org/sweeper/)) which uses it to clear history
 * from running Konqueror processes.
 */
class LIBKONQ_EXPORT KonqHistoryProvider : public HistoryProvider
{
    Q_OBJECT

public:
    /**
     * @brief Provides access to the only KonqHistoryProvider instance
     *
     * This relies on a KonqHistoryProvider instance being created first!
     * This is a bit like "qApp": you can access the instance anywhere, but you need to
     * create it first.
     *
     * @note Unlike HistoryProvider::self() this doesn't create the instance if it
     * doesn't already exist. You must explicitly create it using the constructor
     *
     * @return The single KonqHistoryProvider instance if it has already been created
     * and `nullptr` otherwise
     */
    static KonqHistoryProvider *self()
    {
        return static_cast<KonqHistoryProvider *>(HistoryProvider::self());
    }

    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    explicit KonqHistoryProvider(QObject *parent = nullptr);
    ~KonqHistoryProvider() override; //!< Destructor

    /**
     * @brief A list of all history entries
     *
     * @returns the list of all history entries, sorted by date (oldest entries first)
     */
    const KonqHistoryList &entries() const;

    /**
     * @returns the current maximum number of history entries.
     */
    int maxCount() const;

    /**
     * @brief The maximum age of history entries
     * @returns the current maximum age (in days) of history entries.
     */
    int maxAge() const;

    /**
     * @brief Emits a signal to notify that the maximum number of history entries
     * has changed
     *
     * Calling this function has the following consequences:
     * - the KonqHistoryProviderPrivate::notifyMaxCount() signal is emitted
     * - other Konqueror instances are notified via D-Bus to update the max count
     * - the maximum history size is changed, truncating the current history if necessary
     * - the history is saved (after notifying the other instances).
     *
     * All this is implemented in KonqHistoryProviderPrivate::slotNotifyMaxCount()
     *
     * @param count the new maximum history length
     */
    void emitSetMaxCount(int count);

    /**
     * @brief Emits a signal to notify that the maximum age of history entries
     * has changed
     *
     * Calling this function has the following consequences:
     * - the KonqHistoryProviderPrivate::notifyMaxAge() signal is emitted
     * - other Konqueror instances are notified via D-Bus to update the maximum age
     * - all entries which are older than @p days, if any, are removed
     * - the history is saved (after notifying the other instances).
     *
     * All this is implemented in KonqHistoryProviderPrivate::slotNotifyMaxAge()
     *
     * @param days the new maximum age. A value of 0 means that there's no maximum age
     */
    void emitSetMaxAge(int days);

    /**
     * @brief Emits a signal to notify that an entry has been removed from history
     *
     * Calling this function has the following consequences:
     * - the KonqHistoryProviderPrivate::notifyRemove() and entryRemoved() signals are emitted
     * - other Konqueror instances are notified via D-Bus to remove the entry
     * - the entry is removed
     * - the history is saved (after notifying the other instances).
     *
     * All this is implemented in KonqHistoryProviderPrivate::slotNotifyRemove()
     *
     * @param url the URL of the entry to remove
     */
    void emitRemoveFromHistory(const QUrl &url);

    /**
     * @brief Emits a signal to notify that several entries have been removed from history
     *
     * Calling this function has the following consequences:
     * - the KonqHistoryProviderPrivate::notifyRemoveList() and entryRemoved() signals are emitted
     * - other Konqueror instances are notified via D-Bus to remove the entries
     * - the entries are removed
     * - the history is saved (after notifying the other instances).
     *
     * All this is implemented in KonqHistoryProviderPrivate::slotNotifyRemoveList()
     *
     * @param urls the list of URLs of the entries to remove
     */
    void emitRemoveListFromHistory(const QList<QUrl> &urls);

    /**
     * @brief Emits a signal to notify that history has been cleared
     *
     * Calling this function has the following consequences:
     * - the KonqHistoryProviderPrivate::notifyClear() and HistoryProvider::clear()
     * signals are emitted
     * - other Konqueror instances are notified via D-Bus to clear history
     * - history is cleared
     * - the history is saved (after notifying the other instances).
     *
     * All this is implemented in KonqHistoryProviderPrivate::slotNotifyClear()
     */
    void emitClear();

    /**
     * @brief Loadss the whole history from disk
     *
     * @note This should be called exactly once.
     */
    bool loadHistory();

Q_SIGNALS:
    /**
     * @brief Signal emitted after a new entry was added
     *
     * @param entry the new entry
     */
    void entryAdded(const KonqHistoryEntry &entry);

    /**
     * @brief Signal emitted after an entry was removed from history
     *
     * @warning @p entry will be deleted immediately after the emission of the signal
     *
     * @param entry the removed entry
     */
    void entryRemoved(const KonqHistoryEntry &entry);

protected: // only to be used by konqueror's KonqHistoryManager

    /**
     * @brief Method called after inserting an item into history
     *
     * The base class implementation saves history if @p isSender is `true`.
     *
     * @param entry the new entry
     * @param isSender `true` if this is the instance which originally added the entry
     * and `false` if it did so in response to the signal emitted by another instances.
     * Some actions, for example saving the history, should only be done by the instance
     * which started it
     */
    virtual void finishAddingEntry(const KonqHistoryEntry &entry, bool isSender);

    /**
     * @brief Performs the removal of an history entry from the list
     *
     * It emits the HistoryProvider::remove() signal.
     *
     * @param it an iterator corresponding to the entry to remove
     */
    virtual void removeEntry(KonqHistoryList::iterator it);

    /**
     * @brief Helper function to look for an history entry
     *
     * It works as KonqHistoryList::findEntry() except that, before looking in
     * the list of entries, it uses HistoryProvider::contains() to check whether
     * the entry is *not* in the list, as this is faster.
     *
     * @warning It can't be used everywhere, because it can't find pending
     * entries, as those are not added to the dictionary used by HistoryProvider.
     * @param url the URL of the entry to retrieve
     * @return the iterator corresponding to the entry with URL @p url or an invalid
     * iterator if such entry doesn't exist
     */
    KonqHistoryList::iterator findEntry(const QUrl &url);

    /**
     * @brief Helper function to look for an history entry
     *
     * It works as KonqHistoryList::constFindEntry() except that, before looking in
     * the list of entries, it uses HistoryProvider::contains() to check whether
     * the entry is *not* in the list, as this is faster.
     *
     * @warning It can't be used everywhere, because it can't find pending
     * entries, as those are not added to the dictionary used by HistoryProvider.
     * @param url the URL of the entry to retrieve
     * @return the iterator corresponding to the entry with URL @p url or an invalid
     * iterator if such entry doesn't exist
     */
    KonqHistoryList::const_iterator constFindEntry(const QUrl &url) const;

    /**
     * Notifies all running instances about a new HistoryEntry via D-Bus.
     */
    /**
     * @brief Emits a signal to notify that an entry has been added to the history
     *
     * Calling this function has the following consequences:
     * - other Konqueror instances are notified via D-Bus to add the entry
     * - the entry is added
     * - the KonqHistoryProviderPrivate::entryAdded() signal is emitted
     * - the history is saved (after notifying the other instances).
     *
     * All this is implemented in KonqHistoryProviderPrivate::slotNotifyHistoryEntry().
     *
     * @param entry entry the added entry
     */
    void emitAddToHistory(const KonqHistoryEntry &entry);

private:
    KonqHistoryProviderPrivate *const d; //!< The d pointer
    friend class KonqHistoryProviderPrivate;
};

#endif /* KONQ_HISTORYPROVIDER_H */
