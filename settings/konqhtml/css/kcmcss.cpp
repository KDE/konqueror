
// Own
#include "kcmcss.h"

// Qt
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QUrl>
#include <QStandardPaths>

// KDE
#include <KColorButton>
#include <KConfig>
#include <KConfigGroup>
#include <KUrlRequester>
#include <KParts/Part>
#include <KParts/OpenUrlArguments>
#include <KParts/PartLoader>

// Local
#include "template.h"

#include "ui_cssconfig.h"

class CSSConfigWidget: public QWidget, public Ui::CSSConfigWidget
{
public:
    CSSConfigWidget(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
    }
};

CSSConfig::CSSConfig(QWidget *parent, const QVariantList &)
    : QWidget(parent)
    , configWidget(new CSSConfigWidget(this))
    , customDialogBase(new QDialog(this))
    , customDialog(new CSSCustomDialog(customDialogBase))
{
    customDialogBase->setObjectName(QStringLiteral("customCSSDialog"));
    customDialogBase->setModal(true);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, customDialogBase);
    buttonBox->button(QDialogButtonBox::Close)->setDefault(true);
    connect(buttonBox, &QDialogButtonBox::rejected, customDialogBase, &QDialog::reject);

    QVBoxLayout *vLayout = new QVBoxLayout(customDialogBase);
    vLayout->addWidget(customDialog);
    vLayout->addStretch(1);
    vLayout->addWidget(buttonBox);

    setToolTip(i18n("<h1>Konqueror Stylesheets</h1> This module allows you to apply your own color"
                      " and font settings to Konqueror by using"
                      " stylesheets (CSS). You can either specify"
                      " options or apply your own self-written"
                      " stylesheet by pointing to its location.<br />"
                      " Note that these settings will always have"
                      " precedence before all other settings made"
                      " by the site author. This can be useful to"
                      " visually impaired people or for web pages"
                      " that are unreadable due to bad design."));

    connect(configWidget->useDefault,    &QAbstractButton::clicked,    this, &CSSConfig::changed);
    connect(configWidget->useAccess,     &QAbstractButton::clicked,    this, &CSSConfig::changed);
    connect(configWidget->useUser,       &QAbstractButton::clicked,    this, &CSSConfig::changed);
    connect(configWidget->urlRequester,  &KUrlRequester::textChanged,  this, &CSSConfig::changed);
    connect(configWidget->customize,     &QAbstractButton::clicked,    this, &CSSConfig::slotCustomize);
    connect(customDialog,                &CSSCustomDialog::changed,    this, &CSSConfig::changed);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(configWidget);
}

void CSSConfig::load()
{
    QSignalBlocker block(customDialog);

    KConfig *c = new KConfig(QStringLiteral("kcmcssrc"), KConfig::NoGlobals);
    KConfigGroup group = c->group("Stylesheet");
    QString u = group.readEntry("Use", "default");
    configWidget->useDefault->setChecked(u == QLatin1String("default"));
    configWidget->useUser->setChecked(u == QLatin1String("user"));
    configWidget->useAccess->setChecked(u == QLatin1String("access"));
    configWidget->urlRequester->setUrl(QUrl::fromUserInput(group.readEntry("SheetName")));

    group = c->group("Font");
    customDialog->basefontsize->setEditText(QString::number(group.readEntry("BaseSize", 12)));
    customDialog->dontScale->setChecked(group.readEntry("DontScale", false));

    const QString fname(group.readEntry("Family", "Arial"));
    for (int i = 0; i < customDialog->fontFamily->count(); ++i) {
        if (customDialog->fontFamily->itemText(i) == fname) {
            customDialog->fontFamily->setCurrentIndex(i);
            break;
        }
    }

    customDialog->sameFamily->setChecked(group.readEntry("SameFamily", false));

    group = c->group("Colors");
    QString m = group.readEntry("Mode", "black-on-white");
    customDialog->blackOnWhite->setChecked(m == QLatin1String("black-on-white"));
    customDialog->whiteOnBlack->setChecked(m == QLatin1String("white-on-black"));
    customDialog->customColor->setChecked(m == QLatin1String("custom"));

    QColor white(Qt::white);
    QColor black(Qt::black);
    customDialog->backgroundColorButton->setColor(group.readEntry("BackColor", white));
    customDialog->foregroundColorButton->setColor(group.readEntry("ForeColor", black));
    customDialog->sameColor->setChecked(group.readEntry("SameColor", false));

    // Images
    group = c->group("Images");
    customDialog->hideImages->setChecked(group.readEntry("Hide", false));
    customDialog->hideBackground->setChecked(group.readEntry("HideBackground", true));

    delete c;
}

