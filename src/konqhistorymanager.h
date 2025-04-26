/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000, 2001 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORYMANAGER_H
#define KONQ_HISTORYMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>

#include <konqprivate_export.h>

#include "konq_historyentry.h"
#include "konq_historyprovider.h"

class QTimer;
class KBookmarkManager;
class KCompletion;

/**
 * @brief A subclass of KonqHistoryProvider which has more Konqueror-specific functionality
 *
 * It provides support for pending URLs, that is URLs which we aren't yet sure exist,
 * synchronizes history with a completion object and access metadata with bookmarks.
 *
 * By default, only remote URLs are added to history, but this can avoided by using
 * insert() to insert the element.
 *
 * This is a singleton class. To use it, you first need to create the instance using
 * the constructor and then you can access it using self().
 */
class KONQUERORPRIVATE_EXPORT KonqHistoryManager : public KonqHistoryProvider
{
    Q_OBJECT

public:
    /**
     * @brief Provides access to the only KonqHistoryManager instance
     *
     * This relies on a KonqHistoryManager instance being created first!
     * This is a bit like "qApp": you can access the instance anywhere, but you need to
     * create it first.
     *
     * @return The single KonqHistoryManager instance if it has already been created
     * and `nullptr` otherwise
     */
    static KonqHistoryManager *kself()
    {
        return static_cast<KonqHistoryManager *>(HistoryProvider::self());
    }

    /**
     * @brief Constructor
     *
     * @param bookmarkManager the bookmark manager to use for this object
     * @param parent the parent object
     */
    explicit KonqHistoryManager(KBookmarkManager *bookmarkManager, QObject *parent = nullptr);

    ~KonqHistoryManager() override; //!< Destructor

    /**
     * @brief Adds a pending entry to the history.
     *
     * A pending entry is an entry which has not been verified yet, i.e. it is
     * not sure @p url does exist at all. You
     * probably don't know the title of the url in that case either.
     * Call @ref confirmPending() as soon you know the entry is good and should
     * be updated.
     *
     * If an entry with @p url already exists,
     * it will be updated (the last visited date will become the current time
     * and the number of visits will be incremented).
     *
     * @param url the URL of the history entry
     * @param typedUrl the string that the user typed, which resulted in @p url
     *                 Doesn't have to be a valid URL, e.g. "slashdot.org"
     * @param title the title of the entry. If it isn't (yet) known, it may be
                    specified in confirmPending()
     */
    void addPending(const QUrl &url, const QString &typedUrl = QString(),
                    const QString &title = QString());

    /**
     * @brief Confirms and updates the entry for a given URL
     *
     * @param url the URL of the entry to confirm
     * @param typedUrl the string that the user typed
     * @param title the title of the entry
     * @see addPending()
     */
    void confirmPending(const QUrl &url,
                        const QString &typedUrl = QString(),
                        const QString &title = QString());

    /**
     * @brief * Removes a pending url from the history
     *
     * This should be called, for example, when the URL does not exist, or when
     * the user aborted loading.
     *
     * @param url the URL of the entry to remove
     */
    void removePending(const QUrl &url);

    /**
     * @brief The completion object the history should be synchronized with
     * @returns the completion object
     */
    KCompletion *completionObject() const
    {
        return m_pCompletion;
    }

    // HistoryProvider interface, let konq handle this
    /**
     * Reimplemented in such a way that all URLs that would be filtered
     * out normally (see @ref filterOut()) will still be added to the history.
     * By default, file:/ urls will be filtered out, but if they come thru
     * the HistoryProvider interface, they are added to the history.
     */
    /**
     * @brief Override of HistoryProvider::insert()
     *
     * It inserts the entry in history, but only if it's a local URL (which aren't
     * usually added to history).
     *
     * @note There's no support for pending entries in this case.
     *
     * @param url the URL to add
     */
    void insert(const QString &url) override;

    /**
     * @brief Override of HistoryProvider::remove()
     *
     * It does nothing.
     */
    void remove(const QString &) override {}

    /**
     * @brief Override of HistoryProvider::clear()
     *
     * It does nothing.
     */
    void clear() override {}

private:
    /**
     * @brief Loads the history and fills the completion object
     *
     * It works as KonqHistoryProvider::loadHistory() except that it also adds
     * entries to the completion object.
     *
     * @note Both the completion object and the list of pending entries are cleared
     * before loading history.
     * @return the value returned by KonqHistoryProvider::loadHistory()
     */
    bool loadHistory();

