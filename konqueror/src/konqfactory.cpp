/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 1999 David Faure <faure@kde.org>
    Copyright (C) 1999 Torben Weis <weis@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// Own
#include "konqfactory.h"

// std
#include <assert.h>

// Qt
#include <QtGui/QWidget>
#include <QtCore/QFile>

// KDE
#include <kaboutdata.h>
#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kparts/factory.h>
#include <kparts/part.h>
#include <kservicetypetrader.h>
#include <kdeversion.h>

// Local
#include "konqsettings.h"
#include "konqmainwindow.h"


static KAboutData *s_aboutData = 0;

KonqViewFactory::KonqViewFactory( KLibFactory *factory, const QStringList &args,
                                  bool createBrowser )
    : m_factory( factory ), m_args( args ), m_createBrowser( createBrowser )
{
    if ( m_createBrowser )
        m_args << QLatin1String( "Browser/View" );
}

KParts::ReadOnlyPart *KonqViewFactory::create( QWidget *parentWidget, QObject * parent )
{
  if ( !m_factory )
    return 0;

  QObject *obj = 0;

  KParts::Factory* kpartsFactory = ::qobject_cast<KParts::Factory *>( m_factory );
  if ( kpartsFactory )
  {
    if ( m_createBrowser )
      obj = kpartsFactory->createPart( parentWidget, parent, "Browser/View", m_args );

    if ( !obj )
      obj = kpartsFactory->createPart( parentWidget, parent, "KParts::ReadOnlyPart", m_args );
  }
  else
  {
    if ( m_createBrowser )
      obj = m_factory->create( parentWidget, "Browser/View", m_args );

    if ( !obj )
      obj = m_factory->create( parentWidget, "KParts::ReadOnlyPart", m_args );
  }

  KParts::ReadOnlyPart* part = ::qobject_cast<KParts::ReadOnlyPart *>( obj );
  if ( !part ) {
    kError(1202) << "Part " << obj << " (" << obj->metaObject()->className() << ") doesn't inherit KParts::ReadOnlyPart !" << endl;
  } else {
    QFrame* frame = qobject_cast<QFrame*>( part->widget() );
    if ( frame ) {
      frame->setFrameStyle( QFrame::NoFrame );
    }
  }
  return static_cast<KParts::ReadOnlyPart *>(obj);
}

KonqViewFactory KonqFactory::createView( const QString &serviceType,
                                         const QString &serviceName,
                                         KService::Ptr *serviceImpl,
                                         KService::List *partServiceOffers,
                                         KService::List *appServiceOffers,
					 bool forceAutoEmbed )
{
  kDebug(1202) << "Trying to create view for \"" << serviceType << "\"";

  // We need to get those in any case
  KService::List offers, appOffers;

  // Query the trader
  getOffers( serviceType, &offers, &appOffers );

  if ( partServiceOffers )
     (*partServiceOffers) = offers;
  if ( appServiceOffers )
     (*appServiceOffers) = appOffers;

  // We ask ourselves whether to do it or not only if no service was specified.
  // If it was (from the View menu or from RMB + Embedding service), just do it.
  forceAutoEmbed = forceAutoEmbed || !serviceName.isEmpty();
  // Or if we have no associated app anyway, then embed.
  forceAutoEmbed = forceAutoEmbed || ( appOffers.isEmpty() && !offers.isEmpty() );
  // Or if the associated app is konqueror itself, then embed.
  if ( !appOffers.isEmpty() )
    forceAutoEmbed = forceAutoEmbed || KonqMainWindow::isMimeTypeAssociatedWithSelf( serviceType, appOffers.first() );

  if ( ! forceAutoEmbed )
  {
    if ( ! KonqFMSettings::settings()->shouldEmbed( serviceType ) )
    {
      kDebug(1202) << "KonqFMSettings says: don't embed this servicetype";
      return KonqViewFactory();
    }
  }

  KService::Ptr service;

  // Look for this service
  if ( !serviceName.isEmpty() )
  {
      KService::List::Iterator it = offers.begin();
      for ( ; it != offers.end() && !service ; ++it )
      {
          if ( (*it)->desktopEntryName() == serviceName )
          {
              kDebug(1202) << "Found requested service " << serviceName;
              service = *it;
          }
      }
  }

  KLibFactory *factory = 0;

  if ( service )
  {
    kDebug(1202) << "Trying to open lib for requested service " << service->desktopEntryName();
    factory = KLibLoader::self()->factory( service->library() );
    if ( !factory )
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                            service->name(), KLibLoader::self()->lastErrorMessage()));
  }

  KService::List::Iterator it = offers.begin();
  for ( ; !factory && it != offers.end() ; ++it )
  {
    service = (*it);
    // Allowed as default ?
    QVariant prop = service->property( "X-KDE-BrowserView-AllowAsDefault" );
    kDebug(1202) << service->desktopEntryName() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid();
    if ( !prop.isValid() || prop.toBool() ) // defaults to true
    {
      //kDebug(1202) << "Trying to open lib for service " << service->name();
      // Try loading factory
      factory = KLibLoader::self()->factory( service->library() );
      if ( !factory )
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                            service->name(), KLibLoader::self()->lastErrorMessage()));
      // If this works, we exit the loop.
    } else
      kDebug(1202) << "Not allowed as default " << service->desktopEntryName();
  }

  if ( serviceImpl )
    (*serviceImpl) = service;

  if ( !factory )
  {
    kWarning(1202) << "KonqFactory::createView : no factory" ;
    return KonqViewFactory();
  }

  QStringList args;

  QVariant prop = service->property( "X-KDE-BrowserView-Args" );

  if ( prop.isValid() )
  {
    QString argStr = prop.toString();
    args = argStr.split( " ");
  }

  return KonqViewFactory( factory, args, service->serviceTypes().contains( "Browser/View" ) );
}

