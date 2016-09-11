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
#include <kcmdlineargs.h>
#include <KLocalizedString>
#include <kprocess.h>
#include "../src/config-konqueror.h"

#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kmimetype.h>
#include <kdebug.h>
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

#include <QDir>
#include <QMimeDatabase>
#include <QRegExp>
#include <QUrl>
#include <QStandardPaths>

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#if KONQ_HAVE_X11
#include <X11/Xlib.h>
#include <QX11Info>
#endif

static const char appName[] = "kfmclient";
static const char programName[] = I18N_NOOP("kfmclient");
static const char description[] = I18N_NOOP("KDE tool for opening URLs from the command line");
static const char version[] = "2.0";

QByteArray ClientApp::startup_id_str;
bool ClientApp::m_ok = true;
bool s_interactive = true;

K_GLOBAL_STATIC_WITH_ARGS(KComponentData, s_instance, ("kfmclient"))

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, appName, 0, ki18n(programName), version, ki18n(description));

    KCmdLineOptions options;

    options.add("noninteractive", ki18n("Non interactive use: no message boxes"));

    options.add("commands", ki18n("Show available commands"));

    options.add("+command", ki18n("Command (see --commands)"));

    options.add("+[URL(s)]", ki18n("Arguments for command"));

    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs::addTempFileOption();

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (argc == 1 || args->isSet("commands")) {
        KCmdLineArgs::enable_i18n();
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

        puts(i18n("  kfmclient openProfile 'profile' ['url']\n"
                  "            # Opens a window using the given profile.\n"
                  "            #   'profile' is a file under ~/.kde/share/apps/konqueror/profiles.\n"
                  "            #   'url' is an optional URL to open.\n\n").toLocal8Bit());

        puts(i18n("  kfmclient exec is deprecated and kept for compatibility with KDE 3. \n"
                  "            # See kioclient exec for more information.\n").toLocal8Bit());

        return 0;
    }

    // Use kfmclient from the session KDE version
    if ((args->arg(0) == "openURL" || args->arg(0) == "newTab")
            && qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
        int version = KCOREADDONS_VERSION_MAJOR;
        if (!qEnvironmentVariableIsSet("KDE_SESSION_VERSION")) {  // this is KDE3
            version = 3;
        } else {
            version = atoi(getenv("KDE_SESSION_VERSION"));
        }
        if (version != 0 && version != KCOREADDONS_VERSION_MAJOR) {
            kDebug() << "Forwarding to kfmclient from KDE version " << version;
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

    // ClientApp internally uses KConfig and hence needs a valid KComponentData
    return ClientApp::doIt() ? 0 /*no error*/ : 1 /*error*/;
}

static int currentScreen()
{
#if KONQ_HAVE_X11
    return QX11Info::appScreen();
#endif
    return 0;
}

static bool s_dbus_initialized = false;
static void needDBus()
{
    if (!s_dbus_initialized) {
        extern void qDBusBindToApplication();
        qDBusBindToApplication();
        if (!QDBusConnection::sessionBus().isConnected()) {
            kFatal(101) << "Session bus not found";
        }
        s_dbus_initialized = true;
    }
}

// when reusing a preloaded konqy, make sure your always use a DBus call which opens a profile !

static QString getPreloadedKonqy()
{
    KConfig konqCfg(QLatin1String("konquerorrc"));
    const KConfigGroup reusingGroup(&konqCfg, "Reusing");
    if (reusingGroup.readEntry("MaxPreloadCount", 1) == 0) {
        return QString();
    }
    needDBus();
    QDBusInterface ref("org.kde.kded5", "/modules/konqy_preloader", "org.kde.konqueror.Preloader", QDBusConnection::sessionBus());
    QDBusReply<QString> reply = ref.call("getPreloadedKonqy", currentScreen());
    if (reply.isValid()) {
        return reply;
    }
    return QString();
}

static QUrl filteredUrl(KCmdLineArgs *args)
{
    if (args) {
        KUriFilterData data;
        data.setData(args->arg(1));
        data.setAbsolutePath(args->cwd());
        data.setCheckForExecutables(false);

        if (KUriFilter::self()->filterUri(data) && data.uriType() != KUriFilterData::Error) {
            return data.uri();
        }
    }
    return QUrl();
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

static bool krun_has_error = false;

bool ClientApp::createNewWindow(const QUrl &url, bool newTab, bool tempFile, const QString &mimetype)
{
    qDebug() << url << "mimetype=" << mimetype;
    Q_ASSERT(qApp);

    if (url.scheme().startsWith(QLatin1String("http"))) {
        KConfig config(QLatin1String("kfmclientrc"));
        KConfigGroup generalGroup(&config, "General");
        const QString browserApp = generalGroup.readEntry("BrowserApplication");
        if (!browserApp.isEmpty() && !browserApp.startsWith("!kfmclient")
                && (browserApp.startsWith('!') || KService::serviceByStorageId(browserApp))) {
            qDebug() << "Using external browser" << browserApp;
            KStartupInfo::appStarted();

            // TODO we don't handle tempFile here, but most likely the external browser doesn't support it,
            // so we should sleep and delete it ourselves....
            KGlobal::setAllowQuit(true);
            KRun *run = new KRun(url, 0, false /* no progress window */);
            QObject::connect(run, SIGNAL(finished()), qApp, SLOT(delayedQuit()));
            QObject::connect(run, SIGNAL(error()), qApp, SLOT(delayedQuit()));
            qApp->exec();
            return !krun_has_error;
        }
    }

    needDBus();
    QDBusConnection dbus = QDBusConnection::sessionBus();
    KConfig cfg(QLatin1String("konquerorrc"));
    KConfigGroup fmSettings = cfg.group("FMSettings");
    if (newTab || fmSettings.readEntry("KonquerorTabforExternalURL", false)) {

        QString foundApp;
        QDBusObjectPath foundObj;
        QDBusReply<QStringList> reply = dbus.interface()->registeredServiceNames();
        if (reply.isValid()) {
            const QStringList allServices = reply;
            for (QStringList::const_iterator it = allServices.begin(), end = allServices.end(); it != end; ++it) {
                const QString service = *it;
                if (service.startsWith("org.kde.konqueror")) {
                    org::kde::Konqueror::Main konq(service, "/KonqMain", dbus);
                    QDBusReply<QDBusObjectPath> windowReply = konq.windowForTab();
                    if (windowReply.isValid()) {
                        QDBusObjectPath path = windowReply;
                        // "/" is the indicator for "no object found", since we can't use an empty path
                        if (path.path() != "/") {
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

    QString appId = getPreloadedKonqy();
    if (!appId.isEmpty()) {
        qDebug() << "ClientApp::createNewWindow using existing konqueror" << appId;
        org::kde::Konqueror::Main konq(appId, "/KonqMain", dbus);
        konq.createNewWindow(url.url(), mimetype, startup_id_str, tempFile);
        sendASNChange();
    } else {
        QString error;
        /* Well, we can't pass a mimetype through startServiceByDesktopPath !
        if ( KToolInvocation::startServiceByDesktopPath( QLatin1String("konqueror.desktop"),
                                                      url.url(), &error ) > 0 )
        {
            kError() << "Couldn't start konqueror from konqueror.desktop: " << error << endl;
            */
        // pass kfmclient's startup id to konqueror using kshell
        KStartupInfoId id;
        id.initId(startup_id_str);
        id.setupStartupEnv();
        QStringList args;
        args << QLatin1String("konqueror");
        if (!mimetype.isEmpty()) {
            args << "-mimetype" << mimetype;
        }
        if (tempFile) {
            args << "-tempfile";
        }
        args << url.url();
#ifdef Q_OS_WIN
        const int pid = KProcess::startDetached(QLatin1String("kwrapper5"), args);
#else
        const int pid = KProcess::startDetached(QLatin1String("kshell5"), args);
#endif
        KStartupInfo::resetStartupEnv();
        qDebug() << "ClientApp::createNewWindow KProcess started, pid=" << pid;
        //}
    }
    return true;
}

bool ClientApp::openProfile(const QString &profileName, const QString &url, const QString &mimetype)
{
    QString appId = getPreloadedKonqy();
    if (appId.isEmpty()) {
        QString error;
        if (KToolInvocation::startServiceByDesktopPath(QLatin1String("konqueror.desktop"),
                QLatin1String("--silent"), &error, &appId, NULL, startup_id_str) > 0) {
            kError() << "Couldn't start konqueror from konqueror.desktop: " << error << endl;
            return false;
        }
        // startServiceByDesktopPath waits for the app to register with DBus
        // so when we arrive here, konq is up and running already, and appId contains the identification
    }

    QString profile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("konqueror/profiles/") + profileName);
    if (profile.isEmpty()) {
        fprintf(stderr, "%s: %s", programName, i18n("Profile '%1' not found\n", profileName).toLocal8Bit().data());
        ::exit(0);
    }
    needDBus();
    org::kde::Konqueror::Main konqy(appId, "/KonqMain", QDBusConnection::sessionBus());
    if (url.isEmpty()) {
        konqy.createBrowserWindowFromProfile(profile, profileName, startup_id_str);
    } else if (mimetype.isEmpty()) {
        konqy.createBrowserWindowFromProfileAndUrl(profile, profileName, url, startup_id_str);
    } else {
        konqy.createBrowserWindowFromProfileUrlAndMimeType(profile, profileName, url, mimetype, startup_id_str);
    }
    sleep(2); // Martin Schenk <martin@schenk.com> says this is necessary to let the server read from the socket
    // ######## so those methods should probably not be ASYNC
    sendASNChange();
    return true;
}

void ClientApp::delayedQuit()
{
    // Quit in 2 seconds. This leaves time for KRun to pop up
    // "app not found" in KProcessRunner, if that was the case.
    QTimer::singleShot(2000, this, SLOT(deref()));
    // don't access the KRun instance later, it will be deleted after calling slots
    if (static_cast< const KRun * >(sender())->hasError()) {
        krun_has_error = true;
    }
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

bool ClientApp::doIt()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    int argc = args->count();
    checkArgumentCount(argc, 1, 0);

    if (!args->isSet("ninteractive")) {
        s_interactive = false;
    }
    QString command = args->arg(0);

    // read ASN env. variable for non-KApp cases
    startup_id_str = KStartupInfo::currentStartupIdEnv().id();

    kDebug() << "Creating ClientApp";
    int fake_argc = 1;
    char *(fake_argv[]) = {"kfmclient"};
    ClientApp app(fake_argc, fake_argv);

    if (command == "openURL" || command == "newTab") {
        checkArgumentCount(argc, 1, 3);
        bool tempFile = KCmdLineArgs::isTempFileSet();
        if (argc == 1) {
            return createNewWindow(QUrl::fromLocalFile(QDir::homePath()), command == "newTab", tempFile);
        }
        if (argc == 2) {
            return createNewWindow(filteredUrl(args), command == "newTab", tempFile);
        }
        if (argc == 3) {
            return createNewWindow(filteredUrl(args), command == "newTab", tempFile, args->arg(2));
        }
    } else if (command == "openProfile") {
        checkArgumentCount(argc, 2, 3);
        QString url;
        if (argc == 3) {
            url = args->url(2).url();
        }
        return openProfile(args->arg(1), url);
    } else if (command == "exec" && argc >= 2) {
        // compatibility with KDE 3 and xdg-open
        QStringList kioclientArgs;
        if (!s_interactive) {
            kioclientArgs << QLatin1String("--noninteractive");
        }
        kioclientArgs << "exec" << args->arg(1);
        if (argc == 3) {
            kioclientArgs << args->arg(2);
        }

        int ret = KProcess::execute("kioclient5", kioclientArgs);
        return ret == 0;
    } else {
        fprintf(stderr, "%s: %s", programName, i18n("Syntax error, unknown command '%1'\n", command).toLocal8Bit().data());
        return false;
    }
    return true;
}

void ClientApp::slotResult(KJob *job)
{
    if (job->error() && s_interactive) {
        KJobWidgets::setWindow(job, 0);
        static_cast<KIO::Job *>(job)->ui()->showErrorMessage();
    }
    m_ok = !job->error();
    quit();
}

void ClientApp::slotDialogCanceled()
{
    m_ok = false;
    quit();
}

