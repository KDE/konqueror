#include "kcmcss.h"

#include <QLabel>
#include <QLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#include <QGroupBox>

#include <kapplication.h>
#include <kcharsets.h>
#include <ksharedconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobal.h>
#include <khtmldefaults.h>
#include <klocale.h>
#include <knuminput.h>
#include <kglobalsettings.h>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KFontComboBox>
#include <KTabWidget>

#if defined Q_WS_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#endif

#include "appearance.moc"

K_PLUGIN_FACTORY_DECLARATION(KcmKonqHtmlFactory)

static const char* const animationValues[]={"Enabled","Disabled","LoopOnce"};
enum AnimationsType { AnimationsAlways=0, AnimationsNever=1, AnimationsLoopOnce=2 };

static const char* const smoothScrollingValues[]={"Enabled","Disabled","WhenEfficient"};
enum SmoothScrollingType { SmoothScrollingAlways=0, SmoothScrollingNever=1, SmoothScrollingWhenEfficient=2 };

enum UnderlineLinkType { UnderlineAlways=0, UnderlineNever=1, UnderlineHover=2 };

KAppearanceOptions::KAppearanceOptions(QWidget *parent, const QVariantList&)
    : KCModule( KcmKonqHtmlFactory::componentData(), parent ), m_groupname("HTML Settings"),
      fSize( 10 ), fMinSize( HTML_DEFAULT_MIN_FONT_SIZE )

