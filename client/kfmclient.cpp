/* This file is part of the KDE project
   Copyright (C) 1999-2006 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfmclient.h"

#include <ktoolinvocation.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>

#include <KLocalizedString>
#include <kprocess.h>
#include "../src/config-konqueror.h"

#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kmimetype.h>
#include <kservice.h>
#include <krun.h>
#include <kcomponentdata.h>
#include <KStartupInfoId>
#include <kurifilter.h>
#include <KConfigGroup>
#include <KJobWidgets>
#include <KService>
#include <KWindowSystem>

#include <kcoreaddons_version.h>

#include <konq_mainwindow_interface.h>
#include <konq_main_interface.h>

#include <QApplication>
#include <QDir>
#include <QMimeDatabase>
#include <QUrl>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QCommandLineOption>

#if KONQ_HAVE_X11
#include <QX11Info>
#endif

static const char appName[] = "kfmclient";
static const char programName[] = I18N_NOOP("kfmclient");
static const char description[] = I18N_NOOP("KDE tool for opening URLs from the command line");
static const char version[] = "2.0";

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QApplication app(argc, argv);

    KAboutData aboutData(appName, i18n(programName), QLatin1String(version));
    aboutData.setShortDescription(i18n(description));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    //qDebug() << "kfmclient starting" << QTime::currentTime();

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("noninteractive"), i18n("Non interactive use: no message boxes")));

    parser.addOption(QCommandLineOption(QStringList() << QLatin1String("commands"), i18n("Show available commands")));

    parser.addPositionalArgument(QLatin1String("command"), i18n("Command (see --commands)"));

    parser.addPositionalArgument(QLatin1String("[URL(s)]"), i18n("Arguments for command"));

    parser.addOption(QCommandLineOption(QStringList{"tempfile"}, i18n("The files/URLs opened by the application will be deleted after use")));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || parser.isSet("commands")) {
        puts(i18n("\nSyntax:\n").toLocal8Bit());
        puts(i18n("  kfmclient openURL 'url' ['mimetype']\n"
                  "            # Opens a window showing 'url'.\n"
                  "            #  'url' may be a relative path\n"
                  "            #   or file name, such as . or subdir/\n"
                  "            #   If 'url' is omitted, $HOME is used instead.\n\n").toLocal8Bit());
        puts(i18n("            # If 'mimetype' is specified, it will be used to determine the\n"
                  "            #   component that Konqueror should use. For instance, set it to\n"
                  "            #   text/html for a web page, to make it appear faster\n\n").toLocal8Bit());

        puts(i18n("  kfmclient newTab 'url' ['mimetype']\n"
                  "            # Same as above but opens a new tab with 'url' in an existing Konqueror\n"
                  "            #   window on the current active desktop if possible.\n\n").toLocal8Bit());

        return 0;
    }

    // Use kfmclient from the session KDE version
    if ((args.at(0) == QLatin1String("openURL") || args.at(0) == QLatin1String("newTab"))
            && qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
        const int version = atoi(getenv("KDE_SESSION_VERSION"));
        if (version != 0 && version != KCOREADDONS_VERSION_MAJOR) {
            qDebug() << "Forwarding to kfmclient from KDE version " << version;
            char wrapper[ 10 ];
            sprintf(wrapper, "kde%d", version);
            char **newargv = new char *[ argc + 2 ];
            newargv[ 0 ] = wrapper;
            for (int i = 0;
                    i < argc;
                    ++i) {
                newargv[ i + 1 ] = argv[ i ];
            }
            newargv[ argc + 1 ] = NULL;
            execvp(wrapper, newargv);
            // just continue if failed
        }
    }

    ClientApp client;
    return client.doIt(parser) ? 0 /*no error*/ : 1 /*error*/;
}

static bool s_dbus_initialized = false;
static void needDBus()
{
    if (!s_dbus_initialized) {
        extern void qDBusBindToApplication();
        qDBusBindToApplication();
        if (!QDBusConnection::sessionBus().isConnected()) {
            qFatal("Session bus not found");
        }
        s_dbus_initialized = true;
    }
}

