/*
    "Misc Options" Tab for KFM configuration

    SPDX-FileCopyrightText: 1998 Sven Radej
    SPDX-FileCopyrightText: 1998 David Faure
    SPDX-FileCopyrightText: 2001 Waldo Bastian <bastian@kde.org>
*/

// Own
#include "htmlopts.h"

#include "konqsettings.h"

// Qt
#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QSpinBox>

// KDE
#include <KLocalizedString>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>

//-----------------------------------------------------------------------------

using namespace Konq;

KMiscHTMLOptions::KMiscHTMLOptions(QObject *parent, const KPluginMetaData &md)
    : KCModule(parent, md), m_groupname(QStringLiteral("HTML Settings"))
{
    m_pConfig = KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);
    QVBoxLayout *lay = new QVBoxLayout(widget());

    QGroupBox *bgBookmarks = new QGroupBox(i18n("Boo&kmarks"));
    QVBoxLayout *laygroup1 = new QVBoxLayout;

    m_pAdvancedAddBookmarkCheckBox = new QCheckBox(i18n("Ask for name and folder when adding bookmarks"));
    laygroup1->addWidget(m_pAdvancedAddBookmarkCheckBox);

    m_pAdvancedAddBookmarkCheckBox->setToolTip(i18n("If this box is checked, Konqueror will allow you to"
            " change the title of the bookmark and choose a folder"
            " in which to store it when you add a new bookmark."));
    connect(m_pAdvancedAddBookmarkCheckBox, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);
    bgBookmarks->setLayout(laygroup1);

    m_pOnlyMarkedBookmarksCheckBox = new QCheckBox(i18n("Show only marked bookmarks in bookmark toolbar"), bgBookmarks);
    laygroup1->addWidget(m_pOnlyMarkedBookmarksCheckBox);
    m_pOnlyMarkedBookmarksCheckBox->setToolTip(i18n("If this box is checked, Konqueror will show only those"
            " bookmarks in the bookmark toolbar which you have marked to do so in the bookmark editor."));
    connect(m_pOnlyMarkedBookmarksCheckBox, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);

    lay->addWidget(bgBookmarks);

    //TODO Settings: form completion isn't implemented in WebEnginePart right now, but it could be done.
    //In the meanwhile don't create the widgets for the corresponding settings
#if 0
    // Form completion
    // m_pFormCompletionCheckBox = new QGroupBox(i18n("Form Com&pletion"), widget());
    // m_pFormCompletionCheckBox->setCheckable(true);
    // QFormLayout *laygroup2 = new QFormLayout(m_pFormCompletionCheckBox);

    m_pFormCompletionCheckBox->setToolTip(i18n("If this box is checked, Konqueror will remember"
                                            " the data you enter in web forms and suggest it in similar fields for all forms."));
    connect(m_pFormCompletionCheckBox, &QGroupBox::toggled, this, &KMiscHTMLOptions::markAsChanged);
    m_pFormCompletionCheckBox->hide();

    m_pMaxFormCompletionItems = new QSpinBox(widget());
    m_pMaxFormCompletionItems->setRange(0, 100);
    laygroup2->addRow(i18n("&Maximum completions:"), m_pMaxFormCompletionItems);
    m_pMaxFormCompletionItems->setToolTip(
        i18n("Here you can select how many values Konqueror will remember for a form field."));
    connect(m_pMaxFormCompletionItems, QOverload<int>::of(&QSpinBox::valueChanged), this, &KMiscHTMLOptions::markAsChanged);
    connect(m_pFormCompletionCheckBox, &QGroupBox::toggled, m_pMaxFormCompletionItems, &QWidget::setEnabled);

    lay->addWidget(m_pFormCompletionCheckBox);
#endif

    //TODO Settings: currently these settings don't work. It might be possible to make them work in WebEnginePart using javascript
    //(but I'm not sure). Uncomment them if they can be made to work again or remove them once decided they can't be made to work
