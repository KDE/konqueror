/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000-2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __konq_events_h__
#define __konq_events_h__

#include <libkonq_export.h>
#include <kfileitem.h>
#include <kconfigbase.h>

#include <QList>
#include <QtGlobal>

#include <QEvent>

namespace KParts
{
class ReadOnlyPart;
}


class LIBKONQ_EXPORT KonqFileSelectionEvent : public QEvent
{
public:
    KonqFileSelectionEvent(const KFileItemList &selection, KParts::ReadOnlyPart *part);
    ~KonqFileSelectionEvent() override;

    KFileItemList selection() const
    {
        return m_selection;
    }
    
    KParts::ReadOnlyPart *part() const
    {
        return m_part;
    }
    
    static bool test(const QEvent *event);

private:

    KFileItemList m_selection;
    KParts::ReadOnlyPart *m_part;
};

class LIBKONQ_EXPORT KonqFileMouseOverEvent : public QEvent
{
public:
    KonqFileMouseOverEvent(const KFileItem &item, KParts::ReadOnlyPart *part);
    ~KonqFileMouseOverEvent() override;

    const KFileItem &item() const
    {
        return m_item;
    }
    KParts::ReadOnlyPart *part() const
    {
        return m_part;
    }

    static bool test(const QEvent *event);

private:
    KFileItem m_item;
    KParts::ReadOnlyPart *m_part;
};

#endif
