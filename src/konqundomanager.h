/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQUNDOMANAGER_H
#define KONQUNDOMANAGER_H

#include "konqprivate_export.h"
#include <QObject>
#include <QString>
#include <QList>
class KonqClosedWindowItem;
class KonqClosedTabItem;
class KonqClosedItem;
class KonqClosedWindowsManager;
class QAction;

/**
 * @brief Class which manages undo actions for each window
 *
 * It allows undo closing tabs and integrates with `KIO::FileUndoManager` to allow
 * undoing file actions.
 *
 * Each command which can be undo is given a serial number, obtained using
 * `KIO::FileUndoManager::newCommandSerialNumber()` which allows to uniquely identify
 * it.
 */
class KONQ_TESTS_EXPORT KonqUndoManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param cwManager the object which manages closed windows
     * @param parent the parent object
     */
    explicit KonqUndoManager(KonqClosedWindowsManager *cwManager, QWidget *parent);
    ~KonqUndoManager() override; //!< Destructor

    /**
     * @brief Whether there are actions which can be undone
     *
     * @return `true` if there are actions which can be undone and `false` otherwise
     */
    bool undoAvailable() const;

    /**
     * @brief The text for the current undo action
     * @return the text for the current undo action
     */
    QString undoText() const;

    /**
     * @brief The serial number to associate to a new undo command
     * @return  the serial number to associate to a new undo command
     */
    quint64 newCommandSerialNumber();

    /**
     * @brief A list of items representing closed windows or tabs
     *
     * @note This method is not constant because it requires to fill the list
     * of closed items (#m_closedItemList), if it hasn't already been done.
     * @return a list of items representing closed windows or tabs
     */
    const QList<KonqClosedItem * > &closedItemsList();

    /**
     * @brief Undoes the closing of a window or tab
     *
     * @param index the index of the item in #m_closedItemList corresponding to
     * the window or tab to close
     */
    void undoClosedItem(int index);

    /**
     * @brief Adds an item representing a closed tab to the list of items whose
     * closing can be undone
     *
     * @param closedTabItem the item to add
     */
    void addClosedTabItem(KonqClosedTabItem *closedTabItem);

    /**
     * @brief Adds an item representing the current window to the list of items
     * whose closing can be undone
     *
     * @note Closed window items are actually managed by the KonqClosedWindowsManager
     * class: this method is mostly a wrapper for KonqClosedWindowsManager::addClosedWindowItem
     */
    void addClosedWindowItem(KonqClosedWindowItem *closedWindowItem);

    /**
     * @brief Enables or disable support for undoing file operations
     *
     * Whether undoing file operations should be enabled or not depends on whether the
     * active part supports file operations.
     *
     * @param enable whether undoing file operations should be enabled or disabled
     */
    void updateSupportsFileUndo(bool enable);

public Q_SLOTS:

    /**
     * @brief Undoes the last operation
     *
     * The operation can be either a file operation or the closing of a tab or window.
     */
    void undo();

    /**
     * @brief Removes items from the list of closed items
     *
     * This either removes all closed items from the list or only removes items
     * corresponding to closed tabs, depending on the argument.
     *
     * @param onlyInthisWindow if `true` only items corresponding to closed tabs
     * will be removed from the list, otherwise both items corresponding to closed
     * tabs and items corresponding to closed windows will be removed
     */
    void clearClosedItemsList(bool onlyInthisWindow = false);

    /**
     * @brief Undoes the last closed item operation
     *
     * Unlike undo(), this ignores file operations and only undoes the closing of
     * a tab or window.
     */
    void undoLastClosedItem();

    /**
     * @brief Undoes the closing of the window or tab corresponding to the given action
     *
     * It works as undoClosedItem() using the data associated with the action as
     * the index of the closed item.
     *
     * @param action the action corresponding to the closed item. It must have the
     * index as its data
     */
    void slotClosedItemsActivated(QAction *action);

    /**
     * @brief Slot called in response to the KonqClosedWindowsManager::addWindowInOtherInstances
     * signal
     *
     * It adds the item corresponding to the closed window to the list of closed items, provided
     * that @p real_sender is not `this`. If this happens, it means that the window has already
     * been added and we shouldn't do that again.
     *
     * @param real_sender the object which requested adding the window to the list in the first place
     * @param closedWindowItem the item to add
     */
    void slotAddClosedWindowItem(KonqUndoManager *real_sender,
                                 KonqClosedWindowItem *closedWindowItem);

