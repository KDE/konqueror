/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000-2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konq_events.h"

const int KonqFileSelectionEventType = QEvent::registerEventType();

KonqFileSelectionEvent::KonqFileSelectionEvent(const KFileItemList &selection, KParts::ReadOnlyPart *part)
    : QEvent(static_cast<QEvent::Type>(KonqFileSelectionEventType))
{
    Q_UNUSED(selection);
    Q_UNUSED(part);
}

KonqFileSelectionEvent::~KonqFileSelectionEvent()
{
}


bool KonqFileSelectionEvent::test(const QEvent *event)
{
    return event->type() == KonqFileSelectionEventType;
}

const int KonqFileMouseOverEventType = QEvent::registerEventType();
KonqFileMouseOverEvent::KonqFileMouseOverEvent(const KFileItem &item, KParts::ReadOnlyPart *part)
    : QEvent(static_cast<QEvent::Type>(KonqFileMouseOverEventType))
{
    Q_UNUSED(item);
    Q_UNUSED(part);
}

KonqFileMouseOverEvent::~KonqFileMouseOverEvent()
{
}

bool KonqFileMouseOverEvent::test(const QEvent *event)
{
    return event->type() == KonqFileMouseOverEventType;
}
