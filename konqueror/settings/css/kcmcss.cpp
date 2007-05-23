
// Own
#include "kcmcss.h"

// Qt
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QLayout>
#include <QtGui/QRadioButton>
#include <Qt3Support/Q3TextBrowser>
#include <QtGui/QBoxLayout>

// KDE
#include <kapplication.h>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfontdialog.h>
#include <kgenericfactory.h>
#include <kglobalsettings.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

// Local
#include "template.h"

typedef KGenericFactory<CSSConfig, QWidget> CSSFactory;
K_EXPORT_COMPONENT_FACTORY( css, CSSFactory("kcmcss") )

CSSConfig::CSSConfig(QWidget *parent, const QStringList &)
  : KCModule(CSSFactory::componentData(), parent)
{
  customDialogBase = new KDialog(this);
  customDialogBase->setObjectName( "customCSSDialog" );
  customDialogBase->setModal( true );
  customDialogBase->setButtons( KDialog::Close );
  customDialogBase->setDefaultButton( KDialog::Close );
  customDialogBase->showButtonSeparator( true );
  customDialog = new CSSCustomDialog(customDialogBase);
  customDialogBase->setMainWidget(customDialog);
  configDialog = new CSSConfigDialog(this);

  setQuickHelp( i18n("<h1>Konqueror Stylesheets</h1> This module allows you to apply your own color"
              " and font settings to Konqueror by using"
              " stylesheets (CSS). You can either specify"
              " options or apply your own self-written"
              " stylesheet by pointing to its location.<br>"
              " Note that these settings will always have"
              " precedence before all other settings made"
              " by the site author. This can be useful to"
              " visually impaired people or for web pages"
              " that are unreadable due to bad design."));


  QStringList fonts;
  KFontChooser::getFontList(fonts, 0);
  customDialog->fontFamily->addItems(fonts);

  connect(configDialog->useDefault, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(configDialog->useAccess, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(configDialog->useUser, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(configDialog->urlRequester, SIGNAL(textChanged(const QString&)),
	  SLOT(changed()));
  connect(configDialog->customize, SIGNAL(clicked()),
          SLOT(slotCustomize()));
  connect(customDialog->basefontsize, SIGNAL(highlighted(int)),
	  SLOT(changed()));
  connect(customDialog->basefontsize, SIGNAL(textChanged(const QString&)),
	  SLOT(changed()));
  connect(customDialog->dontScale, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->blackOnWhite, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->whiteOnBlack, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->customColor, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->foregroundColorButton, SIGNAL(changed(const QColor &)),
	  SLOT(changed()));
  connect(customDialog->backgroundColorButton, SIGNAL(changed(const QColor &)),
	  SLOT(changed()));
  connect(customDialog->fontFamily, SIGNAL(highlighted(int)),
	  SLOT(changed()));
  connect(customDialog->fontFamily, SIGNAL(textChanged(const QString&)),
	  SLOT(changed()));
  connect(customDialog->sameFamily, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->preview, SIGNAL(clicked()),
          SLOT(slotPreview()));
  connect(customDialog->sameColor, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->hideImages, SIGNAL(clicked()),
	  SLOT(changed()));
  connect(customDialog->hideBackground, SIGNAL(clicked()),
	  SLOT(changed()));

  QVBoxLayout *vbox = new QVBoxLayout(this);
  vbox->setMargin(0);
  vbox->setSpacing(0);
  vbox->addWidget(configDialog);

  load();
}


void CSSConfig::load()
{
  KConfig *c = new KConfig("kcmcssrc", KConfig::NoGlobals);
  KConfigGroup group = c->group("Stylesheet");
  QString u = group.readEntry("Use", "default");
  configDialog->useDefault->setChecked(u == "default");
  configDialog->useUser->setChecked(u == "user");
  configDialog->useAccess->setChecked(u == "access");
  configDialog->urlRequester->setUrl(group.readEntry("SheetName"));

  group = c->group("Font");
  customDialog->basefontsize->setEditText(QString::number(group.readEntry("BaseSize", 12)));
  customDialog->dontScale->setChecked(group.readEntry("DontScale", false));

  QString fname = group.readEntry("Family", "Arial");
  for (int i=0; i < customDialog->fontFamily->count(); ++i)
    if (customDialog->fontFamily->itemText(i) == fname)
      {
	customDialog->fontFamily->setCurrentIndex(i);
	break;
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

  delete c;
}


void CSSConfig::save()
{
  // write to config file
  KConfig *c = new KConfig("kcmcssrc", KConfig::NoGlobals);
  KConfigGroup group = c->group("Stylesheet");
  if (configDialog->useDefault->isChecked())
    group.writeEntry("Use", "default");
  if (configDialog->useUser->isChecked())
    group.writeEntry("Use", "user");
  if (configDialog->useAccess->isChecked())
    group.writeEntry("Use", "access");
  group.writeEntry("SheetName", configDialog->urlRequester->url().url());

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
  QString templ = KStandardDirs::locate("data", "kcmcss/template.css");
  QString dest;
  if (!templ.isEmpty())
    {
      CSSTemplate css(templ);

      dest = KGlobal::mainComponent().dirs()->saveLocation("data", "kcmcss");
      dest += "/override.css";

      css.expand(dest, cssDict());
    }

  // make konqueror use the right stylesheet
  c = new KConfig("konquerorrc", KConfig::NoGlobals);
  group = c->group("HTML Settings");
  group.writeEntry("UserStyleSheetEnabled", !configDialog->useDefault->isChecked());

  if (configDialog->useUser->isChecked())
    group.writeEntry("UserStyleSheet", configDialog->urlRequester->url().url());
  if (configDialog->useAccess->isChecked())
    group.writeEntry("UserStyleSheet", dest);

  c->sync();
  delete c;
  emit changed(false);
}


void CSSConfig::defaults()
{
  configDialog->useDefault->setChecked(true);
  configDialog->useUser->setChecked(false);
  configDialog->useAccess->setChecked(false);
  configDialog->urlRequester->setUrl(KUrl());

  customDialog->basefontsize->setEditText(QString::number(12));
  customDialog->dontScale->setChecked(false);

  QString fname =  "Arial";
  for (int i=0; i < customDialog->fontFamily->count(); ++i)
    if (customDialog->fontFamily->itemText(i) == fname)
      {
	customDialog->fontFamily->setCurrentIndex(i);
	break;
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
  emit changed(true);
}


QString px(int i, double scale)
{
  QString px;
  px.setNum(static_cast<int>(i * scale));
  px += "px";
  return px;
}


QMap<QString,QString> CSSConfig::cssDict()
{
  QMap<QString,QString> dict;

  // Fontsizes ------------------------------------------------------

  int bfs = customDialog->basefontsize->currentText().toInt();
  dict.insert("fontsize-base", px(bfs, 1.0));

  if (customDialog->dontScale->isChecked())
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

  if (customDialog->blackOnWhite->isChecked())
    {
      dict.insert("background-color", "White");
      dict.insert("foreground-color", "Black");
    }
  else if (customDialog->whiteOnBlack->isChecked())
    {
      dict.insert("background-color", "Black");
      dict.insert("foreground-color", "White");
    }
  else
    {
      dict.insert("background-color", customDialog->backgroundColorButton->color().name());
      dict.insert("foreground-color", customDialog->foregroundColorButton->color().name());
    }

  if (customDialog->sameColor->isChecked())
    dict.insert("force-color", "! important");
  else
    dict.insert("force-color", "");

  // Fonts -------------------------------------------------------------
  dict.insert("font-family", customDialog->fontFamily->currentText());
  if (customDialog->sameFamily->isChecked())
    dict.insert("force-font", "! important");
  else
    dict.insert("force-font", "");

  // Images

  if (customDialog->hideImages->isChecked())
    dict.insert("display-images", "background-image : none ! important");
  else
    dict.insert("display-images", "");
  if (customDialog->hideBackground->isChecked())
    dict.insert("display-background", "background-image : none ! important");
  else
    dict.insert("display-background", "");

  return dict;
}


void CSSConfig::slotCustomize()
{
  customDialogBase->exec();
}


void CSSConfig::slotPreview()
{

  Q3StyleSheetItem *h1 = new Q3StyleSheetItem(Q3StyleSheet::defaultSheet(), "h1");
  Q3StyleSheetItem *h2 = new Q3StyleSheetItem(Q3StyleSheet::defaultSheet(), "h2");
  Q3StyleSheetItem *h3 = new Q3StyleSheetItem(Q3StyleSheet::defaultSheet(), "h3");
  Q3StyleSheetItem *text = new Q3StyleSheetItem(Q3StyleSheet::defaultSheet(), "p");

  // Fontsize

  int bfs = customDialog->basefontsize->currentText().toInt();
  text->setFontSize(bfs);
  if (customDialog->dontScale->isChecked())
    {
      h1->setFontSize(bfs);
      h2->setFontSize(bfs);
      h3->setFontSize(bfs);
    }
  else
    {
      h1->setFontSize(static_cast<int>(bfs * 1.8));
      h2->setFontSize(static_cast<int>(bfs * 1.6));
      h3->setFontSize(static_cast<int>(bfs * 1.4));
    }

  // Colors

  QColor back, fore;

  if (customDialog->blackOnWhite->isChecked())
    {
      back = Qt::white;
      fore = Qt::black;
    }
  else if (customDialog->whiteOnBlack->isChecked())
    {
      back = Qt::black;
      fore = Qt::white;
    }
  else
    {
      back = customDialog->backgroundColorButton->color();
      fore = customDialog->foregroundColorButton->color();
    }

  h1->setColor(fore);
  h2->setColor(fore);
  h3->setColor(fore);
  text->setColor(fore);

  // Fonts

  h1->setFontFamily(customDialog->fontFamily->currentText());
  h2->setFontFamily(customDialog->fontFamily->currentText());
  h3->setFontFamily(customDialog->fontFamily->currentText());
  text->setFontFamily(customDialog->fontFamily->currentText());

  // Show the preview
  PreviewDialog *dlg = new PreviewDialog(this);
  dlg->preview->setPaper(back);
  dlg->preview->viewport()->setFont(QFont(KGlobalSettings::generalFont().family(), bfs));

  dlg->exec();

  delete dlg;
}




#include "kcmcss.moc"

