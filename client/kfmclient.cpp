/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1999-2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kfmclient.h"

#include <kio/job.h>

#include <KLocalizedString>
#include <kprocess.h>
#include <config-konqueror.h>

#include <kmessagebox.h>
#include <kservice.h>
#include <KIO/CommandLauncherJob>
#include <KIO/ApplicationLauncherJob>
#include <KStartupInfo>
#include <kurifilter.h>
#include <KConfig>
#include <KConfigGroup>
#include <KService>
#include <KAboutData>
#include <KWindowSystem>
#include <KShell>
#include <KSharedConfig>

#include <kcoreaddons_version.h>

#include <QApplication>
#include <QDBusConnection>
#include <QDir>
#include <QMimeDatabase>
#include <QUrl>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTimer>

#ifdef WIN32
#include <process.h>
#endif

#include <unistd.h>

#include "konqclientrequest.h"
#include "kfmclient_debug.h"

static const char appName[] = "kfmclient";
static const char version[] = "2.0";

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kfmclient");

    KAboutData aboutData(appName, i18n("kfmclient"), QLatin1String(version));
    aboutData.setShortDescription(i18n("KDE tool for opening URLs from the command line"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    //qCDebug(KFMCLIENT_LOG) << "kfmclient starting" << QTime::currentTime();

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("noninteractive"), i18n("Non interactive use: no message boxes")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("commands"), i18n("Show available commands")));

    //This option is needed to fix a bug caused by the fact that Plasma inserts kfmclient_html in the Favorites section of the K menu.
    //kfmclient_html calls "kfmclient_openURL %u text/html", where %u is replaced by an URL. However, when the user
    //activates it from the K menu, there's no URL, so the command becomes "kfmclient openURL text/html", which causes
    //kfmclient to attempt to open the URL text/html. Adding this option allows to change the exec line in kfmclient_html
    //to kfmclient --mimetype text/html openURL %u, allowing an easy fix of the bug (see doIt()).
    //TODO: remove the old syntax of specifying the mimetype after the URL when building for KF6
    parser.addOption({{QStringLiteral("mimetype"), QStringLiteral("t")},
        i18n("The mimetype of the URL. Allows Konqueror to determine in advance which component to use, making it start faster."),
        i18nc("the name for a the value of an option on the command line help", "type"), QString()});

    parser.addPositionalArgument(QStringLiteral("command"), i18n("Command (see --commands)"));

    parser.addPositionalArgument(QStringLiteral("[URL(s)]"), i18n("Arguments for command"));

    parser.addOption(QCommandLineOption(QStringList{"tempfile"}, i18n("The files/URLs opened by the application will be deleted after use")));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || parser.isSet(QStringLiteral("commands"))) {
        QTextStream ts(stdout, QIODevice::WriteOnly);
        ts << i18n("\nSyntax:\n");
        ts << i18n("  kfmclient openURL 'url' ['mimetype']\n"
                  "            # Opens a window showing 'url'.\n"
                  "            #  'url' may be a relative path\n"
                  "            #   or file name, such as . or subdir/\n"
                  "            #   If 'url' is omitted, the start page is shown.\n\n");
        ts << i18n("            # If 'mimetype' is specified, it will be used to determine the\n"
                  "            #   component that Konqueror should use. For instance, set it to\n"
                  "            #   text/html for a web page, to make it appear faster\n"
                  "            # Note: this way of specifying mimetype is deprecated.\n"
                  "            #   Please use the --mimetype option\n\n");
        ts << i18n("  kfmclient newTab 'url' ['mimetype']\n"
                  "            # Same as above but opens a new tab with 'url' in an existing Konqueror\n"
                  "            #   window on the current active desktop if possible.\n\n");
        return 0;
    }

    // Use kfmclient from the session KDE version
    if ((args.at(0) == QLatin1String("openURL") || args.at(0) == QLatin1String("newTab"))
            && qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
        const int version = qEnvironmentVariableIntValue("KDE_SESSION_VERSION");
        if (version != 0 && version != KCOREADDONS_VERSION_MAJOR) {
            qCDebug(KFMCLIENT_LOG) << "Forwarding to kfmclient from KDE version " << version;
            char wrapper[ 10 ];
            sprintf(wrapper, "kde%d", version);
            char **newargv = new char *[ argc + 2 ];
            newargv[ 0 ] = wrapper;
            for (int i = 0;
                    i < argc;
                    ++i) {
                newargv[ i + 1 ] = argv[ i ];
            }
            newargv[ argc + 1 ] = nullptr;
#ifdef WIN32
            _execvp(wrapper, newargv);
#else
            execvp(wrapper, newargv);
#endif
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

static QUrl filteredUrl(const QString &url)
{
    KUriFilterData data;
    data.setData(url);
    data.setAbsolutePath(QDir::currentPath());
    data.setCheckForExecutables(false);

    return data.uri();
}

ClientApp::ClientApp()
{
}

ClientApp::BrowserApplicationParsingResult ClientApp::parseBrowserApplicationString(const QString& str)
{
    // There is a configured browser application.
    // See whether it is a literal command (starting with '!')
    // or a service (no '!').
    BrowserApplicationParsingResult res;
    if (str.isEmpty()) {
        return res;
    }
    res.isCommand = str.startsWith('!');
    if (res.isCommand) {
        // A literal command.  Split the string up into a shell
        // command and arguments.
        res.args = KShell::splitArgs(str.mid(1), KShell::AbortOnMeta);
        res.isValid = !res.args.isEmpty();
        if (res.isValid) {
            res.commandOrService = res.args.takeFirst();
        }
        else {
            res.error = "Parsing browser command failed";
        }
    } else {
        res.commandOrService = str;
        res.isValid = true;
    }
    // Ensure that we are not calling ourselves recursively;
    // that is, the external command is not "kfmclient" or
    // any variation of it.
    if (res.commandOrService.startsWith("kfmclient")) {
        res.isValid = false;
        res.error = "Recursive external browser command or service detected";
    }
    return res;
}

bool ClientApp::launchExternalBrowser(const ClientApp::BrowserApplicationParsingResult& parseResult, const QUrl& url, bool tempFile)
{
    KJob *job = nullptr;
    if (parseResult.isCommand) {
        QStringList args(parseResult.args);
        args << url.url();
        KStartupInfo::appStarted();
        job =  new KIO::CommandLauncherJob(parseResult.commandOrService, args);
    } else {
        KService::Ptr service = KService::serviceByStorageId(parseResult.commandOrService);
        if (!service) {
            qCWarning(KFMCLIENT_LOG) << "External browser service not known:" << parseResult.commandOrService;
            return false;
        }
        auto launcherJob = new KIO::ApplicationLauncherJob(service);
        launcherJob->setUrls({url});
        if (tempFile) {
            launcherJob->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
        }
        job = launcherJob;
    }
    QObject::connect(job, &KJob::result, this, &ClientApp::slotResult);
    job->setUiDelegate(nullptr);
    job->start();
    return qApp->exec() == 0;
}

bool ClientApp::createNewWindow(const QUrl &url, bool newTab, bool tempFile, const QString &mimetype)
{
    qCDebug(KFMCLIENT_LOG) << url << "mimetype=" << mimetype;

    bool launched = false;

    if (url.scheme().startsWith(QLatin1String("http"))) {
        KConfig config(QStringLiteral("kfmclientrc"));
        KConfigGroup generalGroup(&config, "General");
        const QString browserApp = generalGroup.readEntry("BrowserApplication");
        if (!browserApp.isEmpty()) {
            //Parse the BrowserApplication string and act accordingly
            BrowserApplicationParsingResult parseRes = parseBrowserApplicationString(browserApp);
            qCDebug(KFMCLIENT_LOG) << "Using external browser" << (parseRes.isCommand ? "command" : "service") << browserApp;
            if (parseRes.isValid) {
                launched = launchExternalBrowser(parseRes, url, tempFile);
            } else {
                qCWarning(KFMCLIENT_LOG) << parseRes.error;
            }
        }
    }

    if (!launched) {
        needDBus();
        // Launch Konqueror, or reuse an existing instance if possible.
        KonqClientRequest req;
        req.setUrl(url);
        req.setNewTab(newTab);
        req.setTempFile(tempFile);
        req.setMimeType(mimetype);
        launched = req.openUrl();
    }

    return launched;
}

bool ClientApp::openProfile(const QString &profileName, const QUrl &url, const QString &mimetype)
{
    Q_UNUSED(profileName); // the concept disappeared
    return createNewWindow(url, false, false, mimetype);
}

void ClientApp::delayedQuit()
{
    // Quit in 2 seconds. This leaves time for OpenUrlJob to pop up
    // "app not found" in KProcessRunner, if that was the case.
    QTimer::singleShot(2000, qApp, &QApplication::quit);
}

static void checkArgumentCount(int count, int min, int max)
{
    if (count < min) {
        fprintf(stderr, "%s: %s",  appName, i18n("Syntax error, not enough arguments\n").toLocal8Bit().data());
        ::exit(1);
    }
    if (max && (count > max)) {
        fprintf(stderr, "%s: %s", appName, i18n("Syntax error, too many arguments\n").toLocal8Bit().data());
        ::exit(1);
    }
}

bool ClientApp::doIt(const QCommandLineParser &parser)
{
    const QStringList args = parser.positionalArguments();
    int argc = args.count();
    checkArgumentCount(argc, 1, 0);

    if (!parser.isSet(QStringLiteral("noninteractive"))) {
        m_interactive = false;
    }
    QString command = args.at(0);

    if (command == QLatin1String("openURL") || command == QLatin1String("newTab")) {
        checkArgumentCount(argc, 1, 3);
        const bool tempFile = parser.isSet(QStringLiteral("tempfile"));

        QUrl url = argc > 1 ? filteredUrl(args.at(1)) : QUrl();

        //If the given URL is empty, show the start page
        if (url.isEmpty()) {
            KConfigGroup grp = KSharedConfig::openConfig(QStringLiteral("konquerorrc"))->group("UserSettings");
            url = QUrl(grp.readEntry("StartURL", QStringLiteral("konq:konqueror")));
        }

        QString mimetype = argc == 3 ? args.at(2) : QString();
        if (mimetype.isEmpty()) {
            mimetype = parser.value(QStringLiteral("mimetype"));
        }

        return createNewWindow(url, command == QLatin1String("newTab"), tempFile, mimetype);
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
        fprintf(stderr, "%s: %s", appName, i18n("Syntax error, unknown command '%1'\n", command).toLocal8Bit().data());
        return false;
    }
    return true;
}


void ClientApp::slotResult(KJob *job)
{
    if (job->error()) {
        qApp->exit(1);
    } else {
        delayedQuit();
    }
}
