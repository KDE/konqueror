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

KonqUndoManager::KonqUndoManager(QObject* parent)
    : QObject(parent)
{
    KonqFileUndoManager::incRef();
    connect( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    connect( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );
}

KonqUndoManager::~KonqUndoManager()
{
    clearClosedTabsList();
    KonqFileUndoManager::decRef();
}

void KonqUndoManager::slotFileUndoAvailable(bool)
{
    emit undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    if (!m_closedTabsList.isEmpty())
        return true;
    else
        return (m_supportsFileUndo && KonqFileUndoManager::self()->undoAvailable());
}

QString KonqUndoManager::undoText() const
{
    if (!m_closedTabsList.isEmpty()) {
        const KonqClosedTabItem* closedTabItem = m_closedTabsList.first();
        if (closedTabItem->serialNumber() > KonqFileUndoManager::self()->currentCommandSerialNumber()) {
            return i18n("Und&o: Closed Tab");
        }
    }
    return KonqFileUndoManager::self()->undoText();
}

void KonqUndoManager::undo()
{
    if (!m_closedTabsList.isEmpty()) {
        const KonqClosedTabItem* closedTabItem = m_closedTabsList.first();

        // Check what to undo
        kDebug() << "closedTabItem->serialNumber:" << closedTabItem->serialNumber()
                 << ", KonqFileUndoManager currentCommandSerialNumber(): " << KonqFileUndoManager::self()->currentCommandSerialNumber();

        if (closedTabItem->serialNumber() > KonqFileUndoManager::self()->currentCommandSerialNumber()) {
            m_closedTabsList.removeFirst();
            emit openClosedTab(*closedTabItem);
            delete closedTabItem;
            emit undoAvailable(this->undoAvailable());
            emit closedTabsListChanged();
        } else {
            KonqFileUndoManager::self()->undo();
        }
    } else {
        KonqFileUndoManager::self()->undo();
    }
}

const QList<KonqClosedTabItem *>& KonqUndoManager::closedTabsList() const
{
    return m_closedTabsList;
}

void KonqUndoManager::undoClosedTab(int index)
{
    Q_ASSERT(!m_closedTabsList.isEmpty());
    KonqClosedTabItem* closedTabItem = m_closedTabsList.at( index );
    m_closedTabsList.removeAt(index);
    emit openClosedTab(*closedTabItem);
    delete closedTabItem;
    emit undoAvailable(this->undoAvailable());
    emit undoTextChanged(this->undoText());
    emit closedTabsListChanged();
}

void KonqUndoManager::slotClosedTabsActivated(QAction* action)
{
    // Open a closed tab
    const int index = action->data().toInt();
    undoClosedTab(index);
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
    m_closedTabsList.prepend(closedTabItem);
    emit undoTextChanged(i18n("Und&o: Closed Tab"));
    emit undoAvailable(true);
}

void KonqUndoManager::updateSupportsFileUndo(bool enable)
{
	m_supportsFileUndo = enable;
	emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::clearClosedTabsList()
{
    qDeleteAll(m_closedTabsList);
    m_closedTabsList.clear();
    emit closedTabsListChanged();
}

void KonqUndoManager::undoLastClosedTab()
{
    undoClosedTab(0);
}