#if 0
    // Mouse behavior
    QGroupBox *bgMouse = new QGroupBox(i18n("Mouse Beha&vior"));
    QVBoxLayout *laygroup3 = new QVBoxLayout(bgMouse);

    m_pOpenMiddleClick = new QCheckBox(i18n("M&iddle click opens URL in selection"), bgMouse);
    laygroup3->addWidget(m_pOpenMiddleClick);
    m_pOpenMiddleClick->setToolTip(i18n(
                                         "If this box is checked, you can open the URL in the selection by middle clicking on a "
                                         "Konqueror view."));
    connect(m_pOpenMiddleClick, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);

    m_pBackRightClick = new QCheckBox(i18n("Right click goes &back in history"), bgMouse);
    laygroup3->addWidget(m_pBackRightClick);
    m_pBackRightClick->setToolTip(i18n(
                                        "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
                                        "To access the context menu, press the right mouse button and move."));
    connect(m_pBackRightClick, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);

    lay->addWidget(bgMouse);
#endif

    // Misc
    QGroupBox *bgMisc = new QGroupBox(i18nc("@title:group", "Miscellaneous"));
    QFormLayout *fl = new QFormLayout(bgMisc);

    //TODO Settings: currently access keys don't work with WebEnginePart. It might be possible to make them work using javascript
    //(but I'm not sure). Uncomment it if they can be made to work again or remove them once decided they can't be made to work
    // Checkbox to enable/disable Access Key activation through the Ctrl key.
#if 0
    m_pAccessKeys = new QCheckBox(i18n("Enable Access Ke&y activation with Ctrl key"), widget());
    m_pAccessKeys->setToolTip(i18n("Pressing the Ctrl key when viewing webpages activates Access Keys. Unchecking this box will disable this accessibility feature. (Konqueror needs to be restarted for this change to take effect.)"));
    connect(m_pAccessKeys, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);
    fl->addRow(m_pAccessKeys);
#endif

    m_pDoNotTrack = new QCheckBox(i18n("Send the DNT header to tell web sites you do not want to be tracked"), widget());
    m_pDoNotTrack->setToolTip(i18n("<p>Check this box if you want to inform a web site that you do not want your web browsing habits tracked.</p><p><strong>Warning:</strong> this header is considered obsolete, so it will likely be ignored by most web pages.</p>"));
    connect(m_pDoNotTrack, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);
    fl->addRow(m_pDoNotTrack);

    m_pOfferToSaveWebsitePassword = new QCheckBox(i18n("Offer to save website passwords"), widget());
    m_pOfferToSaveWebsitePassword->setToolTip(i18n("Uncheck this box to prevent being prompted to save website passwords"));
    connect(m_pOfferToSaveWebsitePassword, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);
    fl->addRow(m_pOfferToSaveWebsitePassword);

    m_pdfViewer = new QCheckBox(i18n("Display online PDF files using WebEngine"));
    m_pdfViewer->setToolTip(i18n("Uncheck this box to display online PDF files as configured in System Settings"));
    fl->addRow(m_pdfViewer);
    connect(m_pdfViewer, &QCheckBox::toggled, this, &KMiscHTMLOptions::markAsChanged);

    lay->addWidget(bgMisc);
    lay->addStretch(5);

    KCModule::save();
}

KMiscHTMLOptions::~KMiscHTMLOptions()
{
}

void KMiscHTMLOptions::loadFromKonquerorrc()
{
    //TODO Settings: uncomment if the corresponding settings can be implemented in WebEnginePart and
    //remove them if they can't
    // m_pOpenMiddleClick->setChecked(Settings::openMiddleClick());
    // m_pBackRightClick->setChecked(Settings::backRightClick());

    //TODO Settings: uncomment when form completion is enabled with WebEnginePart
    // m_pFormCompletionCheckBox->setChecked(cg.readEntry("FormCompletion", true));
    // m_pMaxFormCompletionItems->setValue(cg.readEntry("MaxFormCompletionItems", 10));
    // m_pMaxFormCompletionItems->setEnabled(m_pFormCompletionCheckBox->isChecked());

    m_pOfferToSaveWebsitePassword->setChecked(Settings::offerToSaveWebsitePassword());

    m_pdfViewer->setChecked(Settings::useInternalPDFViewer());
}

