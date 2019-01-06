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
#include "konqsessionmanager.h"
#include "konqview.h"
#include "konqsettingsxt.h"

#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>

#include <config-konqueror.h>
#include <konqueror-version.h>

#include "konqdebug.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>

#include <KDBusService>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <KStartupInfo>
#include <KWindowSystem>

static void listSessions()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "sessions/";
    QDirIterator it(dir, QDir::Readable | QDir::NoDotAndDotDot | QDir::Dirs);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        printf("%s\n", QFile::encodeName(fileInfo.baseName()).constData());
    }
}

static KonqMainWindow* handleCommandLine(QCommandLineParser &parser, const QString &workingDirectory, int *ret)
{
    *ret = 0;
    const QStringList args = parser.positionalArguments();
    qCDebug(KONQUEROR_LOG) << "args=" << args;
    // First the invocations that do not take urls.
    if (parser.isSet("sessions")) {
        listSessions();
        return nullptr;
    } else if (parser.isSet("open-session")) {
        const QString session = parser.value("open-session");
        QString sessionPath = session;
        if (!session.startsWith('/')) {
            sessionPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QLatin1Char('/') + "sessions/" + session;
        }

        QDirIterator it(sessionPath, QDir::Readable | QDir::Files);
        if (!it.hasNext()) {
            qCWarning(KONQUEROR_LOG) << "session" << session << "not found or empty";
            *ret = 1;
            return nullptr;
        }

        KonqSessionManager::self()->restoreSessions(sessionPath);
        return nullptr;
    }

    // Ask the user to recover session if applicable
    KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();

    if (args.isEmpty()) {
        // No args. If --silent, do nothing, otherwise create a default window.
        if (!parser.isSet("silent")) {
            // If session recovery created some windows, no need for an empty window here.
            QList<KonqMainWindow *> *mainWindowList = KonqMainWindow::mainWindowList();
            if (mainWindowList && !mainWindowList->isEmpty()) {
                return mainWindowList->at(0);
            }

            KonqMainWindow *mainWin = KonqMainWindowFactory::createNewWindow();
            mainWin->show();
            return mainWin;
        }
    } else {
        // Now is a good time to parse each argument as a URL.
        QList<QUrl> urlList;
        for (int i = 0; i < args.count(); i++) {
            // KonqMisc::konqFilteredURL doesn't cope with local files... A bit of hackery below
            const QUrl url = QUrl::fromUserInput(args.at(i), workingDirectory);
            if (url.isLocalFile() && QFile::exists(url.toLocalFile())) { // "konqueror index.html"
                urlList += url;
            } else {
                urlList += KonqMisc::konqFilteredURL(nullptr, args.at(i));    // "konqueror slashdot.org"
            }
        }

        QList<QUrl> filesToSelect;

        if (parser.isSet("select")) {
            // Get all distinct directories from 'files' and open a tab
            // for each directory.
            QList<QUrl> dirs;
            Q_FOREACH (const QUrl &url, urlList) {
                const QUrl dir(url.adjusted(QUrl::RemoveFilename));
                if (!dirs.contains(dir)) {
                    dirs.append(dir);
                }
            }
            filesToSelect = urlList;
            urlList = dirs;
        }

        QUrl firstUrl = urlList.takeFirst();

        KParts::OpenUrlArguments urlargs;
        if (parser.isSet("mimetype")) {
            urlargs.setMimeType(parser.value("mimetype"));
        }

        KonqOpenURLRequest req;
        req.args = urlargs;
        req.filesToSelect = filesToSelect;
        req.tempFile = parser.isSet("tempfile");
        req.serviceName = parser.value("part");

        KonqMainWindow *mainwin = KonqMainWindowFactory::createNewWindow(firstUrl, req);
        mainwin->show();
        if (!urlList.isEmpty()) {
            // Open the other urls as tabs in that window
            mainwin->openMultiURL(urlList);
        }
        return mainwin;
    }
    return nullptr;
}

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // says QtWebEngine
    KonquerorApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("konqueror");

    KAboutData aboutData("konqueror", i18n("Konqueror"), KONQUEROR_VERSION);
    aboutData.setShortDescription(i18n("Web browser, file manager and document viewer."));
    aboutData.addLicense(KAboutLicense::GPL_V2);
    aboutData.setCopyrightStatement(i18n("(C) 1999-2016, The Konqueror developers"));
    aboutData.setHomepage("https://konqueror.org/");

    aboutData.addAuthor(i18n("Stefano Crocco"), i18n("Current maintainer"), "stefano.crocco@alice.it");
    aboutData.addAuthor(i18n("David Faure"), i18n("Developer (framework, parts, JavaScript, I/O library) and former maintainer"), "faure@kde.org");
    aboutData.addAuthor(i18n("Simon Hausmann"), i18n("Developer (framework, parts)"), "hausmann@kde.org");
    aboutData.addAuthor(i18n("Michael Reiher"), i18n("Developer (framework)"), "michael.reiher@gmx.de");
    aboutData.addAuthor(i18n("Matthias Welk"), i18n("Developer"), "welk@fokus.gmd.de");
    aboutData.addAuthor(i18n("Alexander Neundorf"), i18n("Developer (List views)"), "neundorf@kde.org");
    aboutData.addAuthor(i18n("Michael Brade"), i18n("Developer (List views, I/O library)"), "brade@kde.org");
    aboutData.addAuthor(i18n("Lars Knoll"), i18n("Developer (HTML rendering engine)"), "knoll@kde.org");
    aboutData.addAuthor(i18n("Dirk Mueller"), i18n("Developer (HTML rendering engine)"), "mueller@kde.org");
    aboutData.addAuthor(i18n("Peter Kelly"), i18n("Developer (HTML rendering engine)"), "pmk@post.com");
    aboutData.addAuthor(i18n("Waldo Bastian"), i18n("Developer (HTML rendering engine, I/O library)"), "bastian@kde.org");
    aboutData.addAuthor(i18n("Germain Garand"), i18n("Developer (HTML rendering engine)"), "germain@ebooksfrance.org");
    aboutData.addAuthor(i18n("Leo Savernik"), i18n("Developer (HTML rendering engine)"), "l.savernik@aon.at");
    aboutData.addAuthor(i18n("Stephan Kulow"), i18n("Developer (HTML rendering engine, I/O library, regression test framework)"), "coolo@kde.org");
    aboutData.addAuthor(i18n("Antti Koivisto"), i18n("Developer (HTML rendering engine)"), "koivisto@kde.org");
    aboutData.addAuthor(i18n("Zack Rusin"),  i18n("Developer (HTML rendering engine)"), "zack@kde.org");
    aboutData.addAuthor(i18n("Tobias Anton"), i18n("Developer (HTML rendering engine)"), "anton@stud.fbi.fh-darmstadt.de");
    aboutData.addAuthor(i18n("Lubos Lunak"), i18n("Developer (HTML rendering engine)"), "l.lunak@kde.org");
    aboutData.addAuthor(i18n("Maks Orlovich"), i18n("Developer (HTML rendering engine, JavaScript)"), "maksim@kde.org");
    aboutData.addAuthor(i18n("Allan Sandfeld Jensen"), i18n("Developer (HTML rendering engine)"), "kde@carewolf.com");
    aboutData.addAuthor(i18n("Apple Safari Developers"), i18n("Developer (HTML rendering engine, JavaScript)"));
    aboutData.addAuthor(i18n("Harri Porten"), i18n("Developer (JavaScript)"), "porten@kde.org");
    aboutData.addAuthor(i18n("Koos Vriezen"), i18n("Developer (Java applets and other embedded objects)"), "koos.vriezen@xs4all.nl");
    aboutData.addAuthor(i18n("Matt Koss"), i18n("Developer (I/O library)"), "koss@miesto.sk");
    aboutData.addAuthor(i18n("Alex Zepeda"), i18n("Developer (I/O library)"), "zipzippy@sonic.net");
    aboutData.addAuthor(i18n("Richard Moore"), i18n("Developer (Java applet support)"), "rich@kde.org");
    aboutData.addAuthor(i18n("Dima Rogozin"), i18n("Developer (Java applet support)"), "dima@mercury.co.il");
    aboutData.addAuthor(i18n("Wynn Wilkes"), i18n("Developer (Java 2 security manager support,\n and other major improvements to applet support)"), "wynnw@calderasystems.com");
    aboutData.addAuthor(i18n("Stefan Schimanski"), i18n("Developer (Netscape plugin support)"), "schimmi@kde.org");
    aboutData.addAuthor(i18n("George Staikos"), i18n("Developer (SSL, Netscape plugins)"), "staikos@kde.org");
    aboutData.addAuthor(i18n("Dawit Alemayehu"), i18n("Developer (I/O library, Authentication support)"), "adawit@kde.org");
    aboutData.addAuthor(i18n("Carsten Pfeiffer"), i18n("Developer (framework)"), "pfeiffer@kde.org");
    aboutData.addAuthor(i18n("Torsten Rahn"), i18n("Graphics/icons"), "torsten@kde.org");
    aboutData.addAuthor(i18n("Torben Weis"), i18n("KFM author"), "weis@kde.org");
    aboutData.addAuthor(i18n("Joseph Wenninger"), i18n("Developer (navigation panel framework)"), "jowenn@kde.org");
    aboutData.addAuthor(i18n("Stephan Binner"), i18n("Developer (misc stuff)"), "binner@kde.org");
    aboutData.addAuthor(i18n("Ivor Hewitt"), i18n("Developer (AdBlock filter)"), "ivor@ivor.org");
    aboutData.addAuthor(i18n("Eduardo Robles Elvira"), i18n("Developer (framework)"), "edulix@gmail.com");

    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    aboutData.setupCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("silent")}, i18n("Start without a default window, when called without URLs")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("preload")}, i18n("Preload for later use. This mode does not support URLs on the command line")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("profile")}, i18n("Profile to open (DEPRECATED, IGNORED)"), i18n("profile")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("sessions")}, i18n("List available sessions")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("open-session")}, i18n("Session to open"), i18n("session")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("mimetype")}, i18n("Mimetype to use for this URL (e.g. text/html or inode/directory)"), i18n("mimetype")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("part")}, i18n("Part to use (e.g. khtml or kwebkitpart)"), i18n("service")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("select")}, i18n("For URLs that point to files, opens the directory and selects the file, instead of opening the actual file")));
    parser.addOption(QCommandLineOption(QStringList{QStringLiteral("tempfile")}, i18n("The files/URLs opened by the application will be deleted after use")));

    parser.addPositionalArgument(QStringLiteral("[URL]"), i18n("Location to open"));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService dbusService(KDBusService::Unique);
    QObject::connect(&dbusService, &KDBusService::activateRequested, [&parser](const QStringList &arguments, const QString &workingDirectory) {
        parser.parse(arguments);
        int ret;
        KonqMainWindow *mainWindow = handleCommandLine(parser, workingDirectory, &ret);
        if (mainWindow) {
            // terminate startup notification and activate the mainwindow:
            KStartupInfo::setNewStartupId(mainWindow, KStartupInfo::startupId());
            KWindowSystem::forceActiveWindow(mainWindow->winId());
        }
    });

    if (app.isSessionRestored()) {
        KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();

        int n = 1;
        while (KonqMainWindow::canBeRestored(n)) {
            const QString className = KXmlGuiWindow::classNameOfToplevel(n);
            if (className == QLatin1String("KonqMainWindow")) {
                (new KonqMainWindow())->restore(n);
            } else {
                qCWarning(KONQUEROR_LOG) << "Unknown class" << className << "in session saved data!";
            }
            ++n;
        }
    } else if (parser.isSet("preload")) {
        new KonqMainWindow(QUrl(QStringLiteral("about:blank"))); // prepare an empty window, with the web renderer preloaded
    } else {
        int ret = 0;
        KonqMainWindow *mainWindow = handleCommandLine(parser, QDir::currentPath(), &ret);
        if (!mainWindow) {
            return ret;
        }
    }

    const int ret = app.exec();

    // Delete all KonqMainWindows, so that we don't have
    // any parts loaded when KLibLoader::cleanUp is called.
    // (and Qt doesn't delete WA_DeleteOnClose widgets on exit anyway :()
    while (KonqMainWindow::mainWindowList() != nullptr) {
        // the list will be deleted by last KonqMainWindow
        delete KonqMainWindow::mainWindowList()->first();
    }

    // Notify the session manager that the instance was closed without errors, and normally.
    KonqSessionManager::self()->disableAutosave();
    KonqSessionManager::self()->deleteOwnedSessions();

    return ret;
}