    /**
     * @brief Performs the addition of a new entry into history
     *
     * This does the work for @ref addPending() and @ref confirmPending().
     *
     * If an entry with @p url already exists, it will be updated
     * (last visited date will become the current time
     * and the number of visits will be incremented).
     *
     * If @p pending is `true`, the entry won't be actually added, but will be
     * confirmed, and removed from the list of pending entries.
     * @param pending `true` if the entry is pending and `false` if it's been
     * confirmed
     * @param url the URL of the entry to add
     * @param typedUrl the text which the user actually entered
     * @param title the title of the URL
     */
    void addToHistory(bool pending, const QUrl &url,
                      const QString &typedUrl = QString(),
                      const QString &title = QString());

    /**
     * @brief Whether a given URL should be excluded from history
     *
     * By default, only remote URLs (according to `QUrl::isLocalFile()`) are
     * added to history.
     *
     * @param url the URL to test
     * @return `true` if @p url is a local file, an URL with a `konq` scheme or
     * an URL with empty host and `false` otherwise.
     */
    virtual bool filterOut(const QUrl &url);

    /**
     * @brief Adds an URL to the list of urls to update
     *
     * This starts a timer which, after 0.5s emits the updated() signal.
     *
     * @param url the URL to add to the list
     */
    void addToUpdateList(const QString &url);

    /**
     * @brief Override of KonqHistoryProvider::finishAddingEntry()
     *
     * It adds the URL to the completion object and the bookmarks access metadata.
     * If this is the instance which added the entry, it also saves the bookmarks.
     *
     * @param entry the entry which was added
     * @param isSender `true` if this is the instance which added the entry and
     * `false` otherwise
     */
    void finishAddingEntry(const KonqHistoryEntry &entry, bool isSender) override;

    /**
     * @brief Removes and deletes all the pending entries
     */
    void clearPending();

    /**
     * @brief Adds an URL to the completion object
     *
     * Both the URL itself and the typed URL are added. The typed URL is added
     * with an artificially increased number of visits to give it higher priority.
     *
     * This function is called both when loading the history and when finishing
     * adding a new entry.
     *
     * @param url the URL to add
     * @param typedUrl the text typed by the user
     * @param numberOfTimesVisited the number of times the URL has been visited
     */
    void addToCompletion(const QString &url, const QString &typedUrl, int numberOfTimesVisited = 1);

    /**
     * @brief Removes an URL from the completion object
     *
     * This actually removes two entries from the completion object: one for the
     * actual URL and one for the text the user typed.
     *
     * @param url the url to remove
     * @param typedUrl the text typed by the user
     */
    void removeFromCompletion(const QString &url, const QString &typedUrl);

private Q_SLOTS:
    /**
     * @brief Emits the HistoryProvider::updated() signal
     *
     * The signal is emitted passing #m_updateURLs as argument. The list is then
     * cleared.
     */
    void slotEmitUpdated();

    /**
     * @brief Clears the list of pending entries and the completion object
     */
    void slotCleared();

    /**
     * @brief Removes an entry from the completion object and marks it for updating
     *
     * @param entry the entry which has been removed
     */
    void slotEntryRemoved(const KonqHistoryEntry &entry);

private:

    /**
     * @brief A list of URLs which need to be updated
     *
     * This is the list which slotEmitUpdated() passes as argument to the updated()
     * signal.
     *
     * Whenever an history entry is modified, it should be added to this list using
     * addToUpdateList().
     */
    QStringList m_updateURLs;

    /**
     * @brief A list of pending entries
     *
     * @note When removing an entry, you have to delete the KonqHistoryEntry
     * of the item you remove.
     */
    QMap<QString, KonqHistoryEntry *> m_pending;

    KCompletion *m_pCompletion; //!< The completion object we sync with

    QTimer *m_updateTimer; //!< Timer for emitting the updated() signal

    KBookmarkManager *m_bookmarkManager; //!< The bookmark manager ti synchronize metadata access with

    /**
     * @brief The history version
     *
     * @note This seems to be unused
     */
    static const int s_historyVersion;
};

#endif // KONQ_HISTORY_H
