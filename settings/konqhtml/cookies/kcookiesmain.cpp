/*
    kcookiesmain.cpp - Cookies configuration

    First version of cookies configuration:
        SPDX-FileCopyrightText: Waldo Bastian <bastian@kde.org>
    This dialog box:
        SPDX-FileCopyrightText: David Faure <faure@kde.org>
*/

// Own
#include "kcookiesmain.h"

// Local
#include "kcookiesmanagement.h"
#include "kcookiespolicies.h"

// Qt
#include <QTabWidget>
#include <QtGlobal>

// KDE
#include <KLocalizedString>
#include <KPluginFactory>

KCookiesMain::KCookiesMain(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
{
    management = nullptr;

    QVBoxLayout *layout = new QVBoxLayout(widget());
    tab = new QTabWidget(widget());
    layout->addWidget(tab);

    policies = new KCookiesPolicies(widget(), md);
    tab->addTab(policies->widget(), i18n("&Policy"));
    management = new KCookiesManagement(widget(), md);
    tab->addTab(management->widget(), i18n("&Management"));

#if QT_VERSION_MAJOR < 6
    connect(policies, QOverload<bool>::of(&KCModule::changed), this, QOverload<bool>::of(&KCModule::changed));
    connect(management, QOverload<bool>::of(&KCModule::changed), this, QOverload<bool>::of(&KCModule::changed));
#else
    connect(policies, &KCModule::needsSaveChanged, this, &KCookiesMain::updateNeedsSave);
    connect(management, &KCModule::needsSaveChanged, this, &KCookiesMain::updateNeedsSave);
#endif
}

KCookiesMain::~KCookiesMain()
{
}

void KCookiesMain::updateNeedsSave()
{
    setNeedsSave(policies->needsSave() || management->needsSave());
}

void KCookiesMain::save()
{
    policies->save();
    if (management) {
        management->save();
    }
    KCModule::save();
}

void KCookiesMain::load()
{
    policies->load();
    if (management) {
        management->load();
    }
    KCModule::load();
}

void KCookiesMain::defaults()
{
    if (tab->currentWidget() == policies->widget()) {
        policies->defaults();
    } else if (management) {
        management->defaults();
    }

#if QT_VERSION_MAJOR > 5
    setRepresentsDefaults(true);
#endif
}