{
  QVBoxLayout* l=new QVBoxLayout(this);
  KTabWidget* tab=new KTabWidget(this);
  l->addWidget(tab);
  QWidget* mainTab=new QWidget(this);
  QWidget* fontsTab=new QWidget(this);
  cssConfig=new CSSConfig(this);
  tab->addTab(mainTab,i18nc("@title:tab","General"));
  tab->addTab(fontsTab,i18nc("@title:tab","Fonts"));
  tab->addTab(cssConfig,i18nc("@title:tab","Stylesheets"));
  
  connect( cssConfig, SIGNAL(changed()), this, SLOT(changed()) );
  connect( cssConfig, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)) );
  
  
  l=new QVBoxLayout(mainTab);

    //Images
  QGroupBox *box = new QGroupBox( i18n("Images"),mainTab);
  l->addWidget(box);
  QFormLayout *fl= new QFormLayout(box);

  m_pAutoLoadImagesCheckBox = new QCheckBox( i18n( "A&utomatically load images"), this );
  m_pAutoLoadImagesCheckBox->setWhatsThis( i18n( "<html>If this box is checked, Konqueror will"
                            " automatically load any images that are embedded in a web page."
                            " Otherwise, it will display placeholders for the images, and"
                            " you can then manually load the images by clicking on the image"
                            " button.<br />Unless you have a very slow network connection, you"
                            " will probably want to check this box to enhance your browsing"
                            " experience.</html>" ) );
  connect(m_pAutoLoadImagesCheckBox, SIGNAL(toggled(bool)), SLOT(changed()));
  fl->addRow( m_pAutoLoadImagesCheckBox);

  m_pUnfinishedImageFrameCheckBox = new QCheckBox( i18n( "Dra&w frame around not completely loaded images"), this );
  m_pUnfinishedImageFrameCheckBox->setWhatsThis( i18n( "<html>If this box is checked, Konqueror will draw"
                            " a frame as a placeholder around images embedded in a web page that are"
                            " not yet fully loaded.<br />You will probably want to check this box to"
                            " enhance your browsing experience, especially if have a slow network"
                            " connection.</html>" ) );
  connect(m_pUnfinishedImageFrameCheckBox, SIGNAL(toggled(bool)), SLOT(changed()));
  fl->addRow( m_pUnfinishedImageFrameCheckBox );


  m_pAnimationsCombo = new QComboBox( this );
  m_pAnimationsCombo->setEditable(false);
  m_pAnimationsCombo->insertItem(AnimationsAlways, i18nc("animations","Enabled"));
  m_pAnimationsCombo->insertItem(AnimationsNever, i18nc("animations","Disabled"));
  m_pAnimationsCombo->insertItem(AnimationsLoopOnce, i18n("Show Only Once"));
  m_pAnimationsCombo->setWhatsThis(i18n("<html>Controls how Konqueror shows animated images:<br />"
            "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
            "<li><b>Disabled</b>: Never show animations, show the starting image only.</li>"
            "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li></ul></html>"));
  connect(m_pAnimationsCombo, SIGNAL(currentIndexChanged(int)), SLOT(changed()));
  fl->addRow(i18n("A&nimations:"), m_pAnimationsCombo);

 
  
  //Other
  box = new QGroupBox(i18nc("@title:group","Miscellaneous"),mainTab);
  l->addWidget(box);
  fl=new QFormLayout(box);
  m_pUnderlineCombo = new QComboBox( this );
  m_pUnderlineCombo->setEditable(false);
  m_pUnderlineCombo->insertItem(UnderlineAlways, i18nc("underline","Enabled"));
  m_pUnderlineCombo->insertItem(UnderlineNever, i18nc("underline","Disabled"));
  m_pUnderlineCombo->insertItem(UnderlineHover, i18n("Only on Hover"));
  fl->addRow(i18n("Und&erline links:"),m_pUnderlineCombo);

  m_pUnderlineCombo->setWhatsThis(i18n(
            "<html>Controls how Konqueror handles underlining hyperlinks:<br />"
            "<ul><li><b>Enabled</b>: Always underline links</li>"
            "<li><b>Disabled</b>: Never underline links</li>"
            "<li><b>Only on Hover</b>: Underline when the mouse is moved over the link</li>"
            "</ul><br /><i>Note: The site's CSS definitions can override this value.</i></html>"));
  connect(m_pUnderlineCombo, SIGNAL(currentIndexChanged(int)), SLOT(changed()));


  m_pSmoothScrollingCombo = new QComboBox( this );
  m_pSmoothScrollingCombo->setEditable(false);
  m_pSmoothScrollingCombo->insertItem(SmoothScrollingWhenEfficient, i18n("When Efficient"));
  m_pSmoothScrollingCombo->insertItem(SmoothScrollingAlways, i18nc("smooth scrolling","Always"));
  m_pSmoothScrollingCombo->insertItem(SmoothScrollingNever, i18nc("soft scrolling","Never"));
  fl->addRow(i18n("S&mooth scrolling:"), m_pSmoothScrollingCombo);
  m_pSmoothScrollingCombo->setWhatsThis(i18n(
            "<html>Determines whether Konqueror should use smooth steps to scroll HTML pages, or whole steps:<br />"
            "<ul><li><b>Always</b>: Always use smooth steps when scrolling.</li>"
            "<li><b>Never</b>: Never use smooth scrolling, scroll with whole steps instead.</li>"
            "<li><b>When Efficient</b>: Only use smooth scrolling on pages where it can be achieved with moderate usage of system resources.</li></ul></html>"));
  connect(m_pSmoothScrollingCombo, SIGNAL(currentIndexChanged(int)), SLOT(changed()));

  l->addStretch(5);
  
  
  
  
  
  m_pConfig = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
  setQuickHelp( i18n("<h1>Konqueror Fonts</h1>On this page, you can configure "
              "which fonts Konqueror should use to display the web "
              "pages you view.") + "<br /><br />" +cssConfig->whatsThis());

  QString empty;
  //initialise fonts list otherwise it crashs
  while (fonts.count() < 7)
    fonts.append(empty);

  QVBoxLayout *lay = new QVBoxLayout(fontsTab);

  QGroupBox* gb = new QGroupBox( i18n("Font Si&ze"));
  lay->addWidget(gb);
  fl=new QFormLayout(gb);
  gb->setWhatsThis( i18n("This is the relative font size Konqueror uses to display web sites.") );

  m_minSize = new KIntNumInput( fMinSize);
  fl->addRow(i18n( "M&inimum font size:" ),m_minSize);  
  m_minSize->setRange( 2, 30 );
  connect( m_minSize, SIGNAL(valueChanged(int)), this, SLOT(slotMinimumFontSize(int)) );
  connect( m_minSize, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
  m_minSize->setWhatsThis( "<qt>" + i18n( "Konqueror will never display text smaller than "
                                    "this size,<br />overriding any other settings." ) + "</qt>" );

  m_MedSize = new KIntNumInput( fSize,m_minSize );
  fl->addRow(i18n( "&Medium font size:"  ),m_MedSize);
  m_MedSize->setRange( 2, 30 );
  connect( m_MedSize, SIGNAL(valueChanged(int)), this, SLOT(slotFontSize(int)) );
  connect( m_MedSize, SIGNAL(valueChanged(int)), this, SLOT(changed()) );
  m_MedSize->setWhatsThis(
                   i18n("This is the relative font size Konqueror uses "
                        "to display web sites.") );


  box = new QGroupBox(/*i18n("Font Families")*/);
  lay->addWidget(box);
  fl=new QFormLayout(box);


  m_pFonts[0] = new KFontComboBox( fontsTab );
  fl->addRow(i18n("S&tandard font:"), m_pFonts[0]);
  m_pFonts[0]->setWhatsThis( i18n("This is the font used to display normal text in a web page.") );
  connect( m_pFonts[0], SIGNAL(currentFontChanged(QFont)),
	   SLOT(slotStandardFont(QFont)) );

  m_pFonts[1] = new KFontComboBox( fontsTab );
  fl->addRow( i18n( "&Fixed font:"),m_pFonts[1]);
  m_pFonts[1]->setWhatsThis( i18n("This is the font used to display fixed-width (i.e. non-proportional) text."));
  connect( m_pFonts[1], SIGNAL(currentFontChanged(QFont)),
          SLOT(slotFixedFont(QFont)) );

  m_pFonts[2] = new KFontComboBox( this );
  fl->addRow(i18n( "S&erif font:" ),  m_pFonts[2]);
  m_pFonts[2]->setWhatsThis( i18n( "This is the font used to display text that is marked up as serif." ) );

  connect( m_pFonts[2], SIGNAL(currentFontChanged(QFont)),
          SLOT(slotSerifFont(QFont)) );

  m_pFonts[3] = new KFontComboBox( this );
  fl->addRow(i18n( "Sa&ns serif font:" ),  m_pFonts[3]);
  m_pFonts[3]->setWhatsThis( i18n( "This is the font used to display text that is marked up as sans-serif." ) );
  connect( m_pFonts[3], SIGNAL(currentFontChanged(QFont)),
          SLOT(slotSansSerifFont(QFont)) );

  m_pFonts[4] = new KFontComboBox( this );
  fl->addRow(i18n( "C&ursive font:" ),  m_pFonts[4]);
  m_pFonts[4]->setWhatsThis( i18n( "This is the font used to display text that is marked up as italic." ) );
  connect( m_pFonts[4], SIGNAL(currentFontChanged(QFont)),
          SLOT(slotCursiveFont(QFont)) );

  m_pFonts[5] = new KFontComboBox( this );
  fl->addRow(i18n( "Fantas&y font:" ), m_pFonts[5]);
  m_pFonts[5]->setWhatsThis( i18n( "This is the font used to display text that is marked up as a fantasy font." ) );
  connect( m_pFonts[5], SIGNAL(currentFontChanged(QFont)),
          SLOT(slotFantasyFont(QFont)) );

  for(int i = 0; i < 6; ++i)
      connect( m_pFonts[i], SIGNAL(currentFontChanged(QFont)),
              SLOT(changed()) );




  m_pFontSizeAdjust = new KIntSpinBox( this );
  m_pFontSizeAdjust->setRange( -5, 5 );
  m_pFontSizeAdjust->setSingleStep( 1 );
  fl->addRow(i18n( "Font &size adjustment for this encoding:" ), m_pFontSizeAdjust);

  connect( m_pFontSizeAdjust, SIGNAL(valueChanged(int)),
	   SLOT(slotFontSizeAdjust(int)) );
  connect( m_pFontSizeAdjust, SIGNAL(valueChanged(int)),
	   SLOT(changed()) );


  m_pEncoding = new QComboBox( this );
  m_pEncoding->setEditable( false );
  encodings = KGlobal::charsets()->availableEncodingNames();
  encodings.prepend(i18n("Use Language Encoding"));
  m_pEncoding->addItems( encodings );
  fl->addRow(i18n( "Default encoding:"), m_pEncoding);

  m_pEncoding->setWhatsThis( i18n( "Select the default encoding to be used; normally, you will be fine with 'Use language encoding' "
               "and should not have to change this.") );

  connect( m_pEncoding, SIGNAL(activated(QString)),
	   SLOT(slotEncoding(QString)) );
  connect( m_pEncoding, SIGNAL(activated(QString)),
	   SLOT(changed()) );

  lay->addStretch(5);
}

