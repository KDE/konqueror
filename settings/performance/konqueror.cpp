/*
 *  Copyright (c) 2003 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
    cb_preload_on_startup->setWhatsThis(
        i18n("<p>If enabled, an instance of Konqueror will be preloaded after the ordinary Plasma "
             "startup sequence.</p>"
             "<p>This will make the first Konqueror window open faster, but "
             "at the expense of longer Plasma startup times (but you will be able to work "
             "while it is loading, so you may not even notice that it is taking longer).</p>"));
    cb_always_have_preloaded->setWhatsThis(
        i18n("<p>If enabled, Konqueror will always try to have one preloaded instance ready; "
             "preloading a new instance in the background whenever there is not one available, "
             "so that windows will always open quickly.</p>"
             "<p><b>Warning:</b> In some cases, it is actually possible that this will "
             "reduce perceived performance.</p>"));
    connect(cb_preload_on_startup, SIGNAL(toggled(bool)), SIGNAL(changed()));
    connect(cb_always_have_preloaded, SIGNAL(toggled(bool)), SIGNAL(changed()));
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
