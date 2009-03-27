/* This file is part of the KDE project
   Copyright 2009 Pino Toscano <pino@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqhistorymodel.h"

#include "konqhistory.h"
#include "konqhistorymanager.h"

#include <kglobal.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kprotocolinfo.h>

#include <QtCore/QHash>
#include <QtCore/QList>

namespace KHM
{

struct Entry
{
    enum Type
    {
        History,
        Group,
        Root
    };

    Entry(Type _type)
        : type(_type)
    {}

    virtual ~Entry()
    {}

    virtual QVariant data(int /*role*/, int /*column*/) const
    { return QVariant(); }

    const Type type;
};

struct HistoryEntry : public Entry
{
    HistoryEntry(const KonqHistoryEntry &_entry, GroupEntry *_parent);

    virtual QVariant data(int role, int column) const;
    void update(const KonqHistoryEntry &entry);

    KonqHistoryEntry entry;
    GroupEntry *parent;
    QIcon icon;
};

struct GroupEntry : public Entry
{
    GroupEntry(const KUrl &_url, const QString &_key);

    ~GroupEntry()
    {
        qDeleteAll(entries);
    }

    virtual QVariant data(int role, int column) const;
    HistoryEntry* findChild(const KonqHistoryEntry &entry, int *index = 0) const;
    KUrl::List urls() const;

    QList<HistoryEntry *> entries;
    KUrl url;
    QString key;
    QIcon icon;
    bool hasFavIcon : 1;
};

struct RootEntry : public Entry
{
    RootEntry()
        : Entry(Root)
    {}

    ~RootEntry()
    {
        qDeleteAll(groups);
    }

    QList<GroupEntry *> groups;
    QHash<QString, GroupEntry *> groupsByName;
};

HistoryEntry::HistoryEntry(const KonqHistoryEntry &_entry, GroupEntry *_parent)
    : Entry(History), entry(_entry), parent(_parent)
{
    parent->entries.append(this);

    update(entry);
}

QVariant HistoryEntry::data(int role, int /*column*/) const
{
    switch (role) {
    case Qt::DisplayRole: {
        QString title = entry.title;
        if (!title.trimmed().isEmpty() && title != entry.url.url()) {
        } else {
            QString path(entry.url.path());
            if (path.isEmpty()) {
                path += '/';
            }
            title = path;
        }
        return title;
    }
    case Qt::DecorationRole:
        return icon;
    case Qt::ToolTipRole:
        return entry.url.url();
    case KonqHistory::TypeRole:
        return (int)KonqHistory::HistoryType;
    case KonqHistory::DetailedToolTipRole:
        return i18n("<qt><center><b>%1</b></center><hr />Last visited: %2<br />"
                    "First visited: %3<br />Number of times visited: %4</qt>",
                    entry.url.url(),
                    KGlobal::locale()->formatDateTime(entry.lastVisited),
                    KGlobal::locale()->formatDateTime(entry.firstVisited),
                    entry.numberOfTimesVisited);
    case KonqHistory::LastVisitedRole:
        return entry.lastVisited;
    case KonqHistory::UrlRole:
        return entry.url;
    }
    return QVariant();
}

void HistoryEntry::update(const KonqHistoryEntry &_entry)
{
    entry = _entry;

    const QString path = entry.url.path();
    if (parent->hasFavIcon && (path.isNull() || path == QLatin1String("/"))) {
        icon = parent->icon;
    } else {
        icon = QIcon(SmallIcon(KProtocolInfo::icon(entry.url.protocol())));
    }
}


GroupEntry::GroupEntry(const KUrl &_url, const QString &_key)
    : Entry(Group), url(_url), key(_key), hasFavIcon(false)
{
    const QString iconPath = KMimeType::favIconForUrl(url);
    if (iconPath.isEmpty()) {
        icon = KIcon("folder");
    } else {
        icon = QIcon(SmallIcon(iconPath));
        hasFavIcon = true;
    }
}

