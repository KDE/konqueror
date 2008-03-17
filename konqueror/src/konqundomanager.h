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

#ifndef KONQUNDOMANAGER_H
#define KONQUNDOMANAGER_H

#include "konqprivate_export.h"

#include <QObject>
#include "konqclosedtabitem.h"
class QAction;

/**
 * Note that there is one KonqUndoManager per mainwindow.
 * It integrates KonqFileUndoManager (undoing file operations)
 * and undoing the closing of tabs.
 */
class KONQ_TESTS_EXPORT KonqUndoManager : public QObject
{
    Q_OBJECT
public:
    explicit KonqUndoManager(QObject* parent);
    ~KonqUndoManager();

    bool undoAvailable() const;
    QString undoText() const;
    quint64 newCommandSerialNumber();

    const QList<KonqClosedItem* >& closedTabsList() const;
    void undoClosedItem(int index);
    void addClosedTabItem(KonqClosedTabItem* closedTabItem);
    void updateSupportsFileUndo(bool enable);
    void addWindowInOtherInstances(const KonqClosedWindowItem
    *closedWindowItem);

public Q_SLOTS:
    void undo();
    void clearClosedItemsList();
    void undoLastClosedItem();
    /**
     * Opens in a new tab/window the item the user selected from the closed tabs 
     * menu (by emitting openClosedTab/Window), and takes it from the list.
     */
    void slotClosedItemsActivated(QAction* action);

Q_SIGNALS:
    void undoAvailable(bool canUndo);
    void undoTextChanged(const QString& text);

    /// Emitted when a closed tab should be reopened
    void openClosedTab(const KonqClosedTabItem&);
    /// Emitted when a closed window should be reopened
    void openClosedWindow(const KonqClosedWindowItem&);
    /// Emitted when closedItemsList() has changed.
    void closedItemsListChanged();

    /// Emitted to be received in other window instances
    void bypassCustomInfo(QVariant &customData);
private Q_SLOTS:
    void slotFileUndoAvailable(bool);
    void slotFileUndoTextChanged(const QString& text);
    
    /**
     * Received from other window instances, removes a reference of a window 
     * from m_closedItemList.
     */
    void slotBypassCustomInfo(QVariant &customData);
private:
    void addClosedWindowItem(KonqClosedWindowItem* closedWindowItem);
    void removeWindowInOtherInstances(const KonqClosedWindowItem
    *closedWindowItem);
    void incRef();
    void decRef();
    
    QList<KonqClosedItem *> m_closedItemList;
    bool m_supportsFileUndo;
};

#endif /* KONQUNDOMANAGER_H */

