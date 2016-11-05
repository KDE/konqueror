/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2016 David Faure <faure@kde.org>

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
#include "konqmainwindowfactory.h"
#include "konqpreloadinghandler.h"
#include "konqsessionmanager.h"
#include "konqview.h"
#include "konqsettingsxt.h"

#include <KLocalizedString>
#include <kcmdlineargs.h>

#include <config-konqueror.h>

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>

#include <KDBusAddons/KDBusService>

static void listSessions()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "sessions/";
    QDirIterator it(dir, QDir::Readable | QDir::NoDotAndDotDot | QDir::Dirs);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        printf("%s\n", QFile::encodeName(fileInfo.baseName()).constData());
    }
}

static KonqPreloadingHandler s_preloadingHandler;

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, KonqFactory::aboutData());

    KCmdLineOptions options;

    options.add("silent", ki18n("Start without a default window, when called without URLs"));

    options.add("preload", ki18n("Preload for later use. This mode does not support URLs on the command line"));

    options.add("profile <profile>", ki18n("Profile to open (DEPRECATED, IGNORED)"));

    options.add("sessions", ki18n("List available sessions"));

    options.add("open-session <session>", ki18n("Session to open"));

    options.add("mimetype <mimetype>", ki18n("Mimetype to use for this URL (e.g. text/html or inode/directory)"));
    options.add("part <service>", ki18n("Part to use (e.g. khtml or kwebkitpart)"));

    options.add("select", ki18n("For URLs that point to files, opens the directory and selects the file, instead of opening the actual file"));

    options.add("+[URL]", ki18n("Location to open"));

    KCmdLineArgs::addCmdLineOptions(options); // Add our own options.
    KCmdLineArgs::addTempFileOption();

    KonquerorApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("konqueror");

    KDBusService dbusService(KDBusService::Multiple);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (app.isSessionRestored()) {
        KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();

        int n = 1;
        while (KonqMainWindow::canBeRestored(n)) {
            const QString className = KXmlGuiWindow::classNameOfToplevel(n);
            if (className == QLatin1String("KonqMainWindow")) {
                (new KonqMainWindow())->restore(n);
            } else {
                qWarning() << "Unknown class" << className << "in session saved data!";
            }
            ++n;
        }
    } else {
        // First the invocations that do not take urls.
        if (args->isSet("preload")) {
            if (!s_preloadingHandler.registerAsPreloaded()) {
                return 0;    // no preloading
            }
        } else if (args->isSet("sessions")) {
            listSessions();
            return 0;
        } else if (args->isSet("open-session")) {
            const QString session = args->getOption("open-session");
            QString sessionPath = session;
            if (!session.startsWith('/')) {
                sessionPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "sessions/" + session;
            }

            QDirIterator it(sessionPath, QDir::Readable | QDir::Files);
            if (!it.hasNext()) {
                qWarning() << "session" << session << "not found or empty";
                return -1;
            }

            KonqSessionManager::self()->restoreSessions(sessionPath);
        } else if (args->count() == 0) {
            // No args. If --silent, do nothing, otherwise create a default window.
            if (!args->isSet("silent")) {
                KonqMainWindow *mainWin = KonqMainWindowFactory::createNewWindow();
                mainWin->show();
            }
        } else {
            // Now is a good time to parse each argument as a URL.
            QList<QUrl> urlList;
            for (int i = 0; i < args->count(); i++) {
                // KonqMisc::konqFilteredURL doesn't cope with local files... A bit of hackery below
                const QUrl url = args->url(i);
                if (url.isLocalFile() && QFile::exists(url.toLocalFile())) { // "konqueror index.html"
                    urlList += url;
                } else {
                    urlList += KonqMisc::konqFilteredURL(0L, args->arg(i));    // "konqueror slashdot.org"
                }
            }

            QStringList filesToSelect;

            if (args->isSet("select")) {
                // Get all distinct directories from 'files' and open a tab
                // for each directory.
                QList<QUrl> dirs;
                Q_FOREACH (const QUrl &url, urlList) {
                    const QUrl dir(url.adjusted(QUrl::RemoveFilename).path());
                    if (!dirs.contains(dir)) {
                        dirs.append(dir);
                    }
                }
                foreach (const QUrl &url, urlList) {
                    filesToSelect << url.url();
                }

                urlList = dirs;
            }

            QUrl firstUrl = urlList.takeFirst();

            KParts::OpenUrlArguments urlargs;
            if (args->isSet("mimetype")) {
                urlargs.setMimeType(args->getOption("mimetype"));
            }

            KonqOpenURLRequest req;
            req.args = urlargs;
            req.filesToSelect = filesToSelect;
            req.tempFile = KCmdLineArgs::isTempFileSet();
            req.serviceName = args->getOption("part");

            KonqMainWindow *mainwin = KonqMainWindowFactory::createNewWindow(firstUrl, req);
            mainwin->show();
            if (!urlList.isEmpty()) {
                // Open the other urls as tabs in that window
                mainwin->openMultiURL(urlList);
            }
        }
    }
    args->clear();

    // In case there is no `konqueror --preload` running, start one
    // (not this process, it might exit before the preloading is used)
    s_preloadingHandler.ensurePreloadedProcessExists();

    const int ret = app.exec();

    // Delete all KonqMainWindows, so that we don't have
    // any parts loaded when KLibLoader::cleanUp is called.
    // (and Qt doesn't delete WA_DeleteOnClose widgets on exit anyway :()
    while (KonqMainWindow::mainWindowList() != NULL) {
        // the list will be deleted by last KonqMainWindow
        delete KonqMainWindow::mainWindowList()->first();
    }

    // Notify the session manager that the instance was closed without errors, and normally.
    KonqSessionManager::self()->disableAutosave();
    KonqSessionManager::self()->deleteOwnedSessions();

    return ret;
}
