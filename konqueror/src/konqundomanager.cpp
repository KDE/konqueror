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
#include <QTimer>
#include <konq_fileundomanager.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>

class KonqUndoManagerCommunicatorPrivate {
public:
    KonqUndoManagerCommunicator instance;
    QList<KonqClosedWindowItem *> m_closedWindowItemList;
    int m_maxNumClosedItems;
};

K_GLOBAL_STATIC(KonqUndoManagerCommunicatorPrivate, myKonqUndoManagerCommunicatorPrivate)

KonqUndoManager::KonqUndoManager(QObject* parent)
    : QObject(parent)
{
    KonqFileUndoManager::incRef();
    connect( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    connect( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );

    connect(KonqUndoManagerCommunicator::self(),
        SIGNAL(addWindowInOtherInstances(KonqUndoManager *, KonqClosedWindowItem *)), this,
        SLOT( slotAddClosedWindowItem(KonqUndoManager *, KonqClosedWindowItem *) ));
    connect(KonqUndoManagerCommunicator::self(),
        SIGNAL(removeWindowInOtherInstances(KonqUndoManager *, const KonqClosedWindowItem *)), this,
        SLOT( slotRemoveClosedWindowItem(KonqUndoManager *, const KonqClosedWindowItem *) ));

    populate();
}

KonqUndoManager::~KonqUndoManager()
{
    disconnect( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    disconnect( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );

    disconnect(KonqUndoManagerCommunicator::self(),
        SIGNAL(addWindowInOtherInstances(KonqUndoManager *, KonqClosedWindowItem *)), this,
        SLOT( slotAddClosedWindowItem(KonqUndoManager *, KonqClosedWindowItem *) ));
    disconnect(KonqUndoManagerCommunicator::self(),
        SIGNAL(removeWindowInOtherInstances(KonqUndoManager *, const KonqClosedWindowItem *)), this,
        SLOT( slotRemoveClosedWindowItem(KonqUndoManager *, const KonqClosedWindowItem *) ));

    KonqFileUndoManager::decRef();
    clearClosedItemsList();
}

void KonqUndoManager::populate()
{
    kDebug();
    const QList<KonqClosedWindowItem *> closedWindowItemList = 
        KonqUndoManagerCommunicator::self()->closedWindowItemList();

    foreach(KonqClosedWindowItem *closedWindowItem, closedWindowItemList)
        slotAddClosedWindowItem(0L, closedWindowItem);
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
                KonqUndoManagerCommunicator::self()->removeClosedWindowItem(this, closedWindowItem);
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

void KonqUndoManager::slotAddClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem *closedWindowItem)
{
    if(real_sender == this)
        return;
    
    if(m_closedItemList.size() >= KonqUndoManagerCommunicator::self()->maxNumClosedItems())
    {
        const KonqClosedItem* last = m_closedItemList.last();
        const KonqClosedTabItem* lastTab =
            dynamic_cast<const KonqClosedTabItem *>(last);
        m_closedItemList.removeLast();
        
        // Delete only if it's a tab
        if(lastTab)
            delete lastTab;
    }
    
    kDebug();
    m_closedItemList.prepend(closedWindowItem);
    emit undoTextChanged(i18n("Und&o: Closed Window"));
    emit undoAvailable(true);
    emit closedItemsListChanged();
}

void KonqUndoManager::addClosedWindowItem(KonqClosedWindowItem *closedWindowItem)
{
    KonqUndoManagerCommunicator::self()->addClosedWindowItem(this, closedWindowItem);
}

void KonqUndoManager::slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const KonqClosedWindowItem *closedWindowItem)
{
    if(real_sender == this)
        return;
    
    QList<KonqClosedItem *>::iterator it = qFind(m_closedItemList.begin(), m_closedItemList.end(), closedWindowItem);
            
    // If the item was found, remove it from the list
    if(it != m_closedItemList.end()) {
        m_closedItemList.erase(it);
        emit undoAvailable(this->undoAvailable());
        emit closedItemsListChanged();
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
        emit removeWindowInOtherInstances(this, closedWindowItem);
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
    if(m_closedItemList.size() >= KonqUndoManagerCommunicator::self()->maxNumClosedItems())
    {
        const KonqClosedItem* last = m_closedItemList.last();
        const KonqClosedTabItem* lastTab =
            dynamic_cast<const KonqClosedTabItem *>(last);
        m_closedItemList.removeLast();
        
        // Delete only if it's a tab
        if(lastTab)
            delete lastTab;
    }
    
    m_closedItemList.prepend(closedTabItem);
    emit undoTextChanged(i18n("Und&o: Closed Tab"));
    emit undoAvailable(true);
}

void KonqUndoManager::updateSupportsFileUndo(bool enable)
{
    m_supportsFileUndo = enable;
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::clearClosedItemsList()
{
// we only DELETE tab items! So we can't do this anymore:
//     qDeleteAll(m_closedItemList);
    QList<KonqClosedItem *>::iterator it = m_closedItemList.begin();
    for (; it != m_closedItemList.end(); ++it)
    {
        KonqClosedItem *closedItem = *it;
        const KonqClosedTabItem* closedTabItem =
            dynamic_cast<const KonqClosedTabItem *>(closedItem);
//         const KonqClosedWindowItem* closedWindowItem =
//             dynamic_cast<const KonqClosedWindowItem *>(closedItem);
        
        m_closedItemList.erase(it);
        if(closedTabItem)
            delete closedTabItem;
        // we never delete closed window items here
    }
    emit closedItemsListChanged();
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::undoLastClosedItem()
{
    undoClosedItem(0);
}

KonqUndoManagerCommunicator::KonqUndoManagerCommunicator()
{
    QTimer::singleShot(0, this, SLOT(readSettings()));
}

KonqUndoManagerCommunicator::~KonqUndoManagerCommunicator()
{
}

KonqUndoManagerCommunicator *KonqUndoManagerCommunicator::self()
{
    return &myKonqUndoManagerCommunicatorPrivate->instance;
}

void KonqUndoManagerCommunicator::addClosedWindowItem(KonqUndoManager
*real_sender, KonqClosedWindowItem *closedWindowItem)
{
    // If we are off the limit, remove the last closed window item
    if(myKonqUndoManagerCommunicatorPrivate->m_closedWindowItemList.size() >= 
        maxNumClosedItems())
    {
        QList<KonqClosedWindowItem *> &closedWindowItemList =
        myKonqUndoManagerCommunicatorPrivate->m_closedWindowItemList;
        KonqClosedWindowItem* last = closedWindowItemList.last();
        emit removeWindowInOtherInstances(0L, last);
        closedWindowItemList.removeLast();
        delete last;
    }
    
    myKonqUndoManagerCommunicatorPrivate->m_closedWindowItemList.prepend(closedWindowItem);
    emit addWindowInOtherInstances(real_sender, closedWindowItem);
}

void KonqUndoManagerCommunicator::removeClosedWindowItem(KonqUndoManager
*real_sender, const KonqClosedWindowItem *closedWindowItem)
{
    QList<KonqClosedWindowItem *> &closedWindowItemList =
        myKonqUndoManagerCommunicatorPrivate->m_closedWindowItemList;
    QList<KonqClosedWindowItem *>::iterator it = qFind(closedWindowItemList.begin(),
    closedWindowItemList.end(), closedWindowItem);
            
    // If the item was found, remove it from the list
    if(it != closedWindowItemList.end()) {
        closedWindowItemList.erase(it);
    }
    emit removeWindowInOtherInstances(real_sender, closedWindowItem);
}

const QList<KonqClosedWindowItem *>& KonqUndoManagerCommunicator::closedWindowItemList()
{
    return myKonqUndoManagerCommunicatorPrivate->m_closedWindowItemList;
}

int KonqUndoManagerCommunicator::maxNumClosedItems()
{
    return myKonqUndoManagerCommunicatorPrivate->m_maxNumClosedItems;
}

void KonqUndoManagerCommunicator::setMaxNumClosedItems(int max)
{
    myKonqUndoManagerCommunicatorPrivate->m_maxNumClosedItems = qMax(1, max);
}

void KonqUndoManagerCommunicator::readSettings(bool global)
{
    KSharedConfigPtr config;

    if (global)
      config = KGlobal::config();
    else
      config = KSharedConfig::openConfig("konquerorrc");

    KConfigGroup configGroup( config, "UndoManagerSettings");
    myKonqUndoManagerCommunicatorPrivate->m_maxNumClosedItems = configGroup.readEntry("Maximum number of Closed Items", 20 );
    myKonqUndoManagerCommunicatorPrivate->m_maxNumClosedItems = qMax(1, myKonqUndoManagerCommunicatorPrivate->m_maxNumClosedItems);
}

void KonqUndoManagerCommunicator::applySettings()
{
    KConfigGroup configGroup(KSharedConfig::openConfig("konquerorrc"), "UndoManagerSettings");

    configGroup.writeEntry("Value youngerThan", myKonqUndoManagerCommunicatorPrivate->m_maxNumClosedItems );
}
