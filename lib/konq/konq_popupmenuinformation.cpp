/* This file is part of the KDE project

   Copyright 2008 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_popupmenuinformation.h"
#include <kfileitem.h>

class KonqPopupMenuInformationPrivate : public QSharedData
{
public:
    KonqPopupMenuInformationPrivate()
        : m_parentWidget(0),
      m_isDirectory(false)
    {}
    QWidget* m_parentWidget;
    KFileItemList m_items;
    KonqFileItemCapabilities m_capabilities;
    KUrl::List m_urlList;
    QString m_mimeType;
    QString m_mimeGroup;
    bool m_isDirectory;
};

KonqPopupMenuInformation::KonqPopupMenuInformation()
    : d(new KonqPopupMenuInformationPrivate)
{
}

KonqPopupMenuInformation::~KonqPopupMenuInformation()
{
}

KonqPopupMenuInformation::KonqPopupMenuInformation(const KonqPopupMenuInformation& other)
    : d(other.d)
{
}

KonqPopupMenuInformation & KonqPopupMenuInformation::operator=(const KonqPopupMenuInformation& other)
{
    d = other.d;
    return *this;
}

void KonqPopupMenuInformation::setItems(const KFileItemList& items)
{
    Q_ASSERT(!items.isEmpty());
    d->m_items = items;
    d->m_capabilities.setItems(items);
    d->m_mimeType = items.first().mimetype();
    d->m_mimeGroup = d->m_mimeType.left(d->m_mimeType.indexOf('/'));
    d->m_isDirectory = items.first().isDir();
    d->m_urlList = items.urlList();
    if (items.count() > 1) {
        KFileItemList::const_iterator kit = items.begin();
        const KFileItemList::const_iterator kend = items.end();
        for ( ; kit != kend; ++kit ) {
            const QString itemMimeType = (*kit).mimetype();
            // Determine if common mimetype among all items
            if (d->m_mimeType != itemMimeType) {
                d->m_mimeType.clear();
                if (d->m_mimeGroup != itemMimeType.left(itemMimeType.indexOf('/')))
                    d->m_mimeGroup.clear(); // mimetype groups are different as well!
            }
            if (d->m_isDirectory && !(*kit).isDir())
                d->m_isDirectory = false;
        }
    }
}

KFileItemList KonqPopupMenuInformation::items() const
{
    return d->m_items;
}

KUrl::List KonqPopupMenuInformation::urlList() const
{
    return d->m_urlList;
}

bool KonqPopupMenuInformation::isDirectory() const
{
    return d->m_isDirectory;
}

void KonqPopupMenuInformation::setParentWidget(QWidget* parentWidget)
{
    d->m_parentWidget = parentWidget;
}

QWidget* KonqPopupMenuInformation::parentWidget() const
{
    return d->m_parentWidget;
}

QString KonqPopupMenuInformation::mimeType() const
{
    return d->m_mimeType;
}

QString KonqPopupMenuInformation::mimeGroup() const
{
    return d->m_mimeGroup;
}

KonqFileItemCapabilities KonqPopupMenuInformation::capabilities() const
{
    return d->m_capabilities;
}
