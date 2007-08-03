// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "toplevel.h"
#include "importers.h"

#include <klocale.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kstandarddirs.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kuniqueapplication.h>

#include <kmessagebox.h>
#include <kwindowsystem.h>
#include <unistd.h>

#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>
#include <toplevel_interface.h>

// TODO - make this register() or something like that and move dialog into main
static bool askUser(const QString& filename, bool &readonly) {

    QString requestedName("keditbookmarks");
    QString interfaceName = "org.kde.keditbookmarks";
    QString appId = interfaceName + '-' +QString().setNum(getpid());

    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusReply<QStringList> reply = dbus.interface()->registeredServiceNames();
    if ( !reply.isValid() )
        return true;
    const QStringList allServices = reply;
    for ( QStringList::const_iterator it = allServices.begin(), end = allServices.end() ; it != end ; ++it ) {
        const QString service = *it;
        if ( service.startsWith( interfaceName ) && service != appId ) {
            org::kde::keditbookmarks keditbookmarks(service,"/keditbookmarks", dbus);
            QDBusReply<QString> bookmarks = keditbookmarks.bookmarkFilename();
            QString name;
            if( bookmarks.isValid())
                name = bookmarks;
            if( name == filename)
            {
                int ret = KMessageBox::warningYesNo(0,
                i18n("Another instance of %1 is already running, do you really "
                "want to open another instance or continue work in the same instance?\n"
                "Please note that, unfortunately, duplicate views are read-only.", KGlobal::caption()),
                i18n("Warning"),
                KGuiItem(i18n("Run Another")),    /* yes */
                KGuiItem(i18n("Continue in Same")) /*  no */);
                if (ret == KMessageBox::No) {
                    QDBusInterface keditinterface(service, "/keditbookmarks/MainWindow_1");
                    //TODO fix me
                    QDBusReply<qlonglong> value = keditinterface.call(QDBus::NoBlock, "winId");
                    qlonglong id = 0;
                    if( value.isValid())
                        id = value;
                    //kDebug()<<" id !!!!!!!!!!!!!!!!!!! :"<<id;
                    KWindowSystem::activateWindow((WId)id);
                    return false;
                } else if (ret == KMessageBox::Yes) {
                    readonly = true;
                }
            }
        }
    }
    return true;
}

#include <kactioncollection.h>

