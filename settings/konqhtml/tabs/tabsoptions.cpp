/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tabsoptions.h"

#include "ui_tabsoptions.h"

#include <KConfigGroup>
#include <QDBusMessage>
#include <QDBusConnection>

TabsOptions::TabsOptions(QObject* parent, const KPluginMetaData& md, const QVariantList& args) : KCModule(parent, md),
    m_ui(new Ui::TabsOptions)
{
    Q_UNUSED(args);
    m_config = KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);
    m_ui->setupUi(this->widget());
    connect(m_ui->m_pShowMMBInTabs, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pDynamicTabbarHide, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pNewTabsInBackground, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pOpenAfterCurrentPage, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pTabConfirm, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pTabCloseActivatePrevious, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pPermanentCloseButton, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pKonquerorTabforExternalURL, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pPopupsWithinTabs, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
    connect(m_ui->m_pMiddleClickClose, &QAbstractButton::toggled, this, [this](bool){setNeedsSave(true);});
}

TabsOptions::~TabsOptions() noexcept
{
}

void TabsOptions::defaults()
{
    bool old = m_config->readDefaults();
    m_config->setReadDefaults(true);
    load();
    m_config->setReadDefaults(old);
}

void TabsOptions::load()
{
    //TODO KF6: move these settings to a Tabs group
    KConfigGroup cg = m_config->group("FMSettings");
    m_ui->m_pShowMMBInTabs->setChecked(cg.readEntry("MMBOpensTab", true));
    m_ui->m_pDynamicTabbarHide->setChecked(!(cg.readEntry("AlwaysTabbedMode", false)));
    m_ui->m_pNewTabsInBackground->setChecked(!(cg.readEntry("NewTabsInFront", false)));
    m_ui->m_pOpenAfterCurrentPage->setChecked(cg.readEntry("OpenAfterCurrentPage", false));
    m_ui->m_pPermanentCloseButton->setChecked(cg.readEntry("PermanentCloseButton", true));
    m_ui->m_pKonquerorTabforExternalURL->setChecked(cg.readEntry("KonquerorTabforExternalURL", false));
    m_ui->m_pPopupsWithinTabs->setChecked(cg.readEntry("PopupsWithinTabs", false));
    m_ui->m_pTabCloseActivatePrevious->setChecked(cg.readEntry("TabCloseActivatePrevious", false));
    m_ui->m_pMiddleClickClose->setChecked(cg.readEntry("MouseMiddleClickClosesTab", false));

    cg = KConfigGroup(m_config, "Notification Messages");
    m_ui->m_pTabConfirm->setChecked(!cg.hasKey("MultipleTabConfirm"));
}

void TabsOptions::save()
{
    //TODO KF6: move these settings to a Tabs group
    KConfigGroup cg(m_config, "FMSettings");
    cg.writeEntry("MMBOpensTab", m_ui->m_pShowMMBInTabs->isChecked());
    cg.writeEntry("AlwaysTabbedMode", !(m_ui->m_pDynamicTabbarHide->isChecked()));

    cg.writeEntry("NewTabsInFront", !(m_ui->m_pNewTabsInBackground->isChecked()));
    cg.writeEntry("OpenAfterCurrentPage", m_ui->m_pOpenAfterCurrentPage->isChecked());
    cg.writeEntry("PermanentCloseButton", m_ui->m_pPermanentCloseButton->isChecked());
    cg.writeEntry("KonquerorTabforExternalURL", m_ui->m_pKonquerorTabforExternalURL->isChecked());
    cg.writeEntry("PopupsWithinTabs", m_ui->m_pPopupsWithinTabs->isChecked());
    cg.writeEntry("TabCloseActivatePrevious", m_ui->m_pTabCloseActivatePrevious->isChecked());
    cg.writeEntry("MouseMiddleClickClosesTab", m_ui->m_pMiddleClickClose->isChecked());
    cg.sync();
    // It only matters whether the key is present, its value has no meaning
    cg = KConfigGroup(m_config, "Notification Messages");
    if (m_ui->m_pTabConfirm->isChecked()) {
        cg.deleteEntry("MultipleTabConfirm");
    } else {
        cg.writeEntry("MultipleTabConfirm", true);
    }
    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);

    setNeedsSave(false);
}



