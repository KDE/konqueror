/*
 * Add here all general options - those that apply to both web browsing and filemanagement mode
 *
 * Copyright (c) Sven Radej 1998
 * Copyright (c) David Faure 1998
 * Copyright (c) 2001 Waldo Bastian <bastian@kde.org>
 * Copyright (c) 2007 Nick Shaforostoff <shafff@ukr.net>
 *
 */

// Own
#include "generalopts.h"
#include <kdebug.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kcombobox.h>

// Qt
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtGui/QGroupBox>
#include <QtGui/QLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

// KDE
#include <kapplication.h>
#include <kurlrequester.h>

// Local
#include "ui_advancedTabOptions.h"
#include "khtml_settings.h"
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY_DECLARATION(KcmKonqHtmlFactory)

// Keep in sync with konqueror.kcfg
static const char* DEFAULT_HOMEPAGE = "http://www.kde.org";
enum StartPage { ShowHomePage, ShowBlankPage, ShowAboutPage };

//-----------------------------------------------------------------------------

KKonqGeneralOptions::KKonqGeneralOptions(QWidget *parent, const QVariantList&)
    : KCModule( KcmKonqHtmlFactory::componentData(), parent )
{
    m_pConfig = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setMargin(0);
    lay->setSpacing(KDialog::spacingHint());

    addHomeUrlWidgets(lay);

    QGroupBox* tabsGroup = new QGroupBox(i18n("Tabbed Browsing"));

    tabOptions = new Ui_advancedTabOptions;
    tabOptions->setupUi(tabsGroup);

    connect(tabOptions->m_pShowMMBInTabs, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pDynamicTabbarHide, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pNewTabsInBackground, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pOpenAfterCurrentPage, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pTabConfirm, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pTabCloseActivatePrevious, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pPermanentCloseButton, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pKonquerorTabforExternalURL, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pPopupsWithinTabs, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    connect(tabOptions->m_pMiddleClickClose, SIGNAL(toggled(bool)), SLOT(slotChanged()));

    lay->addWidget(tabsGroup);

    load();
    emit changed(false);
}

void KKonqGeneralOptions::addHomeUrlWidgets(QVBoxLayout* lay)
{
    QHBoxLayout *startLayout = new QHBoxLayout;
    lay->addLayout(startLayout);

    QLabel* startLabel = new QLabel(i18nc("@label:listbox", "When &Konqueror starts:"), this);
    startLayout->addWidget(startLabel);

    m_startCombo = new KComboBox(this);
    m_startCombo->setEditable(false);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show the introduction page"), ShowAboutPage);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show my home page"), ShowHomePage);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show a blank page"), ShowBlankPage);
    startLayout->addWidget(m_startCombo);
    connect(m_startCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotChanged()));

    startLabel->setBuddy(m_startCombo);

    ////

    QHBoxLayout *homeLayout = new QHBoxLayout;
    QLabel *label = new QLabel(i18n("Home Page:"), this);
    homeLayout->addWidget(label);

    homeURL = new KUrlRequester(this);
    homeURL->setMode(KFile::Directory);
    homeURL->setWindowTitle(i18n("Select Home Page"));
    homeLayout->addWidget(homeURL);
    connect(homeURL, SIGNAL(textChanged(QString)), SLOT(slotChanged()));
    label->setBuddy(homeURL);

    lay->addLayout(homeLayout);

    QString homestr = i18n("This is the URL of the web page where "
                           "Konqueror (as web browser) will jump to when "
                           "the \"Home\" button is pressed. When Konqueror is "
                           "started as a file manager, that button makes it jump "
                           "to your local home folder instead.");
    label->setWhatsThis(homestr);
    homeURL->setWhatsThis(homestr);
}

KKonqGeneralOptions::~KKonqGeneralOptions()
{
    delete tabOptions;
}

static QString readStartUrlFromProfile()
{
    const QString blank = "about:blank";
    const QString profile = KStandardDirs::locate("data", QLatin1String("konqueror/profiles/webbrowsing"));
    if (profile.isEmpty())
        return blank;
    KConfig cfg(profile, KConfig::SimpleConfig);
    KConfigGroup profileGroup(&cfg, "Profile");
    const QString rootItem = profileGroup.readEntry("RootItem");
    if (rootItem.isEmpty())
        return blank;
    if (rootItem.startsWith("View")) {
        const QString prefix = rootItem + '_';
        const QString urlKey = QString("URL").prepend(prefix);
        return profileGroup.readPathEntry(urlKey, blank);
    }
    // simplify the other cases: whether root is a splitter or directly the tabwidget,
    // we want to look at the first view inside the tabs, i.e. ViewT0.
    return profileGroup.readPathEntry("ViewT0_URL", blank);
}

static StartPage urlToStartPageEnum(const QString& startUrl)
{
    if (startUrl == "about:blank")
        return ShowBlankPage;
    if (startUrl == "about:" || startUrl == "about:konqueror")
        return ShowAboutPage;
    return ShowHomePage;
}