QVariant GroupEntry::data(int role, int /*column*/) const
{
    switch (role) {
    case Qt::DisplayRole:
        return key;
    case Qt::DecorationRole:
        return icon;
    case KonqHistory::TypeRole:
        return (int)KonqHistory::GroupType;
    case KonqHistory::LastVisitedRole: {
        if (entries.isEmpty()) {
            return QDateTime();
        }
        QDateTime dt = entries.first()->entry.lastVisited;
        Q_FOREACH (HistoryEntry *e, entries) {
            if (e->entry.lastVisited > dt) {
                dt = e->entry.lastVisited;
            }
        }
        return dt;
    }
    }
    return QVariant();
}

HistoryEntry* GroupEntry::findChild(const KonqHistoryEntry &entry, int *index) const
{
    HistoryEntry *item = 0;
    QList<HistoryEntry *>::const_iterator it = entries.constBegin(), itEnd = entries.constEnd();
    int i = 0;
    for ( ; it != itEnd; ++it, ++i) {
        if ((*it)->entry.url == entry.url) {
            item = *it;
            break;
        }
    }
    if (index) {
        *index = item ? i : -1;
    }
    return item;
}

KUrl::List GroupEntry::urls() const
{
    KUrl::List list;
    Q_FOREACH (HistoryEntry *e, entries) {
        list.append(e->entry.url);
    }
    return list;
}

}


static QString groupForUrl(const KUrl &url)
{
   static const QString &misc = KGlobal::staticQString(i18n("Miscellaneous"));
   return url.host().isEmpty() ? misc : url.host();
}


KonqHistoryModel::KonqHistoryModel(QObject *parent)
    : QAbstractItemModel(parent), m_root(new KHM::RootEntry())
{
    KonqHistoryManager *manager = KonqHistoryManager::kself();

    connect(manager, SIGNAL(cleared()), this, SLOT(clear()));
    connect(manager, SIGNAL(entryAdded(const KonqHistoryEntry &)),
            this, SLOT(slotEntryAdded(const KonqHistoryEntry &)));
    connect(manager, SIGNAL(entryRemoved(const KonqHistoryEntry &)),
            this, SLOT(slotEntryRemoved(const KonqHistoryEntry &)));

    KonqHistoryList entries(manager->entries());

    KonqHistoryList::const_iterator it = entries.constBegin();
    const KonqHistoryList::const_iterator end = entries.constEnd();
    for ( ; it != end ; ++it) {
        KHM::GroupEntry *group = getGroupItem((*it).url);
        KHM::HistoryEntry *item = new KHM::HistoryEntry((*it), group);
        Q_UNUSED(item);
    }
}

KonqHistoryModel::~KonqHistoryModel()
{
    delete m_root;
}

int KonqHistoryModel::columnCount(const QModelIndex &parent) const
{
    KHM::Entry* entry = entryFromIndex(parent, true);
    switch (entry->type) {
    case KHM::Entry::History:
        return 0;
    case KHM::Entry::Group:
    case KHM::Entry::Root:
        return 1;
    }
    return 0;
}

QVariant KonqHistoryModel::data(const QModelIndex &index, int role) const
{
    KHM::Entry* entry = entryFromIndex(index);
    if (!entry) {
        return QVariant();
    }

    return entry->data(role, index.column());
}

QModelIndex KonqHistoryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    KHM::Entry* entry = entryFromIndex(parent, true);
    switch (entry->type) {
    case KHM::Entry::History:
        return QModelIndex();
    case KHM::Entry::Group: {
        const KHM::GroupEntry *ge = static_cast<KHM::GroupEntry *>(entry);
        if (row >= ge->entries.count()) {
            return QModelIndex();
        }
        return createIndex(row, column, ge->entries.at(row));
    }
    case KHM::Entry::Root: {
        const KHM::RootEntry *re = static_cast<KHM::RootEntry *>(entry);
        if (row >= re->groups.count()) {
            return QModelIndex();
        }
        return createIndex(row, column, re->groups.at(row));
    }
    }
    return QModelIndex();
}

QModelIndex KonqHistoryModel::parent(const QModelIndex &index) const
{
    KHM::Entry* entry = entryFromIndex(index);
    if (!entry) {
        return QModelIndex();
    }
    switch (entry->type) {
    case KHM::Entry::History:
        return indexFor(static_cast<KHM::HistoryEntry *>(entry)->parent);
    case KHM::Entry::Group:
    case KHM::Entry::Root:
        return QModelIndex();
    }
    return QModelIndex();
}

