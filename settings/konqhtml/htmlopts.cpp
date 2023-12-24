/*
    "Misc Options" Tab for KFM configuration

    SPDX-FileCopyrightText: 1998 Sven Radej
    SPDX-FileCopyrightText: 1998 David Faure
    SPDX-FileCopyrightText: 2001 Waldo Bastian <bastian@kde.org>
*/

// Own
#include "htmlopts.h"

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

KMiscHTMLOptions::KMiscHTMLOptions(QObject *parent, const KPluginMetaData &md, const QVariantList &)
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

    // Form completion
    m_pFormCompletionCheckBox = new QGroupBox(i18n("Form Com&pletion"), widget());
    m_pFormCompletionCheckBox->setCheckable(true);
    QFormLayout *laygroup2 = new QFormLayout(m_pFormCompletionCheckBox);

    m_pFormCompletionCheckBox->setToolTip(i18n("If this box is checked, Konqueror will remember"
                                            " the data you enter in web forms and suggest it in similar fields for all forms."));
    connect(m_pFormCompletionCheckBox, &QGroupBox::toggled, this, &KMiscHTMLOptions::markAsChanged);

    m_pMaxFormCompletionItems = new QSpinBox(widget());
    m_pMaxFormCompletionItems->setRange(0, 100);
    laygroup2->addRow(i18n("&Maximum completions:"), m_pMaxFormCompletionItems);
    m_pMaxFormCompletionItems->setToolTip(
        i18n("Here you can select how many values Konqueror will remember for a form field."));
    connect(m_pMaxFormCompletionItems, QOverload<int>::of(&QSpinBox::valueChanged), this, &KMiscHTMLOptions::markAsChanged);
    connect(m_pFormCompletionCheckBox, &QGroupBox::toggled, m_pMaxFormCompletionItems, &QWidget::setEnabled);

    lay->addWidget(m_pFormCompletionCheckBox);

    // Mouse behavior
    QGroupBox *bgMouse = new QGroupBox(i18n("Mouse Beha&vior"));
    QVBoxLayout *laygroup3 = new QVBoxLayout(bgMouse);

    m_cbCursor = new QCheckBox(i18n("Chan&ge cursor over links"));
    laygroup3->addWidget(m_cbCursor);
    m_cbCursor->setToolTip(i18n("If this option is set, the shape of the cursor will change "
                                  "(usually to a hand) if it is moved over a hyperlink."));
    connect(m_cbCursor, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);

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

    // Misc
    QGroupBox *bgMisc = new QGroupBox(i18nc("@title:group", "Miscellaneous"));
    QFormLayout *fl = new QFormLayout(bgMisc);

    m_pAutoRedirectCheckBox = new QCheckBox(i18n("Allow automatic delayed &reloading/redirecting"), widget());
    m_pAutoRedirectCheckBox->setToolTip(i18n("Some web pages request an automatic reload or redirection after"
                                          " a certain period of time. By unchecking this box Konqueror will ignore these requests."));
    connect(m_pAutoRedirectCheckBox, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);
    fl->addRow(m_pAutoRedirectCheckBox);

    // Checkbox to enable/disable Access Key activation through the Ctrl key.
    m_pAccessKeys = new QCheckBox(i18n("Enable Access Ke&y activation with Ctrl key"), widget());
    m_pAccessKeys->setToolTip(i18n("Pressing the Ctrl key when viewing webpages activates Access Keys. Unchecking this box will disable this accessibility feature. (Konqueror needs to be restarted for this change to take effect.)"));
    connect(m_pAccessKeys, QOverload<bool>::of(&QAbstractButton::toggled), this, &KMiscHTMLOptions::markAsChanged);
    fl->addRow(m_pAccessKeys);

    m_pDoNotTrack = new QCheckBox(i18n("Send the DNT header to tell web sites you do not want to be tracked"), widget());
    m_pDoNotTrack->setToolTip(i18n("Check this box if you want to inform a web site that you do not want your web browsing habits tracked."));
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

    setNeedsSave(false);
}

