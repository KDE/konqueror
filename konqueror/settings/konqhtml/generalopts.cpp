////
// Add here all general options - those that apply to both web browsing and filemanagement mode
//
// (c) Sven Radej 1998
// (c) David Faure 1998
// (c) 2001 Waldo Bastian <bastian@kde.org>
// (c) 2007 Nick Shaforostoff <shafff@ukr.net>

// Own
#include "generalopts.h"

// Qt
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtGui/QGroupBox>
#include <QtGui/QLayout>

// KDE
#include <konq_defaults.h> // include default values directly from konqueror
#include <kapplication.h>
#include <kgenericfactory.h>

// Local
#include "ui_advancedTabOptions.h"
#include "khtml_settings.h"

typedef KGenericFactory<KKonqGeneralOptions, QWidget> KKonqGeneralOptionsFactory;
K_EXPORT_COMPONENT_FACTORY( khtml_general, KKonqGeneralOptionsFactory("kcmkonqhtml") )

//-----------------------------------------------------------------------------

KKonqGeneralOptions::KKonqGeneralOptions(QWidget *parent, const QStringList&)
    : KCModule( KKonqGeneralOptionsFactory::componentData(), parent )
{
    m_pConfig = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    int row = 0;
    QGridLayout *lay = new QGridLayout(this);
    QGroupBox* tabsGroup = new QGroupBox(i18n("Tabbed Browsing"));

    tabOptions = new Ui_advancedTabOptions;
    tabOptions->setupUi(tabsGroup);

    connect(tabOptions->m_pShowMMBInTabs, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pDynamicTabbarHide, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pNewTabsInBackground, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pOpenAfterCurrentPage, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pTabConfirm, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pTabCloseActivatePrevious, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pPermanentCloseButton, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pKonquerorTabforExternalURL, SIGNAL(clicked()), SLOT(slotChanged()));
    connect(tabOptions->m_pPopupsWithinTabs, SIGNAL(clicked()), SLOT(slotChanged()));

    lay->addWidget( tabsGroup, row, 0, 1, 2 );
//    row++;

    lay->setRowStretch(row, 1);

    load();
    emit changed(false);
}

KKonqGeneralOptions::~KKonqGeneralOptions()
{
    delete tabOptions;
}

void KKonqGeneralOptions::load()
{
    KConfigGroup cg(m_pConfig, QByteArray(""));

    cg.changeGroup("FMSettings");
    tabOptions->m_pShowMMBInTabs->setChecked( cg.readEntry( "MMBOpensTab", false ) );
    tabOptions->m_pDynamicTabbarHide->setChecked( ! (cg.readEntry( "AlwaysTabbedMode", false )) );

    tabOptions->m_pNewTabsInBackground->setChecked( ! (cg.readEntry( "NewTabsInFront", false)) );
    tabOptions->m_pOpenAfterCurrentPage->setChecked( cg.readEntry( "OpenAfterCurrentPage", false) );
    tabOptions->m_pPermanentCloseButton->setChecked( cg.readEntry( "PermanentCloseButton", false) );
    tabOptions->m_pKonquerorTabforExternalURL->setChecked( cg.readEntry( "KonquerorTabforExternalURL", false) );
    tabOptions->m_pPopupsWithinTabs->setChecked( cg.readEntry( "PopupsWithinTabs", false) );
    tabOptions->m_pTabCloseActivatePrevious->setChecked( cg.readEntry( "TabCloseActivatePrevious", false) );

    cg.changeGroup("Notification Messages");
    tabOptions->m_pTabConfirm->setChecked( !cg.hasKey("MultipleTabConfirm") );

}

void KKonqGeneralOptions::defaults()
{
    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
}

void KKonqGeneralOptions::save()
{
    KConfigGroup cg(m_pConfig, "FMSettings");
    cg.writeEntry( "MMBOpensTab", tabOptions->m_pShowMMBInTabs->isChecked() );
    cg.writeEntry( "AlwaysTabbedMode", !(tabOptions->m_pDynamicTabbarHide->isChecked()) );

    cg.writeEntry( "NewTabsInFront", !(tabOptions->m_pNewTabsInBackground->isChecked()) );
    cg.writeEntry( "OpenAfterCurrentPage", tabOptions->m_pOpenAfterCurrentPage->isChecked() );
    cg.writeEntry( "PermanentCloseButton", tabOptions->m_pPermanentCloseButton->isChecked() );
    cg.writeEntry( "KonquerorTabforExternalURL", tabOptions->m_pKonquerorTabforExternalURL->isChecked() );
    cg.writeEntry( "PopupsWithinTabs", tabOptions->m_pPopupsWithinTabs->isChecked() );
    cg.writeEntry( "TabCloseActivatePrevious", tabOptions->m_pTabCloseActivatePrevious->isChecked() );

    cg.sync();

    // It only matters wether the key is present, its value has no meaning
    cg.changeGroup("Notification Messages");
    if ( tabOptions->m_pTabConfirm->isChecked() )
        cg.deleteEntry( "MultipleTabConfirm" );
    else
        cg.writeEntry( "MultipleTabConfirm", true );

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);

    emit changed(false);
}


void KKonqGeneralOptions::slotChanged()
{
    emit changed(true);
}

#include "generalopts.moc"