KAppearanceOptions::~KAppearanceOptions()
{
}

void KAppearanceOptions::slotFontSize( int i )
{
    fSize = i;
    if ( fSize < fMinSize ) {
        m_minSize->setValue( fSize );
        fMinSize = fSize;
    }
}


void KAppearanceOptions::slotMinimumFontSize( int i )
{
    fMinSize = i;
    if ( fMinSize > fSize ) {
        m_MedSize->setValue( fMinSize );
        fSize = fMinSize;
    }
}


void KAppearanceOptions::slotStandardFont(const QFont& n )
{
    fonts[0] = n.family();
}


void KAppearanceOptions::slotFixedFont(const QFont& n )
{
    fonts[1] = n.family();
}


void KAppearanceOptions::slotSerifFont( const QFont& n )
{
    fonts[2] = n.family();
}


void KAppearanceOptions::slotSansSerifFont( const QFont& n )
{
    fonts[3] = n.family();
}


void KAppearanceOptions::slotCursiveFont( const QFont& n )
{
    fonts[4] = n.family();
}


void KAppearanceOptions::slotFantasyFont( const QFont& n )
{
    fonts[5] = n.family();
}

void KAppearanceOptions::slotFontSizeAdjust( int value )
{
    fonts[6] = QString::number( value );
}