void KonqFactory::getOffers( const QString & serviceType,
                             KService::List *partServiceOffers,
                             KService::List *appServiceOffers )
{
#ifdef __GNUC__
#warning Temporary hack
#endif
    if ( partServiceOffers && serviceType[0].isUpper() ) {
        *partServiceOffers = KServiceTypeTrader::self()->query( serviceType,
                    "DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'");
        return;

    }
    if ( appServiceOffers )
    {
        *appServiceOffers = KMimeTypeTrader::self()->query( serviceType, "Application",
"DesktopEntryName != 'kfmclient' and DesktopEntryName != 'kfmclient_dir' and DesktopEntryName != 'kfmclient_html'");
    }

    if ( partServiceOffers )
    {
        *partServiceOffers = KMimeTypeTrader::self()->query( serviceType, "KParts/ReadOnlyPart" );
    }
}


const KAboutData *KonqFactory::aboutData()
{
  if (!s_aboutData)
  {
    s_aboutData = new KAboutData( "konqueror", 0, ki18n("Konqueror"),
                        KDE_VERSION_STRING,
                        ki18n("Web browser, file manager and document viewer."),
                        KAboutData::License_GPL,
                        ki18n("(C) 1999-2007, The Konqueror developers"),
                        KLocalizedString(),
                        I18N_NOOP("http://konqueror.kde.org") );
    s_aboutData->addAuthor( ki18n("David Faure"), ki18n("Developer (framework, parts, JavaScript, I/O library) and maintainer"), "faure@kde.org" );
    s_aboutData->addAuthor( ki18n("Simon Hausmann"), ki18n("Developer (framework, parts)"), "hausmann@kde.org" );
    s_aboutData->addAuthor( ki18n("Michael Reiher"), ki18n("Developer (framework)"), "michael.reiher@gmx.de" );
    s_aboutData->addAuthor( ki18n("Matthias Welk"), ki18n("Developer"), "welk@fokus.gmd.de" );
    s_aboutData->addAuthor( ki18n("Alexander Neundorf"), ki18n("Developer (List views)"), "neundorf@kde.org" );
    s_aboutData->addAuthor( ki18n("Michael Brade"), ki18n("Developer (List views, I/O library)"), "brade@kde.org" );
    s_aboutData->addAuthor( ki18n("Lars Knoll"), ki18n("Developer (HTML rendering engine)"), "knoll@kde.org" );
    s_aboutData->addAuthor( ki18n("Dirk Mueller"), ki18n("Developer (HTML rendering engine)"), "mueller@kde.org" );
    s_aboutData->addAuthor( ki18n("Peter Kelly"), ki18n("Developer (HTML rendering engine)"), "pmk@post.com" );
    s_aboutData->addAuthor( ki18n("Waldo Bastian"), ki18n("Developer (HTML rendering engine, I/O library)"), "bastian@kde.org" );
    s_aboutData->addAuthor( ki18n("Germain Garand"), ki18n("Developer (HTML rendering engine)"), "germain@ebooksfrance.org" );
    s_aboutData->addAuthor( ki18n("Leo Savernik"), ki18n("Developer (HTML rendering engine)"), "l.savernik@aon.at" );
    s_aboutData->addAuthor( ki18n("Stephan Kulow"), ki18n("Developer (HTML rendering engine, I/O library, regression test framework)"), "coolo@kde.org" );
    s_aboutData->addAuthor( ki18n("Antti Koivisto"), ki18n("Developer (HTML rendering engine)"), "koivisto@kde.org" );
    s_aboutData->addAuthor( ki18n("Zack Rusin"),  ki18n("Developer (HTML rendering engine)"), "zack@kde.org" );
    s_aboutData->addAuthor( ki18n("Tobias Anton"), ki18n( "Developer (HTML rendering engine)" ), "anton@stud.fbi.fh-darmstadt.de" );
    s_aboutData->addAuthor( ki18n("Lubos Lunak"), ki18n( "Developer (HTML rendering engine)" ), "l.lunak@kde.org" );
    s_aboutData->addAuthor( ki18n("Allan Sandfeld Jensen"), ki18n( "Developer (HTML rendering engine)" ), "kde@carewolf.com" );
    s_aboutData->addAuthor( ki18n("Apple Safari Developers"), ki18n("Developer (HTML rendering engine, JavaScript)"));
    s_aboutData->addAuthor( ki18n("Harri Porten"), ki18n("Developer (JavaScript)"), "porten@kde.org" );
    s_aboutData->addAuthor( ki18n("Koos Vriezen"), ki18n("Developer (Java applets and other embedded objects)"), "koos.vriezen@xs4all.nl" );
    s_aboutData->addAuthor( ki18n("Matt Koss"), ki18n("Developer (I/O library)"), "koss@miesto.sk" );
    s_aboutData->addAuthor( ki18n("Alex Zepeda"), ki18n("Developer (I/O library)"), "zipzippy@sonic.net" );
    s_aboutData->addAuthor( ki18n("Richard Moore"), ki18n("Developer (Java applet support)"), "rich@kde.org" );
    s_aboutData->addAuthor( ki18n("Dima Rogozin"), ki18n("Developer (Java applet support)"), "dima@mercury.co.il" );
    s_aboutData->addAuthor( ki18n("Wynn Wilkes"), ki18n("Developer (Java 2 security manager support,\n and other major improvements to applet support)"), "wynnw@calderasystems.com" );
    s_aboutData->addAuthor( ki18n("Stefan Schimanski"), ki18n("Developer (Netscape plugin support)"), "schimmi@kde.org" );
    s_aboutData->addAuthor( ki18n("George Staikos"), ki18n("Developer (SSL, Netscape plugins)"), "staikos@kde.org" );
    s_aboutData->addAuthor( ki18n("Dawit Alemayehu"),ki18n("Developer (I/O library, Authentication support)"), "adawit@kde.org" );
    s_aboutData->addAuthor( ki18n("Carsten Pfeiffer"),ki18n("Developer (framework)"), "pfeiffer@kde.org" );
    s_aboutData->addAuthor( ki18n("Torsten Rahn"), ki18n("Graphics/icons"), "torsten@kde.org" );
    s_aboutData->addAuthor( ki18n("Torben Weis"), ki18n("KFM author"), "weis@kde.org" );
    s_aboutData->addAuthor( ki18n("Joseph Wenninger"), ki18n("Developer (navigation panel framework)"),"jowenn@kde.org");
    s_aboutData->addAuthor( ki18n("Stephan Binner"), ki18n("Developer (misc stuff)"),"binner@kde.org");
    s_aboutData->addAuthor( ki18n("Ivor Hewitt"), ki18n("Developer (AdBlock filter)"),"ivor@ivor.org");
  }
  return s_aboutData;
}

