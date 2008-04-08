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

#include <QObject>
#include <QString>
#include <QList>
#include <kconfig.h>
#include "konqcloseditem.h"
#include "konqundomanager_interface.h"
class QAction;
class KonqClosedWindowsManagerPrivate;
class QDBusMessage;

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
    /**
     * Add current window as a closed window item to other windows
     */
    void addClosedWindowItem(KonqClosedWindowItem *closedWindowItem);
    void updateSupportsFileUndo(bool enable);

public Q_SLOTS:
    void undo();
    void clearClosedItemsList(bool onlyInthisWindow = false);
    void undoLastClosedItem();
    /**
     * Opens in a new tab/window the item the user selected from the closed tabs 
     * menu (by emitting openClosedTab/Window), and takes it from the list.
     */
    void slotClosedItemsActivated(QAction* action);
    void slotAddClosedWindowItem(KonqUndoManager *real_sender,
        KonqClosedWindowItem *closedWindowItem);

Q_SIGNALS:
    void undoAvailable(bool canUndo);
    void undoTextChanged(const QString& text);

    /// Emitted when a closed tab should be reopened
    void openClosedTab(const KonqClosedTabItem&);
    /// Emitted when a closed window should be reopened
    void openClosedWindow(const KonqClosedWindowItem&);
    /// Emitted when closedItemsList() has changed.
    void closedItemsListChanged();

    /// Emitted to be received in other window instances, uing the singleton
    /// communicator
    void removeWindowInOtherInstances(KonqUndoManager *real_sender, const 
        KonqClosedWindowItem *closedWindowItem);
    void addWindowInOtherInstances(KonqUndoManager *real_sender,
        KonqClosedWindowItem *closedWindowItem);
private Q_SLOTS:
    void slotFileUndoAvailable(bool);
    void slotFileUndoTextChanged(const QString& text);
    
    /**
     * Received from other window instances, removes/adds a reference of a 
     * window from m_closedItemList.
     */
    void slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const
        KonqClosedWindowItem *closedWindowItem);
private:
    /// Fill the m_closedItemList with closed windows
    void populate();
    
    QList<KonqClosedItem *> m_closedItemList;
    bool m_supportsFileUndo;
};

/**
 * Provides a shared singleton for all Konq window instances.
 * This class is a singleton, use self() to access its only instance.
 * 
 *  - it synchronizes the closed window list with other
 * Konqueror instances via DBUS.
 * 
 *  - When a konqueror instance is closed, the closed window list is written
 * to disk so that the undo closed windows list can be retrieved even when
 * no other konqueror instance is running.
 */
class KONQ_TESTS_EXPORT KonqClosedWindowsManager : public QObject
{
    Q_OBJECT
public:
    friend class KonqClosedWindowsManagerPrivate;
    
    static KonqClosedWindowsManager *self();
    
    const QList<KonqClosedWindowItem *>& closedWindowItemList();
    
    void addClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem
        *closedWindowItem, bool propagate = true);
    
    void removeClosedWindowItem(KonqUndoManager *real_sender, const
    
    KonqClosedWindowItem *closedWindowItem, bool propagate = true);
    
    void applySettings();
    
    int maxNumClosedItems();
    
    void setMaxNumClosedItems(int max);
    
    KConfig* config();

public Q_SLOTS:
    void readSettings();
    
    /**
     * Reads the list of closed window from the configuration file if it couldn't
     * be retrieved from running konqueror windows
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
    
    /**
     * Called by readSettings(), which is called by a singleShot timer inside
     * the constructor. Fills the closed window list with items from other 
     * konqueror instances, being the list retrieved via DBUS using the method
     * localClosedWindowItems();
     *
     * It also sets a timer so that if no other konqueror instance pings back in
     * X miliseconds, then the closed window list is read from disk.
     */
    void populate();
    
    /**
     * Called by the destructor. Saves the closed windows list to disk inside a
     * config file
     */
    void saveConfig();

    KonqClosedRemoteWindowItem* findClosedRemoteWindowItem(const QString& configFileName,
        const QString& configGroup);
    
    KonqClosedWindowItem* findClosedLocalWindowItem(const QString& configFileName,
        const QString& configGroup);
    
    /**
     * This function removes all the files in appdata/closeditem/. Thus, it's 
     * used to clean the config files inside that directory.
     */
    void removeClosedItemsConfigFiles();
private:
    QList<KonqClosedWindowItem *> m_closedWindowItemList;
    KConfig *m_konqClosedItemsConfig;
    int m_maxNumClosedItems;
    bool m_amIalone;
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

    /**
     * Ask over  D-BUS to all running konqueror instances to return their list
     * of local closed window items.
     */
    void requestLocalClosedWindowItems();
public Q_SLOTS: // DBUS methods
    /**
     * This is the answer call when a konqueror instance receives a call from
     * requestLocalClosedWindowItems().
     */
    void localClosedWindowItems(const QList<QVariant>& windowItems,
        const QString& dbusService);
        
    /**
     * This signal is received from other konqueror instances in order to let
     * us know that there are other konqueror instances from which we can
     * retrieve the list of closed window items, so that we don't need to read
     * them from disk.
     */
    void pong();

private Q_SLOTS:// connected to DBUS signals
    void slotNotifyClosedWindowItem( const QString& title, const int& numTabs,
        const QString& configFileName, const QString& configGroup,
        const QString& service );
        
    void slotNotifyClosedWindowItem( const QString& title, const int& numTabs,
        const QString& configFileName, const QString& configGroup,
        const QDBusMessage& msg );
    
    void slotNotifyRemove( const QString& configFileName,
        const QString& configGroup, const QDBusMessage& msg );
    
    void slotRequestLocalClosedWindowItems( const QDBusMessage& msg );

private:
    void emitNotifyClosedWindowItem(const KonqClosedWindowItem *closedWindowItem);
    
    void emitNotifyRemove(const KonqClosedWindowItem *closedWindowItem);
    
    void emitPong(const QString & service);
};

#endif /* KONQUNDOMANAGER_H */

