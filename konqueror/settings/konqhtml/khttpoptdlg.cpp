// File khttpoptdlg.cpp by Jacek Konieczny <jajcus@zeus.posl.gliwice.pl>
// Port to KControl by David Faure <faure@kde.org>

#include <QLayout> //CT

#include <klocale.h>
#include <kglobal.h>
#include "khttpoptdlg.h"


KHTTPOptions::KHTTPOptions(KSharedConfig::Ptr config, QString group, const KComponentData &componentData, QWidget *parent)
  : KCModule( componentData, parent ), m_pConfig(config), m_groupname(group)
{
  QVBoxLayout *lay = new QVBoxLayout(this);
  lay->setMargin(10);
  lay->setSpacing(5);

  lay->addWidget( new QLabel(i18n("Accept languages:"), this) );

  le_languages = new QLineEdit(this);
  lay->addWidget( le_languages );
  connect(le_languages, SIGNAL(textChanged(const QString&)),
	  this, SLOT(slotChanged()));

  lay->addSpacing(10);
  lay->addWidget( new QLabel(i18n("Accept character sets:"), this) );

  le_charsets = new QLineEdit(this);
  lay->addWidget( le_charsets );
  connect(le_charsets, SIGNAL(textChanged(const QString&)),
	  this, SLOT(slotChanged()));

  lay->addStretch(10);

  // defaultCharsets = QString("utf-8 ")+klocale->charset()+" iso-8859-1";
  defaultCharsets = QString("utf-8 ")+" iso-8859-1"; // TODO
  // if lang=ru or uk then +cp1251+koi8u - these are most used encodings (though locale may/should be utf8) --nick shaforostoff

  // finaly read the options
  load();
}


void KHTTPOptions::load()
{
  QString tmp;
  KConfigGroup cg(m_pConfig, "Browser Settings/HTTP");
  tmp = cg.readEntry( "AcceptLanguages",KGlobal::locale()->languageList().join(","));
  le_languages->setText( tmp );
  tmp = cg.readEntry( "AcceptCharsets",defaultCharsets);
  le_charsets->setText( tmp );
}

void KHTTPOptions::save()
{
  KConfigGroup cg(m_pConfig, "Browser Settings/HTTP");
  cg.writeEntry( "AcceptLanguages", le_languages->text());
  cg.writeEntry( "AcceptCharsets", le_charsets->text());
  cg.sync();
}

void KHTTPOptions::defaults()
{
  le_languages->setText( KGlobal::locale()->languageList().join(",") );
  le_charsets->setText( defaultCharsets );
}


void KHTTPOptions::slotChanged()
{
  emit changed(true);
}


#include "khttpoptdlg.moc"
