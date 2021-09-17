/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_APPLICATION_H
#define KONQ_APPLICATION_H

#include "konqprivate_export.h"
#include <QApplication>

class QDBusMessage;

class KONQ_TESTS_EXPORT KonquerorApplication : public QApplication
{
    Q_OBJECT
public:
    KonquerorApplication(int &argc, char **argv);

public slots:
    void slotReparseConfiguration();

private slots:
    void slotAddToCombo(const QString &url, const QDBusMessage &msg);
    void slotRemoveFromCombo(const QString &url, const QDBusMessage &msg);
    void slotComboCleared(const QDBusMessage &msg);
};

#endif
