/*
 * "Misc Options" Tab for KFM configuration
 *
 * Copyright (C) Sven Radej 1998
 * Copyright (C) David Faure 1998
 * Copyright (C) 2001 Waldo Bastian <bastian@kde.org>
 *
 */

// Own
#include "htmlopts.h"

// Qt
#include <QtGui/QGroupBox>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>

// KDE
#include <kglobalsettings.h> // get default for DEFAULT_CHANGECURSOR
#include <klocale.h>
#include <kdialog.h>
#include <knuminput.h>
#include <kseparator.h>
#include <kapplication.h>

// Local
#include "khtml_settings.h"
#include <KPluginFactory>
#include <KPluginLoader>

K_PLUGIN_FACTORY_DECLARATION(KcmKonqHtmlFactory)

//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(QWidget *parent, const QVariantList&)
    : KCModule( KcmKonqHtmlFactory::componentData(), parent ), m_groupname("HTML Settings")
{
    m_pConfig = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    int row = 0;
    QVBoxLayout *lay = new QVBoxLayout(this);

    // Bookmarks
    setQuickHelp( i18n("<h1>Konqueror Browser</h1> Here you can configure Konqueror's browser "
              "functionality. Please note that the file manager "
              "functionality has to be configured using the \"File Manager\" "
              "configuration module. You can make some "
              "settings how Konqueror should handle the HTML code in "
              "the web pages it loads. It is usually not necessary to "
              "change anything here."));

    QGroupBox *bgBookmarks = new QGroupBox( i18n("Boo&kmarks"));
    QVBoxLayout *laygroup1 = new QVBoxLayout;

    laygroup1->setSpacing(KDialog::spacingHint());

    m_pAdvancedAddBookmarkCheckBox = new QCheckBox(i18n( "Ask for name and folder when adding bookmarks" ));
    laygroup1->addWidget(m_pAdvancedAddBookmarkCheckBox);

    m_pAdvancedAddBookmarkCheckBox->setWhatsThis( i18n( "If this box is checked, Konqueror will allow you to"
                                                        " change the title of the bookmark and choose a folder"
							" in which to store it when you add a new bookmark." ) );
    connect(m_pAdvancedAddBookmarkCheckBox, SIGNAL(toggled(bool)), SLOT(changed()));
    bgBookmarks->setLayout(laygroup1);

    m_pOnlyMarkedBookmarksCheckBox = new QCheckBox(i18n( "Show only marked bookmarks in bookmark toolbar" ), bgBookmarks);
    laygroup1->addWidget(m_pOnlyMarkedBookmarksCheckBox);
    m_pOnlyMarkedBookmarksCheckBox->setWhatsThis( i18n( "If this box is checked, Konqueror will show only those"
                     " bookmarks in the bookmark toolbar which you have marked to do so in the bookmark editor." ) );
    connect(m_pOnlyMarkedBookmarksCheckBox, SIGNAL(toggled(bool)), SLOT(changed()));

    lay->addWidget( bgBookmarks);

     // Form completion
    m_pFormCompletionCheckBox = new QGroupBox( i18n("Form Com&pletion"), this );
    m_pFormCompletionCheckBox->setCheckable(true);
    QFormLayout *laygroup2 = new QFormLayout(m_pFormCompletionCheckBox);
    laygroup2->setSpacing(KDialog::spacingHint());

    m_pFormCompletionCheckBox->setWhatsThis( i18n( "If this box is checked, Konqueror will remember"
                  " the data you enter in web forms and suggest it in similar fields for all forms." ) );
    connect(m_pFormCompletionCheckBox, SIGNAL(toggled(bool)), SLOT(changed()));

    m_pMaxFormCompletionItems = new KIntNumInput(this);
    m_pMaxFormCompletionItems->setRange( 0, 100 );
    laygroup2->addRow(i18n( "&Maximum completions:" ), m_pMaxFormCompletionItems );
    m_pMaxFormCompletionItems->setWhatsThis(
        i18n( "Here you can select how many values Konqueror will remember for a form field." ) );
    connect(m_pMaxFormCompletionItems, SIGNAL(valueChanged(int)), SLOT(changed()));
    connect(m_pFormCompletionCheckBox, SIGNAL(toggled(bool)), m_pMaxFormCompletionItems, SLOT(setEnabled(bool)));

    lay->addWidget( m_pFormCompletionCheckBox);

    // Mouse behavior
    QGroupBox *bgMouse = new QGroupBox( i18n("Mouse Beha&vior") );
    QVBoxLayout *laygroup3 = new QVBoxLayout(bgMouse);
    laygroup3->setSpacing(KDialog::spacingHint());

    m_cbCursor = new QCheckBox(i18n("Chan&ge cursor over links") );
    laygroup3->addWidget( m_cbCursor );
    m_cbCursor->setWhatsThis( i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );
    connect(m_cbCursor, SIGNAL(toggled(bool)), SLOT(changed()));

    m_pOpenMiddleClick = new QCheckBox( i18n ("M&iddle click opens URL in selection" ), bgMouse);
    laygroup3->addWidget( m_pOpenMiddleClick );
    m_pOpenMiddleClick->setWhatsThis( i18n (
      "If this box is checked, you can open the URL in the selection by middle clicking on a "
      "Konqueror view." ) );
    connect(m_pOpenMiddleClick, SIGNAL(toggled(bool)), SLOT(changed()));

    m_pBackRightClick = new QCheckBox( i18n( "Right click goes &back in history" ),bgMouse);
    laygroup3->addWidget( m_pBackRightClick );
    m_pBackRightClick->setWhatsThis( i18n(
      "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
      "To access the context menu, press the right mouse button and move." ) );
    connect(m_pBackRightClick, SIGNAL(toggled(bool)), SLOT(changed()));

    lay->addWidget( bgMouse);

    // Misc
    QGroupBox *bgMisc = new QGroupBox( i18nc("@title:group","Miscellaneous"));
    QFormLayout *fl=new QFormLayout(bgMisc);


    m_pAutoRedirectCheckBox = new QCheckBox( i18n( "Allow automatic delayed &reloading/redirecting"), this );
    m_pAutoRedirectCheckBox->setWhatsThis( i18n( "Some web pages request an automatic reload or redirection after"
			    " a certain period of time. By unchecking this box Konqueror will ignore these requests." ) );
    connect(m_pAutoRedirectCheckBox, SIGNAL(toggled(bool)), SLOT(changed()));
    fl->addRow( m_pAutoRedirectCheckBox );

    lay->addWidget( bgMisc);

    // Checkbox to enable/disable Access Key activation through the Ctrl key.
    m_pAccessKeys = new QCheckBox( i18n( "Enable/disable Access Ke&y activation with Ctrl key"), this );//TODO remove Enable/disable part
    m_pAccessKeys->setWhatsThis( i18n( "Pressing the Ctrl key when viewing webpages activates KDE's Access Keys. Unchecking this box will disable this accessibility feature. (Konqueror needs to be restarted for changes to take effect.)" ) );
    connect(m_pAccessKeys, SIGNAL(toggled(bool)), SLOT(changed()));
    fl->addRow( m_pAccessKeys);

    lay->addStretch(5);

    emit changed(false);
}

