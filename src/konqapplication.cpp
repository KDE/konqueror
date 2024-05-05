/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtGlobal>

#include "konqapplication.h"
#include "konqsettings.h"
#include <konqueror-version.h>
#include "konqmainwindow.h"
#include "KonquerorAdaptor.h"
#include "konqviewmanager.h"
#include "konqurl.h"
#include "konqsettingsxt.h"
#include "konqsessionmanager.h"
#include "konqclosedwindowsmanager.h"
#include "konqdebug.h"
#include "konqmainwindow.h"
#include "konqmainwindowfactory.h"
#include "konqmisc.h"
#include <config-konqueror.h>
#include "implementations/konqbrowser.h"

#include "interfaces/browser.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>
#include <QProcess>
#include <QDirIterator>
#include <QTextStream>

#include <QtGui/private/qtx11extras_p.h>

#include <KCrash>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KDBusService>
#include <KStartupInfo>
#include <KWindowSystem>
#include <kwindowsystem_version.h>
#include <KX11Extras>
#include <KMessageBox>

#ifdef KActivities_FOUND
#include "activitymanager.h"
#include <PlasmaActivities/Consumer>
#endif

#include <iostream>
#include <unistd.h>

#ifdef KActivities_FOUND
#endif

using namespace KonqInterfaces;

QString KonquerorApplication::currentActivity()
{
#ifdef KActivities_FOUND
    KonquerorApplication* app = qobject_cast<KonquerorApplication*>(QApplication::instance());
    if (!app || !app->m_activityConsumer || app->m_activityConsumer->serviceStatus() != KActivities::Consumer::Running) {
        return QString();
    }
    return app->m_activityConsumer->currentActivity();
#else
    return QString();
#endif
}

KonquerorApplication::KonquerorApplication(int &argc, char **argv)
    : QApplication(argc, argv), m_browser(nullptr)
#ifdef KActivities_FOUND
    , m_activityConsumer(new KActivities::Consumer(this))
#endif
{
    new KonquerorAdaptor; // not really an adaptor
    const QString dbusInterface = QStringLiteral("org.kde.Konqueror.Main");
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, QStringLiteral("reparseConfiguration"), this, SLOT(slotReparseConfiguration()));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, QStringLiteral("addToCombo"), this,
                 SLOT(slotAddToCombo(QString,QDBusMessage)));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, QStringLiteral("removeFromCombo"), this,
                 SLOT(slotRemoveFromCombo(QString,QDBusMessage)));
    dbus.connect(QString(), KONQ_MAIN_PATH, dbusInterface, QStringLiteral("comboCleared"), this, SLOT(slotComboCleared(QDBusMessage)));

#ifdef WEBENGINEPART_OWN_DICTIONARY_DIR
    //If the user alredy set QTWEBENGINE_DICTIONARIES_PATH, don't override it
    if (!qEnvironmentVariableIsSet("QTWEBENGINE_DICTIONARIES_PATH")) {
        qputenv("QTWEBENGINE_DICTIONARIES_PATH", WEBENGINEPART_OWN_DICTIONARY_DIR);
    }
#endif

    m_runningAsRootBehavior = checkRootBehavior();

    QByteArray flags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    flags.append(" --enable-features=WebRTCPipeWireCapturer");
    if (m_runningAsRootBehavior == RunInDangerousMode) {
        flags.append (" --no-sandbox");
    }

    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags);
    KLocalizedString::setApplicationDomain("konqueror");

#ifdef KActivities_FOUND
    m_activityConsumer = new KActivities::Consumer(this);
#endif
}

KonquerorApplication::KonquerorAsRootBehavior KonquerorApplication::checkRootBehavior()
{
    uid_t uid = geteuid();
    if (uid == 0) {
        QString msg = i18n("<p>You're running Konqueror as root. This requires enabling a highly insecure mode in the browser component.</p><p>What do you want to do?</p>");
        KGuiItem enableInsecureMode(QLatin1String("Enable the insecure mode")); //Continue
        KGuiItem exitKonq(QLatin1String("Exit Konqueror")); //Cancel
        KMessageBox::ButtonCode ans = KMessageBox::warningContinueCancel(nullptr, msg, QString(), enableInsecureMode, exitKonq);
        return ans == KMessageBox::Continue ? RunInDangerousMode : PreventRunningAsRoot;
    } else {
        return NotRoot;
    }
}

