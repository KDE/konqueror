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
#include <QtCore/QCoreApplication>

// KDE
#include <kaboutdata.h>
#include <kdebug.h>
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
static void cleanupKAboutData()
{
    delete s_aboutData;
}

KonqViewFactory::KonqViewFactory(const QString& libName, KLibFactory *factory)
    : m_libName(libName), m_factory(factory),
      m_args()
{
}

void KonqViewFactory::setArgs(const QVariantList &args)
{
    m_args = args;
}

KParts::ReadOnlyPart *KonqViewFactory::create( QWidget *parentWidget, QObject * parent )
{
    if ( !m_factory )
        return 0;

    KParts::ReadOnlyPart* part = m_factory->create<KParts::ReadOnlyPart>( parentWidget, parent, QString(), m_args );

    if ( !part ) {
        kError(1202) << "No KParts::ReadOnlyPart created from" << m_libName;
    } else {
        QFrame* frame = qobject_cast<QFrame*>( part->widget() );
        if ( frame ) {
            frame->setFrameStyle( QFrame::NoFrame );
        }
    }
    return part;
}

static KonqViewFactory tryLoadingService(KService::Ptr service)
{
    KPluginLoader pluginLoader(*service);
    KPluginFactory* factory = pluginLoader.factory();
    if (!factory) {
        KMessageBox::error(0,
                           i18n("There was an error loading the module %1.\nThe diagnostics is:\n%2",
                                service->name(), pluginLoader.errorString()));
        return KonqViewFactory();
    }
    else {
        return KonqViewFactory(service->library(), factory);
    }
}

KonqViewFactory KonqFactory::createView( const QString &serviceType,
                                         const QString &serviceName,
                                         KService::Ptr *serviceImpl,
                                         KService::List *partServiceOffers,
                                         KService::List *appServiceOffers,
					 bool forceAutoEmbed )
{
  kDebug(1202) << "Trying to create view for" << serviceType << serviceName;

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
    if (!serviceName.isEmpty()) {
        KService::List::const_iterator it = offers.begin();
        for ( ; it != offers.end() && !service ; ++it ) {
            if ( (*it)->desktopEntryName() == serviceName ) {
                kDebug(1202) << "Found requested service" << serviceName;
                service = *it;
            }
        }
    }

    KonqViewFactory viewFactory;
    if (service) {
        kDebug(1202) << "Trying to open lib for requested service " << service->desktopEntryName();
        viewFactory = tryLoadingService(service);
        // If this fails, then return an error.
        // When looking for konq_sidebartng or konq_aboutpage, we don't want to end up
        // with khtml or another Browser/View part in case of an error...
    } else {
        KService::List::Iterator it = offers.begin();
        for ( ; viewFactory.isNull() /* exit as soon as we get one */ && it != offers.end() ; ++it ) {
            service = (*it);
            // Allowed as default ?
            QVariant prop = service->property( "X-KDE-BrowserView-AllowAsDefault" );
            kDebug(1202) << service->desktopEntryName() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid();
            if ( !prop.isValid() || prop.toBool() ) { // defaults to true
                //kDebug(1202) << "Trying to open lib for service " << service->name();
                viewFactory = tryLoadingService(service);
                // If this works, we exit the loop.
            } else {
                kDebug(1202) << "Not allowed as default " << service->desktopEntryName();
            }
        }
    }

    if (serviceImpl)
        (*serviceImpl) = service;

    if (viewFactory.isNull()) {
        if (offers.isEmpty())
            kWarning(1202) << "no part was associated with" << serviceType;
        else
            kWarning(1202) << "no part could be loaded"; // full error was shown to user already
        return viewFactory;
    }

    QVariantList args;
    const QVariant prop = service->property( "X-KDE-BrowserView-Args" );
    if (prop.isValid()) {
        Q_FOREACH(const QString& str, prop.toString().split(' '))
            args << QVariant(str);
    }

    if (service->serviceTypes().contains("Browser/View"))
        args << QLatin1String("Browser/View");

    viewFactory.setArgs(args);
    return viewFactory;
}

void KonqFactory::getOffers( const QString & serviceType,
                             KService::List *partServiceOffers,
                             KService::List *appServiceOffers )
{
#ifdef __GNUC__
#warning Temporary hack -- must separate mimetypes and servicetypes better
#endif
    if ( partServiceOffers && serviceType.length() > 0 && serviceType[0].isUpper() ) {
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
    s_aboutData->addAuthor( ki18n("Eduardo Robles Elvira"), ki18n("Developer (Session Management, Undo closed item)"),"edulix@gmail.com");
    qAddPostRoutine(cleanupKAboutData);
  }
  return s_aboutData;
}
