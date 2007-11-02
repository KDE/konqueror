/**
 *  Copyright (c) 2001 David Faure <david@mandrakesoft.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Behaviour options for konqueror

// Own
#include "behaviour.h"

// Qt
#include <QtDBus/QtDBus>
#include <QtGui/QCheckBox>
#include <QtGui/QLayout>
#include <QtGui/QLabel>

// KDE
#include <klocale.h>
#include <konq_defaults.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kconfiggroup.h>

// Local
#include "konqkcmfactory.h"

KBehaviourOptions::KBehaviourOptions(QWidget *parent, const QVariantList &)
    : KCModule(KonqKcmFactory::componentData(), parent)
    , g_pConfig(KSharedConfig::openConfig("konquerorrc", KConfig::IncludeGlobals))
    , groupname("FMSettings")
{
    setQuickHelp(i18n("<h1>Konqueror Behavior</h1> You can configure how Konqueror behaves as a file manager here."));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox * miscGb = new QGroupBox(i18n("Misc Options"), this);
    QHBoxLayout *miscHLayout = new QHBoxLayout;
    QVBoxLayout *miscLayout = new QVBoxLayout;

    winPixmap = new QLabel(this);
    winPixmap->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    winPixmap->setPixmap(QPixmap(KStandardDirs::locate("data", "kcontrol/pics/onlyone.png")));
    winPixmap->setFixedSize(winPixmap->sizeHint());

    cbNewWin = new QCheckBox(i18n("Open folders in separate &windows"), this);
    cbNewWin->setWhatsThis(i18n("If this option is checked, Konqueror will open a new window when "
                                "you open a folder, rather than showing that folder's contents in the current window."));
    connect(cbNewWin, SIGNAL(clicked()), this, SLOT(changed()));
    connect(cbNewWin, SIGNAL(toggled(bool)), SLOT(updateWinPixmap(bool)));

    miscLayout->addWidget(cbNewWin);

    cbShowTips = new QCheckBox(i18n("Show file &tips"), this);
    connect(cbShowTips, SIGNAL(clicked()), this, SLOT(changed()));

    cbShowTips->setWhatsThis( i18n("Here you can control if, when moving the mouse over a file, you want to see a "
                                    "small popup window with additional information about that file"));

    connect(cbShowTips, SIGNAL(toggled(bool)), SLOT(slotShowTips(bool)));

    miscLayout->addWidget(cbShowTips);
/*
    //connect(cbShowTips, SIGNAL(toggled(bool)), sbToolTip, SLOT(setEnabled(bool)));
    //connect(cbShowTips, SIGNAL(toggled(bool)), fileTips, SLOT(setEnabled(bool)));
    fileTips->setBuddy(sbToolTip);
    QString tipstr = i18n("If you move the mouse over a file, you usually see a small popup window that shows some "
                          "additional information about that file. Here, you can set how many items of information "
                          "are displayed");
    QWhatsThis::add( fileTips, tipstr );
    QWhatsThis::add( sbToolTip, tipstr );
*/

    QHBoxLayout *previewLayout = new QHBoxLayout;
    QWidget* spacer = new QWidget(this);
    spacer->setMinimumSize( 20, 0 );
    spacer->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

    previewLayout->addWidget(spacer);

    cbShowPreviewsInTips = new QCheckBox(i18n("Show &previews in file tips"), this);
    connect(cbShowPreviewsInTips, SIGNAL(clicked()), this, SLOT(changed()));

    cbShowPreviewsInTips->setWhatsThis( i18n("Here you can control if you want the "
                                             "popup window to contain a larger preview for the file, "
                                             "when moving the mouse over it."));

    previewLayout->addWidget(cbShowPreviewsInTips);

    miscLayout->addLayout(previewLayout);

    miscHLayout->addLayout(miscLayout);
    miscHLayout->addWidget(winPixmap);

    miscGb->setLayout(miscHLayout);

    mainLayout->addWidget(miscGb);

    QGroupBox *bg = new QGroupBox(this);
    bg->setTitle(i18n("Ask Confirmation For"));
    bg->setWhatsThis(i18n("This option tells Konqueror whether to ask"
                          " for a confirmation when you \"delete\" a file."
                          " <ul><li><em>Move To Trash:</em> moves the file to your trash folder,"
                          " from where it can be recovered very easily.</li>"
                          " <li><em>Delete:</em> simply deletes the file.</li>"
                          " </ul>"));

    cbMoveToTrash = new QCheckBox(i18n("&Move to trash"), bg);
    connect(cbMoveToTrash, SIGNAL(clicked()), this, SLOT(changed()));

    cbDelete = new QCheckBox(i18n("D&elete"), bg);
    connect(cbDelete, SIGNAL(clicked()), this, SLOT(changed()));

    QVBoxLayout *confirmationLayout = new QVBoxLayout;
    confirmationLayout->addWidget(cbMoveToTrash);
    confirmationLayout->addWidget(cbDelete);
    bg->setLayout(confirmationLayout);

    mainLayout->addWidget(bg);

    cbShowDeleteCommand = new QCheckBox(i18n("Show 'Delete' context me&nu entries which bypass the trashcan"), this);
    mainLayout->addWidget(cbShowDeleteCommand);
    connect(cbShowDeleteCommand, SIGNAL(clicked()), this, SLOT(changed()));

    cbShowDeleteCommand->setWhatsThis(i18n("Check this if you want 'Delete' menu commands to be displayed "
                                           "on the desktop and in the file manager's context menus. "
                                           "You can always delete files by holding the Shift key "
                                           "while calling 'Move to Trash'."));

    mainLayout->addStretch();

    load();
}