extern "C" KDE_EXPORT int kdemain(int argc, char **argv) {
    KAboutData aboutData("keditbookmarks", 0, ki18n("Bookmark Editor"), KDE_VERSION_STRING,
            ki18n("Konqueror Bookmarks Editor"),
            KAboutData::License_GPL,
            ki18n("(c) 2000 - 2003, KDE developers") );
    aboutData.addAuthor(ki18n("David Faure"), ki18n("Initial author"), "faure@kde.org");
    aboutData.addAuthor(ki18n("Alexander Kellett"), ki18n("Author"), "lypanov@kde.org");

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineArgs::addStdCmdLineOptions();

    KCmdLineOptions options;
    options.add("importmoz <filename>", ki18n("Import bookmarks from a file in Mozilla format"));
    options.add("importns <filename>", ki18n("Import bookmarks from a file in Netscape (4.x and earlier) format"));
    options.add("importie <filename>", ki18n("Import bookmarks from a file in Internet Explorer's Favorites format"));
    options.add("importopera <filename>", ki18n("Import bookmarks from a file in Opera format"));
    options.add("exportmoz <filename>", ki18n("Export bookmarks to a file in Mozilla format"));
    options.add("exportns <filename>", ki18n("Export bookmarks to a file in Netscape (4.x and earlier) format"));
    options.add("exporthtml <filename>", ki18n("Export bookmarks to a file in a printable HTML format"));
    options.add("exportie <filename>", ki18n("Export bookmarks to a file in Internet Explorer's Favorites format"));
    options.add("exportopera <filename>", ki18n("Export bookmarks to a file in Opera format"));
    options.add("address <address>", ki18n("Open at the given position in the bookmarks file"));
    options.add("customcaption <caption>", ki18n("Set the user readable caption for example \"Konsole\""));
    options.add("nobrowser", ki18n("Hide all browser related functions"));
    options.add("dbusObjectName <name>", ki18n("A unique name that represents this bookmark collection, usually the kinstance name.\n"
                                 "This should be \"konqueror\" for the konqueror bookmarks, \"kfile\" for KFileDialog bookmarks, etc.\n"
                                 "The final DBus object path is /KBookmarkManager/dbusObjectName"));
    options.add("+[file]", ki18n("File to edit"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    bool isGui = !(args->isSet("exportmoz") || args->isSet("exportns") || args->isSet("exporthtml")
                || args->isSet("exportie") || args->isSet("exportopera")
                || args->isSet("importmoz") || args->isSet("importns")
                || args->isSet("importie") || args->isSet("importopera"));

    bool browser = args->isSet("browser");

    //KApplication::disableAutoDcopRegistration();
    KApplication app(isGui);

    bool gotFilenameArg = (args->count() == 1);

    QString filename = gotFilenameArg
        ? args->arg(0)
        : KStandardDirs::locateLocal("data", QLatin1String("konqueror/bookmarks.xml"));

    if (!isGui) {
        CurrentMgr::self()->createManager(filename, QString());
        CurrentMgr::ExportType exportType = CurrentMgr::MozillaExport; // uumm.. can i just set it to -1 ?
        int got = 0;
        const char *arg, *arg2 = 0, *importType = 0;
        if (arg = "exportmoz",  args->isSet(arg)) { exportType = CurrentMgr::MozillaExport;  arg2 = arg; got++; }
        if (arg = "exportns",   args->isSet(arg)) { exportType = CurrentMgr::NetscapeExport; arg2 = arg; got++; }
        if (arg = "exporthtml", args->isSet(arg)) { exportType = CurrentMgr::HTMLExport;     arg2 = arg; got++; }
        if (arg = "exportie",   args->isSet(arg)) { exportType = CurrentMgr::IEExport;       arg2 = arg; got++; }
        if (arg = "exportopera", args->isSet(arg)) { exportType = CurrentMgr::OperaExport;    arg2 = arg; got++; }
        if (arg = "importmoz",  args->isSet(arg)) { importType = "Moz";   arg2 = arg; got++; }
        if (arg = "importns",   args->isSet(arg)) { importType = "NS";    arg2 = arg; got++; }
        if (arg = "importie",   args->isSet(arg)) { importType = "IE";    arg2 = arg; got++; }
        if (arg = "importopera", args->isSet(arg)) { importType = "Opera"; arg2 = arg; got++; }
        if (!importType && arg2) {
            Q_ASSERT(arg2);
            // TODO - maybe an xbel export???
            if (got > 1) // got == 0 isn't possible as !isGui is dependant on "export.*"
                KCmdLineArgs::usage(I18N_NOOP("You may only specify a single --export option."));
            QString path = args->getOption(arg2);
            CurrentMgr::self()->doExport(exportType, path);
        } else if (importType) {
            if (got > 1) // got == 0 isn't possible as !isGui is dependant on "import.*"
                KCmdLineArgs::usage(I18N_NOOP("You may only specify a single --import option."));
            QString path = args->getOption(arg2);
            ImportCommand *importer = ImportCommand::importerFactory(importType);
            importer->import(path, true);
            importer->execute();
            CurrentMgr::self()->managerSave();
            CurrentMgr::self()->notifyManagers();
        }
        return 0; // error flag on exit?, 1?
    }

    QString address = args->isSet("address")
        ? args->getOption("address")
        : QString("/0");

    QString caption = args->isSet("customcaption")
        ? args->getOption("customcaption")
        : QString();

    QString dbusObjectName;
    if(args->isSet("dbusObjectName"))
    {
        dbusObjectName = args->getOption("dbusObjectName");
    }
    else
    {
        if(gotFilenameArg)
          dbusObjectName = QString();
        else
          dbusObjectName = "konqueror";
    }

    args->clear();

    bool readonly = false; // passed by ref

    if (askUser((gotFilenameArg ? filename : QString()), readonly)) {
        KEBApp *toplevel = new KEBApp(filename, readonly, address, browser, caption, dbusObjectName);
        toplevel->show();
        return app.exec();
    }

    return 0;
}