void KAppearanceOptions::slotEncoding(const QString& n)
{
    encodingName = n;
}


static int stringToIndex(const char* const *possibleValues, int possibleValuesCount, int defaultValue, const QString& value)
{
    int i=possibleValuesCount;
    while (--i>=0)
        if (possibleValues[i]==value) break;
    if (i==-1) i=defaultValue;
    return i;
}

void KAppearanceOptions::load()
{
    KConfigGroup khtmlrc(KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals), "");
    KConfigGroup cg(m_pConfig, "");
#define SET_GROUP(x) cg = KConfigGroup(m_pConfig,x); khtmlrc = KConfigGroup(KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals),x)
#define READ_NUM(x,y) cg.readEntry(x, khtmlrc.readEntry(x, y))
#define READ_ENTRY(x,y) cg.readEntry(x, khtmlrc.readEntry(x, y))
#define READ_LIST(x) cg.readEntry(x, khtmlrc.readEntry(x, QStringList() ))
#define READ_BOOL(x,y) cg.readEntry(x, khtmlrc.readEntry(x, y))
#define READ_ENTRYNODEFAULT(x) cg.readEntry(x, khtmlrc.readEntry(x))

    SET_GROUP(m_groupname);
    fSize = READ_NUM( "MediumFontSize", 12 );
    fMinSize = READ_NUM( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE );
    if (fSize < fMinSize)
      fSize = fMinSize;

    defaultFonts = QStringList();
    defaultFonts.append( READ_ENTRY( "StandardFont", KGlobalSettings::generalFont().family() ) );
    defaultFonts.append( READ_ENTRY( "FixedFont", KGlobalSettings::fixedFont().family() ) );
    defaultFonts.append( READ_ENTRY( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT ) );
    defaultFonts.append( READ_ENTRY( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT ) );
    defaultFonts.append( READ_ENTRY( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT ) );
    defaultFonts.append( READ_ENTRY( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT ) );
    defaultFonts.append( QString("0") ); // default font size adjustment

    if (cg.hasKey("Fonts"))
       fonts = cg.readEntry( "Fonts" , QStringList() );
    else
       fonts = khtmlrc.readEntry( "Fonts" , QStringList() );
    while (fonts.count() < 7)
       fonts.append(QString());

    encodingName = READ_ENTRYNODEFAULT( "DefaultEncoding");//TODO move 
    //kDebug(0) << "encoding = " << encodingName;




    m_pAutoLoadImagesCheckBox->setChecked( READ_BOOL( "AutoLoadImages", true ) );
    m_pUnfinishedImageFrameCheckBox->setChecked( READ_BOOL( "UnfinishedImageFrame", true ) );

    m_pAnimationsCombo->setCurrentIndex(      stringToIndex(animationValues,      sizeof(animationValues)/sizeof(char*),      /*default*/2, READ_ENTRYNODEFAULT( "ShowAnimations" )) );
    m_pSmoothScrollingCombo->setCurrentIndex( stringToIndex(smoothScrollingValues,sizeof(smoothScrollingValues)/sizeof(char*),/*default*/2, READ_ENTRYNODEFAULT( "SmoothScrolling")) );
    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (READ_BOOL("HoverLinks", true))
        m_pUnderlineCombo->setCurrentIndex( UnderlineHover );
    else
        m_pUnderlineCombo->setCurrentIndex( READ_BOOL("UnderlineLinks",true)? UnderlineAlways : UnderlineNever);


    cssConfig->load();



    updateGUI();
    emit changed(false);
}

