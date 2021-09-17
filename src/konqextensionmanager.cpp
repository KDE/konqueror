/*
    Extension Manager for Konqueror

    SPDX-FileCopyrightText: 2003 Martijn Klingens <klingens@kde.org>
    SPDX-FileCopyrightText: 2004 Arend van Beelen jr. <arend@auton.nl>

    SPDX-License-Identifier: GPL-2.0-or-later
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
#include <kparts_version.h>
#if KPARTS_VERSION >= QT_VERSION_CHECK(5, 77, 0)
#include <KPluginMetaData>
#else
#include <KAboutData>
#endif
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
    setWindowTitle(i18nc("@title:window", "Configure Extensions"));

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
#if KPARTS_VERSION >= QT_VERSION_CHECK(5, 77, 0)
        const QString pluginId = activePart->metaData().pluginId();
#else
        const QString pluginId = activePart->componentData().componentName();
#endif
        d->pluginSelector->addPlugins(pluginId, i18n("Extensions"), QStringLiteral("Tools"));
        d->pluginSelector->addPlugins(pluginId, i18n("Extensions"), QStringLiteral("Statusbar"));
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