void KMiscHTMLOptions::load()
{
    loadFromKonquerorrc();

    //TODO Settings: uncomment if the corresponding settings can be implemented in WebEnginePart and
    //remove them if they can't
    // cg2 = KConfigGroup(khtmlrcConfig, "Access Keys");
    // m_pAccessKeys->setChecked(cg2.readEntry("Enabled", true));

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals), "Bookmarks");
    m_pAdvancedAddBookmarkCheckBox->setChecked(cg.readEntry("AdvancedAddBookmarkDialog", false));
    m_pOnlyMarkedBookmarksCheckBox->setChecked(cg.readEntry("FilteredToolbar", false));

    //TODO Settings: move to konquerorrc?
    cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals), QString());
    m_pDoNotTrack->setChecked(cg.readEntry("DoNotTrack", false));

    KCModule::load();
}

void KMiscHTMLOptions::defaults()
{
    Settings::self()->withDefaults([this]{loadFromKonquerorrc();});

    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
    m_pAdvancedAddBookmarkCheckBox->setChecked(false);
    m_pOnlyMarkedBookmarksCheckBox->setChecked(false);
    m_pDoNotTrack->setChecked(false);
    setRepresentsDefaults(true);
}

void KMiscHTMLOptions::save()
{
    //TODO Settings: uncomment if the corresponding settings can be implemented in WebEnginePart and
    //remove them if they can't
    // Settings::setOpenMiddleClick(m_pOpenMiddleClick->isChecked());
    // Settings::setBackRightClick(m_pBackRightClick->isChecked());

    //TODO Settings: uncomment when form completion is enabled with WebEnginePart
    // Settings::setFormCompletion(m_pFormCompletionCheckBox->isChecked());
    // Settings::setMaxFormCompletionItems(m_pMaxFormCompletionItems->value());

    Settings::setOfferToSaveWebsitePassword(m_pOfferToSaveWebsitePassword->isChecked());
    Settings::setUseInternalPDFViewer(m_pdfViewer->isChecked());

    Settings::self()->save();

    //TODO Settings: uncomment if the corresponding settings can be implemented in WebEnginePart and
    //remove them if they can't. If the functionality is implemented, move the setting to konquerorrc
    // Writes the value of m_pAccessKeys into khtmlrc to affect all applications using KHTML
    // cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::NoGlobals), "Access Keys");
    // cg.writeEntry("Enabled", m_pAccessKeys->isChecked());
    // cg.sync();

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals), "Bookmarks");
    cg.writeEntry("AdvancedAddBookmarkDialog", m_pAdvancedAddBookmarkCheckBox->isChecked());
    cg.writeEntry("FilteredToolbar", m_pOnlyMarkedBookmarksCheckBox->isChecked());
    cg.sync();

    cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals), QString());
    cg.writeEntry("DoNotTrack", m_pDoNotTrack->isChecked());
    cg.sync();

    // Send signal to all konqueror instances
    QDBusConnection sessionBus(QDBusConnection::sessionBus());
    sessionBus.send(QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration")));
    sessionBus.send(QDBusMessage::createSignal(QStringLiteral("/KBookmarkManager/konqueror"), QStringLiteral("org.kde.KIO.KBookmarkManager"), QStringLiteral("bookmarkConfigChanged")));
    sessionBus.send(QDBusMessage::createSignal(QStringLiteral("/KIO/Scheduler"), QStringLiteral("org.kde.KIO.Scheduler"), QStringLiteral("reparseSlaveConfiguration")));

    KCModule::save();
}
