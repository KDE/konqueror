/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2003 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __delayedinitializer_h__
#define __delayedinitializer_h__

#include <QObject>
class QEvent;

class DelayedInitializer : public QObject
{
    Q_OBJECT
public:
    DelayedInitializer(int eventType, QObject *parent);

protected:
    bool eventFilter(QObject *receiver, QEvent *event) override;

Q_SIGNALS:
    void initialize();

private Q_SLOTS:
    void slotInitialize();
private:
    int m_eventType;
    bool m_signalEmitted;
};

#endif
