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
 * This class maintains and manages a history of all URLs visited by one
 * Konqueror instance. Additionally it synchronizes the history with other
 * Konqueror instances via DBUS to keep one global and persistent history.
 *
 * It keeps the history in sync with one KCompletion object
 */
class KONQUERORPRIVATE_EXPORT KonqHistoryManager : public KonqHistoryProvider
{
    Q_OBJECT

public:
    /**
     * Returns the KonqHistoryManager instance. This relies on a KonqHistoryManager instance
     * being created first!
     * This is a bit like "qApp": you can access the instance anywhere, but you need to
     * create it first.
     */
    static KonqHistoryManager *kself()
    {
        return static_cast<KonqHistoryManager *>(HistoryProvider::self());
    }

    explicit KonqHistoryManager(KBookmarkManager *bookmarkManager, QObject *parent = nullptr);
    ~KonqHistoryManager() override;

    /**
     * Adds a pending entry to the history. Pending means, that the entry is
     * not verified yet, i.e. it is not sure @p url does exist at all. You
     * probably don't know the title of the url in that case either.
     * Call @ref confirmPending() as soon you know the entry is good and should
     * be updated.
     *
     * If an entry with @p url already exists,
     * it will be updated (lastVisited date will become the current time
     * and the number of visits will be incremented).
     *
     * @param url The url of the history entry
     * @param typedUrl the string that the user typed, which resulted in url
     *                 Doesn't have to be a valid url, e.g. "slashdot.org".
     * @param title The title of the URL. If you don't know it (yet), you may
                    specify it in @ref confirmPending().
     */
    void addPending(const QUrl &url, const QString &typedUrl = QString(),
                    const QString &title = QString());

    /**
     * Confirms and updates the entry for @p url.
     */
    void confirmPending(const QUrl &url,
                        const QString &typedUrl = QString(),
                        const QString &title = QString());

    /**
     * Removes a pending url from the history, e.g. when the url does not
     * exist, or the user aborted loading.
     */
    void removePending(const QUrl &url);

    /**
     * @returns the KCompletion object.
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
    void insert(const QString &) override;
    void remove(const QString &) override {}
    void clear() override {}

private:
    /**
     * Loads the history and fills the completion object.
     */
    bool loadHistory();

    /**
     * Does the work for @ref addPending() and @ref confirmPending().
     *
     * Adds an entry to the history. If an entry with @p url already exists,
     * it will be updated (lastVisited date will become the current time
     * and the number of visits will be incremented).
     * @p pending means, the entry has not been "verified", it's been added
     * right after typing the url.
     * If @p pending is false, @p url will be removed from the pending urls
     * (if available) and NOT be added again in that case.
     */
    void addToHistory(bool pending, const QUrl &url,
                      const QString &typedUrl = QString(),
                      const QString &title = QString());

    /**
     * @returns true if the given @p url should be filtered out and not be
     * added to the history. By default, all local urls (url.isLocalFile())
     * will return true, as well as urls with an empty host.
     */
    virtual bool filterOut(const QUrl &url);

    void addToUpdateList(const QString &url);

    /**
     * The list of urls that is going to be emitted in slotEmitUpdated. Add
     * urls to it whenever you modify the list of history entries and start
     * m_updateTimer.
     */
    QStringList m_updateURLs;

private Q_SLOTS:
    /**
     * Called by the updateTimer to emit the KParts::HistoryProvider::updated()
     * signal so that khtml can repaint the updated links.
     */
    void slotEmitUpdated();

    void slotCleared();
    void slotEntryRemoved(const KonqHistoryEntry &entry);

private:
    void finishAddingEntry(const KonqHistoryEntry &entry, bool isSender) override;
    void clearPending();

    void addToCompletion(const QString &url, const QString &typedUrl, int numberOfTimesVisited = 1);
    void removeFromCompletion(const QString &url, const QString &typedUrl);

    /**
     * List of pending entries, which were added to the history, but not yet
     * confirmed (i.e. not yet added with pending = false).
     * Note: when removing an entry, you have to delete the KonqHistoryEntry
     * of the item you remove.
     */
    QMap<QString, KonqHistoryEntry *> m_pending;

    KCompletion *m_pCompletion; // the completion object we sync with

    /**
     * A timer that will emit the KParts::HistoryProvider::updated() signal
     * thru the slotEmitUpdated slot.
     */
    QTimer *m_updateTimer;

    KBookmarkManager *m_bookmarkManager;

    static const int s_historyVersion;
};

#endif // KONQ_HISTORY_H
