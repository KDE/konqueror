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

KCookiesMain::KCookiesMain(QObject *parent, const KPluginMetaData &md)
    : KCModule(parent, md)
{
    QVBoxLayout *layout = new QVBoxLayout(widget());
    tab = new QTabWidget(widget());
    layout->addWidget(tab);

    policies = new KCookiesPolicies(widget(), md);
    tab->addTab(policies->widget(), i18n("&Policy"));
    management = new KCookiesManagement(widget(), md);
    tab->addTab(management->widget(), i18n("&Management"));

    connect(policies, &KCModule::needsSaveChanged, this, &KCookiesMain::updateNeedsSave);
    connect(management, &KCModule::needsSaveChanged, this, &KCookiesMain::updateNeedsSave);
}

KCookiesMain::~KCookiesMain()
{
}

KCookiesMain::KCModule* KCookiesMain::currentModule() const
{
    if (tab->currentWidget() == policies->widget()) {
        return policies;
    } else {
        return management;
    }
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
    currentModule()->defaults();
    setRepresentsDefaults(true);
    KCModule::defaults();
}
