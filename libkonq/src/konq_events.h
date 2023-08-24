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

#if QT_VERSION_MAJOR < 6
#include <kparts/event.h>
#else
#include <QEvent>
#endif


namespace KParts
{
class ReadOnlyPart;
}


#if QT_VERSION_MAJOR < 6
class LIBKONQ_EXPORT KonqFileSelectionEvent : public KParts::Event
#else 
class LIBKONQ_EXPORT KonqFileSelectionEvent : public QEvent
#endif
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
#if QT_VERSION_MAJOR < 6
    Q_DISABLE_COPY(KonqFileSelectionEvent)
    static const char *const s_fileItemSelectionEventName;
#endif

    KFileItemList m_selection;
    KParts::ReadOnlyPart *m_part;
};

#if QT_VERSION_MAJOR < 6
class LIBKONQ_EXPORT KonqFileMouseOverEvent : public KParts::Event
#else
class LIBKONQ_EXPORT KonqFileMouseOverEvent : public QEvent
#endif
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
#if QT_VERSION_MAJOR < 6
    Q_DISABLE_COPY(KonqFileMouseOverEvent)
    static const char *const s_fileItemMouseOverEventName;
#endif

    KFileItem m_item;
    KParts::ReadOnlyPart *m_part;
};

#endif
