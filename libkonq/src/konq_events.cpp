/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000-2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konq_events.h"

#if QT_VERSION_MAJOR < 6
const char *const KonqFileSelectionEvent::s_fileItemSelectionEventName = "Konqueror/FileSelection";
const char *const KonqFileMouseOverEvent::s_fileItemMouseOverEventName = "Konqueror/FileMouseOver";
#endif

const int KonqFileSelectionEventType = QEvent::registerEventType();

KonqFileSelectionEvent::KonqFileSelectionEvent(const KFileItemList &selection, KParts::ReadOnlyPart *part)
#if QT_VERSION_MAJOR < 6
    : KParts::Event(s_fileItemSelectionEventName), m_selection(selection), m_part(part)
#else
    : QEvent(static_cast<QEvent::Type>(KonqFileSelectionEventType))
#endif
{
}

KonqFileSelectionEvent::~KonqFileSelectionEvent()
{
}


bool KonqFileSelectionEvent::test(const QEvent *event)
{
        
#if QT_VERSION_MAJOR < 6
        return KParts::Event::test(event, s_fileItemSelectionEventName);
#else
        return event->type() == KonqFileSelectionEventType;
#endif
}

const int KonqFileMouseOverEventType = QEvent::registerEventType();
KonqFileMouseOverEvent::KonqFileMouseOverEvent(const KFileItem &item, KParts::ReadOnlyPart *part)
#if QT_VERSION_MAJOR < 6
    : KParts::Event(s_fileItemMouseOverEventName), m_item(item), m_part(part)
#else
    : QEvent(static_cast<QEvent::Type>(KonqFileMouseOverEventType))
#endif
{
}

KonqFileMouseOverEvent::~KonqFileMouseOverEvent()
{
}

bool KonqFileMouseOverEvent::test(const QEvent *event)
{
        
#if QT_VERSION_MAJOR < 6
        return KParts::Event::test(event, s_fileItemMouseOverEventName);
#else
        return event->type() == KonqFileMouseOverEventType;
#endif
}
