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

// Qt
#include <QDBusConnection>
#include <QDBusMessage>
#include <QComboBox>
#include <QDebug>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QStandardPaths>
#include <QUrl>

// KDE
#include <kbuildsycocaprogressdialog.h>
#include <kmimetypetrader.h>
#include <kservice.h>
#include <KConfigGroup>
#include <KSharedConfig>

// Local
#include "ui_advancedTabOptions.h"

// Keep in sync with konqueror.kcfg
static const char DEFAULT_STARTPAGE[] = "about:konqueror";
static const char DEFAULT_HOMEPAGE[] = "https://www.kde.org/";
// Keep in sync with the order in the combo
enum StartPage { ShowAboutPage, ShowStartUrlPage, ShowBlankPage, ShowBookmarksPage };

//-----------------------------------------------------------------------------

KKonqGeneralOptions::KKonqGeneralOptions(QWidget *parent, const QVariantList &)
    : KCModule(parent)
{
    m_pConfig = KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);

    addHomeUrlWidgets(lay);

    QGroupBox *tabsGroup = new QGroupBox(i18n("Tabbed Browsing"));

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

    emit changed(false);
}

void KKonqGeneralOptions::addHomeUrlWidgets(QVBoxLayout *lay)
{
    QFormLayout *formLayout = new QFormLayout;
    lay->addLayout(formLayout);

    QLabel *startLabel = new QLabel(i18nc("@label:listbox", "When &Konqueror starts:"), this);

    QWidget *containerWidget = new QWidget(this);
    QHBoxLayout *hboxLayout = new QHBoxLayout(containerWidget);
    hboxLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(startLabel, containerWidget);

    m_startCombo = new QComboBox(this);
    m_startCombo->setEditable(false);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show Introduction Page"), ShowAboutPage);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show My Start Page"), ShowStartUrlPage);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show Blank Page"), ShowBlankPage);
    m_startCombo->addItem(i18nc("@item:inlistbox", "Show My Bookmarks"), ShowBookmarksPage);
    startLabel->setBuddy(m_startCombo);
    connect(m_startCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotChanged()));
    hboxLayout->addWidget(m_startCombo);

    startURL = new QLineEdit(this);
    startURL->setWindowTitle(i18nc("@title:window", "Select Start Page"));
    hboxLayout->addWidget(startURL);
    connect(startURL, SIGNAL(textChanged(QString)), SLOT(slotChanged()));

    QString startstr = i18n("This is the URL of the web page "
                           "Konqueror will show when starting.");
    startURL->setWhatsThis(startstr);
    connect(m_startCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int idx) {
            startURL->setEnabled(idx == ShowStartUrlPage);
            });
    startURL->setEnabled(false);

    ////

    QLabel *label = new QLabel(i18n("Home page:"), this);

    homeURL = new QLineEdit(this);
    homeURL->setWindowTitle(i18nc("@title:window", "Select Home Page"));
    formLayout->addRow(label, homeURL);
    connect(homeURL, SIGNAL(textChanged(QString)), SLOT(slotChanged()));
    label->setBuddy(homeURL);

    QString homestr = i18n("This is the URL of the web page where "
                           "Konqueror will jump to when "
                           "the \"Home\" button is pressed.");
    label->setWhatsThis(homestr);
    homeURL->setWhatsThis(homestr);

    ////

    QLabel *webLabel = new QLabel(i18n("Default web browser engine:"), this);

    m_webEngineCombo = new QComboBox(this);
    m_webEngineCombo->setEditable(false);
    m_webEngineCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    formLayout->addRow(webLabel, m_webEngineCombo);
    webLabel->setBuddy(m_webEngineCombo);
    connect(m_webEngineCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotChanged()));
}

KKonqGeneralOptions::~KKonqGeneralOptions()
{
    delete tabOptions;
}

static StartPage urlToStartPageEnum(const QString &startUrl)
{
    if (startUrl == QLatin1String("about:blank")) {
        return ShowBlankPage;
    }
    if (startUrl == QLatin1String("about:") || startUrl == QLatin1String("about:konqueror")) {
        return ShowAboutPage;
    }
    if (startUrl == QLatin1String("bookmarks:") || startUrl == QLatin1String("bookmarks:/")) {
        return ShowBookmarksPage;
    }
    return ShowStartUrlPage;
}

static QString startPageEnumToUrl(StartPage startPage)
{
    switch (startPage) {
        case ShowBlankPage:
            return QStringLiteral("about:blank");
        case ShowAboutPage:
            return QStringLiteral("about:konqueror");
        case ShowBookmarksPage:
            return QStringLiteral("bookmarks:/");
        case ShowStartUrlPage:
            return QString();
    }
    return QString();
}

