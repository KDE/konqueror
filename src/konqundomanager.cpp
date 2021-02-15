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
#include "konqsettingsxt.h"
#include "konqcloseditem.h"
#include "konqclosedwindowsmanager.h"
#include <QAction>
#include <kio/fileundomanager.h>
#include "konqdebug.h"
#include <KLocalizedString>

KonqUndoManager::KonqUndoManager(KonqClosedWindowsManager *cwManager, QWidget *parent)
    : QObject(parent), m_cwManager(cwManager)
{
    connect(KIO::FileUndoManager::self(), SIGNAL(undoAvailable(bool)),
            this, SLOT(slotFileUndoAvailable(bool)));
    connect(KIO::FileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
            this, SLOT(slotFileUndoTextChanged(QString)));

    connect(m_cwManager, SIGNAL(addWindowInOtherInstances(KonqUndoManager*,KonqClosedWindowItem*)),
            this, SLOT(slotAddClosedWindowItem(KonqUndoManager*,KonqClosedWindowItem*)));
    connect(m_cwManager, SIGNAL(removeWindowInOtherInstances(KonqUndoManager*,const KonqClosedWindowItem*)),
            this, SLOT(slotRemoveClosedWindowItem(KonqUndoManager*,const KonqClosedWindowItem*)));
}

KonqUndoManager::~KonqUndoManager()
{
    disconnect(KIO::FileUndoManager::self(), SIGNAL(undoAvailable(bool)),
               this, SLOT(slotFileUndoAvailable(bool)));
    disconnect(KIO::FileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
               this, SLOT(slotFileUndoTextChanged(QString)));

    disconnect(m_cwManager, SIGNAL(addWindowInOtherInstances(KonqUndoManager*,KonqClosedWindowItem*)),
               this, SLOT(slotAddClosedWindowItem(KonqUndoManager*,KonqClosedWindowItem*)));
    disconnect(m_cwManager, SIGNAL(removeWindowInOtherInstances(KonqUndoManager*,const KonqClosedWindowItem*)),
               this, SLOT(slotRemoveClosedWindowItem(KonqUndoManager*,const KonqClosedWindowItem*)));

    // Clear the closed item lists but only remove closed windows items
    // in this window
    clearClosedItemsList(true);
}

void KonqUndoManager::populate()
{
    if (m_populated) {
        return;
    }
    m_populated = true;

    const QList<KonqClosedWindowItem *> closedWindowItemList =
        m_cwManager->closedWindowItemList();

    QListIterator<KonqClosedWindowItem *> i(closedWindowItemList);

    // This loop is done backwards because slotAddClosedWindowItem prepends the
    // elements to the list, so if we do it forwards the we would get an inverse
    // order of closed windows
    for (i.toBack(); i.hasPrevious();) {
        slotAddClosedWindowItem(nullptr, i.previous());
    }
}

void KonqUndoManager::slotFileUndoAvailable(bool)
{
    Q_EMIT undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    if (!m_closedItemList.isEmpty() || m_cwManager->undoAvailable()) {
        return true;
    } else {
        return (m_supportsFileUndo && KIO::FileUndoManager::self()->undoAvailable());
    }
}

QString KonqUndoManager::undoText() const
{
    if (!m_closedItemList.isEmpty()) {
        const KonqClosedItem *closedItem = m_closedItemList.first();
        if (!m_supportsFileUndo || !KIO::FileUndoManager::self()->undoAvailable() || closedItem->serialNumber() > KIO::FileUndoManager::self()->currentCommandSerialNumber()) {
            const KonqClosedTabItem *closedTabItem =
                dynamic_cast<const KonqClosedTabItem *>(closedItem);
            if (closedTabItem) {
                return i18n("Und&o: Closed Tab");
            } else {
                return i18n("Und&o: Closed Window");
            }
        } else {
            return KIO::FileUndoManager::self()->undoText();
        }

    } else if (m_supportsFileUndo && KIO::FileUndoManager::self()->undoAvailable()) {
        return KIO::FileUndoManager::self()->undoText();
    }

    else if (m_cwManager->undoAvailable()) {
        return i18n("Und&o: Closed Window");
    } else {
        return i18n("Und&o");
    }
}

void KonqUndoManager::undo()
{
    populate();
    KIO::FileUndoManager *fileUndoManager = KIO::FileUndoManager::self();
    if (!m_closedItemList.isEmpty()) {
        KonqClosedItem *closedItem = m_closedItemList.first();

        // Check what to undo
        if (!m_supportsFileUndo || !fileUndoManager->undoAvailable() || closedItem->serialNumber() > fileUndoManager->currentCommandSerialNumber()) {
            undoClosedItem(0);
            return;
        }
    }
    fileUndoManager->uiInterface()->setParentWidget(qobject_cast<QWidget *>(parent()));
    fileUndoManager->undo();
}

void KonqUndoManager::slotAddClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem *closedWindowItem)
{
    if (real_sender == this) {
        return;
    }

    populate();

    if (m_closedItemList.size() >= KonqSettings::maxNumClosedItems()) {
        const KonqClosedItem *last = m_closedItemList.last();
        const KonqClosedTabItem *lastTab =
            dynamic_cast<const KonqClosedTabItem *>(last);
        m_closedItemList.removeLast();

        // Delete only if it's a tab
        if (lastTab) {
            delete lastTab;
        }
    }

    m_closedItemList.prepend(closedWindowItem);
    Q_EMIT undoTextChanged(i18n("Und&o: Closed Window"));
    Q_EMIT undoAvailable(true);
    Q_EMIT closedItemsListChanged();
}

