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

#ifndef KONQCLOSEDWINDOWSMANAGER_H
#define KONQCLOSEDWINDOWSMANAGER_H

#include "konqprivate_export.h"
#include <QList>
#include <QObject>
class KonqClosedRemoteWindowItem;
class KonqUndoManager;
class KConfig;
class QDBusMessage;
class KonqClosedWindowItem;
class QString;
class KonqClosedWindowsManagerPrivate;

/**
 * Provides a shared singleton for all Konq window instances.
 * This class is a singleton, use self() to access its only instance.
 *
 *  - it synchronizes the closed window list with other
 * Konqueror instances via DBUS.
 *
 */
class KONQ_TESTS_EXPORT KonqClosedWindowsManager : public QObject
{
    Q_OBJECT
public:
    friend class KonqClosedWindowsManagerPrivate;

    static KonqClosedWindowsManager *self();

    const QList<KonqClosedWindowItem *>& closedWindowItemList();

    /**
     * When a window is closed it's added with this function to
     * m_closedWindowItemList.
     */
    void addClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem
        *closedWindowItem, bool propagate = true);

    void removeClosedWindowItem(KonqUndoManager *real_sender, const

    KonqClosedWindowItem *closedWindowItem, bool propagate = true);

    /**
     * Returns an anonymous config (which exists only in memory). Only used by
     * KonqClosedItems for storing in memory closed items.
     */
    KConfig* memoryStore();

    /**
     * Called by the KonqUndoManager when a local window is being closed.
     * Saves the closed windows list to disk inside a config file.
     */
    void saveConfig();

    bool undoAvailable() const;

public Q_SLOTS:
    void readSettings();

    /**
     * Reads the list of closed window from the configuration file if it couldn't
     * be retrieved from running konqueror windows and if it hasn't been read
     * already. By default the closeditems_list file is not read, so each
     * function which needs that file to be read first must call this function
     * to ensure the closeditems list is filled.
     */
    void readConfig();

Q_SIGNALS:
    /**
     * Notifies the addition the closed window list in all the konqueror windows of
     * this konqueror instance.
     */
    void addWindowInOtherInstances(KonqUndoManager *real_sender,
        KonqClosedWindowItem *closedWindowItem);

    /**
     * Notifies the removal the closed window list in all the konqueror windows of
     * this konqueror instance.
     */
    void removeWindowInOtherInstances(KonqUndoManager *real_sender, const
        KonqClosedWindowItem *closedWindowItem);
private:
    KonqClosedWindowsManager();

    virtual ~KonqClosedWindowsManager();

    KonqClosedRemoteWindowItem* findClosedRemoteWindowItem(const QString& configFileName,
        const QString& configGroup);

    KonqClosedWindowItem* findClosedLocalWindowItem(const QString& configFileName,
        const QString& configGroup);

    /**
     * This function removes all the closed items temporary files. Only done if
     * there's no other konqueror process running than us, otherwise that process
     * might want to use that temporary file.
     */
    void removeClosedItemsConfigFiles();
private:
    QList<KonqClosedWindowItem *> m_closedWindowItemList;
    int m_numUndoClosedItems;
    KConfig *m_konqClosedItemsConfig, *m_konqClosedItemsStore;
    int m_maxNumClosedItems;
    /**
     * This bool var is used internally to allow delayed initialization of the
     * closed items list. When active, this flag prevents addClosedWindowItem()
     * from emiting addWindowInOtherInstances() as the windows are already
     * being dealt with inside KonqUndoManager::populate().
     */
    bool m_blockClosedItems;
Q_SIGNALS: // DBUS signals
    /**
     * Every konqueror instance broadcasts new closed windows to other
     * konqueror instances.
     */
    void notifyClosedWindowItem( const QString& title, const int& numTabs,
        const QString& configFileName, const QString& configGroup );

    /**
     * Every konqueror instance broadcasts removed closed windows to other
     * konqueror instances.
     */
    void notifyRemove( const QString& configFileName,
        const QString& configGroup );

private Q_SLOTS:// connected to DBUS signals
    void slotNotifyClosedWindowItem( const QString& title, const int& numTabs,
        const QString& configFileName, const QString& configGroup,
        const QString& service );

    void slotNotifyClosedWindowItem( const QString& title, const int& numTabs,
        const QString& configFileName, const QString& configGroup,
        const QDBusMessage& msg );

    void slotNotifyRemove( const QString& configFileName,
        const QString& configGroup, const QDBusMessage& msg );

private:
    void emitNotifyClosedWindowItem(const KonqClosedWindowItem *closedWindowItem);

    void emitNotifyRemove(const KonqClosedWindowItem *closedWindowItem);
};

#endif /* KONQCLOSEDWINDOWSMANAGER_H */

