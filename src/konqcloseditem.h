/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQCLOSEDITEM_H
#define KONQCLOSEDITEM_H

#include "konqprivate_export.h"
#include <kconfiggroup.h>
#include <QString>

class KConfig;

/**
 * @brief Abstract class containing the information to reopen a closed item (tab, window, ...)
 *
 * Each item is identified by a serial number and stores the information to restore it in a `KConfig`
 * object under a specific group.
 */
class KONQ_TESTS_EXPORT KonqClosedItem : public QObject
{
public:
    ~KonqClosedItem() override; //!< Destructor

    /**
     * @brief The configuration group containing the information to reopen the item
     * @return the configuration group to use to reopen the item
     */
    virtual KConfigGroup &configGroup()
    {
        return m_configGroup;
    }

    /**
     * @brief Overload of configGroup()
     */
    virtual const KConfigGroup &configGroup() const
    {
        return m_configGroup;
    }

    /**
     * @brief A number which identifies the item
     * @return A number which identifies this item
     */
    quint64 serialNumber() const
    {
        return m_serialNumber;
    }

    /**
     * @brief The title of the closed item
     * @return The title of the closed item
     */
    QString title() const
    {
        return m_title;
    }

    /**
     * @brief The icon associated with the closed item
     * @return The icon associated with the closed item
     */
    virtual QPixmap icon() const = 0;

protected:

    /**
     * @brief Constructor
     * @param title the item's title
     * @param config the configuration object where the information to reopen the item must be saved to
     * @param group the name of the configuration group where to save information to reopen the item
     * @param serialNumber the number used to identify the object
     */
    KonqClosedItem(const QString &title, KConfig *config, const QString &group, quint64 serialNumber);

    QString m_title; //!< The title of the item
    KConfigGroup m_configGroup; //!< The name of the configuration group where to save information to reopen the item
    quint64 m_serialNumber; //!< The number to identify the object
};

/**
 * @brief Specialization of KonqClosedItem containing the information to reopen a closed tab
 */
class KONQ_TESTS_EXPORT KonqClosedTabItem : public KonqClosedItem
{
public:

    /**
     * @brief Constructor
     * @param url the tab URL
     * @param config the configuration object where the information to reopen the tab must be saved to
     * @param title the tab title
     * @param index the tab index
     * @param serialNumber the number used to identify the tab
     */
    KonqClosedTabItem(const QString &url, KConfig *config, const QString &title, int index, quint64 serialNumber);

    ~KonqClosedTabItem() override; //!< Destructor

    /**
     * @brief Implementation of KonqClosedItem::icon()
     *
     * @return The icon for the tab URL
     */
    QPixmap icon() const override;

    /**
     * @brief The URL of the tab
     * @return the URL of the tab
     */
    QString url() const
    {
        return m_url;
    }

    /**
     * @brief The position inside the tab bar that the tab had when it was  closed
     * @return the position inside the tab bar that the tab had when it was  closed
     */
    int pos() const
    {
        return m_pos;
    }

protected:
    QString m_url; //!< The URL of the tab
    int m_pos; //!< The position of the tab in the tab bar
};

/**
 * @brief Specialization of KonqClosedItem containing the information to reopen a closed window
 */
class KONQ_TESTS_EXPORT KonqClosedWindowItem : public KonqClosedItem
{
public:
    /**
     * @brief Constructor
     * @param title the window title
     * @param config the configuration object where the information to reopen the window must be saved to
     * @param serialNumber the number used to identify the window
     * @param numTabs the number of tabs in the window
     */
    KonqClosedWindowItem(const QString &title, KConfig *config, quint64 serialNumber, int numTabs);
    ~KonqClosedWindowItem() override; //!< Destructor

    /**
     * @brief Implementation of KonqClosedItem::icon()
     *
     * @return An icon made by the Konqueror icon with the number of tabs over it
     */
    QPixmap icon() const override;

    /**
     * @brief The number of tabs in the window
     * @return the number of tabs in the window
     */
    int numTabs() const;

protected:
    int m_numTabs; //!< The number of tabs in the window
};

#endif /* KONQCLOSEDITEM_H */

