/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqundomanager.h"
#include "konqsettings.h"
#include "konqcloseditem.h"
#include "konqclosedwindowsmanager.h"
#include <QAction>
#include <kio/fileundomanager.h>
#include "konqdebug.h"
#include <KLocalizedString>
#include <QWidget>

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
    emit undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    if (!m_closedItemList.isEmpty() || m_cwManager->undoAvailable()) {
        return true;
    } else {
        return (m_supportsFileUndo && KIO::FileUndoManager::self()->isUndoAvailable());
    }
}

QString KonqUndoManager::undoText() const
{
    if (!m_closedItemList.isEmpty()) {
        const KonqClosedItem *closedItem = m_closedItemList.first();
        if (!m_supportsFileUndo || !KIO::FileUndoManager::self()->isUndoAvailable() || closedItem->serialNumber() > KIO::FileUndoManager::self()->currentCommandSerialNumber()) {
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

    } else if (m_supportsFileUndo && KIO::FileUndoManager::self()->isUndoAvailable()) {
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
        if (!m_supportsFileUndo || !fileUndoManager->isUndoAvailable() || closedItem->serialNumber() > fileUndoManager->currentCommandSerialNumber()) {
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

    if (m_closedItemList.size() >= Konq::Settings::maxNumClosedItems()) {
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
    emit undoTextChanged(i18n("Und&o: Closed Window"));
    emit undoAvailable(true);
    emit closedItemsListChanged();
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
        emit undoAvailable(this->undoAvailable());
        emit closedItemsListChanged();
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
    KonqClosedWindowItem *closedWindowItem =
        dynamic_cast<KonqClosedWindowItem *>(closedItem);
    if (closedTabItem) {
        emit openClosedTab(*closedTabItem);
    } else if (closedWindowItem) {
        m_cwManager->removeClosedWindowItem(this, closedWindowItem);
        emit openClosedWindow(*closedWindowItem);
        closedWindowItem->configGroup().deleteGroup();

        // Save config so that this window won't appear in new konqueror processes
        m_cwManager->saveConfig();
    }
    delete closedItem;
    emit undoAvailable(this->undoAvailable());
    emit undoTextChanged(this->undoText());
    emit closedItemsListChanged();
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
    emit undoTextChanged(undoText());
}

quint64 KonqUndoManager::newCommandSerialNumber()
{
    return KIO::FileUndoManager::self()->newCommandSerialNumber();
}

void KonqUndoManager::addClosedTabItem(KonqClosedTabItem *closedTabItem)
{
    populate();

    if (m_closedItemList.size() >= Konq::Settings::maxNumClosedItems()) {
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
    emit undoTextChanged(i18n("Und&o: Closed Tab"));
    emit undoAvailable(true);
}

void KonqUndoManager::updateSupportsFileUndo(bool enable)
{
    m_supportsFileUndo = enable;
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::clearClosedItemsList(bool onlyInthisWindow)
{
    populate();
    QList<KonqClosedItem *>::iterator it = m_closedItemList.begin();
    for (; it != m_closedItemList.end();) {
        KonqClosedItem *closedItem = *it;
        const KonqClosedTabItem *closedTabItem =
            dynamic_cast<const KonqClosedTabItem *>(closedItem);
        const KonqClosedWindowItem *closedWindowItem =
            dynamic_cast<const KonqClosedWindowItem *>(closedItem);

        it = m_closedItemList.erase(it);
        if (closedTabItem) {
            delete closedTabItem;
        } else if (closedWindowItem && !onlyInthisWindow) {
            m_cwManager->removeClosedWindowItem(this, closedWindowItem);
            delete closedWindowItem;
        }
    }

    emit closedItemsListChanged();
    emit undoAvailable(this->undoAvailable());

    // Save config so that this window won't appear in new konqueror processes
    m_cwManager->saveConfig();
}

void KonqUndoManager::undoLastClosedItem()
{
    undoClosedItem(0);
}

