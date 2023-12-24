#include "appearance.h"
#include "../../src/htmldefaults.h"

#include "kcmcss.h"

#include <QFormLayout>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QGroupBox>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QSpinBox>
#include <QTabWidget>

#include <kcharsets.h>
#include <ksharedconfig.h>
#include <KLocalizedString>
#include <KConfigGroup>

static const char *const animationValues[] = {"Enabled", "Disabled", "LoopOnce"};
enum AnimationsType { AnimationsAlways = 0, AnimationsNever = 1, AnimationsLoopOnce = 2 };

static const char *const smoothScrollingValues[] = {"Enabled", "Disabled", "WhenEfficient"};
enum SmoothScrollingType { SmoothScrollingAlways = 0, SmoothScrollingNever = 1, SmoothScrollingWhenEfficient = 2 };

enum UnderlineLinkType { UnderlineAlways = 0, UnderlineNever = 1, UnderlineHover = 2 };

KAppearanceOptions::KAppearanceOptions(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md), m_groupname(QStringLiteral("HTML Settings")),
      fSize(10), fMinSize(HTML_DEFAULT_MIN_FONT_SIZE)

{
    QVBoxLayout *l = new QVBoxLayout(widget());
    QTabWidget *tabWidget = new QTabWidget(widget());
    l->addWidget(tabWidget);
    QWidget *mainTab = new QWidget(widget());
    QWidget *fontsTab = new QWidget(widget());
    cssConfig = new CSSConfig(widget());
    tabWidget->addTab(mainTab, i18nc("@title:tab", "General"));
    tabWidget->addTab(fontsTab, i18nc("@title:tab", "Fonts"));
    tabWidget->addTab(cssConfig, i18nc("@title:tab", "Stylesheets"));

#if QT_VERSION_MAJOR < 6
    connect(cssConfig, &CSSConfig::changed, this, &KAppearanceOptions::markAsChanged);
#else
    connect(cssConfig, &CSSConfig::changed, this, [this](){setNeedsSave(true);});
#endif

    l = new QVBoxLayout(mainTab);

    //Images
    QGroupBox *box = new QGroupBox(i18n("Images"), mainTab);
    l->addWidget(box);
    QFormLayout *fl = new QFormLayout(box);

    m_pAutoLoadImagesCheckBox = new QCheckBox(i18n("A&utomatically load images"), widget());
    m_pAutoLoadImagesCheckBox->setToolTip(i18n("<html>If this box is checked, Konqueror will"
                                               " automatically load any images that are embedded in a web page."
                                               " Otherwise, it will display placeholders for the images, and"
                                               " you can then manually load the images by clicking on the image"
                                               " button.<br />Unless you have a very slow network connection, you"
                                               " will probably want to check this box to enhance your browsing"
                                               " experience.</html>"));
    connect(m_pAutoLoadImagesCheckBox, &QAbstractButton::toggled, this, &KAppearanceOptions::markAsChanged);
    fl->addRow(m_pAutoLoadImagesCheckBox);

    m_pUnfinishedImageFrameCheckBox = new QCheckBox(i18n("Dra&w frame around not completely loaded images"), widget());
    m_pUnfinishedImageFrameCheckBox->setToolTip(i18n("<html>If this box is checked, Konqueror will draw"
                                                     " a frame as a placeholder around images embedded in a web page that are"
                                                     " not yet fully loaded.<br />You will probably want to check this box to"
                                                     " enhance your browsing experience, especially if have a slow network"
                                                     " connection.</html>"));
    connect(m_pUnfinishedImageFrameCheckBox, &QAbstractButton::toggled, this, &KAppearanceOptions::markAsChanged);
    fl->addRow(m_pUnfinishedImageFrameCheckBox);

    m_pAnimationsCombo = new QComboBox(widget());
    m_pAnimationsCombo->setEditable(false);
    m_pAnimationsCombo->insertItem(AnimationsAlways, i18nc("animations", "Enabled"));
    m_pAnimationsCombo->insertItem(AnimationsNever, i18nc("animations", "Disabled"));
    m_pAnimationsCombo->insertItem(AnimationsLoopOnce, i18n("Show Only Once"));
    m_pAnimationsCombo->setToolTip(i18n("<html>Controls how Konqueror shows animated images:<br />"
                                        "<ul><li><b>Enabled</b>: Show all animations completely.</li>"
                                        "<li><b>Disabled</b>: Never show animations, show the starting image only.</li>"
                                        "<li><b>Show only once</b>: Show all animations completely but do not repeat them.</li></ul></html>"));
    connect(m_pAnimationsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KAppearanceOptions::markAsChanged);
    fl->addRow(i18n("A&nimations:"), m_pAnimationsCombo);

    //Other
    box = new QGroupBox(i18nc("@title:group", "Miscellaneous"), mainTab);
    l->addWidget(box);
    fl = new QFormLayout(box);
    m_pUnderlineCombo = new QComboBox(widget());
    m_pUnderlineCombo->setEditable(false);
    m_pUnderlineCombo->insertItem(UnderlineAlways, i18nc("underline", "Enabled"));
    m_pUnderlineCombo->insertItem(UnderlineNever, i18nc("underline", "Disabled"));
    m_pUnderlineCombo->insertItem(UnderlineHover, i18n("Only on Hover"));
    fl->addRow(i18n("Und&erline links:"), m_pUnderlineCombo);

    m_pUnderlineCombo->setToolTip(i18n(
                                      "<html>Controls how Konqueror handles underlining hyperlinks:<br />"
                                      "<ul><li><b>Enabled</b>: Always underline links</li>"
                                      "<li><b>Disabled</b>: Never underline links</li>"
                                      "<li><b>Only on Hover</b>: Underline when the mouse is moved over the link</li>"
                                      "</ul><br /><i>Note: The site's CSS definitions can override this value.</i></html>"));
    connect(m_pUnderlineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KAppearanceOptions::markAsChanged);

    m_pSmoothScrollingCombo = new QComboBox(widget());
    m_pSmoothScrollingCombo->setEditable(false);
    m_pSmoothScrollingCombo->insertItem(SmoothScrollingWhenEfficient, i18n("When Efficient"));
    m_pSmoothScrollingCombo->insertItem(SmoothScrollingAlways, i18nc("smooth scrolling", "Always"));
    m_pSmoothScrollingCombo->insertItem(SmoothScrollingNever, i18nc("soft scrolling", "Never"));
    fl->addRow(i18n("S&mooth scrolling:"), m_pSmoothScrollingCombo);
    m_pSmoothScrollingCombo->setToolTip(i18n(
                                            "<html>Determines whether Konqueror should use smooth steps to scroll HTML pages, or whole steps:<br />"
                                            "<ul><li><b>Always</b>: Always use smooth steps when scrolling.</li>"
                                            "<li><b>Never</b>: Never use smooth scrolling, scroll with whole steps instead.</li>"
                                            "<li><b>When Efficient</b>: Only use smooth scrolling on pages where it can be achieved with moderate usage of system resources.</li></ul></html>"));
    connect(m_pSmoothScrollingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KAppearanceOptions::markAsChanged);

    l->addStretch(5);

    m_pConfig = KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);

    QString empty;
    //initialise fonts list otherwise it crashs
    while (fonts.count() < 7) {
        fonts.append(empty);
    }

    QVBoxLayout *lay = new QVBoxLayout(fontsTab);

    QGroupBox *gb = new QGroupBox(i18n("Font Si&ze"));
    lay->addWidget(gb);
    fl = new QFormLayout(gb);
    gb->setToolTip(i18n("This is the relative font size Konqueror uses to display web sites."));

    m_minSize = new QSpinBox;
    m_minSize->setValue(fMinSize);
    fl->addRow(i18n("M&inimum font size:"), m_minSize);
    m_minSize->setRange(2, 30);
    connect(m_minSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &KAppearanceOptions::slotMinimumFontSize);
    connect(m_minSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &KAppearanceOptions::markAsChanged);
    m_minSize->setToolTip("<qt>" + i18n("Konqueror will never display text smaller than "
                                        "this size,<br />overriding any other settings.") + "</qt>");

    m_MedSize = new QSpinBox(m_minSize);
    m_MedSize->setValue(fSize);
    fl->addRow(i18n("&Medium font size:"), m_MedSize);
    m_MedSize->setRange(2, 30);
    connect(m_MedSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &KAppearanceOptions::slotFontSize);
    connect(m_MedSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &KAppearanceOptions::markAsChanged);
    m_MedSize->setToolTip(
        i18n("This is the relative font size Konqueror uses "
             "to display web sites."));

    box = new QGroupBox(/*i18n("Font Families")*/);
    lay->addWidget(box);
    fl = new QFormLayout(box);

    m_pFonts[0] = new QFontComboBox(fontsTab);
    fl->addRow(i18n("S&tandard font:"), m_pFonts[0]);
    m_pFonts[0]->setToolTip(i18n("This is the font used to display normal text in a web page."));
    connect(m_pFonts[0], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::slotStandardFont);

    m_pFonts[1] = new QFontComboBox(fontsTab);
    fl->addRow(i18n("&Fixed font:"), m_pFonts[1]);
    m_pFonts[1]->setToolTip(i18n("This is the font used to display fixed-width (i.e. non-proportional) text."));
    connect(m_pFonts[1], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::slotFixedFont);

    m_pFonts[2] = new QFontComboBox(widget());
    fl->addRow(i18n("S&erif font:"),  m_pFonts[2]);
    m_pFonts[2]->setToolTip(i18n("This is the font used to display text that is marked up as serif."));
    connect(m_pFonts[2], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::slotSerifFont);

    m_pFonts[3] = new QFontComboBox(widget());
    fl->addRow(i18n("Sa&ns serif font:"),  m_pFonts[3]);
    m_pFonts[3]->setToolTip(i18n("This is the font used to display text that is marked up as sans-serif."));
    connect(m_pFonts[3], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::slotSansSerifFont);

    m_pFonts[4] = new QFontComboBox(widget());
    fl->addRow(i18n("C&ursive font:"),  m_pFonts[4]);
    m_pFonts[4]->setToolTip(i18n("This is the font used to display text that is marked up as italic."));
    connect(m_pFonts[4], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::slotCursiveFont);

    m_pFonts[5] = new QFontComboBox(widget());
    fl->addRow(i18n("Fantas&y font:"), m_pFonts[5]);
    m_pFonts[5]->setToolTip(i18n("This is the font used to display text that is marked up as a fantasy font."));
    connect(m_pFonts[5], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::slotFantasyFont);

    for (int i = 0; i < 6; ++i)
        connect(m_pFonts[i], &QFontComboBox::currentFontChanged, this, &KAppearanceOptions::markAsChanged);

    m_pFontSizeAdjust = new QSpinBox(widget());
    m_pFontSizeAdjust->setRange(-5, 5);
    m_pFontSizeAdjust->setSingleStep(1);
    fl->addRow(i18n("Font &size adjustment for this encoding:"), m_pFontSizeAdjust);

    connect(m_pFontSizeAdjust, QOverload<int>::of(&QSpinBox::valueChanged), this, &KAppearanceOptions::slotFontSizeAdjust);
    connect(m_pFontSizeAdjust, QOverload<int>::of(&QSpinBox::valueChanged), this, &KAppearanceOptions::markAsChanged);

    m_pEncoding = new QComboBox(widget());
    m_pEncoding->setEditable(false);
    encodings = KCharsets::charsets()->availableEncodingNames();
    encodings.prepend(i18n("Use Language Encoding"));
    m_pEncoding->addItems(encodings);
    fl->addRow(i18n("Default encoding:"), m_pEncoding);

    m_pEncoding->setToolTip(i18n("Select the default encoding to be used; normally, you will be fine with 'Use language encoding' "
                                 "and should not have to change this."));
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    connect(m_pEncoding, QOverload<int>::of(&QComboBox::activated), this, [this](int n){slotEncoding(m_pEncoding->itemText(n));});
    connect(m_pEncoding, QOverload<int>::of(&QComboBox::activated), this, &KAppearanceOptions::markAsChanged);
