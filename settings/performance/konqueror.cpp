/*
    SPDX-FileCopyrightText: 2003 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqueror.h"

#include <kconfig.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QCheckBox>
#include <KLocalizedString>
#include <KConfigGroup>

namespace KCMPerformance
{

Konqueror::Konqueror(QWidget *parent_P)
    : Konqueror_ui(parent_P)
{
    // TODO move these strings to konqueror.kcfg and use that from here
    cb_preload_on_startup->setToolTip(
        i18n("<p>If enabled, an instance of Konqueror will be preloaded after the ordinary Plasma "
             "startup sequence.</p>"
             "<p>This will make the first Konqueror window open faster, but "
             "at the expense of longer Plasma startup times (but you will be able to work "
             "while it is loading, so you may not even notice that it is taking longer).</p>"));
    cb_always_have_preloaded->setToolTip(
        i18n("<p>If enabled, Konqueror will always try to have one preloaded instance ready; "
             "preloading a new instance in the background whenever there is not one available, "
             "so that windows will always open quickly.</p>"
             "<p><b>Warning:</b> In some cases, it is actually possible that this will "
             "reduce perceived performance.</p>"));
    connect(cb_preload_on_startup, &QAbstractButton::toggled, this, &Konqueror::changed);
    connect(cb_always_have_preloaded, &QAbstractButton::toggled, this, &Konqueror::changed);
    defaults();
}

void Konqueror::load()
{
    KConfig _cfg(QStringLiteral("konquerorrc"));
    KConfigGroup cfg(&_cfg, "Reusing");
    cb_preload_on_startup->setChecked(cfg.readEntry("PreloadOnStartup", false));
    cb_always_have_preloaded->setChecked(cfg.readEntry("AlwaysHavePreloaded", true));
}

void Konqueror::save()
{
    KConfig _cfg(QStringLiteral("konquerorrc"));
    KConfigGroup cfg(&_cfg, "Reusing");
    cfg.writeEntry("PreloadOnStartup", cb_preload_on_startup->isChecked());
    cfg.writeEntry("AlwaysHavePreloaded", cb_always_have_preloaded->isChecked());
    cfg.sync();
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void Konqueror::defaults()
{
    cb_preload_on_startup->setChecked(false);
    cb_always_have_preloaded->setChecked(true);
}

} // namespace
