/*
    SPDX-FileCopyrightText: 2003 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _KCM_PERF_KONQUEROR_H
#define _KCM_PERF_KONQUEROR_H

#include "ui_konqueror_ui.h"

namespace KCMPerformance
{

class Konqueror_ui : public QWidget, public Ui::Konqueror_ui
{
public:
    Konqueror_ui(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
        layout()->setContentsMargins(0, 0, 0, 0);
    }
};

class Konqueror
    : public Konqueror_ui
{
    Q_OBJECT
public:
    Konqueror(QWidget *parent_P = nullptr);
    void load();
    void save();
    void defaults();
Q_SIGNALS:
    void changed();
};

}  // namespace

#endif
