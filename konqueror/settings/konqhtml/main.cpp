/*
 * main.cpp
 *
 * Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 * Copyright (c) 2000 Daniel Molkentin <molkentin@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Own
#include "main.h"

// std
#include <unistd.h>

// Qt
#include <QtGui/QTabWidget>
#include <QtGui/QLayout>
#include <QtDBus/QtDBus>

// KDE
#include <kapplication.h>
#include <kaboutdata.h>

// Local
#include "jsopts.h"
#include "javaopts.h"
#include "pluginopts.h"
#include "appearance.h"
#include "htmlopts.h"
#include "filteropts.h"


#include <kgenericfactory.h>
typedef KGenericFactory<KJSParts, QWidget> KJSPartsFactory;
K_EXPORT_COMPONENT_FACTORY( khtml_java_js, KJSPartsFactory("kcmkonqhtml") )

KJSParts::KJSParts(QWidget *parent, const QStringList&)
	: KCModule(KJSPartsFactory::componentData(), parent)
{
  mConfig = KSharedConfig::openConfig("konquerorrc", KConfig::NoGlobals);
  KAboutData *about =
  new KAboutData(I18N_NOOP("kcmkonqhtml"), 0, ki18n("Konqueror Browsing Control Module"),
                0, KLocalizedString(), KAboutData::License_GPL,
                ki18n("(c) 1999 - 2001 The Konqueror Developers"));

  about->addAuthor(ki18n("Waldo Bastian"),KLocalizedString(),"bastian@kde.org");
  about->addAuthor(ki18n("David Faure"),KLocalizedString(),"faure@kde.org");
  about->addAuthor(ki18n("Matthias Kalle Dalheimer"),KLocalizedString(),"kalle@kde.org");
  about->addAuthor(ki18n("Lars Knoll"),KLocalizedString(),"knoll@kde.org");
  about->addAuthor(ki18n("Dirk Mueller"),KLocalizedString(),"mueller@kde.org");
  about->addAuthor(ki18n("Daniel Molkentin"),KLocalizedString(),"molkentin@kde.org");
  about->addAuthor(ki18n("Wynn Wilkes"),KLocalizedString(),"wynnw@caldera.com");

  about->addCredit(ki18n("Leo Savernik"),ki18n("JavaScript access controls\n"
    			"Per-domain policies extensions"),
			"l.savernik@aon.at");

  setAboutData( about );

  QVBoxLayout *layout = new QVBoxLayout(this);
  tab = new QTabWidget(this);
  layout->addWidget(tab);

  // ### the groupname is duplicated in KJSParts::save
  java = new KJavaOptions( mConfig, "Java/JavaScript Settings", componentData(), this );
  tab->addTab( java, i18n( "&Java" ) );
  connect( java, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );

  javascript = new KJavaScriptOptions( mConfig, "Java/JavaScript Settings", componentData(), this );
  tab->addTab( javascript, i18n( "Java&Script" ) );
  connect( javascript, SIGNAL( changed( bool ) ), SIGNAL( changed( bool ) ) );
}

void KJSParts::load()
{
  javascript->load();
  java->load();
}


void KJSParts::save()
{
  javascript->save();
  java->save();

  // delete old keys after they have been migrated
  if (javascript->_removeJavaScriptDomainAdvice
      || java->_removeJavaScriptDomainAdvice) {
    mConfig->group("Java/JavaScript Settings").deleteEntry("JavaScriptDomainAdvice");
    javascript->_removeJavaScriptDomainAdvice = false;
    java->_removeJavaScriptDomainAdvice = false;
  }

  mConfig->sync();

  // Send signal to konqueror
  // Warning. In case something is added/changed here, keep kfmclient in sync
  QDBusMessage message =
      QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
  QDBusConnection::sessionBus().send(message);
}


void KJSParts::defaults()
{
  javascript->defaults();
  java->defaults();
}

QString KJSParts::quickHelp() const
{
  return i18n("<h2>JavaScript</h2>On this page, you can configure "
              "whether JavaScript programs embedded in web pages should "
              "be allowed to be executed by Konqueror."
              "<h2>Java</h2>On this page, you can configure "
              "whether Java applets embedded in web pages should "
              "be allowed to be executed by Konqueror."
              "<br><br><b>Note:</b> Active content is always a "
              "security risk, which is why Konqueror allows you to specify very "
              "fine-grained from which hosts you want to execute Java and/or "
              "JavaScript programs." );
}

#include "main.moc"

