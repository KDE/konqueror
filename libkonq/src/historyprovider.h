/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KHISTORYPROVIDER_H
#define KHISTORYPROVIDER_H

#include <libkonq_export.h>

#include <QObject>

class HistoryProviderPrivate;

/**
 * @brief Basic class to manage a history of "items". This class is only meant
 * for fast lookup, if an item is in the history or not.
 *
 * This class may be subclassed to implement a persistent history for example.
 * For usage with khtml, just create your provider and call the
 * HistoryProvider constructor _before_ you do any khtml stuff. That way,
 * khtml, using the self()-method, will use your subclassed provider.
 *
 * This is a singleton class. The single instance of this class can be obtained
 * calling self(), which will also create it the first time it's used.
 *
 * @author Carsten Pfeiffer <pfeiffer@kde.org>
 */
class LIBKONQ_EXPORT HistoryProvider : public QObject
{
    Q_OBJECT
    friend class ::HistoryProviderPrivate;

public:
    /**
     * @brief The single instance of this class
     *
     * The first time this function is called, it creates a new instance of the class
     * @return the single instance of this class
     */
    static HistoryProvider *self();

    /**
     * @brief Whether or not an instance of this class has already been created
     * @returns true if a provider has already been created.
     * @since 4.4
     */
    static bool exists();

    /**
     * @brief Whether an item is in the history
     * @returns `true` if @p item is present in the history and `false` otherwise.
     */
    virtual bool contains(const QString &item) const;

    /**
     * @brief Inserts an item into the history
     *
     * Emits the inserted() signal.
     * @param item the item to insert
     */
    virtual void insert(const QString &item);

    /**
     * @brief Removes an item from the history
     *
     * If the item doesn't exist in the history, nothing is done.
     *
     * @param item the item to remove
     */
    virtual void remove(const QString &item);

    /**
     * @brief Clears the history
     *
     * The cleared() signal is emitted after clearing.
     */
    virtual void clear();

Q_SIGNALS:
    /**
     * @brief Signal emitted after the history has been cleared
     */
    void cleared();

    /**
     * @brief Signal emitted to notify that history has changed
     *
     * This signal is never emitted by this class, but it can be emited by subclasses.
     *
     * @param items the items which were added or removed
     */
    void updated(const QStringList &items);

    /**
     * @brief Signal emitted after an item has been inserted
     *
     * @param item the item which has been inserted
     */
    void inserted(const QString &item);

protected:
    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    HistoryProvider(QObject *parent = nullptr);

    ~HistoryProvider() override; //!< Destructor

private:
    HistoryProviderPrivate *const d; //!< The d-pointer
};

#endif // KHISTORYPROVIDER_H