void CSSConfig::save()
{
    // write to config file
    KConfig *c = new KConfig(QStringLiteral("kcmcssrc"), KConfig::NoGlobals);
    KConfigGroup group = c->group("Stylesheet");
    if (configWidget->useDefault->isChecked()) {
        group.writeEntry("Use", "default");
    }
    if (configWidget->useUser->isChecked()) {
        group.writeEntry("Use", "user");
    }
    if (configWidget->useAccess->isChecked()) {
        group.writeEntry("Use", "access");
    }
    group.writeEntry("SheetName", configWidget->urlRequester->url().url());

    group = c->group("Font");
    group.writeEntry("BaseSize", customDialog->basefontsize->currentText());
    group.writeEntry("DontScale", customDialog->dontScale->isChecked());
    group.writeEntry("SameFamily", customDialog->sameFamily->isChecked());
    group.writeEntry("Family", customDialog->fontFamily->currentText());

    group = c->group("Colors");
    if (customDialog->blackOnWhite->isChecked()) {
        group.writeEntry("Mode", "black-on-white");
    }
    if (customDialog->whiteOnBlack->isChecked()) {
        group.writeEntry("Mode", "white-on-black");
    }
    if (customDialog->customColor->isChecked()) {
        group.writeEntry("Mode", "custom");
    }
    group.writeEntry("BackColor", customDialog->backgroundColorButton->color());
    group.writeEntry("ForeColor", customDialog->foregroundColorButton->color());
    group.writeEntry("SameColor", customDialog->sameColor->isChecked());

    group = c->group("Images");
    group.writeEntry("Hide", customDialog->hideImages->isChecked());
    group.writeEntry("HideBackground", customDialog->hideBackground->isChecked());

    c->sync();
    delete c;

    // generate CSS template
    QString dest;
    const QString templ(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcmcss/template.css")));
    if (!templ.isEmpty()) {
        CSSTemplate css(templ);
        dest = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kcmcss/";
        QDir().mkpath(dest);
        dest += QLatin1String("override.css");
        css.expandToFile(dest, customDialog->cssDict());
    }

    // make konqueror use the right stylesheet
    c = new KConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);
    group = c->group("HTML Settings");
    group.writeEntry("UserStyleSheetEnabled", !configWidget->useDefault->isChecked());

    if (configWidget->useUser->isChecked()) {
        group.writeEntry("UserStyleSheet", configWidget->urlRequester->url().url());
    }
    if (configWidget->useAccess->isChecked()) {
        group.writeEntry("UserStyleSheet", dest);
    }

    c->sync();
    delete c;
}

void CSSConfig::defaults()
{
    configWidget->useDefault->setChecked(true);
    configWidget->useUser->setChecked(false);
    configWidget->useAccess->setChecked(false);
    configWidget->urlRequester->setUrl(QUrl());

    customDialog->basefontsize->setEditText(QString::number(12));
    customDialog->dontScale->setChecked(false);

    const QString fname(QStringLiteral("Arial"));
    for (int i = 0; i < customDialog->fontFamily->count(); ++i) {
        if (customDialog->fontFamily->itemText(i) == fname) {
            customDialog->fontFamily->setCurrentIndex(i);
            break;
        }
    }

    customDialog->sameFamily->setChecked(false);
    customDialog->blackOnWhite->setChecked(true);
    customDialog->whiteOnBlack->setChecked(false);
    customDialog->customColor->setChecked(false);
    customDialog->backgroundColorButton->setColor(Qt::white);
    customDialog->foregroundColorButton->setColor(Qt::black);
    customDialog->sameColor->setChecked(false);

    customDialog->hideImages->setChecked(false);
    customDialog->hideBackground->setChecked(true);
}

static QString px(int i, double scale)
{
    QString px;
    px.setNum(static_cast<int>(i * scale));
    px += QLatin1String("px");
    return px;
}

