/*
    Extension Manager for Konqueror

    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004      by Arend van Beelen jr.  <arend@auton.nl>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

// Own
#include "konqextensionmanager.h"

// Qt
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

// KDE
#include <kxmlguifactory.h>
#include <kconfig.h>
#include "konqdebug.h"
#include <KAboutData>
#include <KLocalizedString>
#include <kparts/plugin.h>
#include <kplugininfo.h>
#include <kpluginselector.h>
#include <ksettings/dispatcher.h>
#include <kstandardguiitem.h>
#include <KSharedConfig>
#include <KConfigGroup>

// Local
#include "konqview.h"
#include "konqmainwindow.h"

class KonqExtensionManagerPrivate
{
public:
    KPluginSelector *pluginSelector;
    KonqMainWindow *mainWindow;
    KParts::ReadOnlyPart *activePart;
    QDialogButtonBox *buttonBox;
    bool isChanged = false;
};

KonqExtensionManager::KonqExtensionManager(QWidget *parent, KonqMainWindow *mainWindow, KParts::ReadOnlyPart *activePart)
    : QDialog(parent)
{
    setObjectName(QStringLiteral("extensionmanager"));
    setWindowTitle(i18nc("@title:window", "Configure"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    d = new KonqExtensionManagerPrivate;

    resize(QSize(640, 480)); // FIXME: hard-coded values ?

    d->pluginSelector = new KPluginSelector(this);
    mainLayout->addWidget(d->pluginSelector);
    connect(d->pluginSelector, SIGNAL(changed(bool)), this, SLOT(setChanged(bool)));
    connect(d->pluginSelector, SIGNAL(configCommitted(QByteArray)),
            this, SLOT(reparseConfiguration(QByteArray)));

    d->mainWindow = mainWindow;
    d->activePart = activePart;

    d->pluginSelector->addPlugins(QStringLiteral("konqueror"), i18n("Extensions"), QStringLiteral("Extensions"), KSharedConfig::openConfig());
    if (activePart) {
        KAboutData componentData = activePart->componentData();
        d->pluginSelector->addPlugins(componentData.componentName(), i18n("Extensions"), QStringLiteral("Tools"));
        d->pluginSelector->addPlugins(componentData.componentName(), i18n("Extensions"), QStringLiteral("Statusbar"));
    }

    d->buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::RestoreDefaults|QDialogButtonBox::Apply);
    QPushButton *okButton = d->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(d->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(d->buttonBox);
    connect(okButton, SIGNAL(clicked()), SLOT(slotOk()));
    connect(d->buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), SLOT(slotApply()));
    connect(d->buttonBox->button(QDialogButtonBox::RestoreDefaults), SIGNAL(clicked()), SLOT(slotDefault()));

    d->pluginSelector->load();
}

KonqExtensionManager::~KonqExtensionManager()
{
    delete d;
}

void KonqExtensionManager::reparseConfiguration(const QByteArray &conf)
{
    KSettings::Dispatcher::reparseConfiguration(conf);
}

void KonqExtensionManager::setChanged(bool c)
{
    d->isChanged = c;
    d->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(c);
}

void KonqExtensionManager::apply()
{
    if (d->isChanged) {
        d->pluginSelector->save();
        setChanged(false);

        if (d->mainWindow) {
            KParts::Plugin::loadPlugins(d->mainWindow, d->mainWindow, QStringLiteral("konqueror"));
            QList<KParts::Plugin *> plugins = KParts::Plugin::pluginObjects(d->mainWindow);

            for (int i = 0; i < plugins.size(); ++i) {
                d->mainWindow->factory()->addClient(plugins.at(i));
            }
        }

        if (d->activePart) {
            KParts::Plugin::loadPlugins(d->activePart, d->activePart, d->activePart->componentName());
            QList<KParts::Plugin *> plugins = KParts::Plugin::pluginObjects(d->activePart);

            for (int i = 0; i < plugins.size(); ++i) {
                d->activePart->factory()->addClient(plugins.at(i));
            }
        }
    }
}

void KonqExtensionManager::slotOk()
{
    apply();
    accept();
}

void KonqExtensionManager::slotApply()
{
    apply();
}

void KonqExtensionManager::slotDefault()
{
    d->pluginSelector->defaults();
}

void KonqExtensionManager::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
}

