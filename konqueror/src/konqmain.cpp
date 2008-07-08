/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqapplication.h"
#include "konqmisc.h"
#include "konqfactory.h"
#include "konqmainwindow.h"
#include "konqsessionmanager.h"
#include "konqview.h"
#include "konqsettingsxt.h"

#include <ktemporaryfile.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QWidget>

#ifdef Q_WS_X11
#include <QX11Info>
#endif

#include <QtDBus/QtDBus>
#include <QDir>

extern "C" KDE_EXPORT int kdemain( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, KonqFactory::aboutData() );


  KCmdLineOptions options;

  options.add("silent", ki18n("Start without a default window"));

  options.add("preload", ki18n("Preload for later use"));

  options.add("profile <profile>", ki18n("Profile to open"));

  options.add("profiles", ki18n("List available profiles"));

  options.add("mimetype <mimetype>", ki18n("Mimetype to use for this URL (e.g. text/html or inode/directory)"));

  options.add("select", ki18n("For URLs that point to files, opens the directory and selects the file, instead of opening the actual file"));

  options.add("+[URL]", ki18n("Location to open"));

  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
  KCmdLineArgs::addTempFileOption();

  KonquerorApplication app;
  app.setQuitOnLastWindowClosed(false);

  KGlobal::locale()->insertCatalog("libkonq"); // needed for apps using libkonq

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  KTemporaryFile crashlog_file;
  crashlog_file.setPrefix("konqueror-crash-");
  crashlog_file.setSuffix(".log");
  crashlog_file.open();
  KonqMainWindow::s_crashlog_file = &crashlog_file;

  if ( app.isSessionRestored() )
  {
    KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();

    int n = 1;
    while ( KonqMainWindow::canBeRestored( n ) )
    {
      QString className = KXmlGuiWindow::classNameOfToplevel( n );
      if( className == QLatin1String( "KonqMainWindow" ))
          (new KonqMainWindow() )->restore( n );
      else
          kWarning() << "Unknown class " << className << " in session saved data!" ;
      n++;
    }
  }
  else
  {
     if (args->isSet("profiles"))
     {
       QStringList profiles = KGlobal::dirs()->findAllResources("data", "konqueror/profiles/*", KStandardDirs::NoDuplicates);
       profiles.sort();
       for(QStringList::ConstIterator it = profiles.begin();
           it != profiles.end(); ++it)
       {
         QString file = *it;
         file = file.mid(file.lastIndexOf('/')+1);
         printf("%s\n", QFile::encodeName(file).data());
       }

       return 0;
     }
     if (args->isSet("profile"))
     {
       QString profile = args->getOption("profile");
       QString profilePath = profile;
       if (profile[0] != '/') {
           profilePath = KStandardDirs::locate( "data", QLatin1String("konqueror/profiles/")+profile );
           if (profilePath.isEmpty()) {
               profile = KonqMisc::defaultProfileName();
               profilePath = KonqMisc::defaultProfilePath();
           }
       }

       QString url;
       QStringList filesToSelect;
       if (args->count() == 1)
           url = args->arg(0);
       KUrl kurl(url);
       KParts::OpenUrlArguments urlargs;
       if (args->isSet("mimetype"))
           urlargs.setMimeType( args->getOption("mimetype") );
       if (args->isSet("select")) {
           QString fn = kurl.fileName(KUrl::ObeyTrailingSlash);
           if( !fn.isEmpty() ){
              filesToSelect += fn;
              kurl.setFileName("");
           }
       }
       kDebug(1202) << "main() -> createBrowserWindowFromProfile mimeType=" << urlargs.mimeType();
       KonqMisc::createBrowserWindowFromProfile( profilePath, profile, kurl, urlargs, KParts::BrowserArguments(), false, filesToSelect );
     }
     else
     {
         if (args->count() == 0)
         {
             if (args->isSet("preload"))
             {
#ifdef Q_WS_X11
                 if( KonqSettings::maxPreloadCount() > 0 )
                 {
                     QDBusInterface ref( "org.kde.kded", "/modules/konqy_preloader", "org.kde.konqueror.Preloader", QDBusConnection::sessionBus() );
                     QX11Info info;
                     QDBusReply<bool> retVal = ref.call( QDBus::Block, "registerPreloadedKonqy", QDBusConnection::sessionBus().baseService(), info.screen());
                     if( !retVal )
                         return 0; // too many preloaded or failed
		     KonqMainWindow* win = new KonqMainWindow; // prepare an empty window too
		     // KonqMainWindow ctor sets always the preloaded flag to false, so create the window before this
                     KonqMainWindow::setPreloadedFlag( true );
		     KonqMainWindow::setPreloadedWindow( win );
                     kDebug(1202) << "Konqy preloaded :" << QDBusConnection::sessionBus().baseService();
                 }
                 else
                 {
                     return 0; // no preloading
                 }
#else
                 return 0; // no preloading
#endif
             }
             else if(!args->isSet("silent")) {
                 const QString profile = KonqMisc::defaultProfileName();
                 KonqMisc::createBrowserWindowFromProfile( KonqMisc::defaultProfilePath(), profile );
             }
             kDebug(1202) << "main() -> no args";
         }
         else
         {
             KUrl::List urlList;
             KonqMainWindow * mainwin = 0L;

             if(args->count() > 0)
                KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();

             QList<KonqMainWindow*> *mainWindowList = KonqMainWindow::mainWindowList();
             if(mainWindowList && !mainWindowList->isEmpty())
                mainwin = mainWindowList->first();

             for ( int i = 0; i < args->count(); i++ )
             {
                 // KonqMisc::konqFilteredURL doesn't cope with local files... A bit of hackery below
                 KUrl url = args->url(i);
                 KUrl urlToOpen;
                 QStringList filesToSelect;

                 if (url.isLocalFile() && QFile::exists(url.path())) // "konqueror index.html"
                     urlToOpen = url;
                 else
                     urlToOpen = KUrl( KonqMisc::konqFilteredURL(0L, args->arg(i)) ); // "konqueror slashdot.org"

                 if ( !mainwin ) {
                     KParts::OpenUrlArguments urlargs;
                     if (args->isSet("mimetype"))
                     {
                         urlargs.setMimeType( args->getOption("mimetype") );
                         kDebug(1202) << "main() : setting mimeType to " << urlargs.mimeType();
                     }
                     if (args->isSet("select"))
                     {
                        QString fn = urlToOpen.fileName(KUrl::ObeyTrailingSlash);
                        if( !fn.isEmpty() ){
                           filesToSelect += fn;
                           urlToOpen.setFileName("");
                        }
                     }
                     const bool tempFile = KCmdLineArgs::isTempFileSet();
                     mainwin = KonqMisc::createNewWindow( urlToOpen, urlargs, KParts::BrowserArguments(), false, filesToSelect, tempFile );
                 } else
                     urlList += urlToOpen;
             }
             if ( mainwin )
                 mainwin->openMultiURL( urlList );
         }
     }
  }
  args->clear();

  app.exec();

  // Delete all KonqMainWindows, so that we don't have
  // any parts loaded when KLibLoader::cleanUp is called.
  // Their deletion was postponed in their event()
  // (and Qt doesn't delete WDestructiveClose widgets on exit anyway :(  )
  while( KonqMainWindow::mainWindowList() != NULL )
  { // the list will be deleted by last KonqMainWindow
      delete KonqMainWindow::mainWindowList()->first();
  }

  // Notify the session manager that the instance was closed without errors, and normally.
  KonqSessionManager::self()->disableAutosave();
  KonqSessionManager::self()->deleteOwnedSessions();

  return 0;
}
