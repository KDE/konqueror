/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQCLOSEDWINDOWSMANAGER_H
#define KONQCLOSEDWINDOWSMANAGER_H

#include "konqprivate_export.h"
#include <QList>
#include <QObject>
#include <QTemporaryFile>

class KonqUndoManager;
class KConfig;
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

    KonqClosedWindowsManager();
    ~KonqClosedWindowsManager() override;

    static KonqClosedWindowsManager *self();
    static void destroy();

    const QList<KonqClosedWindowItem *> &closedWindowItemList();

    /**
     * When a window is closed it's added with this function to
     * m_closedWindowItemList.
     */
    void addClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem
                             *closedWindowItem, bool propagate = true);

    void removeClosedWindowItem(KonqUndoManager *real_sender, const KonqClosedWindowItem *closedWindowItem);

    /**
     * Returns an anonymous config (which exists only in memory). Only used by
     * KonqClosedItems for storing in memory closed items.
     */
    KConfig *memoryStore();

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

    KonqClosedWindowItem *findClosedWindowItem(const QString &configFileName,
            const QString &configGroup);

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
     * from emitting addWindowInOtherInstances() as the windows are already
     * being dealt with inside KonqUndoManager::populate().
     */
    bool m_blockClosedItems;

private:
    QTemporaryFile m_memoryStoreBackend;
};

#endif /* KONQCLOSEDWINDOWSMANAGER_H */

