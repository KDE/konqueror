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
#include "konqdebug.h"
#include "konq_kpart_plugin.h"
#include <KPluginMetaData>
#include <KConfigGroup>
#include <KFileUtils>
#include <KLocalizedString>
#include <KPluginWidget>
#include <KSharedConfig>
#include <ksettings/dispatcher.h>
#include <kstandardguiitem.h>

// Local
#include "konqview.h"
#include "konqmainwindow.h"

class KonqExtensionManagerPrivate
{
public:
    KPluginWidget *pluginSelector;
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

    d->pluginSelector = new KPluginWidget(this);
    mainLayout->addWidget(d->pluginSelector);
    connect(d->pluginSelector, SIGNAL(changed(bool)), this, SLOT(setChanged(bool)));
    connect(d->pluginSelector, &KPluginWidget::pluginConfigSaved, this, [this](const QString &pluginId) {
        reparseConfiguration(pluginId.toLocal8Bit());
    });

    d->mainWindow = mainWindow;
    d->activePart = activePart;

    auto addPluginForId = [this](const QString &pluginId) {
        QVector<KPluginMetaData> metaDataList = KPluginMetaData::findPlugins(pluginId + QStringLiteral("/kpartplugins"));
        d->pluginSelector->addPlugins(metaDataList, i18n("Extensions"));
    };
    if (activePart) {
        d->pluginSelector->setConfig(KSharedConfig::openConfig(activePart->metaData().pluginId() + QLatin1String("rc"))->group("KParts Plugins"));
        addPluginForId(activePart->metaData().pluginId());
    } else {
        d->pluginSelector->setConfig(KSharedConfig::openConfig(QStringLiteral("konquerorrc"))->group("KParts Plugins"));
    }
    addPluginForId(QStringLiteral("konqueror")); // Always add the plugins from the konqueror namespace

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
}

KonqExtensionManager::~KonqExtensionManager()
{
    delete d;
}

void KonqExtensionManager::reparseConfiguration(const QByteArray &conf)
{
    KSharedConfig::openConfig(conf + QLatin1String("rc"))->reparseConfiguration();
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
            QList<KonqParts::Plugin *> plugins = KonqParts::Plugin::pluginObjects(d->mainWindow);

            for (int i = 0; i < plugins.size(); ++i) {
                d->mainWindow->factory()->addClient(plugins.at(i));
            }
        }

        if (d->activePart) {
            KonqParts::Plugin::loadPlugins(d->activePart, d->activePart, d->activePart->componentName());
            QList<KonqParts::Plugin *> plugins = KonqParts::Plugin::pluginObjects(d->activePart);

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

