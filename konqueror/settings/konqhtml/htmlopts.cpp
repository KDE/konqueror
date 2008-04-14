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
#include <QtGui/QLayout>
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

enum UnderlineLinkType { UnderlineAlways=0, UnderlineNever=1, UnderlineHover=2 };
enum AnimationsType { AnimationsAlways=0, AnimationsNever=1, AnimationsLoopOnce=2 };
enum SmoothScrollingType { SmoothScrollingAlways=0, SmoothScrollingNever=1, SmoothScrollingWhenEfficient=2 };
//-----------------------------------------------------------------------------

KMiscHTMLOptions::KMiscHTMLOptions(QWidget *parent, const QVariantList&)
    : KCModule( KcmKonqHtmlFactory::componentData(), parent ), m_groupname("HTML Settings")
{
    m_pConfig = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
    int row = 0;
    QGridLayout *lay = new QGridLayout(this);

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
    connect(m_pAdvancedAddBookmarkCheckBox, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    bgBookmarks->setLayout(laygroup1);

    m_pOnlyMarkedBookmarksCheckBox = new QCheckBox(i18n( "Show only marked bookmarks in bookmark toolbar" ), bgBookmarks);
    laygroup1->addWidget(m_pOnlyMarkedBookmarksCheckBox);
    m_pOnlyMarkedBookmarksCheckBox->setWhatsThis( i18n( "If this box is checked, Konqueror will show only those"
                     " bookmarks in the bookmark toolbar which you have marked to do so in the bookmark editor." ) );
    connect(m_pOnlyMarkedBookmarksCheckBox, SIGNAL(toggled(bool)), SLOT(slotChanged()));

    lay->addWidget( bgBookmarks, row, 0, 1, 2 );
    row++;

     // Form completion

    QGroupBox *bgForm = new QGroupBox( i18n("Form Com&pletion") );

    QVBoxLayout *laygroup2 = new QVBoxLayout();
    laygroup2->setSpacing(KDialog::spacingHint());

    m_pFormCompletionCheckBox = new QCheckBox(i18n( "Enable completion of &forms" ));
    laygroup2->addWidget( m_pFormCompletionCheckBox );

    m_pFormCompletionCheckBox->setWhatsThis( i18n( "If this box is checked, Konqueror will remember"
                  " the data you enter in web forms and suggest it in similar fields for all forms." ) );
    connect(m_pFormCompletionCheckBox, SIGNAL(toggled(bool)), SLOT(slotChanged()));

    m_pMaxFormCompletionItems = new KIntNumInput;
    m_pMaxFormCompletionItems->setLabel( i18n( "&Maximum completions:" ) );
    m_pMaxFormCompletionItems->setRange( 0, 100 );
    laygroup2->addWidget( m_pMaxFormCompletionItems );
    m_pMaxFormCompletionItems->setWhatsThis(
        i18n( "Here you can select how many values Konqueror will remember for a form field." ) );
    connect(m_pMaxFormCompletionItems, SIGNAL(valueChanged(int)), SLOT(slotChanged()));
    bgForm->setLayout(laygroup2);

    lay->addWidget( bgForm, row, 0, 1, 2 );
    row++;

    // Mouse behavior

    QGroupBox *bgMouse = new QGroupBox( i18n("Mouse Beha&vior") );
    QVBoxLayout *laygroup3 = new QVBoxLayout();
    laygroup3->setSpacing(KDialog::spacingHint());

    m_cbCursor = new QCheckBox(i18n("Chan&ge cursor over links") );
    laygroup3->addWidget( m_cbCursor );
    m_cbCursor->setWhatsThis( i18n("If this option is set, the shape of the cursor will change "
       "(usually to a hand) if it is moved over a hyperlink.") );
    connect(m_cbCursor, SIGNAL(toggled(bool)), SLOT(slotChanged()));

    m_pOpenMiddleClick = new QCheckBox( i18n ("M&iddle click opens URL in selection" ) );
    laygroup3->addWidget( m_pOpenMiddleClick );
    m_pOpenMiddleClick->setWhatsThis( i18n (
      "If this box is checked, you can open the URL in the selection by middle clicking on a "
      "Konqueror view." ) );
    connect(m_pOpenMiddleClick, SIGNAL(toggled(bool)), SLOT(slotChanged()));

    m_pBackRightClick = new QCheckBox( i18n( "Right click goes &back in history" ) );
    laygroup3->addWidget( m_pBackRightClick );
    m_pBackRightClick->setWhatsThis( i18n(
      "If this box is checked, you can go back in history by right clicking on a Konqueror view. "
      "To access the context menu, press the right mouse button and move." ) );
    connect(m_pBackRightClick, SIGNAL(toggled(bool)), SLOT(slotChanged()));

    bgMouse->setLayout(laygroup3);
    lay->addWidget( bgMouse, row, 0, 1, 2 );
    row++;

    // Misc

    m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( "A&utomatically load images"), this );
    m_pAutoLoadImagesCheckBox->setWhatsThis( i18n( "<html>If this box is checked, Konqueror will"
			    " automatically load any images that are embedded in a web page."
			    " Otherwise, it will display placeholders for the images, and"
			    " you can then manually load the images by clicking on the image"
			    " button.<br />Unless you have a very slow network connection, you"
			    " will probably want to check this box to enhance your browsing"
			    " experience.</html>" ) );
    connect(m_pAutoLoadImagesCheckBox, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    lay->addWidget( m_pAutoLoadImagesCheckBox, row, 0, 1, 2 );
    row++;

    m_pUnfinishedImageFrameCheckBox = new QCheckBox( i18n( "Dra&w frame around not completely loaded images"), this );
    m_pUnfinishedImageFrameCheckBox->setWhatsThis( i18n( "<html>If this box is checked, Konqueror will draw"
			    " a frame as a placeholder around images embedded in a web page that are"
			    " not yet fully loaded.<br />You will probably want to check this box to"
			    " enhance your browsing experience, especially if have a slow network"
			    " connection.</html>" ) );
    connect(m_pUnfinishedImageFrameCheckBox, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    lay->addWidget( m_pUnfinishedImageFrameCheckBox, row, 0, 1, 2 );
    row++;

    m_pAutoRedirectCheckBox = new QCheckBox( i18n( "Allow automatic delayed &reloading/redirecting"), this );
    m_pAutoRedirectCheckBox->setWhatsThis( i18n( "Some web pages request an automatic reload or redirection after"
			    " a certain period of time. By unchecking this box Konqueror will ignore these requests." ) );
    connect(m_pAutoRedirectCheckBox, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    lay->addWidget( m_pAutoRedirectCheckBox, row, 0, 1, 2 );
    row++;

    // Checkbox to enable/disable Access Key activation through the Ctrl key.

    m_pAccessKeys = new QCheckBox( i18n( "Enable/disable Access Ke&y activation with Ctrl key"), this );
    m_pAccessKeys->setWhatsThis( i18n( "Pressing the Ctrl key when viewing webpages activates KDE's Access Keys. Unchecking this box will disable this accessibility feature. (Konqueror needs to be restarted for changes to take effect.)" ) );
    connect(m_pAccessKeys, SIGNAL(toggled(bool)), SLOT(slotChanged()));
    lay->addMultiCellWidget( m_pAccessKeys, row, row, 0, 1 );
    row++;

    // More misc

    KSeparator *sep = new KSeparator(this);
    lay->addWidget(sep, row, 0, 1, 2 );
    row++;

    QLabel *label = new QLabel( i18n("Und&erline links:"), this );
    m_pUnderlineCombo = new QComboBox( this );
    label->setBuddy(m_pUnderlineCombo);
    m_pUnderlineCombo->setEditable(false);
    m_pUnderlineCombo->insertItem(UnderlineAlways, i18nc("underline","Enabled"));
    m_pUnderlineCombo->insertItem(UnderlineNever, i18nc("underline","Disabled"));
    m_pUnderlineCombo->insertItem(UnderlineHover, i18n("Only on Hover"));
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pUnderlineCombo, row, 1);
    row++;
    QString whatsThis = i18n("<html>Controls how Konqueror handles underlining hyperlinks:<br />"
	    "<ul><li><b>Enabled</b>: Always underline links</li>"
	    "<li><b>Disabled</b>: Never underline links</li>"
	    "<li><b>Only on Hover</b>: Underline when the mouse is moved over the link</li>"
	    "</ul><br /><i>Note: The site's CSS definitions can override this value.</i></html>");
    label->setWhatsThis(whatsThis);
    m_pUnderlineCombo->setWhatsThis(whatsThis);
    connect(m_pUnderlineCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotChanged()));



    label = new QLabel( i18n("A&nimations:"), this );
    m_pAnimationsCombo = new QComboBox( this );
    label->setBuddy(m_pAnimationsCombo);
    m_pAnimationsCombo->setEditable(false);
    m_pAnimationsCombo->insertItem(AnimationsAlways, i18nc("animations","Enabled"));
    m_pAnimationsCombo->insertItem(AnimationsNever, i18nc("animations","Disabled"));
    m_pAnimationsCombo->insertItem(AnimationsLoopOnce, i18n("Show Only Once"));
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pAnimationsCombo, row, 1);
    row++;
    whatsThis = i18n("<html>Controls how Konqueror shows animated images:<br />"
	    "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
	    "<li><b>Disabled</b>: Never show animations, show the starting image only.</li>"
	    "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li></ul></html>");
    label->setWhatsThis(whatsThis);
    m_pAnimationsCombo->setWhatsThis(whatsThis);
    connect(m_pAnimationsCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotChanged()));

    label = new QLabel( i18n("S&mooth scrolling:"), this );
    m_pSmoothScrollingCombo = new QComboBox( this );
    label->setBuddy(m_pSmoothScrollingCombo);
    m_pSmoothScrollingCombo->setEditable(false);
    m_pSmoothScrollingCombo->insertItem(SmoothScrollingWhenEfficient, i18n("When Efficient"));
    m_pSmoothScrollingCombo->insertItem(SmoothScrollingAlways, i18nc("smooth scrolling","Always"));
    m_pSmoothScrollingCombo->insertItem(SmoothScrollingNever, i18nc("soft scrolling","Never"));
    lay->addWidget(label, row, 0);
    lay->addWidget(m_pSmoothScrollingCombo, row, 1);
    row++;
    whatsThis = i18n("<html>Determines whether Konqueror should use smooth steps to scroll HTML pages, or whole steps:<br />"
	    "<ul><li><b>Always</b>: Always use smooth steps when scrolling.</li>"
	    "<li><b>Never</b>: Never use smooth scrolling, scroll with whole steps instead.</li>"
	    "<li><b>When Efficient</b>: Only use smooth scrolling on pages where it can be achieved with moderate usage of system resources.</li></ul></html>");
    label->setWhatsThis(whatsThis);
    m_pSmoothScrollingCombo->setWhatsThis(whatsThis);
    connect(m_pSmoothScrollingCombo, SIGNAL(currentIndexChanged(int)), SLOT(slotChanged()));

    lay->setRowStretch(row, 1);

    load();
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
    bool underlineLinks = READ_BOOL("UnderlineLinks", true /*was DEFAULT_UNDERLINELINKS, but comes from khtml in fact */ );
    bool hoverLinks = READ_BOOL("HoverLinks", true);
    bool bAutoLoadImages = READ_BOOL( "AutoLoadImages", true );
    bool bUnfinishedImageFrame = READ_BOOL( "UnfinishedImageFrame", true );
    QString strAnimations = READ_ENTRY( "ShowAnimations" ).toLower();
    QString strSmoothScrolling = READ_ENTRY( "SmoothScrolling" ).toLower();

    bool bAutoRedirect = cg.readEntry( "AutoDelayedActions", true );

    // *** apply to GUI ***
    m_cbCursor->setChecked( changeCursor );
    m_pAutoLoadImagesCheckBox->setChecked( bAutoLoadImages );
    m_pUnfinishedImageFrameCheckBox->setChecked( bUnfinishedImageFrame );
    m_pAutoRedirectCheckBox->setChecked( bAutoRedirect );
    m_pOpenMiddleClick->setChecked( bOpenMiddleClick );
    m_pBackRightClick->setChecked( bBackRightClick );

    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (hoverLinks)
    {
        m_pUnderlineCombo->setCurrentIndex( UnderlineHover );
    }
    else
    {
        if (underlineLinks)
            m_pUnderlineCombo->setCurrentIndex( UnderlineAlways );
        else
            m_pUnderlineCombo->setCurrentIndex( UnderlineNever );
    }
    if (strAnimations == "disabled")
       m_pAnimationsCombo->setCurrentIndex( AnimationsNever );
    else if (strAnimations == "looponce")
       m_pAnimationsCombo->setCurrentIndex( AnimationsLoopOnce );
    else
       m_pAnimationsCombo->setCurrentIndex( AnimationsAlways );

    if (strSmoothScrolling == "disabled")
       m_pSmoothScrollingCombo->setCurrentIndex( SmoothScrollingNever );
    else if (strSmoothScrolling == "enabled")
       m_pSmoothScrollingCombo->setCurrentIndex( SmoothScrollingAlways );
    else
       m_pSmoothScrollingCombo->setCurrentIndex( SmoothScrollingWhenEfficient );

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
    cg.writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
    cg.writeEntry( "UnfinishedImageFrame", m_pUnfinishedImageFrameCheckBox->isChecked() );
    cg.writeEntry( "AutoDelayedActions", m_pAutoRedirectCheckBox->isChecked() );
    switch(m_pUnderlineCombo->currentIndex())
    {
      case UnderlineAlways:
        cg.writeEntry( "UnderlineLinks", true );
        cg.writeEntry( "HoverLinks", false );
        break;
      case UnderlineNever:
        cg.writeEntry( "UnderlineLinks", false );
        cg.writeEntry( "HoverLinks", false );
        break;
      case UnderlineHover:
        cg.writeEntry( "UnderlineLinks", false );
        cg.writeEntry( "HoverLinks", true );
        break;
    }
    switch(m_pAnimationsCombo->currentIndex())
    {
      case AnimationsAlways:
        cg.writeEntry( "ShowAnimations", "Enabled" );
        break;
      case AnimationsNever:
        cg.writeEntry( "ShowAnimations", "Disabled" );
        break;
      case AnimationsLoopOnce:
        cg.writeEntry( "ShowAnimations", "LoopOnce" );
        break;
    }

    switch(m_pSmoothScrollingCombo->currentIndex())
    {
      case SmoothScrollingAlways:
        cg.writeEntry( "SmoothScrolling", "Enabled" );
        break;
      case SmoothScrollingNever:
        cg.writeEntry( "SmoothScrolling", "Disabled" );
        break;
      case SmoothScrollingWhenEfficient:
        cg.writeEntry( "SmoothScrolling", "WhenEfficient" );
        break;
    }

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


void KMiscHTMLOptions::slotChanged()
{
    m_pMaxFormCompletionItems->setEnabled( m_pFormCompletionCheckBox->isChecked() );
    emit changed(true);
}

#include "htmlopts.moc"