// keep in sync with konqpreloadinghandler.cpp
static const char s_preloadDBusName[] = "org.kde.konqueror.preloaded";

static QUrl filteredUrl(const QString &url)
{
    KUriFilterData data;
    data.setData(url);
    data.setAbsolutePath(QDir::currentPath());
    data.setCheckForExecutables(false);

    if (KUriFilter::self()->filterUri(data) && data.uriType() != KUriFilterData::Error) {
        return data.uri();
    }
    return QUrl();
}

ClientApp::ClientApp()
{
}

void ClientApp::sendASNChange()
{
#if KONQ_HAVE_X11
    if (KWindowSystem::platform() == KWindowSystem::Platform::X11) {
        KStartupInfoId id;
        id.initId(startup_id_str);
        KStartupInfoData data;
        data.addPid(0);     // say there's another process for this ASN with unknown PID
        data.setHostname(); // ( no need to bother to get this konqy's PID )
        KStartupInfo::sendChangeXcb(QX11Info::connection(), QX11Info::appScreen(), id, data);
    }
#endif
}

bool ClientApp::createNewWindow(const QUrl &url, bool newTab, bool tempFile, const QString &mimetype)
{
    qDebug() << url << "mimetype=" << mimetype;

    if (url.scheme().startsWith(QLatin1String("http"))) {
        KConfig config(QStringLiteral("kfmclientrc"));
        KConfigGroup generalGroup(&config, "General");
        const QString browserApp = generalGroup.readEntry("BrowserApplication");
        if (!browserApp.isEmpty() && !browserApp.startsWith(QLatin1String("!kfmclient"))
                && (browserApp.startsWith('!') || KService::serviceByStorageId(browserApp))) {
            qDebug() << "Using external browser" << browserApp;
            KStartupInfo::appStarted();

            // TODO we don't handle tempFile here, but most likely the external browser doesn't support it,
            // so we should sleep and delete it ourselves....
            KRun *run = new KRun(url, 0, false /* no progress window */);
            QObject::connect(run, &KRun::finished, this, &ClientApp::delayedQuit);
            QObject::connect(run, &KRun::error, this, [](){ qApp->exit(1); });
            return qApp->exec();
        }
    }

    needDBus();
    QDBusConnection dbus = QDBusConnection::sessionBus();
    KConfig cfg(QStringLiteral("konquerorrc"));
    KConfigGroup fmSettings = cfg.group("FMSettings");
    if (newTab || fmSettings.readEntry("KonquerorTabforExternalURL", false)) {

        QString foundApp;
        QDBusObjectPath foundObj;
        QDBusReply<QStringList> reply = dbus.interface()->registeredServiceNames();
        if (reply.isValid()) {
            const QStringList allServices = reply;
            for (QStringList::const_iterator it = allServices.begin(), end = allServices.end(); it != end; ++it) {
                const QString service = *it;
                if (service.startsWith(QLatin1String("org.kde.konqueror"))) {
                    org::kde::Konqueror::Main konq(service, QStringLiteral("/KonqMain"), dbus);
                    QDBusReply<QDBusObjectPath> windowReply = konq.windowForTab();
                    if (windowReply.isValid()) {
                        QDBusObjectPath path = windowReply;
                        // "/" is the indicator for "no object found", since we can't use an empty path
                        if (path.path() != QLatin1String("/")) {
                            foundApp = service;
                            foundObj = path;
                        }
                    }
                }
            }
        }

        if (!foundApp.isEmpty()) {
            org::kde::Konqueror::MainWindow konqWindow(foundApp, foundObj.path(), dbus);
            QDBusReply<void> newTabReply = konqWindow.newTabASNWithMimeType(url.url(), mimetype, startup_id_str, tempFile);
            if (newTabReply.isValid()) {
                sendASNChange();
                return true;
            }
        }
    }

    const QString appId = QString::fromLatin1(s_preloadDBusName);
    org::kde::Konqueror::Main konq(appId, QStringLiteral("/KonqMain"), dbus);
    QDBusReply<QDBusObjectPath> reply = konq.createNewWindow(url.url(), mimetype, startup_id_str, tempFile);
    if (reply.isValid()) {
        sendASNChange();
    } else {
        QString error;
        // pass kfmclient's startup id to konqueror using kshell
        KStartupInfoId id;
        id.initId(startup_id_str);
        id.setupStartupEnv();
        QStringList args;
        args << QStringLiteral("konqueror");
        if (!mimetype.isEmpty()) {
            args << QStringLiteral("-mimetype") << mimetype;
        }
        if (tempFile) {
            args << QStringLiteral("-tempfile");
        }
        args << url.url();
#ifdef Q_OS_WIN
        const int pid = KProcess::startDetached(QLatin1String("kwrapper5"), args);
#else
        const int pid = KProcess::startDetached(QStringLiteral("kshell5"), args);
#endif
        KStartupInfo::resetStartupEnv();
        qDebug() << "ClientApp::createNewWindow KProcess started, pid=" << pid;
    }
    return true;
}

