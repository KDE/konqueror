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
#include <QApplication>
#include <QWidget>

#ifdef Q_WS_X11
#include <QX11Info>
#endif

#include <QtDBus/QtDBus>
#include <QDir>

static void listProfiles()
{
    QStringList profiles = KGlobal::dirs()->findAllResources("data", "konqueror/profiles/*", KStandardDirs::NoDuplicates);
    profiles.sort();
    Q_FOREACH(const QString& _file, profiles)
    {
        const QString file = _file.mid(_file.lastIndexOf('/')+1);
        printf("%s\n", QFile::encodeName(file).constData());
    }
}

static void listSessions()
{
    const QString dir = KStandardDirs::locateLocal("appdata", "sessions/");
    QDirIterator it(dir, QDir::Readable|QDir::NoDotAndDotDot|QDir::Dirs);
    while (it.hasNext())
    {
        QFileInfo fileInfo(it.next());
        printf("%s\n", QFile::encodeName(fileInfo.baseName()).constData());
    }
}

static bool tryPreload()
{
#ifdef Q_WS_X11
    if(KonqSettings::maxPreloadCount() > 0) {
        QDBusInterface ref("org.kde.kded", "/modules/konqy_preloader", "org.kde.konqueror.Preloader", QDBusConnection::sessionBus());
        QX11Info info;
        QDBusReply<bool> retVal = ref.call(QDBus::Block, "registerPreloadedKonqy", QDBusConnection::sessionBus().baseService(), info.screen());
        if(!retVal)
            return false; // too many preloaded or failed
        KonqMainWindow* win = new KonqMainWindow; // prepare an empty window too
        // KonqMainWindow ctor sets always the preloaded flag to false, so create the window before this
        KonqMainWindow::setPreloadedFlag(true);
        KonqMainWindow::setPreloadedWindow(win);
        kDebug() << "Konqy preloaded :" << QDBusConnection::sessionBus().baseService();
        return true;
    } else {
        return false; // no preloading
    }
#else
    return false; // no preloading
#endif
}

extern "C" KDE_EXPORT int kdemain(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, KonqFactory::aboutData());


    KCmdLineOptions options;

    options.add("silent", ki18n("Start without a default window, when called without URLs"));

    options.add("preload", ki18n("Preload for later use. This mode does not support URLs on the command line"));

    options.add("profile <profile>", ki18n("Profile to open"));

    options.add("profiles", ki18n("List available profiles"));

    options.add("sessions", ki18n("List available sessions"));

    options.add("open-session <session>", ki18n("Session to open"));

    options.add("mimetype <mimetype>", ki18n("Mimetype to use for this URL (e.g. text/html or inode/directory)"));
    options.add("part <service>", ki18n("Part to use (e.g. khtml or kwebkitpart)"));

    options.add("select", ki18n("For URLs that point to files, opens the directory and selects the file, instead of opening the actual file"));

    options.add("+[URL]", ki18n("Location to open"));

    KCmdLineArgs::addCmdLineOptions(options); // Add our own options.
    KCmdLineArgs::addTempFileOption();

    KonquerorApplication app;
    app.setQuitOnLastWindowClosed(false);

    KGlobal::locale()->insertCatalog("libkonq"); // needed for apps using libkonq

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (app.isSessionRestored()) {
        KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();

        int n = 1;
        while (KonqMainWindow::canBeRestored(n))
        {
            const QString className = KXmlGuiWindow::classNameOfToplevel(n);
            if (className == QLatin1String("KonqMainWindow"))
                (new KonqMainWindow())->restore(n);
            else
                kWarning() << "Unknown class" << className << "in session saved data!" ;
            ++n;
        }
    } else {
        // First the invocations that do not take urls.
        if (args->isSet("preload")) {
            if (!tryPreload())
                return 0; // no preloading
        } else if (args->isSet("profiles")) {
            listProfiles();
            return 0;
        } else if (args->isSet("sessions")) {
            listSessions();
            return 0;
        } else if (args->isSet("open-session")) {
            const QString session = args->getOption("open-session");
            QString sessionPath = session;
            if (!session.startsWith('/')) {
                sessionPath = KStandardDirs::locateLocal("appdata", "sessions/" + session);
            }

            QDirIterator it(sessionPath, QDir::Readable|QDir::Files);
            if (!it.hasNext()) {
                kError() << "session" << session << "not found or empty";
                return -1;
            }

            KonqSessionManager::self()->restoreSessions(sessionPath);
        } else if (args->count() == 0) {
            // No args. If --silent, do nothing, otherwise create a default window.
            if (!args->isSet("silent")) {
                const QString profile = args->getOption("profile");
                KonqMainWindow* mainWin = KonqMisc::createBrowserWindowFromProfile(QString(), profile);
                mainWin->show();
            }
        } else {
            // Now is a good time to parse each argument as a URL.
            KUrl::List urlList;
            for (int i = 0; i < args->count(); i++) {
                // KonqMisc::konqFilteredURL doesn't cope with local files... A bit of hackery below
                const KUrl url = args->url(i);
                if (url.isLocalFile() && QFile::exists(url.toLocalFile())) // "konqueror index.html"
                    urlList += url;
                else
                    urlList += KUrl(KonqMisc::konqFilteredURL(0L, args->arg(i))); // "konqueror slashdot.org"
            }

            QStringList filesToSelect;

            if (args->isSet("select")) {
                // Get all distinct directories from 'files' and open a tab
                // for each directory.
                QList<KUrl> dirs;
                Q_FOREACH(const KUrl& url, urlList) {
                    const KUrl dir(url.directory());
                    if (!dirs.contains(dir)) {
                        dirs.append(dir);
                    }
                }
                filesToSelect = urlList.toStringList();
                urlList = dirs;
            }

            KUrl firstUrl = urlList.takeFirst();

            KParts::OpenUrlArguments urlargs;
            if (args->isSet("mimetype"))
                urlargs.setMimeType(args->getOption("mimetype"));

            KonqOpenURLRequest req;
            req.args = urlargs;
            req.filesToSelect = filesToSelect;
            req.tempFile = KCmdLineArgs::isTempFileSet();
            req.serviceName = args->getOption("part");

            KonqMainWindow * mainwin = 0;
            if (args->isSet("profile")) {
                const QString profile = args->getOption("profile");
                //kDebug() << "main() -> createBrowserWindowFromProfile mimeType=" << urlargs.mimeType();
                mainwin = KonqMisc::createBrowserWindowFromProfile(QString(), profile, firstUrl, req);
            } else {
                mainwin = KonqMisc::createNewWindow(firstUrl, req);
            }
            mainwin->show();
            if (!urlList.isEmpty()) {
                // Open the other urls as tabs in that window
                mainwin->openMultiURL(urlList);
            }
        }
    }
    args->clear();

    app.exec();

    // Delete all KonqMainWindows, so that we don't have
    // any parts loaded when KLibLoader::cleanUp is called.
    // Their deletion was postponed in their event()
    // (and Qt doesn't delete WDestructiveClose widgets on exit anyway :()
    while(KonqMainWindow::mainWindowList() != NULL)
    { // the list will be deleted by last KonqMainWindow
        delete KonqMainWindow::mainWindowList()->first();
    }

    // Notify the session manager that the instance was closed without errors, and normally.
    KonqSessionManager::self()->disableAutosave();
    KonqSessionManager::self()->deleteOwnedSessions();

    return 0;
}
