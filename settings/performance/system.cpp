/*
    SPDX-FileCopyrightText: 2004 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "system.h"

#include <kconfig.h>

#include <QCheckBox>
#include <KLocalizedString>
#include <KConfigGroup>

namespace KCMPerformance
{

SystemWidget::SystemWidget(QWidget *parent_P)
    : System_ui(parent_P)
{
    QString tmp =
        i18n("<p>During startup KDE needs to perform a check of its system configuration"
             " (mimetypes, installed applications, etc.), and in case the configuration"
             " has changed since the last time, the system configuration cache (KSyCoCa)"
             " needs to be updated.</p>"
             "<p>This option delays the check, which avoid scanning all directories containing"
             " files describing the system during KDE startup, thus"
             " making KDE startup faster. However, in the rare case the system configuration"
             " has changed since the last time, and the change is needed before this"
             " delayed check takes place, this option may lead to various problems"
             " (missing applications in the K Menu, reports from applications about missing"
             " required mimetypes, etc.).</p>"
             "<p>Changes of system configuration mostly happen by (un)installing applications."
             " It is therefore recommended to turn this option temporarily off while"
             " (un)installing applications.</p>");
    cb_disable_kbuildsycoca->setToolTip(tmp);
    label_kbuildsycoca->setToolTip(tmp);
    connect(cb_disable_kbuildsycoca, &QAbstractButton::clicked, this, &SystemWidget::changed);
    defaults();
}

void SystemWidget::load()
{
    KConfig _cfg(QStringLiteral("kdedrc"));
    KConfigGroup cfg(&_cfg, "General");
    cb_disable_kbuildsycoca->setChecked(cfg.readEntry("DelayedCheck", false));
}

void SystemWidget::save()
{
    KConfig _cfg(QStringLiteral("kdedrc"));
    KConfigGroup cfg(&_cfg, "General");
    cfg.writeEntry("DelayedCheck", cb_disable_kbuildsycoca->isChecked());
}

void SystemWidget::defaults()
{
    cb_disable_kbuildsycoca->setChecked(false);
}

} // namespace