void KKonqGeneralOptions::load()
{
    KConfigGroup userSettings(m_pConfig, "UserSettings");
    const QUrl homeUrl(QUrl(userSettings.readEntry("HomeURL", DEFAULT_HOMEPAGE)));
    const QUrl startUrl(QUrl(userSettings.readEntry("StartURL", DEFAULT_STARTPAGE)));
    homeURL->setText(homeUrl.toString());
    startURL->setText(startUrl.toString());
    const StartPage startPage = urlToStartPageEnum(startUrl.toString());
    const int startComboIndex = m_startCombo->findData(startPage);
    Q_ASSERT(startComboIndex != -1);
    m_startCombo->setCurrentIndex(startComboIndex);

    m_webEngineCombo->clear();
    // ## Well, the problem with using the trader to find the available parts, is that if a user
    // removed a part in keditfiletype text/html, it won't be in the list anymore. Oh well.
    const KService::List partOfferList = KMimeTypeTrader::self()->query(QStringLiteral("text/html"), QStringLiteral("KParts/ReadOnlyPart"), QStringLiteral("not ('KParts/ReadWritePart' in ServiceTypes)"));
    // Sorted list, so the first one is the preferred one, no need for a setCurrentIndex.
    Q_FOREACH (const KService::Ptr partService, partOfferList) {
        // We want only the HTML-capable parts, not any text/plain part (via inheritance)
        // This is a small "private inheritance" hack, pending a more general solution
        if (!partService->hasMimeType(QStringLiteral("text/plain"))) {
            m_webEngineCombo->addItem(QIcon::fromTheme(partService->icon()), partService->name(),
                                      QVariant(partService->storageId()));
        }
    }

    KConfigGroup cg(m_pConfig, "FMSettings"); // ### what a wrong group name for these settings...

    tabOptions->m_pShowMMBInTabs->setChecked(cg.readEntry("MMBOpensTab", true));
    tabOptions->m_pDynamicTabbarHide->setChecked(!(cg.readEntry("AlwaysTabbedMode", false)));

    tabOptions->m_pNewTabsInBackground->setChecked(!(cg.readEntry("NewTabsInFront", false)));
    tabOptions->m_pOpenAfterCurrentPage->setChecked(cg.readEntry("OpenAfterCurrentPage", false));
    tabOptions->m_pPermanentCloseButton->setChecked(cg.readEntry("PermanentCloseButton", true));
    tabOptions->m_pKonquerorTabforExternalURL->setChecked(cg.readEntry("KonquerorTabforExternalURL", false));
    tabOptions->m_pPopupsWithinTabs->setChecked(cg.readEntry("PopupsWithinTabs", false));
    tabOptions->m_pTabCloseActivatePrevious->setChecked(cg.readEntry("TabCloseActivatePrevious", false));
    tabOptions->m_pMiddleClickClose->setChecked(cg.readEntry("MouseMiddleClickClosesTab", false));

    cg = KConfigGroup(m_pConfig, "Notification Messages");
    tabOptions->m_pTabConfirm->setChecked(!cg.hasKey("MultipleTabConfirm"));

}

void KKonqGeneralOptions::defaults()
{
    homeURL->setText(QUrl(DEFAULT_HOMEPAGE).toString());
    startURL->setText(QUrl(DEFAULT_STARTPAGE).toString());

    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
}

void KKonqGeneralOptions::save()
{
    KConfigGroup userSettings(m_pConfig, "UserSettings");
    const int startComboIndex = m_startCombo->currentIndex();
    const int choice = m_startCombo->itemData(startComboIndex).toInt();
    QString startUrl(startPageEnumToUrl(static_cast<StartPage>(choice)));
    if (startUrl.isEmpty())
        startUrl = startURL->text();
    userSettings.writeEntry("StartURL", startUrl);
    userSettings.writeEntry("HomeURL", homeURL->text());

    if (m_webEngineCombo->currentIndex() > 0) {
        // The user changed the preferred web engine, save into mimeapps.list.
        const QString preferredWebEngine = m_webEngineCombo->itemData(m_webEngineCombo->currentIndex()).toString();
        //qDebug() << "preferredWebEngine=" << preferredWebEngine;

        KSharedConfig::Ptr profile = KSharedConfig::openConfig(QStringLiteral("mimeapps.list"), KConfig::NoGlobals, QStandardPaths::ConfigLocation);
        KConfigGroup addedServices(profile, "Added KDE Service Associations");
        Q_FOREACH (const QString &mimeType, QStringList() << "text/html" << "application/xhtml+xml" << "application/xml") {
            QStringList services = addedServices.readXdgListEntry(mimeType);
            services.removeAll(preferredWebEngine);
            services.prepend(preferredWebEngine); // make it the preferred one
            addedServices.writeXdgListEntry(mimeType, services);
        }
        profile->sync();

        // kbuildsycoca is the one reading mimeapps.list, so we need to run it now
        KBuildSycocaProgressDialog::rebuildKSycoca(this);
    }

    KConfigGroup cg(m_pConfig, "FMSettings");
    cg.writeEntry("MMBOpensTab", tabOptions->m_pShowMMBInTabs->isChecked());
    cg.writeEntry("AlwaysTabbedMode", !(tabOptions->m_pDynamicTabbarHide->isChecked()));

    cg.writeEntry("NewTabsInFront", !(tabOptions->m_pNewTabsInBackground->isChecked()));
    cg.writeEntry("OpenAfterCurrentPage", tabOptions->m_pOpenAfterCurrentPage->isChecked());
    cg.writeEntry("PermanentCloseButton", tabOptions->m_pPermanentCloseButton->isChecked());
    cg.writeEntry("KonquerorTabforExternalURL", tabOptions->m_pKonquerorTabforExternalURL->isChecked());
    cg.writeEntry("PopupsWithinTabs", tabOptions->m_pPopupsWithinTabs->isChecked());
    cg.writeEntry("TabCloseActivatePrevious", tabOptions->m_pTabCloseActivatePrevious->isChecked());
    cg.writeEntry("MouseMiddleClickClosesTab", tabOptions->m_pMiddleClickClose->isChecked());
    cg.sync();
    // It only matters whether the key is present, its value has no meaning
    cg = KConfigGroup(m_pConfig, "Notification Messages");
    if (tabOptions->m_pTabConfirm->isChecked()) {
        cg.deleteEntry("MultipleTabConfirm");
    } else {
        cg.writeEntry("MultipleTabConfirm", true);
    }
    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);

    emit changed(false);
}

void KKonqGeneralOptions::slotChanged()
{
    emit changed(true);
}

