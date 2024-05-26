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

class KONQ_TESTS_EXPORT KonqClosedItem : public QObject
{
public:
    ~KonqClosedItem() override;
    virtual KConfigGroup &configGroup()
    {
        return m_configGroup;
    }
    virtual const KConfigGroup &configGroup() const
    {
        return m_configGroup;
    }
    quint64 serialNumber() const
    {
        return m_serialNumber;
    }
    QString title() const
    {
        return m_title;
    }
    virtual QPixmap icon() const = 0;

protected:
    KonqClosedItem(const QString &title, KConfig *config, const QString &group, quint64 serialNumber);
    QString m_title;
    KConfigGroup m_configGroup;
    quint64 m_serialNumber;
};

/**
 * This class stores all the needed information about a closed tab
 * in order to be able to reopen it if requested
 */
class KONQ_TESTS_EXPORT KonqClosedTabItem : public KonqClosedItem
{
public:
    KonqClosedTabItem(const QString &url, KConfig *config, const QString &title, int index, quint64 serialNumber);
    ~KonqClosedTabItem() override;
    QPixmap icon() const override;
    QString url() const
    {
        return m_url;
    }
    /// The position inside the tabbar that the tab had when it was  closed
    int pos() const
    {
        return m_pos;
    }

protected:
    QString m_url;
    int m_pos;
};

/**
 * This class stores all the needed information about a closed tab
 * in order to be able to reopen it if requested
 */
class KONQ_TESTS_EXPORT KonqClosedWindowItem : public KonqClosedItem
{
public:
    KonqClosedWindowItem(const QString &title, KConfig *config, quint64 serialNumber, int numTabs);
    ~KonqClosedWindowItem() override;
    QPixmap icon() const override;
    int numTabs() const;

protected:
    int m_numTabs;
};

#endif /* KONQCLOSEDITEM_H */

