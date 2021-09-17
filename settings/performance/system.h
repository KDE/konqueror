/*
    SPDX-FileCopyrightText: 2004 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _KCM_PERF_SYSTEM_H
#define _KCM_PERF_SYSTEM_H

#include <kcmodule.h>

#include "ui_system_ui.h"

class System_ui : public QWidget, public Ui::System_ui
{
public:
    System_ui(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
    }
};

namespace KCMPerformance
{

class SystemWidget
    : public System_ui
{
    Q_OBJECT
public:
    SystemWidget(QWidget *parent_P = nullptr);
    void load();
    void save();
    void defaults();
Q_SIGNALS:
    void changed();
};

}  // namespace

#endif