KMiscHTMLOptions::~KMiscHTMLOptions()
{
}

void KMiscHTMLOptions::load()
{
    KConfigGroup khtmlrc(KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals), "");
    KConfigGroup cg(m_pConfig, "");
#define SET_GROUP(x) cg = KConfigGroup(m_pConfig, x); khtmlrc = KConfigGroup(KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals),x)
#define READ_BOOL(x,y) cg.readEntry(x, khtmlrc.readEntry(x, y))
#define READ_ENTRY(x) cg.readEntry(x, khtmlrc.readEntry(x))


    // *** load ***
    SET_GROUP( "MainView Settings" );
    bool bOpenMiddleClick = READ_BOOL( "OpenMiddleClick", true );
    bool bBackRightClick = READ_BOOL( "BackRightClick", false );
    SET_GROUP( "HTML Settings" );
    bool changeCursor = READ_BOOL("ChangeCursor", KDE_DEFAULT_CHANGECURSOR);
    bool bAutoRedirect = cg.readEntry( "AutoDelayedActions", true );

    // *** apply to GUI ***
    m_cbCursor->setChecked( changeCursor );
    m_pAutoRedirectCheckBox->setChecked( bAutoRedirect );
    m_pOpenMiddleClick->setChecked( bOpenMiddleClick );
    m_pBackRightClick->setChecked( bBackRightClick );

    m_pFormCompletionCheckBox->setChecked( cg.readEntry( "FormCompletion", true ) );
    m_pMaxFormCompletionItems->setValue( cg.readEntry( "MaxFormCompletionItems", 10 ) );
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );

    // Reads in the value of m_accessKeysEnabled by calling accessKeysEnabled() in khtml_settings.cpp
    KHTMLSettings settings;
    m_pAccessKeys->setChecked( settings.accessKeysEnabled() );

    KConfigGroup config(KSharedConfig::openConfig("kbookmarkrc", KConfig::NoGlobals), "Bookmarks");
    m_pAdvancedAddBookmarkCheckBox->setChecked( config.readEntry("AdvancedAddBookmarkDialog", false) );
    m_pOnlyMarkedBookmarksCheckBox->setChecked( config.readEntry("FilteredToolbar", false) );
}

void KMiscHTMLOptions::defaults()
{
    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);
    m_pAdvancedAddBookmarkCheckBox->setChecked(false);
    m_pOnlyMarkedBookmarksCheckBox->setChecked(false);
}

void KMiscHTMLOptions::save()
{
    KConfigGroup cg(m_pConfig, "MainView Settings");
    cg.writeEntry( "OpenMiddleClick", m_pOpenMiddleClick->isChecked() );
    cg.writeEntry( "BackRightClick", m_pBackRightClick->isChecked() );
    cg = KConfigGroup(m_pConfig, "HTML Settings" );
    cg.writeEntry( "ChangeCursor", m_cbCursor->isChecked() );
    cg.writeEntry( "AutoDelayedActions", m_pAutoRedirectCheckBox->isChecked() );
    cg.writeEntry( "FormCompletion", m_pFormCompletionCheckBox->isChecked() );
    cg.writeEntry( "MaxFormCompletionItems", m_pMaxFormCompletionItems->value() );

    cg.sync();

    // Writes the value of m_pAccessKeys into khtmlrc to affect all applications using KHTML
    KConfig _khtmlconfig("khtmlrc", KConfig::NoGlobals);
    KConfigGroup khtmlconfig(&_khtmlconfig, "Access Keys");
    khtmlconfig.writeEntry( "Enabled", m_pAccessKeys->isChecked() );
    khtmlconfig.sync();

    KConfigGroup config(KSharedConfig::openConfig("kbookmarkrc", KConfig::NoGlobals), "Bookmarks");
    config.writeEntry("AdvancedAddBookmarkDialog", m_pAdvancedAddBookmarkCheckBox->isChecked());
    config.writeEntry("FilteredToolbar", m_pOnlyMarkedBookmarksCheckBox->isChecked());
    config.sync();
    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);

    message = QDBusMessage::createSignal("/KBookmarkManager/konqueror", "org.kde.KIO.KBookmarkManager", "bookmarkConfigChanged" );
    QDBusConnection::sessionBus().send(message);

    emit changed(false);
}


#include "htmlopts.moc"