void KKonqGeneralOptions::load()
{
    KConfigGroup userSettings(m_pConfig, "UserSettings");
    homeURL->setUrl(userSettings.readEntry("HomeURL", DEFAULT_HOMEPAGE));
    const QString startUrl = readStartUrlFromProfile();
    const StartPage startPage = urlToStartPageEnum(readStartUrlFromProfile());
    const int startComboIndex = m_startCombo->findData(startPage);
    Q_ASSERT(startComboIndex != -1);
    m_startCombo->setCurrentIndex(startComboIndex);

    KConfigGroup cg(m_pConfig, "FMSettings"); // ### what a wrong group name for these settings...

    tabOptions->m_pShowMMBInTabs->setChecked( cg.readEntry( "MMBOpensTab", false ) );
    tabOptions->m_pDynamicTabbarHide->setChecked( ! (cg.readEntry( "AlwaysTabbedMode", false )) );

    tabOptions->m_pNewTabsInBackground->setChecked( ! (cg.readEntry( "NewTabsInFront", false)) );
    tabOptions->m_pOpenAfterCurrentPage->setChecked( cg.readEntry( "OpenAfterCurrentPage", false) );
    tabOptions->m_pPermanentCloseButton->setChecked( cg.readEntry( "PermanentCloseButton", false) );
    tabOptions->m_pKonquerorTabforExternalURL->setChecked( cg.readEntry( "KonquerorTabforExternalURL", false) );
    tabOptions->m_pPopupsWithinTabs->setChecked( cg.readEntry( "PopupsWithinTabs", false) );
    tabOptions->m_pTabCloseActivatePrevious->setChecked( cg.readEntry( "TabCloseActivatePrevious", false) );
    tabOptions->m_pMiddleClickClose->setChecked( cg.readEntry( "MouseMiddleClickClosesTab", false ) );

    cg = KConfigGroup(m_pConfig, "Notification Messages");
    tabOptions->m_pTabConfirm->setChecked( !cg.hasKey("MultipleTabConfirm") );

}

void KKonqGeneralOptions::defaults()
{
    homeURL->setUrl(KUrl(DEFAULT_HOMEPAGE));

    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
}

static void updateWebbrowsingProfile(const QString& homeUrl, StartPage startPage)
{
    QString url;
    QString serviceType;
    QString serviceName;
    switch(startPage) {
    case ShowHomePage:
        url = homeUrl;
        serviceType = "text/html";
        serviceName = "khtml";
        break;
    case ShowAboutPage:
        url = "about:";
        serviceType = "KonqAboutPage";
        serviceName = "konq_aboutpage";
        break;
    case ShowBlankPage:
        url = "about:blank";
        serviceType = "text/html";
        serviceName = "khtml";
        break;
    }

    const QString profileFileName = "webbrowsing";

    // Create local copy of the profile if needed -- copied from KonqViewManager::setCurrentProfile
    const QString localPath = KStandardDirs::locateLocal("data", QString::fromLatin1("konqueror/profiles/") +
                                                         profileFileName, KGlobal::mainComponent());
    KSharedConfigPtr cfg = KSharedConfig::openConfig(localPath, KConfig::SimpleConfig);
    if (!QFile::exists(localPath)) {
        const QString globalFile = KStandardDirs::locate("data", QString::fromLatin1("konqueror/profiles/") +
                                                         profileFileName, KGlobal::mainComponent());
        if (!globalFile.isEmpty()) {
            KSharedConfigPtr globalCfg = KSharedConfig::openConfig(globalFile, KConfig::SimpleConfig);
            globalCfg->copyTo(localPath, cfg.data());
        }
    }
    KConfigGroup profileGroup(cfg, "Profile");

    QString rootItem = profileGroup.readEntry("RootItem");
    if (rootItem.isEmpty()) {
        rootItem = "View0";
        profileGroup.writeEntry("RootItem", rootItem);
    }
    QString prefix;
    if (rootItem.startsWith("View")) {
        prefix = rootItem + '_';
    } else {
        // simplify the other cases: whether root is a splitter or directly the tabwidget,
        // we want to look at the first view inside the tabs, i.e. ViewT0.
        prefix = "ViewT0_";
    }
    profileGroup.writeEntry(prefix + "URL", url);
    profileGroup.writeEntry(prefix + "ServiceType", serviceType);
    profileGroup.writeEntry(prefix + "ServiceName", serviceName);
}

void KKonqGeneralOptions::save()
{
    KConfigGroup userSettings(m_pConfig, "UserSettings");
    userSettings.writeEntry("HomeURL", homeURL->url().url());
    const int startComboIndex = m_startCombo->currentIndex();
    const int choice = m_startCombo->itemData(startComboIndex).toInt();
    updateWebbrowsingProfile(homeURL->url().url(), static_cast<StartPage>(choice));

    // TODO create local webbrowsing profile,
    // look for View0_ServiceName=konq_aboutpage or ViewT0_ServiceName=khtml
    // and replace with
    // ViewT0_ServiceName=khtml (if http)
    // ViewT0_ServiceType=text/html (if http)
    // ViewT0_URL[$e]=http://www.kde.org/

    KConfigGroup cg(m_pConfig, "FMSettings");
    cg.writeEntry( "MMBOpensTab", tabOptions->m_pShowMMBInTabs->isChecked() );
    cg.writeEntry( "AlwaysTabbedMode", !(tabOptions->m_pDynamicTabbarHide->isChecked()) );

    cg.writeEntry( "NewTabsInFront", !(tabOptions->m_pNewTabsInBackground->isChecked()) );
    cg.writeEntry( "OpenAfterCurrentPage", tabOptions->m_pOpenAfterCurrentPage->isChecked() );
    cg.writeEntry( "PermanentCloseButton", tabOptions->m_pPermanentCloseButton->isChecked() );
    cg.writeEntry( "KonquerorTabforExternalURL", tabOptions->m_pKonquerorTabforExternalURL->isChecked() );
    cg.writeEntry( "PopupsWithinTabs", tabOptions->m_pPopupsWithinTabs->isChecked() );
    cg.writeEntry( "TabCloseActivatePrevious", tabOptions->m_pTabCloseActivatePrevious->isChecked() );
    cg.writeEntry( "MouseMiddleClickClosesTab", tabOptions->m_pMiddleClickClose->isChecked() );

    cg.sync();

    // It only matters whether the key is present, its value has no meaning
    cg = KConfigGroup(m_pConfig,"Notification Messages");
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