KBehaviourOptions::~KBehaviourOptions()
{
}

void KBehaviourOptions::slotShowTips(bool b)
{
//    sbToolTip->setEnabled( b );
    cbShowPreviewsInTips->setEnabled( b );
//    fileTips->setEnabled( b );

}

void KBehaviourOptions::load()
{
	KConfigGroup cg(g_pConfig, groupname);
    cbNewWin->setChecked( cg.readEntry("AlwaysNewWin", false) );
    updateWinPixmap(cbNewWin->isChecked());

    bool stips = cg.readEntry( "ShowFileTips", true);
    cbShowTips->setChecked( stips );
    slotShowTips( stips );

    bool showPreviewsIntips = cg.readEntry( "ShowPreviewsInFileTips", true);
    cbShowPreviewsInTips->setChecked( showPreviewsIntips );

    KSharedConfig::Ptr globalconfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
	KConfigGroup cg2(globalconfig, "KDE");
    cbShowDeleteCommand->setChecked( cg2.readEntry("ShowDeleteCommand", false) );

//    if (!stips) sbToolTip->setEnabled( false );
    if (!stips) cbShowPreviewsInTips->setEnabled( false );

//    sbToolTip->setValue( g_pConfig->readEntry( "FileTipItems", 6 ) );

	KConfigGroup cg4(g_pConfig, "Trash");
    cbMoveToTrash->setChecked( cg4.readEntry("ConfirmTrash", bool(DEFAULT_CONFIRMTRASH)) );
    cbDelete->setChecked( cg4.readEntry("ConfirmDelete", bool(DEFAULT_CONFIRMDELETE)) );
}

void KBehaviourOptions::defaults()
{
    cbNewWin->setChecked(false);

    cbShowTips->setChecked( true );
    //sbToolTip->setEnabled( true );
    //sbToolTip->setValue( 6 );

    cbShowPreviewsInTips->setChecked( true );
    cbShowPreviewsInTips->setEnabled( true );

    cbMoveToTrash->setChecked( DEFAULT_CONFIRMTRASH );
    cbDelete->setChecked( DEFAULT_CONFIRMDELETE );
    cbShowDeleteCommand->setChecked( false );
}

void KBehaviourOptions::save()
{
	KConfigGroup cg(g_pConfig, groupname);

    cg.writeEntry( "AlwaysNewWin", cbNewWin->isChecked() );
    cg.writeEntry( "ShowFileTips", cbShowTips->isChecked() );
    cg.writeEntry( "ShowPreviewsInFileTips", cbShowPreviewsInTips->isChecked() );
//    g_pConfig->writeEntry( "FileTipsItems", sbToolTip->value() );

    KSharedConfig::Ptr globalconfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
	KConfigGroup cg2(globalconfig, "KDE");
    cg2.writeEntry( "ShowDeleteCommand", cbShowDeleteCommand->isChecked());
    cg2.sync();

	KConfigGroup cg3(globalconfig, "Trash");
    cg3.writeEntry( "ConfirmTrash", cbMoveToTrash->isChecked());
    cg3.writeEntry( "ConfirmDelete", cbDelete->isChecked());
    cg3.sync();

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);
}

void KBehaviourOptions::updateWinPixmap(bool b)
{
  if (b)
    winPixmap->setPixmap(QPixmap(KStandardDirs::locate("data",
                                        "kcontrol/pics/overlapping.png")));
  else
    winPixmap->setPixmap(QPixmap(KStandardDirs::locate("data",
                                        "kcontrol/pics/onlyone.png")));
}

#include "behaviour.moc"