KMiscHTMLOptions::~KMiscHTMLOptions()
{
}

void KMiscHTMLOptions::load()
{
    KSharedConfigPtr khtmlrcConfig = KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::NoGlobals);

    // Read the configuration from konquerorrc with khtmlrc as fall back.
    KConfigGroup cg(m_pConfig, "MainView Settings");
    KConfigGroup cg2(khtmlrcConfig, "MainView Settings");
    m_pOpenMiddleClick->setChecked(cg.readEntry("OpenMiddleClick", cg2.readEntry("OpenMiddleClick", true)));
    m_pBackRightClick->setChecked(cg.readEntry("BackRightClick", cg2.readEntry("BackRightClick", false)));

    cg = KConfigGroup(m_pConfig, "HTML Settings");
    cg2 = KConfigGroup(khtmlrcConfig, "HTML Settings");
    m_cbCursor->setChecked(cg.readEntry("ChangeCursor", cg2.readEntry("ChangeCursor", true)));
    m_pAutoRedirectCheckBox->setChecked(cg.readEntry("AutoDelayedActions", true));
    m_pFormCompletionCheckBox->setChecked(cg.readEntry("FormCompletion", true));
    m_pMaxFormCompletionItems->setValue(cg.readEntry("MaxFormCompletionItems", 10));
    m_pMaxFormCompletionItems->setEnabled(m_pFormCompletionCheckBox->isChecked());
    m_pOfferToSaveWebsitePassword->setChecked(cg.readEntry("OfferToSaveWebsitePassword", true));

    m_pdfViewer->setChecked(cg.readEntry("InternalPdfViewer", false));

    cg2 = KConfigGroup(khtmlrcConfig, "Access Keys");
    m_pAccessKeys->setChecked(cg2.readEntry("Enabled", true));

    cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals), "Bookmarks");
    m_pAdvancedAddBookmarkCheckBox->setChecked(cg.readEntry("AdvancedAddBookmarkDialog", false));
    m_pOnlyMarkedBookmarksCheckBox->setChecked(cg.readEntry("FilteredToolbar", false));

    cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kioslaverc"), KConfig::NoGlobals), QString());
    m_pDoNotTrack->setChecked(cg.readEntry("DoNotTrack", false));

    KCModule::load();
}

void KMiscHTMLOptions::defaults()
{
    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
    m_pAdvancedAddBookmarkCheckBox->setChecked(false);
    m_pOnlyMarkedBookmarksCheckBox->setChecked(false);
    m_pDoNotTrack->setChecked(false);
    m_pOfferToSaveWebsitePassword->setChecked(true);

    m_pdfViewer->setChecked(false);

#if QT_VERSION_MAJOR > 5
    setRepresentsDefaults(true);
#endif
}

void KMiscHTMLOptions::save()
{
    KConfigGroup cg(m_pConfig, "MainView Settings");
    cg.writeEntry("OpenMiddleClick", m_pOpenMiddleClick->isChecked());
    cg.writeEntry("BackRightClick", m_pBackRightClick->isChecked());

    cg = KConfigGroup(m_pConfig, "HTML Settings");
    cg.writeEntry("ChangeCursor", m_cbCursor->isChecked());
    cg.writeEntry("AutoDelayedActions", m_pAutoRedirectCheckBox->isChecked());
    cg.writeEntry("FormCompletion", m_pFormCompletionCheckBox->isChecked());
    cg.writeEntry("MaxFormCompletionItems", m_pMaxFormCompletionItems->value());
    cg.writeEntry("OfferToSaveWebsitePassword", m_pOfferToSaveWebsitePassword->isChecked());
    cg.writeEntry("InternalPdfViewer", m_pdfViewer->isChecked());
    cg.sync();

    // Writes the value of m_pAccessKeys into khtmlrc to affect all applications using KHTML
    cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::NoGlobals), "Access Keys");
    cg.writeEntry("Enabled", m_pAccessKeys->isChecked());
    cg.sync();

    cg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kbookmarkrc"), KConfig::NoGlobals), "Bookmarks");
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
