/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KONQ_HISTORYLOADER_H
#define KONQ_HISTORYLOADER_H

#include "libkonq_export.h"
#include <QObject>

class KonqHistoryList;
class KonqHistoryLoaderPrivate;

/**
 * @internal
 * This class loads the Konqueror history file.
 * @since 4.3
 */
class KonqHistoryLoader : public QObject
{
    Q_OBJECT

public:
    explicit KonqHistoryLoader(QObject *parent = nullptr);
    ~KonqHistoryLoader() override;

    /**
     * Load the history. No need to call this more than once...
     */
    bool loadHistory();

    /**
     * @returns the list of all history entries, sorted by date
     * (oldest entries first)
     */
    const KonqHistoryList &entries() const;

    static int historyVersion();

private:
    KonqHistoryLoaderPrivate *const d;
};

#endif /* KONQ_HISTORYLOADER_H */