int KonqHistoryModel::rowCount(const QModelIndex &parent) const
{
    KHM::Entry* entry = entryFromIndex(parent, true);
    switch (entry->type) {
    case KHM::Entry::History:
        return 0;
    case KHM::Entry::Group:
        return static_cast<KHM::GroupEntry *>(entry)->entries.count();
    case KHM::Entry::Root:
        return static_cast<KHM::RootEntry *>(entry)->groups.count();
    }
    return 0;
}

void KonqHistoryModel::deleteItem(const QModelIndex &index)
{
    KHM::Entry* entry = entryFromIndex(index);
    if (!entry) {
        return;
    }

    KonqHistoryManager *manager = KonqHistoryManager::kself();
    switch (entry->type) {
    case KHM::Entry::History:
        manager->emitRemoveFromHistory(static_cast<KHM::HistoryEntry *>(entry)->entry.url);
        break;
    case KHM::Entry::Group:
        manager->emitRemoveListFromHistory(static_cast<KHM::GroupEntry *>(entry)->urls());
        break;
    case KHM::Entry::Root:
        break;
    }
}

void KonqHistoryModel::clear()
{
    if (m_root->groups.isEmpty()) {
        return;
    }

    delete m_root;
    m_root = new KHM::RootEntry();
    reset();
}

void KonqHistoryModel::slotEntryAdded(const KonqHistoryEntry &entry)
{
    KHM::GroupEntry *group = getGroupItem(entry.url);
    KHM::HistoryEntry *item = group->findChild(entry);
    if (!item) {
        beginInsertRows(indexFor(group), group->entries.count(), group->entries.count());
        item = new KHM::HistoryEntry(entry, group);
        endInsertRows();
    } else {
        item->update(entry);
        const QModelIndex index = indexFor(item);
        emit dataChanged(index, index);
        // update the parent item, so the sorting by date is updated accordingly
        const QModelIndex groupIndex = indexFor(group);
        emit dataChanged(groupIndex, groupIndex);
    }
}

void KonqHistoryModel::slotEntryRemoved(const KonqHistoryEntry &entry)
{
    const QString groupKey = groupForUrl(entry.url);
    KHM::GroupEntry *group = m_root->groupsByName.value(groupKey);
    if (!group) {
        return;
    }

    int index = 0;
    KHM::HistoryEntry *item = group->findChild(entry, &index);
    if (index == -1) {
        return;
    }

    if (group->entries.count() > 1) {
        beginRemoveRows(indexFor(group), index, index);
        group->entries.removeAt(index);
        delete item;
        endRemoveRows();
    } else {
        index = m_root->groups.indexOf(group);
        if (index == -1) {
            return;
        }

        beginRemoveRows(QModelIndex(), index, index);
        m_root->groupsByName.remove(groupKey);
        m_root->groups.removeAt(index);
        delete group;
        endRemoveRows();
    }
}

KHM::Entry* KonqHistoryModel::entryFromIndex(const QModelIndex &index, bool returnRootIfNull) const
{
    if (index.isValid()) {
        return reinterpret_cast<KHM::Entry *>(index.internalPointer());
    }
    return returnRootIfNull ? m_root : 0;
}

KHM::GroupEntry* KonqHistoryModel::getGroupItem(const KUrl &url)
{
    const QString &groupKey = groupForUrl(url);
    KHM::GroupEntry *group = m_root->groupsByName.value(groupKey);
    if (!group) {
        beginInsertRows(QModelIndex(), m_root->groups.count(), m_root->groups.count());
        group = new KHM::GroupEntry(url, groupKey);
        m_root->groups.append(group);
        m_root->groupsByName.insert(groupKey, group);
        endInsertRows();
    }

    return group;
}

QModelIndex KonqHistoryModel::indexFor(KHM::HistoryEntry *entry) const
{
    const int row = entry->parent->entries.indexOf(entry);
    if (row < 0) {
        return QModelIndex();
    }
    return createIndex(row, 0, entry);
}

QModelIndex KonqHistoryModel::indexFor(KHM::GroupEntry *entry) const
{
    const int row = m_root->groups.indexOf(entry);
    if (row < 0) {
        return QModelIndex();
    }
    return createIndex(row, 0, entry);
}

#include "konqhistorymodel.moc"
