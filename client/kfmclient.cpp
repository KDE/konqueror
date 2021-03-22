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
#include <config-konqueror.h>

#include <kmessagebox.h>
#include <kmimetypetrader.h>
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

#include "konqclientrequest.h"
#include "kfmclient_debug.h"

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
    aboutData.setupCommandLine(&parser);

    //qCDebug(KFMCLIENT_LOG) << "kfmclient starting" << QTime::currentTime();

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("noninteractive"), i18n("Non interactive use: no message boxes")));

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("commands"), i18n("Show available commands")));

    parser.addPositionalArgument(QStringLiteral("command"), i18n("Command (see --commands)"));

    parser.addPositionalArgument(QStringLiteral("[URL(s)]"), i18n("Arguments for command"));

    parser.addOption(QCommandLineOption(QStringList{"tempfile"}, i18n("The files/URLs opened by the application will be deleted after use")));

    parser.process(app);
    aboutData.processCommandLine(&parser);

    const QStringList args = parser.positionalArguments();

    if (args.isEmpty() || parser.isSet(QStringLiteral("commands"))) {
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

    if (KUriFilter::self()->filterUri(data) && data.uriType() != KUriFilterData::Error) {
        return data.uri();
    }
    return QUrl();
}

ClientApp::ClientApp()
{
}

bool ClientApp::createNewWindow(const QUrl &url, bool newTab, bool tempFile, const QString &mimetype)
{
    qCDebug(KFMCLIENT_LOG) << url << "mimetype=" << mimetype;

    if (url.scheme().startsWith(QLatin1String("http"))) {
        KConfig config(QStringLiteral("kfmclientrc"));
        KConfigGroup generalGroup(&config, "General");
        const QString browserApp = generalGroup.readEntry("BrowserApplication");

        if (!browserApp.isEmpty()) {
            // There is a configured browser application.
            // See whether it is a literal command (starting with '!')
            // or a service (no '!').
            if (browserApp.startsWith('!')) {
                // A literal command.  Split the string up into a shell
                // command and arguments.
                qCDebug(KFMCLIENT_LOG) << "Using external browser command" << browserApp;
                KStartupInfo::appStarted();

                QStringList shellArgs = KShell::splitArgs(browserApp.mid(1), KShell::AbortOnMeta);
                if (!shellArgs.isEmpty()) {
                    // The command name is the first list item.
                    QString executable = shellArgs.takeFirst();
                    // Ensure that we are not calling ourselves recursively;
                    // that is, the external command is not "kfmclient" or
                    // any variation of it.  If it is, then fall through to
                    // open the URL in Konqueror directly.
                    if (executable.startsWith("kfmclient")) {
                        goto launchKonq;
                    }

                    // Append the URL to be opened to the arguments.
                    shellArgs.append(url.url());
                    // Then launch the command.  It is not possible to automatically
                    // delete a temporary file in this case, but no temporary file
                    // download should have happened anyway.
                    auto *job = new KIO::CommandLauncherJob(executable, shellArgs);
                    QObject::connect(job, &KJob::result, this, &ClientApp::slotResult);
                    job->setUiDelegate(nullptr);
                    job->start();
                    return qApp->exec();
                } else {
                    qCWarning(KFMCLIENT_LOG) << "Parsing browser command failed";
                }
            } else {
                // The configured browser is specified as a service.
                qCDebug(KFMCLIENT_LOG) << "Using external browser service" << browserApp;
                // First ensure that we are not calling ourselves recursively;
                // that is, the service is not "kfmclient" or any variation
                // of it.  If it is, then fall through to open the URL in Konqueror
                // directly.
                if (browserApp.startsWith("kfmclient")) {
                    goto launchKonq;
                }

                KService::Ptr service = KService::serviceByStorageId(browserApp);
                if (service) {
                    // Launch the service to open the URL.
                    auto *job = new KIO::ApplicationLauncherJob(service);
                    QObject::connect(job, &KJob::result, this, &ClientApp::slotResult);
                    job->setUrls({url});
                    if (tempFile) {
                        job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
                    }
                    job->setUiDelegate(nullptr);
                    job->start();
                    return qApp->exec();
                } else {
                    qCWarning(KFMCLIENT_LOG) << "External browser service not known";
                }
            }

            // Fall through to here if the external browser command or service
            // could not be found or run.  Do not fall back to the original action
            // of simply opening the specified URL in its default application,
            // because if that turns out to be ourselves then there will again
            // be an infinite recursion.  Just exit with an error.
            qCWarning(KFMCLIENT_LOG) << "Unable to launch external browser";
            return false;

launchKonq:
            // The configured external browser command or service appears to be
            // trying to call ourselves recursively.  Simply fall through to
            // launch Konqueror.
            qCDebug(KFMCLIENT_LOG) << "Recursive external browser command or service detected";
        }
    }

    needDBus();

    // Launch Konqueror, or reuse an existing instance if possible.
    KonqClientRequest req;
    req.setUrl(url);
    req.setNewTab(newTab);
    req.setTempFile(tempFile);
    req.setMimeType(mimetype);
    return req.openUrl();
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

    if (!parser.isSet(QStringLiteral("noninteractive"))) {
        m_interactive = false;
    }
    QString command = args.at(0);

    if (command == QLatin1String("openURL") || command == QLatin1String("newTab")) {
        checkArgumentCount(argc, 1, 3);
        const bool tempFile = parser.isSet(QStringLiteral("tempfile"));
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
    if (job->error()) {
        qApp->exit(1);
    } else {
        delayedQuit();
    }
}
