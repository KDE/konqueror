/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QMainWindow>

// SCW == Switch (or Set) Central Widget
class SCWMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    SCWMainWindow(QWidget *parent = nullptr);

private slots:
    void slotSwitchCentralWidget();
};

#endif