#else
    connect(m_pEncoding, &QComboBox::textActivated, this, &KAppearanceOptions::slotEncoding);
    connect(m_pEncoding, &QComboBox::textActivated, this, &KAppearanceOptions::markAsChanged);
#endif

    lay->addStretch(5);
}

KAppearanceOptions::~KAppearanceOptions()
{
}

void KAppearanceOptions::slotFontSize(int i)
{
    fSize = i;
    if (fSize < fMinSize) {
        m_minSize->setValue(fSize);
        fMinSize = fSize;
    }
}

void KAppearanceOptions::slotMinimumFontSize(int i)
{
    fMinSize = i;
    if (fMinSize > fSize) {
        m_MedSize->setValue(fMinSize);
        fSize = fMinSize;
    }
}

void KAppearanceOptions::slotStandardFont(const QFont &n)
{
    fonts[0] = n.family();
}

void KAppearanceOptions::slotFixedFont(const QFont &n)
{
    fonts[1] = n.family();
}

void KAppearanceOptions::slotSerifFont(const QFont &n)
{
    fonts[2] = n.family();
}

void KAppearanceOptions::slotSansSerifFont(const QFont &n)
{
    fonts[3] = n.family();
}

void KAppearanceOptions::slotCursiveFont(const QFont &n)
{
    fonts[4] = n.family();
}

