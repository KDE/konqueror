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
#if QT_VERSION_MAJOR < 6
    connect(konqueror_widget, &Konqueror::changed, this, &Config::markAsChanged);
#else
    connect(konqueror_widget, &Konqueror::changed, this, [this](){setNeedsSave(true);});
#endif
    tabs->addTab(konqueror_widget, i18n("Konqueror"));
    system_widget = new SystemWidget;
#if QT_VERSION_MAJOR < 6
    connect(system_widget, &SystemWidget::changed, this, &Config::markAsChanged);
#else
    connect(system_widget, &SystemWidget::changed, this, [this](){setNeedsSave(true);});
#endif
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

KonquerorConfig::KonquerorConfig(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
{
    QVBoxLayout *topLayout = new QVBoxLayout(widget());
    topLayout->setContentsMargins(0, 0, 0, 0);
    m_widget = new Konqueror(widget());
#if QT_VERSION_MAJOR < 6
    connect(m_widget, &Konqueror::changed, this, &KonquerorConfig::markAsChanged);
#else
    connect(m_widget, &Konqueror::changed, this, [this](){setNeedsSave(true);});
#endif
    topLayout->addWidget(m_widget);
}

void KonquerorConfig::load()
{
    m_widget->load();
}

void KonquerorConfig::save()
{
    m_widget->save();
}

void KonquerorConfig::defaults()
{
    m_widget->defaults();
}

} // namespace

#include "kcmperformance.moc"