void KAppearanceOptions::defaults()
{
    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);

    cssConfig->defaults();
    emit changed(true);
}

void KAppearanceOptions::updateGUI()
{
    //kDebug() << "KAppearanceOptions::updateGUI " << charset;
    for ( int f = 0; f < 6; f++ ) {
        QString ff = fonts[f];
        if (ff.isEmpty())
           ff = defaultFonts[f];
        m_pFonts[f]->setCurrentFont(ff);
    }

    int i = 0;
    for ( QStringList::const_iterator it = encodings.constBegin(); it != encodings.constEnd(); ++it, ++i )
        if ( encodingName == *it )
            m_pEncoding->setCurrentIndex( i );
    if(encodingName.isEmpty())
        m_pEncoding->setCurrentIndex( 0 );
    m_pFontSizeAdjust->setValue( fonts[6].toInt() );
    m_MedSize->blockSignals(true);
    m_MedSize->setValue( fSize );
    m_MedSize->blockSignals(false);
    m_minSize->blockSignals(true);
    m_minSize->setValue( fMinSize );
    m_minSize->blockSignals(false);
}

void KAppearanceOptions::save()
{
    KConfigGroup cg(m_pConfig, m_groupname);
    cg.writeEntry( "MediumFontSize", fSize );
    cg.writeEntry( "MinimumFontSize", fMinSize );
    cg.writeEntry( "Fonts", fonts );

    //TODO move to behaviour
    // If the user chose "Use language encoding", write an empty string
    if (encodingName == i18n("Use Language Encoding"))
        encodingName = "";
    cg.writeEntry( "DefaultEncoding", encodingName );



    //Images
    cg.writeEntry( "AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked() );
    cg.writeEntry( "UnfinishedImageFrame", m_pUnfinishedImageFrameCheckBox->isChecked() );
    cg.writeEntry( "ShowAnimations", animationValues[m_pAnimationsCombo->currentIndex()] );
    cg.writeEntry( "UnderlineLinks", m_pUnderlineCombo->currentIndex()==UnderlineAlways);
    cg.writeEntry( "HoverLinks", m_pUnderlineCombo->currentIndex()==UnderlineHover);
    cg.writeEntry( "SmoothScrolling", smoothScrollingValues[m_pSmoothScrollingCombo->currentIndex()] );

    cssConfig->save();

    cg.sync();
    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);


  emit changed(false);
}