void KonquerorApplication::slotReparseConfiguration()
{
    KSharedConfig::openConfig()->reparseConfiguration();
    KonqFMSettings::reparseConfiguration();
    m_browser->applyConfiguration();

    QList<KonqMainWindow *> *mainWindows = KonqMainWindow::mainWindowList();
    if (mainWindows) {
        for (KonqMainWindow *window: *mainWindows) {
            window->reparseConfiguration();
        }
    }

    emit configurationChanged();
}

void KonquerorApplication::slotAddToCombo(const QString &url, const QDBusMessage &msg)
{
    KonqMainWindow::comboAction(KonqMainWindow::ComboAdd, url, msg.service());
}

void KonquerorApplication::slotRemoveFromCombo(const QString &url, const QDBusMessage &msg)
{
    KonqMainWindow::comboAction(KonqMainWindow::ComboRemove, url, msg.service());
}

void KonquerorApplication::slotComboCleared(const QDBusMessage &msg)
{
    KonqMainWindow::comboAction(KonqMainWindow::ComboClear, QString(), msg.service());
}

void KonquerorApplication::setupAboutData()
{
    KAboutData m_aboutData("konqueror", i18n("Konqueror"), KONQUEROR_VERSION);
    m_aboutData.setShortDescription(i18n("Web browser, file manager and document viewer."));
    m_aboutData.addLicense(KAboutLicense::GPL_V2);
    m_aboutData.setCopyrightStatement(i18n("(C) 1999-2023, The Konqueror developers"));
    m_aboutData.setHomepage("https://apps.kde.org/konqueror");

    m_aboutData.addAuthor(i18n("Stefano Crocco"), i18n("Current maintainer"), "stefano.crocco@alice.it");
    m_aboutData.addAuthor(i18n("David Faure"), i18n("Developer (framework, parts, JavaScript, I/O library) and former maintainer"), "faure@kde.org");
    m_aboutData.addAuthor(i18n("Simon Hausmann"), i18n("Developer (framework, parts)"), "hausmann@kde.org");
    m_aboutData.addAuthor(i18n("Michael Reiher"), i18n("Developer (framework)"), "michael.reiher@gmx.de");
    m_aboutData.addAuthor(i18n("Matthias Welk"), i18n("Developer"), "welk@fokus.gmd.de");
    m_aboutData.addAuthor(i18n("Alexander Neundorf"), i18n("Developer (List views)"), "neundorf@kde.org");
    m_aboutData.addAuthor(i18n("Michael Brade"), i18n("Developer (List views, I/O library)"), "brade@kde.org");
    m_aboutData.addAuthor(i18n("Lars Knoll"), i18n("Developer (HTML rendering engine)"), "knoll@kde.org");
    m_aboutData.addAuthor(i18n("Dirk Mueller"), i18n("Developer (HTML rendering engine)"), "mueller@kde.org");
    m_aboutData.addAuthor(i18n("Peter Kelly"), i18n("Developer (HTML rendering engine)"), "pmk@post.com");
    m_aboutData.addAuthor(i18n("Waldo Bastian"), i18n("Developer (HTML rendering engine, I/O library)"), "bastian@kde.org");
    m_aboutData.addAuthor(i18n("Germain Garand"), i18n("Developer (HTML rendering engine)"), "germain@ebooksfrance.org");
    m_aboutData.addAuthor(i18n("Leo Savernik"), i18n("Developer (HTML rendering engine)"), "l.savernik@aon.at");
    m_aboutData.addAuthor(i18n("Stephan Kulow"), i18n("Developer (HTML rendering engine, I/O library, regression test framework)"), "coolo@kde.org");
    m_aboutData.addAuthor(i18n("Antti Koivisto"), i18n("Developer (HTML rendering engine)"), "koivisto@kde.org");
    m_aboutData.addAuthor(i18n("Zack Rusin"),  i18n("Developer (HTML rendering engine)"), "zack@kde.org");
    m_aboutData.addAuthor(i18n("Tobias Anton"), i18n("Developer (HTML rendering engine)"), "anton@stud.fbi.fh-darmstadt.de");
    m_aboutData.addAuthor(i18n("Lubos Lunak"), i18n("Developer (HTML rendering engine)"), "l.lunak@kde.org");
    m_aboutData.addAuthor(i18n("Maks Orlovich"), i18n("Developer (HTML rendering engine, JavaScript)"), "maksim@kde.org");
    m_aboutData.addAuthor(i18n("Allan Sandfeld Jensen"), i18n("Developer (HTML rendering engine)"), "kde@carewolf.com");
    m_aboutData.addAuthor(i18n("Apple Safari Developers"), i18n("Developer (HTML rendering engine, JavaScript)"));
    m_aboutData.addAuthor(i18n("Harri Porten"), i18n("Developer (JavaScript)"), "porten@kde.org");
    m_aboutData.addAuthor(i18n("Koos Vriezen"), i18n("Developer (Java applets and other embedded objects)"), "koos.vriezen@xs4all.nl");
    m_aboutData.addAuthor(i18n("Matt Koss"), i18n("Developer (I/O library)"), "koss@miesto.sk");
    m_aboutData.addAuthor(i18n("Alex Zepeda"), i18n("Developer (I/O library)"), "zipzippy@sonic.net");
    m_aboutData.addAuthor(i18n("Richard Moore"), i18n("Developer (Java applet support)"), "rich@kde.org");
    m_aboutData.addAuthor(i18n("Dima Rogozin"), i18n("Developer (Java applet support)"), "dima@mercury.co.il");
    m_aboutData.addAuthor(i18n("Wynn Wilkes"), i18n("Developer (Java 2 security manager support,\n and other major improvements to applet support)"), "wynnw@calderasystems.com");
    m_aboutData.addAuthor(i18n("Stefan Schimanski"), i18n("Developer (Netscape plugin support)"), "schimmi@kde.org");
    m_aboutData.addAuthor(i18n("George Staikos"), i18n("Developer (SSL, Netscape plugins)"), "staikos@kde.org");
    m_aboutData.addAuthor(i18n("Dawit Alemayehu"), i18n("Developer (I/O library, Authentication support)"), "adawit@kde.org");
    m_aboutData.addAuthor(i18n("Carsten Pfeiffer"), i18n("Developer (framework)"), "pfeiffer@kde.org");
    m_aboutData.addAuthor(i18n("Torsten Rahn"), i18n("Graphics/icons"), "torsten@kde.org");
    m_aboutData.addAuthor(i18n("Torben Weis"), i18n("KFM author"), "weis@kde.org");
    m_aboutData.addAuthor(i18n("Joseph Wenninger"), i18n("Developer (navigation panel framework)"), "jowenn@kde.org");
    m_aboutData.addAuthor(i18n("Stephan Binner"), i18n("Developer (misc stuff)"), "binner@kde.org");
    m_aboutData.addAuthor(i18n("Ivor Hewitt"), i18n("Developer (AdBlock filter)"), "ivor@ivor.org");
    m_aboutData.addAuthor(i18n("Eduardo Robles Elvira"), i18n("Developer (framework)"), "edulix@gmail.com");
    m_aboutData.addAuthor(i18n("Matthias Kalle Dalheimer"), QLatin1String(""), QStringLiteral("kalle@kde.org"));
    m_aboutData.addAuthor(i18n("Daniel Molkentin"), QLatin1String(""), QStringLiteral("molkentin@kde.org"));
    m_aboutData.addCredit(i18n("Leo Savernik"), i18n("JavaScript access controls\n"
                     "Per-domain policies extensions"),
                     QStringLiteral("l.savernik@aon.at"));
    KAboutData::setApplicationData(m_aboutData);
}