void KonqUndoManager::addClosedWindowItem(KonqClosedWindowItem *closedWindowItem)
{
    populate();
    m_cwManager->addClosedWindowItem(this, closedWindowItem);
}

void KonqUndoManager::slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const KonqClosedWindowItem *closedWindowItem)
{
    if (real_sender == this) {
        return;
    }

    populate();

    QList<KonqClosedItem *>::iterator it = std::find(m_closedItemList.begin(), m_closedItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if (it != m_closedItemList.end()) {
        m_closedItemList.erase(it);
        Q_EMIT undoAvailable(this->undoAvailable());
        Q_EMIT closedItemsListChanged();
    }
}

const QList<KonqClosedItem *> &KonqUndoManager::closedItemsList()
{
    populate();
    return m_closedItemList;
}

void KonqUndoManager::undoClosedItem(int index)
{
    populate();
    Q_ASSERT(!m_closedItemList.isEmpty());
    KonqClosedItem *closedItem = m_closedItemList.at(index);
    m_closedItemList.removeAt(index);

    const KonqClosedTabItem *closedTabItem =
        dynamic_cast<const KonqClosedTabItem *>(closedItem);
    KonqClosedRemoteWindowItem *closedRemoteWindowItem =
        dynamic_cast<KonqClosedRemoteWindowItem *>(closedItem);
    KonqClosedWindowItem *closedWindowItem =
        dynamic_cast<KonqClosedWindowItem *>(closedItem);
    if (closedTabItem) {
        Q_EMIT openClosedTab(*closedTabItem);
    } else if (closedRemoteWindowItem) {
        m_cwManager->removeClosedWindowItem(this, closedRemoteWindowItem);
        Q_EMIT openClosedWindow(*closedRemoteWindowItem);
    } else if (closedWindowItem) {
        m_cwManager->removeClosedWindowItem(this, closedWindowItem);
        Q_EMIT openClosedWindow(*closedWindowItem);
        closedWindowItem->configGroup().deleteGroup();

        // Save config so that this window won't appear in new konqueror processes
        m_cwManager->saveConfig();
    }
    delete closedItem;
    Q_EMIT undoAvailable(this->undoAvailable());
    Q_EMIT undoTextChanged(this->undoText());
    Q_EMIT closedItemsListChanged();
}

void KonqUndoManager::slotClosedItemsActivated(QAction *action)
{
    // Open a closed tab
    const int index = action->data().toInt();
    undoClosedItem(index);
}

void KonqUndoManager::slotFileUndoTextChanged(const QString & /*text*/)
{
    // We will forward this call but changing the text, because if for example
    // there' no more files to undo, the text will be "Und&o" but maybe
    // we want it to be "Und&o: Closed Tab" if we have a closed tab that can be
    // reopened.
    Q_EMIT undoTextChanged(undoText());
}

quint64 KonqUndoManager::newCommandSerialNumber()
{
    return KIO::FileUndoManager::self()->newCommandSerialNumber();
}

void KonqUndoManager::addClosedTabItem(KonqClosedTabItem *closedTabItem)
{
    populate();

    if (m_closedItemList.size() >= KonqSettings::maxNumClosedItems()) {
        const KonqClosedItem *last = m_closedItemList.last();
        const KonqClosedTabItem *lastTab =
            dynamic_cast<const KonqClosedTabItem *>(last);
        m_closedItemList.removeLast();

        // Delete only if it's a tab
        if (lastTab) {
            delete lastTab;
        }
    }

    m_closedItemList.prepend(closedTabItem);
    Q_EMIT undoTextChanged(i18n("Und&o: Closed Tab"));
    Q_EMIT undoAvailable(true);
}

void KonqUndoManager::updateSupportsFileUndo(bool enable)
{
    m_supportsFileUndo = enable;
    Q_EMIT undoAvailable(this->undoAvailable());
}

void KonqUndoManager::clearClosedItemsList(bool onlyInthisWindow)
{
    populate();
    QList<KonqClosedItem *>::iterator it = m_closedItemList.begin();
    for (; it != m_closedItemList.end(); ++it) {
        KonqClosedItem *closedItem = *it;
        const KonqClosedTabItem *closedTabItem =
            dynamic_cast<const KonqClosedTabItem *>(closedItem);
        const KonqClosedWindowItem *closedWindowItem =
            dynamic_cast<const KonqClosedWindowItem *>(closedItem);

        m_closedItemList.erase(it);
        if (closedTabItem) {
            delete closedTabItem;
        } else if (closedWindowItem && !onlyInthisWindow) {
            m_cwManager->removeClosedWindowItem(this, closedWindowItem, true);
            delete closedWindowItem;
        }
    }

    Q_EMIT closedItemsListChanged();
    Q_EMIT undoAvailable(this->undoAvailable());

    // Save config so that this window won't appear in new konqueror processes
    m_cwManager->saveConfig();
}

void KonqUndoManager::undoLastClosedItem()
{
    undoClosedItem(0);
}

