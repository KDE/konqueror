/*
    SPDX-FileCopyrightText: 2003 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "kcmperformance.h"

// Qt
#include <QTabWidget>

// KDE
#include <KLocalizedString>
// Local
#include "konqueror.h"
#include "system.h"
#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KCMPerformanceConfigFactory, "kcmperformance.json", registerPlugin<KCMPerformance::KonquerorConfig>();)

namespace KCMPerformance
{

Config::Config(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
{
    QVBoxLayout *topLayout = new QVBoxLayout(widget());
    QTabWidget *tabs = new QTabWidget(widget());
    konqueror_widget = new Konqueror;
    connect(konqueror_widget, &Konqueror::changed, this, [this](){setNeedsSave(true);});
    tabs->addTab(konqueror_widget, i18n("Konqueror"));
    system_widget = new SystemWidget;
    connect(system_widget, &SystemWidget::changed, this, [this](){setNeedsSave(true);});
    tabs->addTab(system_widget, i18n("System"));
    topLayout->addWidget(tabs);
}

void Config::load()
{
    konqueror_widget->load();
    system_widget->load();
    KCModule::load();
}

void Config::save()
{
    konqueror_widget->save();
    system_widget->save();
    KCModule::save();
}

void Config::defaults()
{
    konqueror_widget->defaults();
    system_widget->defaults();
    setRepresentsDefaults(true);
}

KonquerorConfig::KonquerorConfig(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
{
    QVBoxLayout *topLayout = new QVBoxLayout(widget());
    topLayout->setContentsMargins(0, 0, 0, 0);
    m_widget = new Konqueror(widget());
    connect(m_widget, &Konqueror::changed, this, [this](){setNeedsSave(true);});
    topLayout->addWidget(m_widget);
}

void KonquerorConfig::load()
{
    m_widget->load();
    KCModule::load();
}

void KonquerorConfig::save()
{
    m_widget->save();
    KCModule::load();
}

void KonquerorConfig::defaults()
{
    m_widget->defaults();
    setRepresentsDefaults(true);
}

} // namespace

#include "kcmperformance.moc"