void KonquerorApplication::setupParser()
{
    m_parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    m_aboutData.setupCommandLine(&m_parser);

    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("silent")}, i18n("Start without a default window, when called without URLs")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("preload")}, i18n("Preload for later use. This mode does not support URLs on the command line")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("profile")}, i18n("Profile to open (DEPRECATED, IGNORED)"), i18n("profile")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("sessions")}, i18n("List available sessions")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("open-session")}, i18n("Session to open"), i18n("session")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("mimetype")}, i18n("Mimetype to use for this URL (e.g. text/html or inode/directory)"), i18n("mimetype")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("part")}, i18n("Part to use (e.g. khtml or kwebkitpart)"), i18n("service")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("select")}, i18n("For URLs that point to files, opens the directory and selects the file, instead of opening the actual file")));
    m_parser.addOption(QCommandLineOption(QStringList{QStringLiteral("tempfile")}, i18n("The files/URLs opened by the application will be deleted after use")));
#ifdef DEVELOPER_MODE
    m_parser.addOption(QCommandLineOption({QStringLiteral("force-new-process")}, i18n("Create a new Konqueror process, even if one already exists. Meant to be used only when developing Konqueror itself")));
#endif
    m_parser.addOption(QCommandLineOption({QStringLiteral("test-restore")}, i18n("TEST")));
    m_parser.addPositionalArgument(QStringLiteral("[URL]"), i18n("Location to open"));
}

