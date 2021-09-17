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
K_PLUGIN_FACTORY(KCMPerformanceConfigFactory,
                 registerPlugin<KCMPerformance::Config>("performance");
                 registerPlugin<KCMPerformance::KonquerorConfig>("konqueror");
                )

namespace KCMPerformance
{

Config::Config(QWidget *parent_P, const QVariantList &)
    : KCModule(parent_P)
{
    setQuickHelp(i18n("<h1>KDE Performance</h1>"
                      " You can configure settings that improve KDE performance here."));

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    QTabWidget *tabs = new QTabWidget(this);
    konqueror_widget = new Konqueror;
    connect(konqueror_widget, &Konqueror::changed, this, &Config::markAsChanged);
    tabs->addTab(konqueror_widget, i18n("Konqueror"));
    system_widget = new SystemWidget;
    connect(system_widget, &SystemWidget::changed, this, &Config::markAsChanged);
    tabs->addTab(system_widget, i18n("System"));
    topLayout->addWidget(tabs);
}

void Config::load()
{
    konqueror_widget->load();
    system_widget->load();
}

void Config::save()
{
    konqueror_widget->save();
    system_widget->save();
}

void Config::defaults()
{
    konqueror_widget->defaults();
    system_widget->defaults();
}

KonquerorConfig::KonquerorConfig(QWidget *parent_P, const QVariantList &)
    : KCModule(parent_P)
{
    setQuickHelp(i18n("<h1>Konqueror Performance</h1>"
                      " You can configure several settings that improve Konqueror performance here."
                      " These include options for reusing already running instances"
                      " and for keeping instances preloaded."));

    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    widget = new Konqueror(this);
    connect(widget, &Konqueror::changed, this, &KonquerorConfig::markAsChanged);
    topLayout->addWidget(widget);
}

void KonquerorConfig::load()
{
    widget->load();
}

void KonquerorConfig::save()
{
    widget->save();
}

void KonquerorConfig::defaults()
{
    widget->defaults();
}

} // namespace

#include "kcmperformance.moc"
