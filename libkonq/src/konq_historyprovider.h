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
 * This class maintains and manages a history of all URLs visited by Konqueror.
 * It synchronizes the history with other KonqHistoryProvider instances in
 * other processes (konqueror, history list, krunner etc.) via D-Bus to keep
 * one global and persistent history.
 */
class LIBKONQ_EXPORT KonqHistoryProvider : public HistoryProvider
{
    Q_OBJECT

public:
    /**
     * Returns the KonqHistoryProvider instance. This relies on a KonqHistoryProvider
     * instance being created first!
     * This is a bit like "qApp": you can access the instance anywhere, but you need to
     * create it first.
     */
    static KonqHistoryProvider *self()
    {
        return static_cast<KonqHistoryProvider *>(HistoryProvider::self());
    }

    explicit KonqHistoryProvider(QObject *parent = nullptr);
    ~KonqHistoryProvider() override;

    /**
     * @returns the list of all history entries, sorted by date
     * (oldest entries first)
     */
    const KonqHistoryList &entries() const;

    /**
     * @returns the current maximum number of history entries.
     */
    int maxCount() const;

    /**
     * @returns the current maximum age (in days) of history entries.
     */
    int maxAge() const;

    /**
     * Sets a new maximum size of history and truncates the current history
     * if necessary. Notifies all other Konqueror instances via D-Bus
     * to do the same.
     *
     * The history is saved after receiving the D-Bus call.
     */
    void emitSetMaxCount(int count);

    /**
     * Sets a new maximum age of history entries and removes all entries that
     * are older than @p days. Notifies all other Konqueror instances via D-Bus
     * to do the same.
     *
     * An age of 0 means no expiry based on the age.
     *
     * The history is saved after receiving the D-Bus call.
     */
    void emitSetMaxAge(int days);

    /**
     * Removes the history entry for @p url, if existent. Tells all other
     * Konqueror instances via D-Bus to do the same.
     *
     * The history is saved after receiving the D-Bus call.
     */
    void emitRemoveFromHistory(const QUrl &url);

    /**
     * Removes the history entries for the given list of @p urls. Tells all
     * other Konqueror instances via D-Bus to do the same.
     *
     * The history is saved after receiving the D-Bus call.
     */
    void emitRemoveListFromHistory(const QList<QUrl> &urls);

    /**
     * Clears the history and tells all other Konqueror instances via D-Bus
     * to do the same.
     * The history is saved afterwards, if necessary.
     */
    void emitClear();

    /**
     * Load the whole history from disk. Call this exactly once.
     */
    bool loadHistory();

Q_SIGNALS:
    /**
     * Emitted after a new entry was added
     */
    void entryAdded(const KonqHistoryEntry &entry);

    /**
     * Emitted after an entry was removed from the history
     * Note, that this entry will be deleted immediately after you got
     * that signal.
     */
    void entryRemoved(const KonqHistoryEntry &entry);

protected: // only to be used by konqueror's KonqHistoryManager

    virtual void finishAddingEntry(const KonqHistoryEntry &entry, bool isSender);
    virtual void removeEntry(KonqHistoryList::iterator it);

    /**
     * a little optimization for KonqHistoryList::findEntry(),
     * checking the dict of KParts::HistoryProvider before traversing the list.
     * Can't be used everywhere, because it always returns end() for "pending"
     * entries, as those are not added to the dict, currently.
     */
    KonqHistoryList::iterator findEntry(const QUrl &url);
    KonqHistoryList::const_iterator constFindEntry(const QUrl &url) const;

    /**
     * Notifies all running instances about a new HistoryEntry via D-Bus.
     */
    void emitAddToHistory(const KonqHistoryEntry &entry);

private:
    KonqHistoryProviderPrivate *const d;
    friend class KonqHistoryProviderPrivate;
};

#endif /* KONQ_HISTORYPROVIDER_H */