static void fixOldStartUrl() {
    QUrl startUrl(KonqSettings::startURL());
    if (startUrl.scheme() == "about") {
        startUrl.setScheme(KonqUrl::scheme());
        KonqSettings::setStartURL(startUrl.url());
        KonqSettings::self()->save();
    }
}

int KonquerorApplication::startFirstInstance()
{
    fixOldStartUrl();

    m_browser = new KonqBrowser(this);
    connect(this, &KonquerorApplication::configurationChanged, m_browser, &KonqBrowser::configurationChanged);

    if (isSessionRestored()) {
        restoreSession();
    } else {
        performStart(QDir::currentPath(), true);
    }

    QString programName = QApplication::applicationFilePath();

    //Ensure a session manager is created
    KonqSessionManager::self();

    const int ret = exec();

    //Don't preload if Konqueror is run as root
    bool alwaysPreload = m_runningAsRootBehavior == NotRoot && KonqSettings::alwaysHavePreloaded();

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

    KonqClosedWindowsManager::destroy();

    if (alwaysPreload) {
        QProcess::startDetached(programName, {"--preload"});
    }

    return ret;
}

int KonquerorApplication::start()
{
    if (m_runningAsRootBehavior == PreventRunningAsRoot) {
        return 0;
    }

    setupAboutData();
    setupParser();

    KCrash::initialize();

    m_parser.process(*this);
    m_aboutData.processCommandLine(&m_parser);

#ifdef DEVELOPER_MODE
    bool forceNewProcess = m_runningAsRootBehavior != NotRoot || m_parser.isSet(QStringLiteral("force-new-process"));
#else
    bool forceNewProcess = m_runningAsRootBehavior != NotRoot;
#endif

    //Explicitly disable reusing existing instances if running as root. The behavior is not clear
    //and could be dangerous if new windows are unknowingly launched as root.
    //I thought that creating the KDBusService instance inside the if block should do it; however, doing
    //so prevents the new window to be created because the new instance produces a critical failure with message:
    //Couldn't register name 'org.kde.konqueror' with DBUS - another process owns it already!
    //Moving the service creation outside the if block seems to solve the issue.
    //
    //This also disables reusing an existing instance when running in developer mode
    KDBusService dbusService(forceNewProcess ? KDBusService::Multiple | KDBusService::NoExitOnFailure : KDBusService::Unique);
    if (!forceNewProcess) {
        auto activateApp = [this](const QStringList &arguments, const QString &workingDirectory) {
            m_parser.parse(arguments);
            performStart(workingDirectory, false);
        };
        QObject::connect(&dbusService, &KDBusService::activateRequested, activateApp);
    }

    return startFirstInstance();
}