void KAppearanceOptions::slotFantasyFont(const QFont &n)
{
    fonts[5] = n.family();
}

void KAppearanceOptions::slotFontSizeAdjust(int value)
{
    fonts[6] = QString::number(value);
}

void KAppearanceOptions::slotEncoding(const QString &n)
{
    encodingName = n;
}

static int stringToIndex(const char *const *possibleValues, int possibleValuesCount, int defaultValue, const QString &value)
{
    int i = possibleValuesCount;
    while (--i >= 0)
        if (possibleValues[i] == value) {
            break;
        }
    if (i == -1) {
        i = defaultValue;
    }
    return i;
}

void KAppearanceOptions::load()
{
    KConfigGroup khtmlrc(KSharedConfig::openConfig(QStringLiteral("khtmlrc"), KConfig::NoGlobals), "");
    KConfigGroup cg(m_pConfig, "");
#define SET_GROUP(x) cg = KConfigGroup(m_pConfig,x); khtmlrc = KConfigGroup(KSharedConfig::openConfig("khtmlrc", KConfig::NoGlobals),x)
#define READ_NUM(x,y) cg.readEntry(x, khtmlrc.readEntry(x, y))
#define READ_LIST(x) cg.readEntry(x, khtmlrc.readEntry(x, QStringList() ))
#define READ_BOOL(x,y) cg.readEntry(x, khtmlrc.readEntry(x, y))
#define READ_ENTRYNODEFAULT(x) cg.readEntry(x, khtmlrc.readEntry(x))

    SET_GROUP(m_groupname);
    fSize = READ_NUM("MediumFontSize", 12);
    fMinSize = READ_NUM("MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE);
    if (fSize < fMinSize) {
        fSize = fMinSize;
    }

    defaultFonts = QStringList();
    defaultFonts.append(QFontDatabase::systemFont(QFontDatabase::GeneralFont).family());
    defaultFonts.append(QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
    defaultFonts.append(HTML_DEFAULT_VIEW_SERIF_FONT);
    defaultFonts.append(HTML_DEFAULT_VIEW_SANSSERIF_FONT);
    defaultFonts.append(HTML_DEFAULT_VIEW_CURSIVE_FONT);
    defaultFonts.append(HTML_DEFAULT_VIEW_FANTASY_FONT);
    defaultFonts.append(QStringLiteral("0"));   // default font size adjustment

    if (cg.hasKey("Fonts")) {
        fonts = cg.readEntry("Fonts", QStringList());
    } else {
        fonts = khtmlrc.readEntry("Fonts", QStringList());
    }
    while (fonts.count() < 7) {
        fonts.append(QString());
    }

    encodingName = READ_ENTRYNODEFAULT("DefaultEncoding"); //TODO move

    m_pAutoLoadImagesCheckBox->setChecked(READ_BOOL("AutoLoadImages", true));
    m_pUnfinishedImageFrameCheckBox->setChecked(READ_BOOL("UnfinishedImageFrame", true));

    m_pAnimationsCombo->setCurrentIndex(stringToIndex(animationValues,      sizeof(animationValues) / sizeof(char *),      /*default*/2, READ_ENTRYNODEFAULT("ShowAnimations")));
    m_pSmoothScrollingCombo->setCurrentIndex(stringToIndex(smoothScrollingValues, sizeof(smoothScrollingValues) / sizeof(char *),/*default*/2, READ_ENTRYNODEFAULT("SmoothScrolling")));
    // we use two keys for link underlining so that this config file
    // is backwards compatible with KDE 2.0.  the HoverLink setting
    // has precedence over the UnderlineLinks setting
    if (READ_BOOL("HoverLinks", true)) {
        m_pUnderlineCombo->setCurrentIndex(UnderlineHover);
    } else {
        m_pUnderlineCombo->setCurrentIndex(READ_BOOL("UnderlineLinks", true) ? UnderlineAlways : UnderlineNever);
    }

    cssConfig->load();

    updateGUI();
    KCModule::load();
}

void KAppearanceOptions::defaults()
{
    bool old = m_pConfig->readDefaults();
    m_pConfig->setReadDefaults(true);
    load();
    m_pConfig->setReadDefaults(old);

    cssConfig->defaults();
    setNeedsSave(true);
#if QT_VERSION_MAJOR > 5
    setRepresentsDefaults(true);
#endif
}

void KAppearanceOptions::updateGUI()
{
    //qCDebug(KONQUEROR_LOG) << "KAppearanceOptions::updateGUI " << charset;
    for (int f = 0; f < 6; f++) {
        QString ff = fonts[f];
        if (ff.isEmpty()) {
            ff = defaultFonts[f];
        }
        m_pFonts[f]->setCurrentFont(ff);
    }

    int i = 0;
    QStringList::const_iterator end = encodings.constEnd();
    for (QStringList::const_iterator it = encodings.constBegin(); it != end; ++it, ++i)
        if (encodingName == *it) {
            m_pEncoding->setCurrentIndex(i);
        }
    if (encodingName.isEmpty()) {
        m_pEncoding->setCurrentIndex(0);
    }
    m_pFontSizeAdjust->setValue(fonts[6].toInt());
    m_MedSize->blockSignals(true);
    m_MedSize->setValue(fSize);
    m_MedSize->blockSignals(false);
    m_minSize->blockSignals(true);
    m_minSize->setValue(fMinSize);
    m_minSize->blockSignals(false);
}

void KAppearanceOptions::save()
{
    KConfigGroup cg(m_pConfig, m_groupname);
    cg.writeEntry("MediumFontSize", fSize);
    cg.writeEntry("MinimumFontSize", fMinSize);
    cg.writeEntry("Fonts", fonts);

    //TODO move to behaviour
    // If the user chose "Use language encoding", write an empty string
    if (encodingName == i18n("Use Language Encoding")) {
        encodingName = QLatin1String("");
    }
    cg.writeEntry("DefaultEncoding", encodingName);

    //Images
    cg.writeEntry("AutoLoadImages", m_pAutoLoadImagesCheckBox->isChecked());
    cg.writeEntry("UnfinishedImageFrame", m_pUnfinishedImageFrameCheckBox->isChecked());
    cg.writeEntry("ShowAnimations", animationValues[m_pAnimationsCombo->currentIndex()]);
    cg.writeEntry("UnderlineLinks", m_pUnderlineCombo->currentIndex() == UnderlineAlways);
    cg.writeEntry("HoverLinks", m_pUnderlineCombo->currentIndex() == UnderlineHover);
    cg.writeEntry("SmoothScrolling", smoothScrollingValues[m_pSmoothScrollingCombo->currentIndex()]);

    cssConfig->save();

    cg.sync();
    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);

    KCModule::save();
}