QMap<QString, QString> CSSCustomDialog::cssDict()
{
    QMap<QString, QString> dict;

    // Fontsizes ------------------------------------------------------

    int bfs = basefontsize->currentText().toInt();
    dict.insert(QStringLiteral("fontsize-base"), px(bfs, 1.0));

    if (dontScale->isChecked()) {
        dict.insert(QStringLiteral("fontsize-small-1"), px(bfs, 1.0));
        dict.insert(QStringLiteral("fontsize-large-1"), px(bfs, 1.0));
        dict.insert(QStringLiteral("fontsize-large-2"), px(bfs, 1.0));
        dict.insert(QStringLiteral("fontsize-large-3"), px(bfs, 1.0));
        dict.insert(QStringLiteral("fontsize-large-4"), px(bfs, 1.0));
        dict.insert(QStringLiteral("fontsize-large-5"), px(bfs, 1.0));
    } else {
        // TODO: use something harmonic here
        dict.insert(QStringLiteral("fontsize-small-1"), px(bfs, 0.8));
        dict.insert(QStringLiteral("fontsize-large-1"), px(bfs, 1.2));
        dict.insert(QStringLiteral("fontsize-large-2"), px(bfs, 1.4));
        dict.insert(QStringLiteral("fontsize-large-3"), px(bfs, 1.5));
        dict.insert(QStringLiteral("fontsize-large-4"), px(bfs, 1.6));
        dict.insert(QStringLiteral("fontsize-large-5"), px(bfs, 1.8));
    }

    // Colors --------------------------------------------------------

    if (customColor->isChecked()) {
        dict.insert(QStringLiteral("background-color"), backgroundColorButton->color().name());
        dict.insert(QStringLiteral("foreground-color"), foregroundColorButton->color().name());
    } else {
        const char *blackOnWhiteFG[2] = {"White", "Black"};
        bool bw = blackOnWhite->isChecked();
        dict.insert(QStringLiteral("foreground-color"), QLatin1String(blackOnWhiteFG[bw]));
        dict.insert(QStringLiteral("background-color"), QLatin1String(blackOnWhiteFG[!bw]));
    }

    const char *notImportant[2] = {"", "! important"};
    dict.insert(QStringLiteral("force-color"), QLatin1String(notImportant[sameColor->isChecked()]));

    // Fonts -------------------------------------------------------------
    dict.insert(QStringLiteral("font-family"), fontFamily->currentText());
    dict.insert(QStringLiteral("force-font"), QLatin1String(notImportant[sameFamily->isChecked()]));

    // Images

    const char *bgNoneImportant[2] = {"", "background-image : none ! important"};
    dict.insert(QStringLiteral("display-images"), QLatin1String(bgNoneImportant[hideImages->isChecked()]));
    dict.insert(QStringLiteral("display-background"), QLatin1String(bgNoneImportant[hideBackground->isChecked()]));

    return dict;
}

void CSSConfig::slotCustomize()
{
    customDialog->slotPreview();
    customDialogBase->exec();
}

CSSCustomDialog::CSSCustomDialog(QWidget *parent)
    : QWidget(parent)
{
    setupUi(this);
    connect(this,                   &CSSCustomDialog::changed,                  this, &CSSCustomDialog::slotPreview);

    connect(basefontsize,           QOverload<int>::of(&QComboBox::activated),  this, &CSSCustomDialog::changed);
    connect(basefontsize,           &QComboBox::editTextChanged,                this, &CSSCustomDialog::changed);
    connect(dontScale,              &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(blackOnWhite,           &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(whiteOnBlack,           &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(customColor,            &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(foregroundColorButton,  &KColorButton::changed,                     this, &CSSCustomDialog::changed);
    connect(backgroundColorButton,  &KColorButton::changed,                     this, &CSSCustomDialog::changed);
    connect(fontFamily,             QOverload<int>::of(&QComboBox::activated),  this, &CSSCustomDialog::changed);
    connect(fontFamily,             &QComboBox::editTextChanged,                this, &CSSCustomDialog::changed);
    connect(sameFamily,             &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(sameColor,              &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(hideImages,             &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);
    connect(hideBackground,         &QAbstractButton::clicked,                  this, &CSSCustomDialog::changed);

    //QStringList fonts;
    //KFontChooser::getFontList(fonts, 0);
    //fontFamily->addItems(fonts);
    part = KParts::PartLoader::createPartInstanceForMimeType<KParts::ReadOnlyPart>(QStringLiteral("text/html"), nullptr, this);
    QVBoxLayout *l = new QVBoxLayout(previewBox);
    l->addWidget(part->widget());
}

static QUrl toDataUri(const QString &content, const QByteArray &contentType)
{
    QByteArray data("data:");
    data += contentType;
    data += ";charset=utf-8;base64,";
    data += content.toUtf8().toBase64();
    return QUrl::fromEncoded(data);
}

void CSSCustomDialog::slotPreview()
{
    const QString templ(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kcmcss/template.css")));

    if (templ.isEmpty()) {
        return;
    }

    CSSTemplate css(templ);

    QString data(i18n("<html>\n<head>\n<style>\n<!--\n"
                      "%1"
                      "\n-->\n</style>\n</head>\n"
                      "<body>\n"
                      "<h1>Heading 1</h1>\n"
                      "<h2>Heading 2</h2>\n"
                      "<h3>Heading 3</h3>\n"
                      "\n"
                      "<p>User-defined stylesheets allow increased\n"
                      "accessibility for visually handicapped\n"
                      "people.</p>\n"
                      "\n"
                      "</body>\n"
                      "</html>\n", css.expandToString(cssDict())));

    KParts::OpenUrlArguments args(part->arguments());
    args.setReload(true); // Make sure the content is always freshly reloaded.
    part->setArguments(args);
    part->openUrl(toDataUri(data, "text/html"));
}

