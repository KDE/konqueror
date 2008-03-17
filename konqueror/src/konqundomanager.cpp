/* This file is part of the KDE project
   Copyright 2007 David Faure <faure@kde.org>
   Copyright 2007 Eduardo Robles Elvira <edulix@gmail.com>

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

#include "konqundomanager.h"
#include <QAction>
#include "konqclosedtabitem.h"
#include <konq_fileundomanager.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>

static unsigned long s_konqUndoManagerRefCnt = 0;

KonqUndoManager::KonqUndoManager(QObject* parent)
    : QObject(parent)
{
    incRef();
    connect( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    connect( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );

    connect(KonqFileUndoManager::self(), SIGNAL(bypassCustomInfo(QVariant &)),
    this, SLOT( slotBypassCustomInfo(QVariant &) ) );
    
    connect(this, SIGNAL( bypassCustomInfo( QVariant &) ),
    KonqFileUndoManager::self(), SIGNAL( bypassCustomInfo( QVariant &) ) );
}

KonqUndoManager::~KonqUndoManager()
{
    disconnect( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    disconnect( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );

    disconnect(KonqFileUndoManager::self(), SIGNAL(bypassCustomInfo(QVariant &)),
    this, SLOT( slotBypassCustomInfo(QVariant &) ) );
    
    disconnect(this, SIGNAL( bypassCustomInfo( QVariant &) ),
        KonqFileUndoManager::self(), SIGNAL( bypassCustomInfo( QVariant &) ) );
    decRef();
    // Important! do decRef() before doing clearClosedItemsList() here:
    clearClosedItemsList();
}

void KonqUndoManager::incRef()
{
    kDebug();
    KonqFileUndoManager::incRef();
    s_konqUndoManagerRefCnt++;
    kDebug() << s_konqUndoManagerRefCnt; 
}

void KonqUndoManager::decRef()
{
    kDebug();
    KonqFileUndoManager::decRef();
    s_konqUndoManagerRefCnt--;
    
}

void KonqUndoManager::slotFileUndoAvailable(bool)
{
    emit undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    if (!m_closedItemList.isEmpty())
        return true;
    else
        return (m_supportsFileUndo && KonqFileUndoManager::self()->undoAvailable());
}

QString KonqUndoManager::undoText() const
{
    if (!m_closedItemList.isEmpty()) {
        const KonqClosedItem* closedItem = m_closedItemList.first();
        if (closedItem->serialNumber() > KonqFileUndoManager::self()->currentCommandSerialNumber()) {
            const KonqClosedTabItem* closedTabItem =
                dynamic_cast<const KonqClosedTabItem *>(closedItem);
            if(closedTabItem)
                return i18n("Und&o: Closed Tab");
            else
                return i18n("Und&o: Closed Window");
        }
    }
    return KonqFileUndoManager::self()->undoText();
}

void KonqUndoManager::undo()
{
    if (!m_closedItemList.isEmpty()) {
        const KonqClosedItem* closedItem = m_closedItemList.first();

        // Check what to undo
        kDebug() << "closedTabItem->serialNumber:" << closedItem->serialNumber()
                 << ", KonqFileUndoManager currentCommandSerialNumber(): " << KonqFileUndoManager::self()->currentCommandSerialNumber();

        if (closedItem->serialNumber() > KonqFileUndoManager::self()->currentCommandSerialNumber()) {
            m_closedItemList.removeFirst();
            const KonqClosedTabItem* closedTabItem =
                dynamic_cast<const KonqClosedTabItem *>(closedItem);
            const KonqClosedWindowItem* closedWindowItem =
                dynamic_cast<const KonqClosedWindowItem *>(closedItem);
            if(closedTabItem)
                emit openClosedTab(*closedTabItem);
            else if(closedWindowItem) {
                emit openClosedWindow(*closedWindowItem);
                removeWindowInOtherInstances(closedWindowItem);
            }
            delete closedItem;
            emit undoAvailable(this->undoAvailable());
            emit closedItemsListChanged();
        } else {
            KonqFileUndoManager::self()->undo();
        }
    } else {
        KonqFileUndoManager::self()->undo();
    }
}

void KonqUndoManager::removeWindowInOtherInstances(const KonqClosedWindowItem
*closedWindowItem)
{
    QList<QVariant> items;
    QString order("remove");
    const QObject *myClosedWindowItem =
        reinterpret_cast<const QObject *>(closedWindowItem);
    items.append(qVariantFromValue(reinterpret_cast<QObject *>(this)));
    items.append(qVariantFromValue(order));
    items.append(qVariantFromValue(const_cast<QObject *>(myClosedWindowItem)));
    QVariant myData(items);
    emit bypassCustomInfo(myData);
}

void KonqUndoManager::addWindowInOtherInstances(const KonqClosedWindowItem
*closedWindowItem)
{
    QList<QVariant> items;
    QString order("add");
    const QObject *myClosedWindowItem =
        reinterpret_cast<const QObject *>(closedWindowItem);
    items.append(qVariantFromValue(reinterpret_cast<QObject *>(this)));
    items.append(qVariantFromValue(order));
    items.append(qVariantFromValue(const_cast<QObject *>(myClosedWindowItem)));
    QVariant myData(items);
    emit bypassCustomInfo(myData);
}

void KonqUndoManager::slotBypassCustomInfo(QVariant &customData)
{
    QList<QVariant> items = customData.toList();
    KonqUndoManager *undoManager =
        static_cast<KonqUndoManager *>(items.first().value<QObject*>());
    QString order = items.at(1).toString();
    KonqClosedWindowItem *closedWindowItem =
        static_cast<KonqClosedWindowItem *>(items.at(2).value<QObject*>());
    if(undoManager != this)
    {
        if(order == "remove")
        {
            QList<KonqClosedItem *>::iterator it = qFind(m_closedItemList.begin(), m_closedItemList.end(), closedWindowItem);
            
            // If the item was found, remove it from the list
            if(it != m_closedItemList.end()) {
                m_closedItemList.erase(it);
                emit undoAvailable(this->undoAvailable());
                emit closedItemsListChanged();
            }
        } else if (order == "add")
        {
            addClosedWindowItem(closedWindowItem);
        }
    }
}

const QList<KonqClosedItem *>& KonqUndoManager::closedTabsList() const
{
    return m_closedItemList;
}

void KonqUndoManager::undoClosedItem(int index)
{
    Q_ASSERT(!m_closedItemList.isEmpty());
    KonqClosedItem* closedItem = m_closedItemList.at( index );
    m_closedItemList.removeAt(index);
        
    const KonqClosedTabItem* closedTabItem =
        dynamic_cast<const KonqClosedTabItem *>(closedItem);
    const KonqClosedWindowItem* closedWindowItem =
        dynamic_cast<const KonqClosedWindowItem *>(closedItem);
    if(closedTabItem)
        emit openClosedTab(*closedTabItem);
    else if(closedWindowItem) {
        emit openClosedWindow(*closedWindowItem);
        removeWindowInOtherInstances(closedWindowItem);
    }
    delete closedTabItem;
    emit undoAvailable(this->undoAvailable());
    emit undoTextChanged(this->undoText());
    emit closedItemsListChanged();
}

void KonqUndoManager::slotClosedItemsActivated(QAction* action)
{
    // Open a closed tab
    const int index = action->data().toInt();
    undoClosedItem(index);
}

void KonqUndoManager::slotFileUndoTextChanged(const QString& text)
{
    // I guess we can always just forward that one?
    emit undoTextChanged(text);
}

quint64 KonqUndoManager::newCommandSerialNumber()
{
    return KonqFileUndoManager::self()->newCommandSerialNumber();
}

void KonqUndoManager::addClosedTabItem(KonqClosedTabItem* closedTabItem)
{
    m_closedItemList.prepend(closedTabItem);
    emit undoTextChanged(i18n("Und&o: Closed Tab"));
    emit undoAvailable(true);
}

void KonqUndoManager::addClosedWindowItem(KonqClosedWindowItem* closedWindowItem)
{
    m_closedItemList.prepend(closedWindowItem);
    emit undoTextChanged(i18n("Und&o: Closed Window"));
    emit undoAvailable(true);
    emit closedItemsListChanged();
}

void KonqUndoManager::updateSupportsFileUndo(bool enable)
{
    m_supportsFileUndo = enable;
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::clearClosedItemsList()
{
// DELETE only tab items! So we can't do this anymore:
//     qDeleteAll(m_closedItemList);
    QList<KonqClosedItem *>::iterator it = m_closedItemList.begin();
    for (; it != m_closedItemList.end(); ++it)
    {
        KonqClosedItem *closedItem = *it;
        const KonqClosedTabItem* closedTabItem =
            dynamic_cast<const KonqClosedTabItem *>(closedItem);
        const KonqClosedWindowItem* closedWindowItem =
            dynamic_cast<const KonqClosedWindowItem *>(closedItem);
        
        m_closedItemList.erase(it);
        if(closedTabItem)
            delete closedTabItem;
        else if(s_konqUndoManagerRefCnt == 0 && closedWindowItem)
            // delete closed windows only if this is the last window and it's
            // being closed too, see destructor.
            delete closedWindowItem;
    }
    emit closedItemsListChanged();
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::undoLastClosedItem()
{
    undoClosedItem(0);
}
