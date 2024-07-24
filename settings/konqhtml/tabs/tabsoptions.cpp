/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "tabsoptions.h"

#include "konqsettings.h"

#include "ui_tabsoptions.h"

#include <QDBusMessage>
#include <QDBusConnection>

using namespace Konq;

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
    connect(m_ui->tabbarPosition, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){setNeedsSave(true);});
}

TabsOptions::~TabsOptions() noexcept
{
}

void TabsOptions::defaults()
{
    Konq::Settings::self()->withDefaults([this]{load();});
    //This settings isn't read using Konq::Settings, so it must be manually set
    //to its default value
    m_ui->m_pTabConfirm->setChecked(true);
    setRepresentsDefaults(true);
    KCModule::defaults();
}

void TabsOptions::load()
{
    //TODO KF6: move these settings to a Tabs group
    m_ui->m_pShowMMBInTabs->setChecked(Settings::mmbOpensTab());
    m_ui->m_pDynamicTabbarHide->setChecked(!Settings::alwaysTabbedMode());
    m_ui->m_pNewTabsInBackground->setChecked(!Settings::newTabsInFront());
    m_ui->m_pOpenAfterCurrentPage->setChecked(Settings::openAfterCurrentPage());
    m_ui->m_pPermanentCloseButton->setChecked(Settings::permanentCloseButton());
    m_ui->m_pKonquerorTabforExternalURL->setChecked(Settings::konquerorTabforExternalURL());
    m_ui->m_pPopupsWithinTabs->setChecked(Settings::popupsWithinTabs());
    m_ui->m_pTabCloseActivatePrevious->setChecked(Settings::tabCloseActivatePrevious());
    m_ui->m_pMiddleClickClose->setChecked(Settings::mouseMiddleClickClosesTab());
    m_ui->tabbarPosition->setCurrentIndex(Settings::tabBarPosition());
    // Because of how KMessageBox works, if the key exists, the dialog is never shown
    // and it's shown only if the key doesn't exist. This means we can't use Konq::Settings
    // to read this
    KConfigGroup cg(m_config, "Notification Messages");
    m_ui->m_pTabConfirm->setChecked(!cg.hasKey("MultipleTabConfirm"));
    KCModule::load();
}

void TabsOptions::save()
{
    KConfigGroup cg(m_config, "FMSettings");
    Settings::setMmbOpensTab(m_ui->m_pShowMMBInTabs->isChecked());
    Settings::setAlwaysTabbedMode(!(m_ui->m_pDynamicTabbarHide->isChecked()));
    Settings::setNewTabsInFront(!(m_ui->m_pNewTabsInBackground->isChecked()));
    Settings::setOpenAfterCurrentPage(m_ui->m_pOpenAfterCurrentPage->isChecked());
    Settings::setPermanentCloseButton(m_ui->m_pPermanentCloseButton->isChecked());
    Settings::setKonquerorTabforExternalURL(m_ui->m_pKonquerorTabforExternalURL->isChecked());
    Settings::setPopupsWithinTabs(m_ui->m_pPopupsWithinTabs->isChecked());
    Settings::setTabCloseActivatePrevious(m_ui->m_pTabCloseActivatePrevious->isChecked());
    Settings::setMouseMiddleClickClosesTab(m_ui->m_pMiddleClickClose->isChecked());
    Settings::setTabBarPosition(m_ui->tabbarPosition->currentIndex());
    Settings::self()->save();
    // Because of how KMessageBox works, if the key exists, the dialog is never shown
    // and it's shown only if the key doesn't exist. This means we can't use Konq::Settings
    // to write this
    // It only matters whether the key is present, its value has no meaning
    cg = KConfigGroup(m_config, "Notification Messages");
    if (m_ui->m_pTabConfirm->isChecked()) {
        cg.deleteEntry("MultipleTabConfirm");
    } else {
        cg.writeEntry("MultipleTabConfirm", 0);
    }
    cg.sync();
    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);

    KCModule::save();
}



