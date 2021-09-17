/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "konq_historyloader_p.h"
#include "konq_historyentry.h"

#include <QDataStream>
#include <QFile>
#include <QStandardPaths>

#include <zlib.h> // for crc32

#include "libkonq_debug.h"

class KonqHistoryLoaderPrivate
{
public:
    KonqHistoryList m_history;
};

KonqHistoryLoader::KonqHistoryLoader(QObject *parent)
    : QObject(parent), d(new KonqHistoryLoaderPrivate)
{
    loadHistory();
}

KonqHistoryLoader::~KonqHistoryLoader()
{
    delete d;
}

/**
 * Ensures that the items are sorted by the lastVisited date
 * (oldest goes first)
 */
static bool lastVisitedOrder(const KonqHistoryEntry &lhs, const KonqHistoryEntry &rhs)
{
    return lhs.lastVisited < rhs.lastVisited;
}

bool KonqHistoryLoader::loadHistory()
{
    d->m_history.clear();

    const QString filename = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/konq_history");
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        if (file.exists()) {
            qCWarning(LIBKONQ_LOG) << "Can't open" << filename;
        }
        return false;
    }

    QDataStream fileStream(&file);
    QByteArray data; // only used for version == 2
    // we construct the stream object now but fill in the data later.
    QDataStream crcStream(&data, QIODevice::ReadOnly);
    KonqHistoryEntry::Flags flags = KonqHistoryEntry::NoFlags;

    if (!fileStream.atEnd()) {
        quint32 version;
        fileStream >> version;

        QDataStream *stream = &fileStream;

        bool crcChecked = false;
        bool crcOk = false;

        if (version >= 2 && version <= 4) {
            quint32 crc;
            crcChecked = true;
            fileStream >> crc >> data;
            crcOk = crc32(0, reinterpret_cast<unsigned char *>(data.data()), data.size()) == crc;
            stream = &crcStream; // pick up the right stream
        }

        // We can't read v3 history anymore, because operator<<(KURL) disappeared.

        if (version == 4) {
            // Use QUrl marshalling for V4 format.
            flags = KonqHistoryEntry::NoFlags;
        }

#if 0 // who cares for versions 1 and 2 nowadays...
        if (version != 0 && version < 3) { //Versions 1,2 (but not 0) are also valid
            //Turn on backwards compatibility mode..
            marshalURLAsStrings = true;
            // it doesn't make sense to save to save maxAge and maxCount  in the
            // binary file, this would make backups impossible (they would clear
            // themselves on startup, because all entries expire).
            // [But V1 and V2 formats did it, so we do a dummy read]
            quint32 dummy;
            *stream >> dummy;
            *stream >> dummy;

            //OK.
            version = 3;
        }
#endif

        if (historyVersion() != int(version) || (crcChecked && !crcOk)) {
            qCWarning(LIBKONQ_LOG) << "The history version doesn't match, aborting loading";
            file.close();
            return false;
        }

        while (!stream->atEnd()) {
            KonqHistoryEntry entry;
            entry.load(*stream, flags);
            // qCDebug(LIBKONQ_LOG) << "loaded entry:" << entry.url << ", Title:" << entry.title;
            d->m_history.append(entry);
        }

        //qCDebug(LIBKONQ_LOG) << "loaded:" << m_history.count() << "entries.";

        std::sort(d->m_history.begin(), d->m_history.end(), lastVisitedOrder);
    }

    // Theoretically, we should emit update() here, but as we only ever
    // load items on startup up to now, this doesn't make much sense.
    // emit KParts::HistoryProvider::update(some list);
    return true;
}

const KonqHistoryList &KonqHistoryLoader::entries() const
{
    return d->m_history;
}

int KonqHistoryLoader::historyVersion()
{
    return 4;
}
