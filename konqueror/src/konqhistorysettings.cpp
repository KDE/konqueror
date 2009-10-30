/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "konqhistorysettings.h"

// KDE
#include <kapplication.h>
#include <kconfig.h>
#include <ksharedconfig.h>
#include <kglobal.h>
#include <kconfiggroup.h>

struct KonqHistorySettingsSingleton
{
    KonqHistorySettings self;
};

K_GLOBAL_STATIC(KonqHistorySettingsSingleton, globalHistorySettings)
KonqHistorySettings* KonqHistorySettings::self()
{
    return &globalHistorySettings->self;
}

KonqHistorySettings::KonqHistorySettings()
    : QObject( 0 )
{
    m_fontOlderThan.setItalic( true ); // default

    new KonqHistorySettingsAdaptor( this );
    const QString dbusPath = "/KonqHistorySettings";
    const QString dbusInterface = "org.kde.Konqueror.SidebarHistorySettings";
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    dbus.connect(QString(), dbusPath, dbusInterface, "notifySettingsChanged", this, SLOT(slotSettingsChanged()));

    readSettings(false);
}

KonqHistorySettings::~KonqHistorySettings()
{
}

void KonqHistorySettings::readSettings(bool reparse)
{
    KSharedConfigPtr config;

    if (KGlobal::mainComponent().componentName() == QLatin1String("konqueror"))
      config = KGlobal::config();
    else
      config = KSharedConfig::openConfig("konquerorrc");

    if (reparse) {
        config->reparseConfiguration();
    }

    const KConfigGroup cg( config, "HistorySettings");
    m_valueYoungerThan = cg.readEntry("Value youngerThan", 1 );
    m_valueOlderThan = cg.readEntry("Value olderThan", 2 );

    const QString days = QString::fromLatin1("days");
    const QString metricY = cg.readEntry("Metric youngerThan", days );
    m_metricYoungerThan = (metricY == days) ? DAYS : MINUTES;
    const QString metricO = cg.readEntry("Metric olderThan", days );
    m_metricOlderThan = (metricO == days) ? DAYS : MINUTES;

    m_fontYoungerThan = cg.readEntry( "Font youngerThan", m_fontYoungerThan );
    m_fontOlderThan   = cg.readEntry( "Font olderThan", m_fontOlderThan );

    m_detailedTips = cg.readEntry("Detailed Tooltips", true);
    m_sortsByName = cg.readEntry( "SortHistory", "byDate" ) == "byName";
}

void KonqHistorySettings::applySettings()
{
    KConfigGroup config(KSharedConfig::openConfig("konquerorrc"), "HistorySettings");

    config.writeEntry("Value youngerThan", m_valueYoungerThan );
    config.writeEntry("Value olderThan", m_valueOlderThan );

    const QString minutes = QString::fromLatin1("minutes");
    const QString days = QString::fromLatin1("days");
    config.writeEntry("Metric youngerThan", m_metricYoungerThan == DAYS ?  days : minutes );
    config.writeEntry("Metric olderThan", m_metricOlderThan == DAYS ?  days : minutes );

    config.writeEntry("Font youngerThan", m_fontYoungerThan );
    config.writeEntry("Font olderThan", m_fontOlderThan );

    config.writeEntry("Detailed Tooltips", m_detailedTips);
    config.writeEntry( "SortHistory", m_sortsByName ? "byName" : "byDate" );

    // notify konqueror instances about the new configuration
    emit notifySettingsChanged();
}

void KonqHistorySettings::slotSettingsChanged()
{
    readSettings(true /*reparse*/);
    emit settingsChanged();
}

#include "konqhistorysettings.moc"