bool ClientApp::openProfile(const QString &profileName, const QUrl &url, const QString &mimetype)
{
    Q_UNUSED(profileName); // the concept disappeared
    return createNewWindow(url, false, false, mimetype);
}

void ClientApp::delayedQuit()
{
    // Quit in 2 seconds. This leaves time for KRun to pop up
    // "app not found" in KProcessRunner, if that was the case.
    QTimer::singleShot(2000, qApp, SLOT(quit()));
}

static void checkArgumentCount(int count, int min, int max)
{
    if (count < min) {
        fprintf(stderr, "%s: %s",  programName, i18n("Syntax error, not enough arguments\n").toLocal8Bit().data());
        ::exit(1);
    }
    if (max && (count > max)) {
        fprintf(stderr, "%s: %s", programName, i18n("Syntax error, too many arguments\n").toLocal8Bit().data());
        ::exit(1);
    }
}

bool ClientApp::doIt(const QCommandLineParser &parser)
{
    const QStringList args = parser.positionalArguments();
    int argc = args.count();
    checkArgumentCount(argc, 1, 0);

    if (!parser.isSet("noninteractive")) {
        m_interactive = false;
    }
    QString command = args.at(0);

    // read ASN env. variable
    startup_id_str = KStartupInfo::currentStartupIdEnv().id();

    if (command == QLatin1String("openURL") || command == QLatin1String("newTab")) {
        checkArgumentCount(argc, 1, 3);
        const bool tempFile = parser.isSet("tempfile");
        if (argc == 1) {
            return createNewWindow(QUrl::fromLocalFile(QDir::homePath()), command == QLatin1String("newTab"), tempFile);
        }
        if (argc == 2) {
            return createNewWindow(filteredUrl(args.at(1)), command == QLatin1String("newTab"), tempFile);
        }
        if (argc == 3) {
            return createNewWindow(filteredUrl(args.at(1)), command == QLatin1String("newTab"), tempFile, args.at(2));
        }
    } else if (command == QLatin1String("openProfile")) { // deprecated command, kept for compat
        checkArgumentCount(argc, 2, 3);
        QUrl url;
        if (argc == 3) {
            url = QUrl::fromUserInput(args.at(2), QDir::currentPath());
        }
        return openProfile(args.at(1), url);
    } else if (command == QLatin1String("exec") && argc >= 2) {
        // compatibility with KDE 3 and xdg-open
        QStringList kioclientArgs;
        if (!m_interactive) {
            kioclientArgs << QStringLiteral("--noninteractive");
        }
        kioclientArgs << QStringLiteral("exec") << args.at(1);
        if (argc == 3) {
            kioclientArgs << args.at(2);
        }

        int ret = KProcess::execute(QStringLiteral("kioclient5"), kioclientArgs);
        return ret == 0;
    } else {
        fprintf(stderr, "%s: %s", programName, i18n("Syntax error, unknown command '%1'\n", command).toLocal8Bit().data());
        return false;
    }
    return true;
}

void ClientApp::slotResult(KJob *job)
{
    if (job->error() && m_interactive) {
        KJobWidgets::setWindow(job, 0);
        static_cast<KIO::Job *>(job)->ui()->showErrorMessage();
    }
    const bool ok = !job->error();
    qApp->exit(ok ? 0 : 1);
}