int KonquerorApplication::performStart(const QString& workingDirectory, bool firstInstance)
{
    const QStringList args = m_parser.positionalArguments();

    if (m_parser.isSet("sessions")) {
        listSessions();
        return 0;
    } else if (m_parser.isSet("open-session")) {
        //If the given session can't be opened for any reason, inform the user
        QString sessionName = m_parser.value("open-session");
        int result = openSession(sessionName);
        if (result != 0) {
            KMessageBox::error(nullptr, i18nc("The session asked by the user doesn't exist or can't be opened",
                                              "Session %1 couldn't be opened", sessionName));
        }
        //If there was an error opening the session and this is the first instance, don't return immediately
        //as this would lead to the application being run but with no windows open. Instead, open an empty
        //window
        if (result != 0 && firstInstance) {
            return result;
        }
    }

    //We check for the --preload switch before attempting recovering session because we shouldn't
    //display windows when the user only asked to preload a window
    if (m_parser.isSet("preload")) {
        preloadWindow(args);
        return 0;
    }

    //Don't attempt to restore session when running as root
    if (!m_sessionRecoveryAttempted && m_runningAsRootBehavior == NotRoot) {
        // Ask the user to recover session if applicable
        KonqSessionManager::self()->askUserToRestoreAutosavedAbandonedSessions();
        m_sessionRecoveryAttempted = true;
    }

    WindowCreationResult result;

    bool sessionRestored = KonqSessionManager::self()->restoreSessionSavedAtExit();
    if (!args.isEmpty()) {
        //KonqSessionManager::restoreSessionSavedAtExit only returns true if there's at least one main window,
        //so there's no need to check whether the list is empty
        result = createWindowsForUrlArguments(args, workingDirectory, sessionRestored ? KonqMainWindow::mainWindows().at(0) : nullptr);
    } else if (sessionRestored){
        result = WindowCreationResult{KonqMainWindow::mainWindows().at(0), 0};
    } else {
        result = createEmptyWindow(firstInstance, true);
    }

    KonqMainWindow *mw = result.first;
    if (!firstInstance && mw) {
        mw ->setAttribute(Qt::WA_NativeWindow, true);
        if (KWindowSystem::isPlatformX11()) {
            KStartupInfo::setNewStartupId(mw->windowHandle(), QX11Info::nextStartupId());
            KX11Extras::forceActiveWindow(mw->winId());
        } else {
            KStartupInfo::setNewStartupId(mw->windowHandle(), qEnvironmentVariable("XDG_ACTIVATION_TOKEN").toUtf8());
        }
    }

    return result.second;
}