Q_SIGNALS:
    /**
     * @brief Signal emitted to inform of whether there are available operations to undo
     *
     * @param canUndo `true` if there are operations to undo and `false` if there aren't any
     * @note For efficiency reasons, it's possible that this signal is emitted even if
     * the availability of undo operations hasn't changed.
     */
    void undoAvailable(bool canUndo);

    /**
     * @brief Signal emitted when the text for the next undo operation changes
     *
     * @param text the text of the next undo operations
     *
     * @note Despite the name, this signal is actually emitted when the next undo operation,
     * not its text, changes. This means that it's actually possible for the undo
     * text not having changed when this signal is emitted. This happens if both
     * the new and the old operation have the same undo text.
     */
    void undoTextChanged(const QString &text);

    /**
     * @brief Signal emitted when the closing of a tab needs to be undone
     *
     * @param it the item corresponding to the closed tab
     */
    void openClosedTab(const KonqClosedTabItem &it);

    /**
     * @brief Signal emitted when the closing of a window needs to be undone
     *
     * @param it the item corresponding to the closed window
     */
    void openClosedWindow(const KonqClosedWindowItem &it);

    /**
     * @brief Signal emitted when the list of closed items has changed
     */
    void closedItemsListChanged();

    /**
     * @brief Signal emitted to inform other undo managers that a window has been removed
     * from the list of closed items
     *
     * @param real_sender the undo manager which emitted the signal (`this`)
     * @param closedWindowItem the item representing the window
     */
    void removeWindowInOtherInstances(KonqUndoManager *real_sender, const
                                      KonqClosedWindowItem *closedWindowItem);
    /**
     * @brief Signal emitted to inform other undo managers that a window has been added
     * to the list of closed items
     *
     * @param real_sender the undo manager which emitted the signal (`this`)
     * @param closedWindowItem the item representing the window
     */
    void addWindowInOtherInstances(KonqUndoManager *real_sender,
                                   KonqClosedWindowItem *closedWindowItem);
private Q_SLOTS:
    /**
     * @brief Slot called in response to the `KIO::FileUndoManager::undoAvailable()` signal
     *
     * It emits the undoAvailable() signal depending on whether any kind of undo
     * operations (not only file-related ones) are available.
     *
     * @param available unused
     */
    void slotFileUndoAvailable(bool available);

    /**
     * @brief Slot called in response to the `KIO::FileUndoManager::undoTextChanged()` signal
     *
     * It emits the undoTextChanged() signal with the new undo text (which can be for any kind
     * of undo operation, not just for file-related ones.
     *
     * @param text unused
     */
    void slotFileUndoTextChanged(const QString &text);

    /**
     * Received from other window instances, removes/adds a reference of a
     * window from m_closedItemList.
     */
    /**
     * @brief Slot connected with KonqClosedWindowsManager::removeWindowInOtherInstances()
     * signal
     *
     * It removes @p closedWindowItem from the list of closed items, provided that
     * @p real_sender is not `this` because in that case the item would already
     * have been added to the list.
     *
     * @param real_sender the KonqUndoManager which asked to remove the item in
     * the first place
     * @param closedWindowItem the item to remove
     */
    void slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const
                                    KonqClosedWindowItem *closedWindowItem);

private:
    /**
     * @brief Inserts the list of closed windows in the list of closed items
     *
     * This function does nothing if it's called more than one time, as the idea
     * is that after importing the list of closed windows from the KonqClosedWindowsManager,
     * subsequent changes to the list are added or removed using signal/slot connections.
     */
    void populate();

    QList<KonqClosedItem *> m_closedItemList; //!< A list of items representing closed tabs and windows
    KonqClosedWindowsManager *m_cwManager; //!< The instance which manages closed windows
    bool m_supportsFileUndo = false; //!< Whether undoing file operations is currently enabled
    bool m_populated = false; //!< Whether or not populate() has been already called
};

#endif /* KONQUNDOMANAGER_H */
