
// Own
#include "kcmcss.h"

// Qt
#include <QCheckBox>
#include <QComboBox>
#include <QLayout>
#include <QRadioButton>
#include <QTextBrowser>
#include <QBoxLayout>

// KDE
#include <kapplication.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfontdialog.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kmimetypetrader.h>
#include <kdebug.h>
#include <kparts/part.h>

// Local
#include "template.h"

#include "ui_cssconfig.h"



class CSSConfigWidget: public QWidget, public Ui::CSSConfigWidget
{
public:
  CSSConfigWidget( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

CSSConfig::CSSConfig(QWidget *parent, const QVariantList &)
  : QWidget(parent)
  , configWidget(new CSSConfigWidget(this))
  , customDialogBase(new KDialog(this))
  , customDialog(new CSSCustomDialog(customDialogBase))
{
  customDialogBase->setObjectName( QLatin1String( "customCSSDialog" ) );
  customDialogBase->setModal( true );
  customDialogBase->setButtons( KDialog::Close );
  customDialogBase->setDefaultButton( KDialog::Close );

  customDialogBase->setMainWidget(customDialog);

//   setQuickHelp( i18n("<h1>Konqueror Stylesheets</h1> This module allows you to apply your own color"
  setWhatsThis( i18n("<h1>Konqueror Stylesheets</h1> This module allows you to apply your own color"
              " and font settings to Konqueror by using"
              " stylesheets (CSS). You can either specify"
              " options or apply your own self-written"
              " stylesheet by pointing to its location.<br />"
              " Note that these settings will always have"
              " precedence before all other settings made"
              " by the site author. This can be useful to"
              " visually impaired people or for web pages"
              " that are unreadable due to bad design."));

  connect(configWidget->useDefault,     SIGNAL(clicked()),      SIGNAL(changed()));
  connect(configWidget->useAccess,      SIGNAL(clicked()),      SIGNAL(changed()));
  connect(configWidget->useUser,        SIGNAL(clicked()),      SIGNAL(changed()));
  connect(configWidget->urlRequester, SIGNAL(textChanged(QString)),SIGNAL(changed()));
  connect(configWidget->customize,      SIGNAL(clicked()),      SLOT(slotCustomize()));
  connect(customDialog,                 SIGNAL(changed()),      SIGNAL(changed()));

  QVBoxLayout *vbox = new QVBoxLayout(this);
  vbox->setMargin(0);
  vbox->addWidget(configWidget);
}


void CSSConfig::load()
{
  const bool signalsBlocked = customDialog->blockSignals(true);

  KConfig *c = new KConfig("kcmcssrc", KConfig::NoGlobals);
  KConfigGroup group = c->group("Stylesheet");
  QString u = group.readEntry("Use", "default");
  configWidget->useDefault->setChecked(u == "default");
  configWidget->useUser->setChecked(u == "user");
  configWidget->useAccess->setChecked(u == "access");
  configWidget->urlRequester->setUrl(group.readEntry("SheetName"));

  group = c->group("Font");
  customDialog->basefontsize->setEditText(QString::number(group.readEntry("BaseSize", 12)));
  customDialog->dontScale->setChecked(group.readEntry("DontScale", false));

  const QString fname (group.readEntry("Family", "Arial"));
  for (int i=0; i < customDialog->fontFamily->count(); ++i) {
    if (customDialog->fontFamily->itemText(i) == fname) {
      customDialog->fontFamily->setCurrentIndex(i);
      break;
    }
  }

  customDialog->sameFamily->setChecked(group.readEntry("SameFamily", false));

  group = c->group("Colors");
  QString m = group.readEntry("Mode", "black-on-white");
  customDialog->blackOnWhite->setChecked(m == "black-on-white");
  customDialog->whiteOnBlack->setChecked(m == "white-on-black");
  customDialog->customColor->setChecked(m == "custom");

  QColor white (Qt::white);
  QColor black (Qt::black);
  customDialog->backgroundColorButton->setColor(group.readEntry("BackColor", white));
  customDialog->foregroundColorButton->setColor(group.readEntry("ForeColor", black));
  customDialog->sameColor->setChecked(group.readEntry("SameColor", false));

  // Images
  group = c->group("Images");
  customDialog->hideImages->setChecked(group.readEntry("Hide", false));
  customDialog->hideBackground->setChecked(group.readEntry("HideBackground", true));

  customDialog->blockSignals(signalsBlocked);
  delete c;
}


void CSSConfig::save()
{
  // write to config file
  KConfig *c = new KConfig("kcmcssrc", KConfig::NoGlobals);
  KConfigGroup group = c->group("Stylesheet");
  if (configWidget->useDefault->isChecked())
    group.writeEntry("Use", "default");
  if (configWidget->useUser->isChecked())
    group.writeEntry("Use", "user");
  if (configWidget->useAccess->isChecked())
    group.writeEntry("Use", "access");
  group.writeEntry("SheetName", configWidget->urlRequester->url().url());

  group = c->group("Font");
  group.writeEntry("BaseSize", customDialog->basefontsize->currentText());
  group.writeEntry("DontScale", customDialog->dontScale->isChecked());
  group.writeEntry("SameFamily", customDialog->sameFamily->isChecked());
  group.writeEntry("Family", customDialog->fontFamily->currentText());

  group = c->group("Colors");
  if (customDialog->blackOnWhite->isChecked())
    group.writeEntry("Mode", "black-on-white");
  if (customDialog->whiteOnBlack->isChecked())
    group.writeEntry("Mode", "white-on-black");
  if (customDialog->customColor->isChecked())
    group.writeEntry("Mode", "custom");
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
  const QString templ(KStandardDirs::locate("data", "kcmcss/template.css"));
  if (!templ.isEmpty()) {
    CSSTemplate css(templ);
    dest = KGlobal::mainComponent().dirs()->saveLocation("data", "kcmcss");
    dest += "override.css";
    css.expandToFile(dest, customDialog->cssDict());
  }

  // make konqueror use the right stylesheet
  c = new KConfig("konquerorrc", KConfig::NoGlobals);
  group = c->group("HTML Settings");
  group.writeEntry("UserStyleSheetEnabled", !configWidget->useDefault->isChecked());

  if (configWidget->useUser->isChecked())
    group.writeEntry("UserStyleSheet", configWidget->urlRequester->url().url());
  if (configWidget->useAccess->isChecked())
    group.writeEntry("UserStyleSheet", dest);

  c->sync();
  delete c;
}


void CSSConfig::defaults()
{
  configWidget->useDefault->setChecked(true);
  configWidget->useUser->setChecked(false);
  configWidget->useAccess->setChecked(false);
  configWidget->urlRequester->setUrl(KUrl());

  customDialog->basefontsize->setEditText(QString::number(12));
  customDialog->dontScale->setChecked(false);

  const QString fname (QLatin1String("Arial"));
  for (int i=0; i < customDialog->fontFamily->count(); ++i) {
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
  customDialog->hideBackground->setChecked( true);
}


static QString px(int i, double scale)
{
  QString px;
  px.setNum(static_cast<int>(i * scale));
  px += "px";
  return px;
}


QMap<QString,QString> CSSCustomDialog::cssDict()
{
  QMap<QString,QString> dict;

  // Fontsizes ------------------------------------------------------

  int bfs = basefontsize->currentText().toInt();
  dict.insert("fontsize-base", px(bfs, 1.0));

  if (dontScale->isChecked())
  {
    dict.insert("fontsize-small-1", px(bfs, 1.0));
    dict.insert("fontsize-large-1", px(bfs, 1.0));
    dict.insert("fontsize-large-2", px(bfs, 1.0));
    dict.insert("fontsize-large-3", px(bfs, 1.0));
    dict.insert("fontsize-large-4", px(bfs, 1.0));
    dict.insert("fontsize-large-5", px(bfs, 1.0));
  }
  else
  {
    // TODO: use something harmonic here
    dict.insert("fontsize-small-1", px(bfs, 0.8));
    dict.insert("fontsize-large-1", px(bfs, 1.2));
    dict.insert("fontsize-large-2", px(bfs, 1.4));
    dict.insert("fontsize-large-3", px(bfs, 1.5));
    dict.insert("fontsize-large-4", px(bfs, 1.6));
    dict.insert("fontsize-large-5", px(bfs, 1.8));
  }

  // Colors --------------------------------------------------------

  if (customColor->isChecked())
  {
    dict.insert("background-color", backgroundColorButton->color().name());
    dict.insert("foreground-color", foregroundColorButton->color().name());
  }
  else
  {
    const char* blackOnWhiteFG[2]={"White","Black"};
    bool bw=blackOnWhite->isChecked();
    dict.insert("foreground-color", QLatin1String(blackOnWhiteFG[bw]));
    dict.insert("background-color", QLatin1String(blackOnWhiteFG[!bw]));
  }

  const char* notImportant[2]={"","! important"};
  dict.insert("force-color", QLatin1String(notImportant[sameColor->isChecked()]));

  // Fonts -------------------------------------------------------------
  dict.insert("font-family", fontFamily->currentText());
  dict.insert("force-font", QLatin1String(notImportant[sameFamily->isChecked()]));

  // Images

  const char* bgNoneImportant[2]={"","background-image : none ! important"};
  dict.insert("display-images", QLatin1String(bgNoneImportant[hideImages->isChecked()]));
  dict.insert("display-background", QLatin1String(bgNoneImportant[hideBackground->isChecked()]));

  return dict;
}


void CSSConfig::slotCustomize()
{
  customDialog->slotPreview();
  customDialogBase->exec();
}

CSSCustomDialog::CSSCustomDialog( QWidget *parent )
  : QWidget( parent )
{
  setupUi( this );
  connect(this,SIGNAL(changed()),SLOT(slotPreview()));

  connect(basefontsize, SIGNAL(activated(int)), SIGNAL(changed()));
  connect(basefontsize, SIGNAL(editTextChanged(QString)),SIGNAL(changed()));
  connect(dontScale,      SIGNAL(clicked()),      SIGNAL(changed()));
  connect(blackOnWhite,   SIGNAL(clicked()),      SIGNAL(changed()));
  connect(whiteOnBlack,   SIGNAL(clicked()),      SIGNAL(changed()));
  connect(customColor,    SIGNAL(clicked()),      SIGNAL(changed()));
  connect(foregroundColorButton, SIGNAL(changed(QColor)),SIGNAL(changed()));
  connect(backgroundColorButton, SIGNAL(changed(QColor)),SIGNAL(changed()));
  connect(fontFamily, SIGNAL(activated(int)),   SIGNAL(changed()));
  connect(fontFamily, SIGNAL(editTextChanged(QString)),SIGNAL(changed()));
  connect(sameFamily,     SIGNAL(clicked()),      SIGNAL(changed()));
  connect(sameColor,      SIGNAL(clicked()),      SIGNAL(changed()));
  connect(hideImages,     SIGNAL(clicked()),      SIGNAL(changed()));
  connect(hideBackground, SIGNAL(clicked()),      SIGNAL(changed()));

  //QStringList fonts;
  //KFontChooser::getFontList(fonts, 0);
  //fontFamily->addItems(fonts);
  part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>(QLatin1String("text/html"), parent, this);
  QVBoxLayout* l= new QVBoxLayout(previewBox);
  l->addWidget(part->widget());
}


static QUrl toDataUri(const QString& content, const QByteArray& contentType)
{
  QByteArray data ("data:");
  data += contentType;
  data += ";charset=utf-8;base64,";
  data += content.toUtf8().toBase64();
  return QUrl::fromEncoded(data);
 }

void CSSCustomDialog::slotPreview()
{
  const QString templ (KStandardDirs::locate("data", "kcmcss/template.css"));

  if (templ.isEmpty())
    return;

  CSSTemplate css(templ);

  QString data (i18n("<html>\n<head>\n<style>\n<!--\n"
               "%1"
               "\n-->\n</style>\n</head>\n"
               "<body>\n"
               "<h1>Heading 1</h1>\n"
               "<h2>Heading 2</h2>\n"
               "<h3>Heading 3</h3>\n"
               "\n"
               "<p>User defined stylesheets allow increased\n"
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

#include "kcmcss.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