KonquerorApplication::WindowCreationResult KonquerorApplication::createWindowsForUrlArguments(const QStringList& args, const QString &workingDirectory, KonqMainWindow *mainwin)
{
    QList<QUrl> urlList;
    urlList.reserve(args.length());

    auto urlFromArg = [workingDirectory](const QString &arg) {
        const QUrl url = QUrl::fromUserInput(arg, workingDirectory);
        // KonqMisc::konqFilteredURL doesn't cope with local files... A bit of hackery below
        if (url.isLocalFile() && QFile::exists(url.toLocalFile())) { // "konqueror index.html"
            return url;
        } else { // "konqueror slashdot.org"
            return KonqMisc::konqFilteredURL(nullptr, arg);
        }
    };

    for (const QString &arg : args) {
        urlList.append(urlFromArg(arg));
    };

    QList<QUrl> filesToSelect;
    if (m_parser.isSet("select")) {
        // Get all distinct directories from 'files' and open a tab
        // for each directory.
        QList<QUrl> dirs;
        for (const QUrl &url : urlList) {
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
    if (m_parser.isSet("mimetype")) {
        urlargs.setMimeType(m_parser.value("mimetype"));
    }

    KonqOpenURLRequest req;
    req.args = urlargs;
    req.filesToSelect = filesToSelect;
    req.tempFile = m_parser.isSet("tempfile");
    req.serviceName = m_parser.value("part");

    if (!mainwin) {
        mainwin = KonqMainWindowFactory::createNewWindow(firstUrl, req);
        if (!mainwin) {
            return qMakePair(nullptr, 1);
        }
    } else {
        req.browserArgs.setNewTab(true);
        mainwin->openUrl(nullptr, firstUrl, req.args.mimeType(), req);
    }

    mainwin->show();
    if (!urlList.isEmpty()) {
        // Open the other urls as tabs in that window
        mainwin->openMultiURL(urlList);
    }

    //Ensure the last tab is activated. This happens automatically if a new main window was created
    //but not if an existing main window was passed as argument, which is the case if the last Konqueror
    //session was restored
    mainwin->activateTab(mainwin->tabsCount()-1);
    return qMakePair(mainwin, 0);
}

void KonquerorApplication::preloadWindow(const QStringList &args)
{
    if (!args.isEmpty()) {
        QTextStream ts(stderr, QIODevice::WriteOnly);
        ts << i18n("You can't pass URLs when using the --preload switch. The URLs will be ignored\n");
    }
    KonqMainWindowFactory::createPreloadWindow();
}

KonquerorApplication::WindowCreationResult KonquerorApplication::createEmptyWindow(bool firstInstance, bool calledFromPerformStart )
{
    //Always create a new window except when called with the --silent switch or a session has been recovered (see #388333)
    if (m_parser.isSet("silent")) {
        return qMakePair(nullptr, 0);
    }

//WORKAROUND for activities
//Consider the following situation:
//- multiple windows, in different activities
//- the user stops one or more activities. The corresponding windows are closed
//- the user exits Konqueror
//- the user restarts Konqueror
//- the user restarts one of the stopped activities which had Konqueror windows on them
//What should happen is that the previously open windows in the restarted activity are restored
//What really happens is that, besides those window, a new, empty window is created. This is because KWin/KSMService attempts
//to start Konqueror again, passing it the information needed to restore the windows. However, for some reason, no information
//is given and Konqueror creates an emtpy window. The following check avoids creating the window.
#ifdef KActivities_FOUND
    ActivityManager *am = KonqSessionManager::self()->activityManager();
    if (calledFromPerformStart && am && am->restoringStoppedActivity()) {
        am->restoringStoppedActivityDone();
        return qMakePair(nullptr, 0);
    }
#endif

    if (firstInstance) { // If session recovery created some windows, no need for an empty window here.
        QList<KonqMainWindow *> *mainWindowList = KonqMainWindow::mainWindowList();
        if (mainWindowList && !mainWindowList->isEmpty()) {
            return qMakePair(mainWindowList->at(0), 0);
        }
    }

    KonqMainWindow *mainWin = KonqMainWindowFactory::createNewWindow();
    if (mainWin) {
        mainWin->show();
        return qMakePair(mainWin, 0);
    } else {
        return qMakePair(nullptr, 1);
    }
}

void KonquerorApplication::listSessions()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + "sessions/";
    QDirIterator it(dir, QDir::Readable | QDir::NoDotAndDotDot | QDir::Dirs);
    QTextStream ts(stdout, QIODevice::WriteOnly);
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        ts << fileInfo.baseName();
    }
}

int KonquerorApplication::openSession(const QString& session)
{
    QString sessionPath = session;
    if (!session.startsWith('/')) {
        sessionPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + "sessions/" + session;
    }

    QDirIterator it(sessionPath, QDir::Readable | QDir::Files);
    if (!it.hasNext()) {
        qCWarning(KONQUEROR_LOG) << "session" << session << "not found or empty";
        return 1;
    }

    KonqSessionManager::self()->restoreSessions(sessionPath);

    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    return (mainWindows && !mainWindows->isEmpty()) ? 0 : 1;
}

void KonquerorApplication::restoreSession()
{
    KonqSessionManager::self()->restoreSessionSavedAtLogout();
}
