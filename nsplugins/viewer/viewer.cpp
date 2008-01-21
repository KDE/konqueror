/*

  This is a standalone application that executes Netscape plugins.


  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <mhk@caldera.de>
                     Stefan Schimanski <1Stein@gmx.de>

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


#include <config-apps.h>

#include <kapplication.h>
#include "nsplugin.h"

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <Qt3Support/Q3PtrList>
#include <QSocketNotifier>
//Added by qt3to4:
#include <QEvent>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <kdefakes.h>

#ifdef Bool
#undef Bool
#endif
#include <kconfig.h>

#include "xtevents.h"
#include <QtDBus/QtDBus>

/**
 *  Use RLIMIT_DATA on systems that don't define RLIMIT_AS,
 *  such as FreeBSD 4, NetBSD and OpenBSD.
 */

#ifndef RLIMIT_AS
#define RLIMIT_AS RLIMIT_DATA
#endif

/*
 * As the plugin viewer needs to be a motif application, I give in to
 * the "old style" and keep lot's of global vars. :-)
 */

static QString g_dbusServiceName;

/**
 * parseCommandLine - get command line parameters
 *
 */
void parseCommandLine(int argc, char *argv[])
{
   for (int i=0; i<argc; i++)
   {
      if (!strcmp(argv[i], "-dbusservice") && (i+1 < argc)) 
      {
         g_dbusServiceName = argv[i+1];
         i++;
      }
   }
}

int main(int argc, char** argv)
{
    // nspluginviewer is a helper app, it shouldn't do session management at all
   setenv( "SESSION_MANAGER", "", 1 );

   setvbuf( stderr, NULL, _IONBF, 0 );

   kDebug(1430) << "2 - parseCommandLine";
   parseCommandLine(argc, argv);

   parseCommandLine(argc, argv);

   kDebug(1430) << "3 - create KApplication";

   // Skip the args.. This is internal, anyway.
   KCmdLineArgs::init(1, argv, "nspluginviewer", "nsplugin", ki18n("nspluginviewer"), "");
   
   KApplication app;

   kDebug(1430) << "4 - create XtEvents";
   XtEvents xtevents;

   {
      KConfig _cfg( "kcmnspluginrc" );
      KConfigGroup cfg(&_cfg, "Misc");
      int v = qBound(0, cfg.readEntry("Nice Level", 0), 19);
      if (v > 0) {
         nice(v);
      }
      v = cfg.readEntry("Max Memory", 0);
      if (v > 0) {
         rlimit rl;
         memset(&rl, 0, sizeof(rl));
         if (0 == getrlimit(RLIMIT_AS, &rl)) {
            rl.rlim_cur = qMin(v, int(rl.rlim_max));
            setrlimit(RLIMIT_AS, &rl);
         }
      }
   }

   kDebug(1430) << "5 - dbus requestName";
   if (!g_dbusServiceName.isEmpty()) {
       QDBusConnectionInterface* bus = QDBusConnection::sessionBus().interface(); // already null-checked by KApplication
       if ( bus->registerService(g_dbusServiceName, QDBusConnectionInterface::DontQueueService) == QDBusConnectionInterface::ServiceNotRegistered ) {
          kError(101) << "Couldn't register name '" << g_dbusServiceName << "' with DBUS - another process owns it already!" << endl;
          ::exit(126);
      }
   }

   // create dcop interface
   kDebug(1430) << "6 - new NSPluginViewer";
   NSPluginViewer *viewer = new NSPluginViewer( 0 );

   // start main loop
   kDebug(1430) << "7 - app.exec()";
   app.exec();

   // delete viewer
   delete viewer;
}
