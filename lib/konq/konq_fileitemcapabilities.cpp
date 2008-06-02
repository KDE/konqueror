/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2008 by George Goldberg <grundleborg@googlemail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "konq_fileitemcapabilities.h"

#include <kfileitem.h>
#include <kprotocolmanager.h>

#include <QFileInfo>

class KonqFileItemCapabilitiesPrivate : public QSharedData
{
public:
    KonqFileItemCapabilitiesPrivate()
        : m_supportsReading(false),
          m_supportsDeleting(false),
          m_supportsWriting(false),
          m_supportsMoving(false),
          m_isLocal(true)
    { }
    bool m_supportsReading : 1;
    bool m_supportsDeleting : 1;
    bool m_supportsWriting : 1;
    bool m_supportsMoving : 1;
    bool m_isLocal : 1;
};


KonqFileItemCapabilities::KonqFileItemCapabilities()
    : d(new KonqFileItemCapabilitiesPrivate)
{
}

KonqFileItemCapabilities::KonqFileItemCapabilities(const KFileItemList& items)
    : d(new KonqFileItemCapabilitiesPrivate)
{
    setItems(items);
}

void KonqFileItemCapabilities::setItems(const KFileItemList& items)
{
    const bool initialValue = !items.isEmpty();
    d->m_supportsReading = initialValue;
    d->m_supportsDeleting = initialValue;
    d->m_supportsWriting = initialValue;
    d->m_supportsMoving = initialValue;
    d->m_isLocal = true;

    QFileInfo parentDirInfo;
    foreach (const KFileItem &item, items) {
        const KUrl url = item.url();
        d->m_isLocal = d->m_isLocal && url.isLocalFile();
        d->m_supportsReading  = d->m_supportsReading  && KProtocolManager::supportsReading(url);
        d->m_supportsDeleting = d->m_supportsDeleting && KProtocolManager::supportsDeleting(url);
        d->m_supportsWriting  = d->m_supportsWriting  && KProtocolManager::supportsWriting(url) && item.isWritable();
        d->m_supportsMoving   = d->m_supportsMoving   && KProtocolManager::supportsMoving(url);

        // For local files we can do better: check if we have write permission in parent directory
        if (d->m_isLocal && (d->m_supportsDeleting || d->m_supportsMoving)) {
            const QString directory = url.directory();
            if (parentDirInfo.filePath() != directory) {
                parentDirInfo.setFile(directory);
            }
            if (!parentDirInfo.isWritable()) {
                d->m_supportsDeleting = false;
                d->m_supportsMoving = false;
            }
        }
    }
}

KonqFileItemCapabilities::KonqFileItemCapabilities(const KonqFileItemCapabilities& other)
    : d(other.d)
{ }

KonqFileItemCapabilities::~KonqFileItemCapabilities()
{
}

bool KonqFileItemCapabilities::supportsReading() const
{
    return d->m_supportsReading;
}

bool KonqFileItemCapabilities::supportsDeleting() const
{
    return d->m_supportsDeleting;
}

bool KonqFileItemCapabilities::supportsWriting() const
{
    return d->m_supportsWriting;
}

bool KonqFileItemCapabilities::supportsMoving() const
{
    return d->m_supportsMoving;
}

bool KonqFileItemCapabilities::isLocal() const
{
    return d->m_isLocal;
}
