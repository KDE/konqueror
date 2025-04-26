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
 * @brief Singleton class which synchronizes the list of closed windows with other Konqueror instances
 *
 * The maximum number of closed windows kept in the list is given by `Konq::Settings::maxNumClosedItems()`
 * and can be configured by the user.
 */
class KONQ_TESTS_EXPORT KonqClosedWindowsManager : public QObject
{
    Q_OBJECT
public:
    friend class KonqClosedWindowsManagerPrivate;

    /**
     * @brief Constructor
     *
     * This constructs an object with an empty window list. This means that, unless
     * readConfig() is called, only windows closed after this object has been created
     * will be stored in the closed window list.
     */
    KonqClosedWindowsManager();
    ~KonqClosedWindowsManager() override; //<! Destructor

    /**
     * @brief The only instance of the class
     *
     * @return the only instance of the class. If no instance exists, a new one is created
     */
    static KonqClosedWindowsManager *self();

    /**
     * @brief Destroys the only instance of the class
     */
    static void destroy();

    /**
     * @brief A list of objects representing the closed windows
     * @return a list of objects representing the closed windows
     * @note This function updates the list of closed windows before returning it
     * (this is why it's not `const`).
     */
    const QList<KonqClosedWindowItem *> &closedWindowItemList();

    /**
     * @brief Adds a new closed window to the list of closed windows
     *
     * If the list contains the maximum number of windows, the last window is removed.
     *
     * @param real_sender the undo manager which requested adding the window to the list
     * @param closedWindowItem the item representing the closed window
     * @param propagate whether or not to save the new window list to the configuration file
     */
    void addClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem
                             *closedWindowItem, bool propagate = true);

    /**
     * @brief Removes a window from the list of closed windows
     * @param real_sender the undo manager which requested removing the window to the list
     * @param closedWindowItem the item representing the window to remove
     */
    void removeClosedWindowItem(KonqUndoManager *real_sender, const KonqClosedWindowItem *closedWindowItem);

    /**
     * @brief The config object used to temporary store closed items
     *
     * This is an anonymous config which is kept in a temporary file and it's only used  by
     * KonqClosedItems for storing in memory closed items.
     *
     * @return the anonymous config object
     */
    KConfig *memoryStore();

    /**
     * @brief Saves the list of closed windows to a permanent configuration file
     *
     * This function is called by KonqUndoManager when a local window is being closed.
     */
    void saveConfig();

    /**
     * @brief Whether or not there are entries in the closed window list
     * @return `true` if the closed window list contains at least one entry and `false` if it's empty
     */
    bool undoAvailable() const;

public Q_SLOTS:

    void readSettings(); //!< As readConfig()

    /**
     * @brief Fills the list of closed windows from the contents of the configuration file
     *
     * This function does nothing if the configuration file has already been read.
     *
     * @note The configuration file isn't read by default, so any function which
     * wants to ensure that the list of closed window contains the windows in the configuration
     * file must call this function.
     */
    void readConfig();

Q_SIGNALS:
    /**
     * @brief Signal emitted when a new item is added to the closed window list
     *
     * This signal is used by other Konqueror instances to synchronize the list of
     * recently closed windows.
     *
     * @todo Since now Konqueror only has a single instance, it's likely that this
     * signal isn't needed anymore
     * @param real_sender the undo manager which requested inserting the new item
     * in the closed window list
     * @param closedWindowItem the item to add
     */
    void addWindowInOtherInstances(KonqUndoManager *real_sender,
                                   KonqClosedWindowItem *closedWindowItem);

    /**
     * @brief Signal emitted when an item is removed from the closed window list
     *
     * This signal is used by other Konqueror instances to synchronize the list of
     * recently closed windows.
     *
     * @todo Since now Konqueror only has a single instance, it's likely that this
     * signal isn't needed anymore
     * @param real_sender the undo manager which requested removing the item
     * from the closed window list
     * @param closedWindowItem the item to remove
     */
    void removeWindowInOtherInstances(KonqUndoManager *real_sender, const
                                      KonqClosedWindowItem *closedWindowItem);
private:

    /**
     * @brief Finds the closed window item corresponding to a configuration file and group
     * @param configFileName the name of the configuration file the window item is stored
     * @param configGroup the name of the group where the window item is stored
     * @return the item stored in group @p configGroup of configuration file @p configFileName
     * or `nullptr` if no such item exists
     */
    KonqClosedWindowItem *findClosedWindowItem(const QString &configFileName,
            const QString &configGroup);

private:
    QList<KonqClosedWindowItem *> m_closedWindowItemList; //!< A list of items representing closed windows
    int m_numUndoClosedItems; //!< The number of available close windows to reopen

    /**
     * @brief The configuration object to save closed windows to
     *
     * Windows saved here persist after closing Konqueror
     */
    KConfig *m_konqClosedItemsConfig;

    /**
     * @brief The temporary configuration object to save closed windows to
     *
     * Windows saved here will only be kept until Konqueror is closed, as this configuration object is backed
     * by a temporary file
     */
    KConfig *m_konqClosedItemsStore;

    /**
     * @brief Flag controlling whether addClosedWindowItem() should emit the addWindowInOtherInstances() signal
     *
     * If this is `true`, the signal won't be emitted as the windows are already
     * being dealt with inside KonqUndoManager::populate().
     */
    bool m_blockClosedItems;

    QTemporaryFile m_memoryStoreBackend; //!< The backend for the temporary configuration file
};

#endif /* KONQCLOSEDWINDOWSMANAGER_H */

