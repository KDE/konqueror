/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2003 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "delayedinitializer.h"
#include <QTimer>
#include "konqdebug.h"
#include <QEvent>

DelayedInitializer::DelayedInitializer(int eventType, QObject *parent)
    : QObject(parent), m_eventType(eventType), m_signalEmitted(false)
{
    parent->installEventFilter(this);
}

bool DelayedInitializer::eventFilter(QObject *receiver, QEvent *event)
{
    if (m_signalEmitted || event->type() != m_eventType) {
        return false;
    }

    m_signalEmitted = true;
    receiver->removeEventFilter(this);

    // Move the emitting of the event to the end of the eventQueue
    // so we are absolutely sure the event we get here is handled before
    // the initialize is fired.
    QTimer::singleShot(0, this, SLOT(slotInitialize()));

    return false;
}

void DelayedInitializer::slotInitialize()
{
    emit initialize();
    deleteLater();
}

