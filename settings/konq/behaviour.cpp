/*
    SPDX-FileCopyrightText: 2001 David Faure <david@mandrakesoft.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Behaviour options for konqueror

// Own
#include "behaviour.h"

// Qt
#include <QDBusConnection>
#include <QDBusMessage>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

// KDE
#include <KLocalizedString>

#include <KPluginFactory>
#include <kurlrequester.h>
#include <kconfiggroup.h>
#include <QStandardPaths>
#include <KSharedConfig>

K_PLUGIN_CLASS_WITH_JSON(KBehaviourOptions, "filebehavior.json")

KBehaviourOptions::KBehaviourOptions(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
    , g_pConfig(KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::IncludeGlobals))
    , groupname(QStringLiteral("FMSettings"))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(widget());

    QGroupBox *miscGb = new QGroupBox(i18n("Misc Options"), widget());
    QHBoxLayout *miscHLayout = new QHBoxLayout;
    QVBoxLayout *miscLayout = new QVBoxLayout;

    winPixmap = new QLabel(widget());
    winPixmap->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    winPixmap->setPixmap(QPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcontrol/pics/onlyone.png"))));
    winPixmap->setFixedSize(winPixmap->sizeHint());

    cbNewWin = new QCheckBox(i18n("Open folders in separate &windows"), widget());
    cbNewWin->setToolTip(i18n("If this option is checked, Konqueror will open a new window when "
                                "you open a folder, rather than showing that folder's contents in the current window."));
    connect(cbNewWin, &QAbstractButton::toggled, this, &KBehaviourOptions::markAsChanged);
    connect(cbNewWin, &QAbstractButton::toggled, this, &KBehaviourOptions::updateWinPixmap);

    miscLayout->addWidget(cbNewWin);

    QHBoxLayout *previewLayout = new QHBoxLayout;
    QWidget *spacer = new QWidget(widget());
    spacer->setMinimumSize(20, 0);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    previewLayout->addWidget(spacer);

    miscLayout->addLayout(previewLayout);

    miscHLayout->addLayout(miscLayout);
    miscHLayout->addWidget(winPixmap);

    miscGb->setLayout(miscHLayout);

    mainLayout->addWidget(miscGb);

    cbShowDeleteCommand = new QCheckBox(i18n("Show 'Delete' me&nu entries which bypass the trashcan"), widget());
    mainLayout->addWidget(cbShowDeleteCommand);
    connect(cbShowDeleteCommand, &QAbstractButton::toggled, this, &KBehaviourOptions::markAsChanged);

    cbShowDeleteCommand->setToolTip(i18n("Check this if you want 'Delete' menu commands to be displayed "
                                           "on the desktop and in the file manager's menus and context menus. "
                                           "You can always delete files by holding the Shift key "
                                           "while calling 'Move to Trash'."));

    mainLayout->addStretch();
}

KBehaviourOptions::~KBehaviourOptions()
{
}

void KBehaviourOptions::load()
{
    KConfigGroup cg(g_pConfig, groupname);
    cbNewWin->setChecked(cg.readEntry("AlwaysNewWin", false));
    updateWinPixmap(cbNewWin->isChecked());

    KSharedConfig::Ptr globalconfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
    KConfigGroup cg2(globalconfig, "KDE");
    cbShowDeleteCommand->setChecked(cg2.readEntry("ShowDeleteCommand", false));
}

void KBehaviourOptions::defaults()
{
    cbNewWin->setChecked(false);

    cbShowDeleteCommand->setChecked(false);
}

void KBehaviourOptions::save()
{
    KConfigGroup cg(g_pConfig, groupname);

    cg.writeEntry("AlwaysNewWin", cbNewWin->isChecked());

    KSharedConfig::Ptr globalconfig = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
    KConfigGroup cg2(globalconfig, "KDE");
    cg2.writeEntry("ShowDeleteCommand", cbShowDeleteCommand->isChecked());
    cg2.sync();

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void KBehaviourOptions::updateWinPixmap(bool b)
{
    if (b) {
        winPixmap->setPixmap(QPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcontrol/pics/overlapping.png"))));
    } else {
        winPixmap->setPixmap(QPixmap(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcontrol/pics/onlyone.png"))));
    }
}

#include "behaviour.moc"
