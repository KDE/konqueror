/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2000-2005 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2007 Daniel García Moreno <danigm@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqmainwindow.h"
#include "konqmouseeventfilter.h"
#include "konqclosedwindowsmanager.h"
#include "konqsessionmanager.h"
#include "konqsessiondlg.h"
#include "konqdraggablelabel.h"
#include "konqcloseditem.h"
#include "konqapplication.h"
#include "konqguiclients.h"
#include "konqmainwindowfactory.h"
#include "KonqMainWindowAdaptor.h"
#include "KonquerorAdaptor.h"
#include "konqview.h"
#include "konqmisc.h"
#include "konqviewmanager.h"
#include "konqframestatusbar.h"
#include "konqtabs.h"
#include "konqactions.h"
#include "konqsettingsxt.h"
#include "konqextensionmanager.h"
#include "konqueror_interface.h"
#include "delayedinitializer.h"
#include "konqextendedbookmarkowner.h"
#include "konqframevisitor.h"
#include "konqbookmarkbar.h"
#include "konqundomanager.h"
#include "konqhistorydialog.h"
#include <config-konqueror.h>
#include <kstringhandler.h>
#include "konqurl.h"
#include "konqbrowserinterface.h"
#include "urlloader.h"
#include "pluginmetadatautils.h"

#include <konq_events.h>
#include <konqpixmapprovider.h>
#include <konqsettings.h>
#include <konq_spellcheckingconfigurationdispatcher.h>

#include <kwidgetsaddons_version.h>
#include <kxmlgui_version.h>
#include <kparts_version.h>
#include <kbookmarkmanager.h>
#include <klineedit.h>
#include <kzip.h>
#include <pwd.h>
// we define STRICT_ANSI to get rid of some warnings in glibc
#ifndef __STRICT_ANSI__
#define __STRICT_ANSI__
#define _WE_DEFINED_IT_
#endif
#include <netdb.h>
#ifdef _WE_DEFINED_IT_
#undef __STRICT_ANSI__
#undef _WE_DEFINED_IT_
#endif
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <KIO/ApplicationLauncherJob>
#include <KIO/OpenUrlJob>

#include <QDesktopServices>
#include <QFile>
#include <QClipboard>
#include <QStackedWidget>
#include <QFileInfo>
#if KONQ_HAVE_X11
#include <QX11Info>
#endif
#include <QEvent>
#include <QKeyEvent>
#include <QByteRef>
#include <QPixmap>
#include <QLineEdit>
#include <QNetworkProxy>

#include <KPluginMetaData>
#include <kaboutdata.h>
#include <ktoolbar.h>
#include <konqbookmarkmenu.h>
#include <kcmultidialog.h>
#include "konqdebug.h"
#include <kdesktopfile.h>
#include <kedittoolbar.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <knewfilemenu.h>
#include <konq_popupmenu.h>
#include "konqsettings.h"
#include "konqanimatedlogo_p.h"
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kstandardshortcut.h>
#include <kstandardaction.h>
#include <ksycoca.h>
#include <QTemporaryFile>
#include <ktogglefullscreenaction.h>
#include <ktoolbarpopupaction.h>
#include <kurlcompletion.h>
#include <kurlrequesterdialog.h>
#include <kurlrequester.h>
#include <kmimetypetrader.h>
#include <KJobWidgets>
#include <KLocalizedString>
#include <QIcon>
#include <kiconloader.h>
#include <QMenu>
#include <kprocess.h>
#include <KIO/JobUiDelegate>
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KIO/FileUndoManager>
#include <kparts/browseropenorsavequestion.h>
#include <KParts/OpenUrlEvent>
#include <KCompletionMatches>
#include <kacceleratormanager.h>
#include <kuser.h>
#include <kxmlguifactory.h>
#include <sonnet/configdialog.h>
#include <kwindowsystem.h>
#include <netwm.h>
#include <kio_version.h>

#include <kauthorized.h>
#include <QDBusConnection>
#include <QDBusMessage>
#include <kconfiggroup.h>
#include <kurlauthorized.h>
#include <QFontDatabase>
#include <QMenuBar>
#include <QStandardPaths>
#include <KSharedConfig>

#include <KProtocolManager>
#include <KParts/PartLoader>
#include <KApplicationTrader>
#include <KX11Extras>

#include <QMetaObject>
#include <QMetaMethod>

template class QList<QPixmap *>;
template class QList<KToggleAction *>;

static KBookmarkManager *s_bookmarkManager = nullptr;
QList<KonqMainWindow *> *KonqMainWindow::s_lstMainWindows = nullptr;
KConfig *KonqMainWindow::s_comboConfig = nullptr;
KCompletion *KonqMainWindow::s_pCompletion = nullptr;

KonqOpenURLRequest KonqOpenURLRequest::null;

static const unsigned short int s_closedItemsListLength = 10;

static void raiseWindow(KonqMainWindow *window)
{
    if (!window) {
        return;
    }

    if (window->isMinimized()) {
        KX11Extras::unminimizeWindow(window->winId());
    }
    window->activateWindow();
    window->raise();
}

KonqExtendedBookmarkOwner::KonqExtendedBookmarkOwner(KonqMainWindow *w)
{
    m_pKonqMainWindow = w;
}

KonqMainWindow::KonqMainWindow(const QUrl &initialURL)
    : KParts::MainWindow()
    , m_paClosedItems(nullptr)
    , m_fullyConstructed(false)
    , m_bLocationBarConnected(false)
    , m_bURLEnterLock(false)
    , m_urlCompletionStarted(false)
    , m_fullScreenData{FullScreenState::NoFullScreen, FullScreenState::NoFullScreen, true, true, false}
    , m_goBuffer(0)
    , m_pBookmarkMenu(nullptr)
    , m_configureDialog(nullptr)
    , m_pURLCompletion(nullptr)
    , m_isPopupWithProxyWindow(false)
{
    if (!s_lstMainWindows) {
        s_lstMainWindows = new QList<KonqMainWindow *>;
    }

    s_lstMainWindows->append(this);

    KonqMouseEventFilter::self(); // create it

    m_pChildFrame = nullptr;
    m_pActiveChild = nullptr;
    m_workingTab = 0;
    (void) new KonqMainWindowAdaptor(this);
    m_paBookmarkBar = nullptr;

    m_viewModesGroup = new QActionGroup(this);
    m_viewModesGroup->setExclusive(true);
    connect(m_viewModesGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(slotViewModeTriggered(QAction*)),
            Qt::QueuedConnection); // Queued so that we don't delete the action from the code that triggered it.

    // This has to be called before any action is created for this mainwindow
    const KAboutData applicationData = KAboutData::applicationData();
    setComponentName(applicationData.componentName(), applicationData.displayName());

    m_pViewManager = new KonqViewManager(this);
    m_viewModeMenu = nullptr;
    m_openWithMenu = nullptr;
    m_paCopyFiles = nullptr;
    m_paMoveFiles = nullptr;
    m_bookmarkBarInitialized = false;

    m_toggleViewGUIClient = new ToggleViewGUIClient(this);

    m_pBookmarksOwner = new KonqExtendedBookmarkOwner(this);

    // init history-manager, load history, get completion object
    if (!s_pCompletion) {
        s_bookmarkManager = KBookmarkManager::userBookmarksManager();

        // let the KBookmarkManager know that we are a browser, equals to "keditbookmarks --browser"
        s_bookmarkManager->setEditorOptions(QStringLiteral("konqueror"), true);

        KonqHistoryManager *mgr = new KonqHistoryManager(s_bookmarkManager);
        s_pCompletion = mgr->completionObject();

        // setup the completion object before createGUI(), so that the combo
        // picks up the correct mode from the HistoryManager (in slotComboPlugged)
        int mode = KonqSettings::settingsCompletionMode();
        s_pCompletion->setCompletionMode(static_cast<KCompletion::CompletionMode>(mode));
    }
    connect(KParts::HistoryProvider::self(), &KParts::HistoryProvider::cleared, this, &KonqMainWindow::slotClearComboHistory);

    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    if (!s_comboConfig) {
        s_comboConfig = new KConfig(QStringLiteral("konq_history"), KConfig::NoGlobals);
        KonqCombo::setConfig(s_comboConfig);
        KConfigGroup locationBarGroup(s_comboConfig, "Location Bar");
        prov->load(locationBarGroup, QStringLiteral("ComboIconCache"));
    }

    connect(prov, SIGNAL(changed()), SLOT(slotIconsChanged()));

    m_pUndoManager = new KonqUndoManager(KonqClosedWindowsManager::self(), this);
    connect(m_pUndoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));

    initCombo();
    initActions();

    setXMLFile(QStringLiteral("konqueror.rc"));

    setStandardToolBarMenuEnabled(true);

    createGUI(nullptr);

    m_combo->setParent(toolBar(QStringLiteral("locationToolBar")));
    m_combo->show();

    checkDisableClearButton();

    connect(toolBarMenuAction(), SIGNAL(triggered()), this, SLOT(slotForceSaveMainWindowSettings()));

    if (!m_toggleViewGUIClient->empty()) {
        plugActionList(QStringLiteral("toggleview"), m_toggleViewGUIClient->actions());
    } else {
        delete m_toggleViewGUIClient;
        m_toggleViewGUIClient = nullptr;
    }

    m_bNeedApplyKonqMainWindowSettings = true;

    if (!initialURL.isEmpty()) {
        openFilteredUrl(initialURL.url());
    } else {
        // silent
        m_bNeedApplyKonqMainWindowSettings = false;
    }

    resize(700, 480);

    updateProxyForWebEngine(false);
    QDBusConnection::sessionBus().connect("", QStringLiteral("/KIO/Scheduler"), QStringLiteral("org.kde.KIO.Scheduler"),
                                          QStringLiteral("reparseSlaveConfiguration"), this, SLOT(updateProxyForWebEngine()));
    setAutoSaveSettings();

    //qCDebug(KONQUEROR_LOG) << this << "created";

    m_fullyConstructed = true;
}

KonqMainWindow::~KonqMainWindow()
{
    //qCDebug(KONQUEROR_LOG) << this;

    delete m_pViewManager;
    m_pViewManager = nullptr;

    if (s_lstMainWindows) {
        s_lstMainWindows->removeAll(this);
        if (s_lstMainWindows->isEmpty()) {
            delete s_lstMainWindows;
            s_lstMainWindows = nullptr;
        } else if (s_lstMainWindows->length() == 1 && s_lstMainWindows->first()->isPreloaded()) {
            //If the only remaining window is preloaded, we want to close it. Otherwise,
            //the application will remain open, even if there are no visible windows.
            //This can be seen, for example, running Konqueror from a terminal with the
            //"Always try to have a preloaded instance": even after closing the last
            //window, Konqueror won't exit (see bug #258124). To avoid this situation,
            //We close the preloaded window here, so that the application can exit,
            //and launch another instance of Konqueror from konqmain.
            s_lstMainWindows->first()->close();
        }
    }

    qDeleteAll(m_openWithActions);
    m_openWithActions.clear();

    delete m_pBookmarkMenu;
    delete m_paBookmarkBar;
    delete m_pBookmarksOwner;
    delete m_pURLCompletion;
    delete m_paClosedItems;

    if (s_lstMainWindows == nullptr) {
        delete s_comboConfig;
        s_comboConfig = nullptr;
    }

    delete m_configureDialog;
    m_configureDialog = nullptr;
    delete m_combo;
    m_combo = nullptr;
    delete m_locationLabel;
    m_locationLabel = nullptr;
    m_pUndoManager->disconnect();
    delete m_pUndoManager;

    //qCDebug(KONQUEROR_LOG) << this << "deleted";
}

QWidget *KonqMainWindow::createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction)
{
    QWidget *res = KParts::MainWindow::createContainer(parent, index, element, containerAction);

    static QString nameBookmarkBar = QStringLiteral("bookmarkToolBar");
    static QString tagToolBar = QStringLiteral("ToolBar");
    if (res && (element.tagName() == tagToolBar) && (element.attribute(QStringLiteral("name")) == nameBookmarkBar)) {
        Q_ASSERT(::qobject_cast<KToolBar *>(res));
        if (!KAuthorized::authorizeAction(QStringLiteral("bookmarks"))) {
            delete res;
            return nullptr;
        }

        if (!m_bookmarkBarInitialized) {
            // The actual menu needs a different action collection, so that the bookmarks
            // don't appear in kedittoolbar
            m_bookmarkBarInitialized = true;
            DelayedInitializer *initializer = new DelayedInitializer(QEvent::Show, res);
            connect(initializer, &DelayedInitializer::initialize, this, &KonqMainWindow::initBookmarkBar);
        }
    }

    if (res && element.tagName() == QLatin1String("Menu")) {
        const QString &menuName = element.attribute(QStringLiteral("name"));
        if (menuName == QLatin1String("edit") || menuName == QLatin1String("tools")) {
            Q_ASSERT(qobject_cast<QMenu *>(res));
            KAcceleratorManager::manage(static_cast<QMenu *>(res));
        }
    }

    return res;
}

void KonqMainWindow::initBookmarkBar()
{
    KToolBar *bar = this->findChild<KToolBar *>(QStringLiteral("bookmarkToolBar"));

    if (!bar) {
        return;
    }

    const bool wasVisible = bar->isVisible();

    delete m_paBookmarkBar;
    m_paBookmarkBar = new KBookmarkBar(s_bookmarkManager, m_pBookmarksOwner, bar, this);

    // hide if empty
    if (bar->actions().count() == 0 || !wasVisible) {
        bar->hide();
    }
}

void KonqMainWindow::removeContainer(QWidget *container, QWidget *parent, QDomElement &element, QAction *containerAction)
{
    static QString nameBookmarkBar = QStringLiteral("bookmarkToolBar");
    static QString tagToolBar = QStringLiteral("ToolBar");

    if (element.tagName() == tagToolBar && element.attribute(QStringLiteral("name")) == nameBookmarkBar) {
        Q_ASSERT(::qobject_cast<KToolBar *>(container));
        if (m_paBookmarkBar) {
            m_paBookmarkBar->clear();
        }
    }

    KParts::MainWindow::removeContainer(container, parent, element, containerAction);
}

// Detect a name filter (e.g. *.txt) in the url.
// Note that KShortURIFilter does the same, but we have no way of getting it from there
//
// Note: this removes the filter from the URL.
QString KonqMainWindow::detectNameFilter(QUrl &url)
{
    if (!KProtocolManager::supportsListing(url)) {
        return QString();
    }

    // Look for wildcard selection
    QString nameFilter;
    QString path = url.path();
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash > -1) {
        if (!url.query().isEmpty() && lastSlash == path.length() - 1) {  //  In /tmp/?foo, foo isn't a query
            path += url.query(); // includes the '?'
        }
        QString fileName = path.mid(lastSlash + 1);
        if (fileName.indexOf('*') != -1 || fileName.indexOf('[') != -1 || fileName.indexOf('?') != -1) {
            // Check that a file or dir with all the special chars in the filename doesn't exist
            if (url.isLocalFile()) {
                if (!QFile(url.toLocalFile()).exists()) {
                    nameFilter = fileName;
                }
            } else { // not a local file
                KIO::StatJob *job = KIO::statDetails(url, KIO::StatJob::DestinationSide, KIO::StatBasic, KIO::HideProgressInfo);
                // if there's an error stat'ing url, then assume it doesn't exist
                nameFilter = !job->exec() ? fileName : QString();
            }

            if (!nameFilter.isEmpty()) {
                url = url.adjusted(QUrl::RemoveFilename | QUrl::RemoveQuery);
                qCDebug(KONQUEROR_LOG) << "Found wildcard. nameFilter=" << nameFilter << "  New url=" << url;
            }
        }
    }

    return nameFilter;
}

void KonqMainWindow::openFilteredUrl(const QString &url, const KonqOpenURLRequest &req)
{
    // Filter URL to build a correct one
    if (m_currentDir.isEmpty() && m_currentView) {
        m_currentDir = m_currentView->url();
    }

    QUrl filteredURL(KonqMisc::konqFilteredURL(this, url, m_currentDir));
    qCDebug(KONQUEROR_LOG) << "url" << url << "filtered into" << filteredURL;

    if (filteredURL.isEmpty()) { // initially empty, or error (e.g. ~unknown_user)
        return;
    }

    m_currentDir.clear();

    openUrl(nullptr, filteredURL, QString(), req);

    // #4070: Give focus to view after URL was entered manually
    // Note: we do it here if the view mode (i.e. part) wasn't changed
    // If it is changed, then it's done in KonqViewManager::doSetActivePart
    if (m_currentView) {
        m_currentView->setFocus();
    }
}

void KonqMainWindow::openFilteredUrl(const QString &_url, bool inNewTab, bool tempFile)
{
    KonqOpenURLRequest req(_url);
    req.browserArgs.setNewTab(inNewTab);
    req.newTabInFront = true;
    req.tempFile = tempFile;

    openFilteredUrl(_url, req);
}

void KonqMainWindow::openFilteredUrl(const QString &_url,  const QString &_mimeType, bool inNewTab, bool tempFile)
{
    KonqOpenURLRequest req(_url);
    req.browserArgs.setNewTab(inNewTab);
    req.newTabInFront = true;
    req.tempFile = tempFile;
    req.args.setMimeType(_mimeType);

    openFilteredUrl(_url, req);
}

void KonqMainWindow::openUrl(KonqView *_view, const QUrl &_url,
                             const QString &_mimeType, const KonqOpenURLRequest &_req,
                             bool trustedSource)
{
#ifndef NDEBUG // needed for req.debug()
    qCDebug(KONQUEROR_LOG) << "url=" << _url << "mimeType=" << _mimeType
             << "_req=" << _req.debug() << "view=" << _view;
#endif
    // We like modifying args in this method :)
    QUrl url(_url);
    QString mimeType(_mimeType);
    KonqOpenURLRequest req(_req);

    if (mimeType.isEmpty()) {
        mimeType = req.args.mimeType();
    }
    if (!url.isValid()) {
        // I think we can't really get here anymore; I tried and didn't succeed.
        // URL filtering catches this case before hand, and in cases without filtering
        // (e.g. HTML link), the url is empty here, not invalid.
        // But just to be safe, let's keep this code path
        url = KParts::BrowserRun::makeErrorUrl(KIO::ERR_MALFORMED_URL, url.url(), url);
    } else if (!KProtocolInfo::isKnownProtocol(url) && url.scheme() != QLatin1String("error") && !KonqUrl::hasKonqScheme(url) && url.scheme() != QLatin1String("mailto") && url.scheme() != QLatin1String("data")) {
        url = KParts::BrowserRun::makeErrorUrl(KIO::ERR_UNSUPPORTED_PROTOCOL, url.scheme(), url);
    }

    const QString nameFilter = detectNameFilter(url);
    if (!nameFilter.isEmpty()) {
        req.nameFilter = nameFilter;
        url = url.adjusted(QUrl::RemoveFilename);
    }

    QLineEdit *edit = comboEdit();
    if (edit) {
        edit->setModified(false);
    }

    KonqView *view = _view;

    UrlLoader *loader = new UrlLoader(this, view, url, mimeType, req, trustedSource, false);
    connect(loader, &UrlLoader::finished, this, &KonqMainWindow::urlLoaderFinished);

    loader->start();

    // The URL should be opened in a new tab. Let's create the tab right away,
    // it gives faster user feedback (#163628). For a short while (kde-4.1-beta1)
    // I removed this entire block so that we wouldn't end up with a useless tab when
    // launching an external application for this mimetype. But user feedback
    // in all cases is more important than empty tabs in some cases.
    if (loader->viewToUse() == UrlLoader::ViewToUse::NewTab) {
        view = createTabForLoadUrlRequest(loader->url(), loader->request());
        if (!view) {
            loader->setNewTab(false);
        }
    } else if (loader->viewToUse() == UrlLoader::ViewToUse::CurrentView) {
        view = m_currentView;
    }

    const QString oldLocationBarURL = locationBarURL();
    if (view) {
        if (view == m_currentView) {
            //will do all the stuff below plus GUI stuff
            abortLoading();
        } else {
            view->stop();
            // Don't change location bar if not current view
        }
    }
    loader->setView(view);
    loader->setOldLocationBarUrl(oldLocationBarURL);

    if (loader->isAsync()) {
        bool earlySetLocationBarURL = false;
        if (!view && !m_currentView) { // no view yet, e.g. starting with url as argument
            earlySetLocationBarURL = true;
        } else if (view == m_currentView && view->url().isEmpty()) { // opening in current view
            earlySetLocationBarURL = true;
        }
        if (req.browserArgs.newTab()) { // it's going into a new tab anyway
            earlySetLocationBarURL = false;
        }
        if (earlySetLocationBarURL) {
            // Show it for now in the location bar, but we'll need to store it in the view
            // later on (can't do it yet since either view == 0 or updateHistoryEntry will be called).
            qCDebug(KONQUEROR_LOG) << "url=" << url;
            setLocationBarURL(url);
        }
        if (view == m_currentView) {
            startAnimation();
        }
    }
    loader->goOn();
}

void KonqMainWindow::urlLoaderFinished(UrlLoader* loader)
{
    if (loader->hasError()) {   // we had an error
        QDBusMessage message = QDBusMessage::createSignal(KONQ_MAIN_PATH, QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("removeFromCombo"));
        message << loader->url().toDisplayString();
        QDBusConnection::sessionBus().send(message);
    }

    KonqView *childView = loader->view();

    // Check if we found a mimetype _and_ we got no error (example: cancel in openwith dialog)
    if (!loader->mimeType().isEmpty() && !loader->hasError()) {

        // We do this here and not in the constructor, because
        // we are waiting for the first view to be set up before doing this...
        // Note: this is only used when konqueror is started from command line.....
        if (m_bNeedApplyKonqMainWindowSettings) {
            m_bNeedApplyKonqMainWindowSettings = false; // only once
            applyKonqMainWindowSettings();
        }

        return;
    }

    // An error happened in UrlLoader - stop wheel etc.

    if (childView) {
        childView->setLoading(false);

        if (childView == m_currentView) {
            stopAnimation();

            // Revert to working URL - unless the URL was typed manually
            if (loader->request().typedUrl.isEmpty() && childView->currentHistoryEntry()) { // not typed
                childView->setLocationBarURL(childView->currentHistoryEntry()->locationBarURL);
            }
        }
    } else { // No view, e.g. starting up empty
        stopAnimation();
    }
}

KonqView * KonqMainWindow::createTabForLoadUrlRequest(const QUrl& url, const KonqOpenURLRequest& request)
{
    KonqView *view = m_pViewManager->addTab(QStringLiteral("text/html"),
                                    QString(),
                                    false,
                                    request.openAfterCurrentPage);
    if (view) {
        view->setCaption(i18nc("@title:tab", "Loading..."));
        view->setLocationBarURL(url);
        if (!request.browserArgs.frameName.isEmpty()) {
            view->setViewName(request.browserArgs.frameName);    // #44961
        }

        if (request.newTabInFront) {
            m_pViewManager->showTab(view);
        }

        updateViewActions(); //A new tab created -- we may need to enable the "remove tab" button (#56318)
    }
    return view;
}


// When opening a new view, for @p mimeType, prefer the part used in @p currentView, if possible.
// Testcase: window.open after manually switching to another web engine, and with "open popups in tabs" off.
static QString preferredService(KonqView *currentView, const QString &mimeType)
{
    if (currentView && !mimeType.isEmpty() && currentView->supportsMimeType(mimeType)) {
        return currentView->service().pluginId();
    }
    return QString();
}

//TODO After removing KonqRun: some of this becomes redundant, at least when called via UrlLoader. Can this be avoided?
bool KonqMainWindow::openView(QString mimeType, const QUrl &_url, KonqView *childView, const KonqOpenURLRequest &req)
{
    // Second argument is referring URL
    if (!KUrlAuthorized::authorizeUrlAction(QStringLiteral("open"), childView ? childView->url() : QUrl(), _url)) {
        QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, _url.toDisplayString());
        QMessageBox::warning(this, i18n("Access denied"), msg);
        return true; // Nothing else to do.
    }

//TODO Remove KonqRun: the following lines prevent embedding script files. If the user chose, from UrlLoader, not to
//execute a script but to open it, it should be embedded or opened in a separate viewer according to the user
//preferences. With the check below, instead, it'll always been opened in the external application.
//Are there situations when this check is needed? When this function is called by UrlLoader, it's not necessary
//because UrlLoader only calls it if the user chose to embed the URL. The only other caller of this function
//seems to be KonqViewManager::loadItem, which uses it when restoring or duplicating a view. Since a view can't
//contain an executed program, I think we can assume that the file should be embedded.
//     if (UrlLoader::isExecutable(mimeType)) {
//         return false;    // execute, don't open
//     }

    // Contract: the caller of this method should ensure the view is stopped first.

#ifndef NDEBUG
    qCDebug(KONQUEROR_LOG) << mimeType << _url << "childView=" << childView << "req:" << req.debug();
#endif
    bool bOthersFollowed = false;

    if (childView) {
        // If we're not already following another view (and if we are not reloading)
        if (!req.followMode && !req.args.reload() && !m_pViewManager->isLoadingProfile()) {
            // When clicking a 'follow active' view (e.g. childView is the sidebar),
            // open the URL in the active view
            // (it won't do anything itself, since it's locked to its location)
            if (childView->isFollowActive() && childView != m_currentView) {
                abortLoading();
                setLocationBarURL(_url);
                KonqOpenURLRequest newreq;
                newreq.forceAutoEmbed = true;
                newreq.followMode = true;
                newreq.args = req.args;
                newreq.browserArgs = req.browserArgs;
                bOthersFollowed = openView(mimeType, _url, m_currentView, newreq);
            }
            // "link views" feature, and "sidebar follows active view" feature
            bOthersFollowed = makeViewsFollow(_url, req.args, req.browserArgs, mimeType, childView) || bOthersFollowed;
        }
        if (childView->isLockedLocation() && !req.args.reload() /* allow to reload a locked view*/) {
            return bOthersFollowed;
        }
    }

    QUrl url(_url);

    // In case we open an index.html, we want the location bar
    // to still display the original URL (so that 'up' uses that URL,
    // and since that's what the user entered).
    // changePart will take care of setting and storing that url.
    QString originalURL = url.toDisplayString(QUrl::PreferLocalFile);
    if (!req.nameFilter.isEmpty()) { // keep filter in location bar
        if (!originalURL.endsWith('/')) {
            originalURL += '/';
        }
        originalURL += req.nameFilter;
    }

    QString serviceName = req.serviceName; // default: none provided

    const QString urlStr = url.url();
    if (KonqUrl::isValidNotBlank(urlStr)) {
        mimeType = QStringLiteral("text/html");
        originalURL = req.typedUrl.isEmpty() ? QString() : req.typedUrl;
    } else if (KonqUrl::isKonqBlank(urlStr) && req.typedUrl.isEmpty()) {
        originalURL.clear();
    }

    bool forceAutoEmbed = req.forceAutoEmbed || req.userRequestedReload;
    if (!req.typedUrl.isEmpty()) { // the user _typed_ the URL, he wants it in Konq.
        forceAutoEmbed = true;
    }
    if (KonqUrl::hasKonqScheme(url) || url.scheme() == QLatin1String("error")) {
        forceAutoEmbed = true;
    }
    // Related to KonqFactory::createView
    if (!forceAutoEmbed && !KonqFMSettings::settings()->shouldEmbed(mimeType)) {
        qCDebug(KONQUEROR_LOG) << "KonqFMSettings says: don't embed this servicetype";
        return false;
    }
    // Do we even have a part to embed? Otherwise don't ask, since we'd ask twice.
    if (!forceAutoEmbed) {
        QVector<KPluginMetaData> partServiceOffers;
        KonqFactory::getOffers(mimeType, &partServiceOffers);
        if (partServiceOffers.isEmpty()) {
            qCDebug(KONQUEROR_LOG) << "No part available for" << mimeType;
            return false;
        }
    }

    // If the protocol doesn't support writing (e.g. HTTP) then we might want to save instead of just embedding.
    // So (if embedding would succeed, hence the checks above) we ask the user
    // Otherwise the user will get asked 'open or save' in openUrl anyway.
    if (!forceAutoEmbed && !KProtocolManager::supportsWriting(url)) {
        QString suggestedFileName;
        UrlLoader *loader = childView ? childView->urlLoader() : nullptr;
        if (loader) {
            suggestedFileName = loader->suggestedFileName();
        }

        KParts::BrowserOpenOrSaveQuestion dlg(this, url, mimeType);
        dlg.setSuggestedFileName(suggestedFileName);
        const KParts::BrowserOpenOrSaveQuestion::Result res = dlg.askEmbedOrSave();
        if (res == KParts::BrowserOpenOrSaveQuestion::Embed) {
            forceAutoEmbed = true;
        } else if (res == KParts::BrowserOpenOrSaveQuestion::Cancel) {
            return true;    // handled, don't do anything else
        } else { // Save
            KParts::BrowserRun::saveUrl(url, suggestedFileName, this, req.args);
            return true; // handled
        }
    }

    bool ok = true;
    if (!childView) {
        if (req.browserArgs.newTab()) {
            KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
            int index = tabContainer->currentIndex();
            childView = m_pViewManager->addTab(mimeType, serviceName, false, req.openAfterCurrentPage);

            if (req.newTabInFront && childView) {
                if (req.openAfterCurrentPage) {
                    tabContainer->setCurrentIndex(index + 1);
                } else {
                    tabContainer->setCurrentIndex(tabContainer->count() - 1);
                }
            }
        }

        else {
            // Create a new view
            // createFirstView always uses force auto-embed even if user setting is "separate viewer",
            // since this window has no view yet - we don't want to keep an empty mainwindow.
            // This can happen with e.g. application/pdf from a target="_blank" link, or window.open.
            childView = m_pViewManager->createFirstView(mimeType, serviceName);

            if (childView) {
                enableAllActions(true);
                m_currentView = childView;
            }
        }

        if (!childView) {
            return false;    // It didn't work out.
        }

        childView->setViewName(m_initialFrameName.isEmpty() ? req.browserArgs.frameName : m_initialFrameName);
        m_initialFrameName.clear();
    } else { // We know the child view
        if (!childView->isLockedViewMode()) {
            if (ok) {

                // When typing a new URL, the current context doesn't matter anymore
                // -> select the preferred part for a given mimetype (even if the current part can handle this mimetype).
                // This fixes the "get katepart and then type a website URL -> loaded into katepart" problem
                // (first fixed in r168902 from 2002!, see also unittest KonqHtmlTest::textThenHtml())

                if (!req.typedUrl.isEmpty() || !serviceName.isEmpty()) {
                    if (childView->isLoading()) { // Stop the view first, #282641.
                        childView->stop();
                    }
                    ok = childView->changePart(mimeType, serviceName, forceAutoEmbed);
                } else {
                    ok = childView->ensureViewSupports(mimeType, forceAutoEmbed);
                }
            }
        }
    }

    if (ok) {
        //qCDebug(KONQUEROR_LOG) << "req.nameFilter= " << req.nameFilter;
        //qCDebug(KONQUEROR_LOG) << "req.typedUrl= " << req.typedUrl;
        //qCDebug(KONQUEROR_LOG) << "Browser extension? " << (childView->browserExtension() ? "YES" : "NO");
        //qCDebug(KONQUEROR_LOG) << "Referrer: " << req.args.metaData()["referrer"];
        childView->setTypedURL(req.typedUrl);
        if (childView->part()) {
            childView->part()->setArguments(req.args);
        }
        if (childView->browserExtension()) {
            childView->browserExtension()->setBrowserArguments(req.browserArgs);
        }

        // see dolphinpart
        childView->part()->setProperty("filesToSelect", QVariant::fromValue(req.filesToSelect));

        if (!url.isEmpty()) {
            childView->openUrl(url, originalURL, req.nameFilter, req.tempFile);
        }
    }
    //qCDebug(KONQUEROR_LOG) << "ok=" << ok << "bOthersFollowed=" << bOthersFollowed
    //             << "returning" << (ok || bOthersFollowed);
    return ok || bOthersFollowed;
}

static KonqView *findChildView(KParts::ReadOnlyPart *callingPart, const QString &name, KonqMainWindow *&mainWindow, KParts::ReadOnlyPart **part)
{
    if (!KonqMainWindow::mainWindowList()) {
        return nullptr;
    }

    foreach (KonqMainWindow *window, *KonqMainWindow::mainWindowList()) {
        KonqView *res = window->childView(callingPart, name, part);
        if (res) {
            mainWindow = window;
            return res;
        }
    }

    return nullptr;
}

void KonqMainWindow::slotOpenURLRequest(const QUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs)
{
    //qCDebug(KONQUEROR_LOG) << "frameName=" << browserArgs.frameName;

    KParts::ReadOnlyPart *callingPart = static_cast<KParts::ReadOnlyPart *>(sender()->parent());
    QString frameName = browserArgs.frameName;

    if (!frameName.isEmpty()) {
        static QString _top = QStringLiteral("_top");
        static QString _self = QStringLiteral("_self");
        static QString _parent = QStringLiteral("_parent");
        static QString _blank = QStringLiteral("_blank");

        if (frameName.toLower() == _blank) {
            KonqMainWindow *mainWindow = (m_popupProxyWindow ? m_popupProxyWindow.data() : this);
            mainWindow->slotCreateNewWindow(url, args, browserArgs);
            if (m_isPopupWithProxyWindow) {
                raiseWindow(mainWindow);
            }
            return;
        }

        if (frameName.toLower() != _top &&
                frameName.toLower() != _self &&
                frameName.toLower() != _parent) {
            KonqView *view = childView(callingPart, frameName, nullptr);
            if (!view) {
                KonqMainWindow *mainWindow = nullptr;
                view = findChildView(callingPart, frameName, mainWindow, nullptr);

                if (!view || !mainWindow) {
                    slotCreateNewWindow(url, args, browserArgs);
                    return;
                }

                mainWindow->openUrlRequestHelper(view, url, args, browserArgs);
                return;
            }

            openUrlRequestHelper(view, url, args, browserArgs);
            return;
        }
    }

    KonqView *view = browserArgs.newTab() ? nullptr : childView(callingPart);
    openUrlRequestHelper(view, url, args, browserArgs);
}

//Called by slotOpenURLRequest
void KonqMainWindow::openUrlRequestHelper(KonqView *childView, const QUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs)
{
    //qCDebug(KONQUEROR_LOG) << "url=" << url;
    KonqOpenURLRequest req;
    req.args = args;
    if (args.metaData().value("konq-temp-file") == "1") {
        req.tempFile = true;
    }
    req.suggestedFileName =args.metaData().value("SuggestedFileName");
    req.browserArgs = browserArgs;
    openUrl(childView, url, args.mimeType(), req, browserArgs.trustedSource);
}

QObject *KonqMainWindow::lastFrame(KonqView *view)
{
    QObject *nextFrame, *viewFrame;
    nextFrame = view->frame();
    viewFrame = nullptr;
    while (nextFrame != nullptr && !::qobject_cast<QStackedWidget *>(nextFrame)) {
        viewFrame = nextFrame;
        nextFrame = nextFrame->parent();
    }
    return nextFrame ? viewFrame : nullptr;
}

// Linked-views feature, plus "sidebar follows URL opened in the active view" feature
bool KonqMainWindow::makeViewsFollow(const QUrl &url,
                                     const KParts::OpenUrlArguments &args,
                                     const KParts::BrowserArguments &browserArgs,
                                     const QString &serviceType, KonqView *senderView)
{
    if (!senderView->isLinkedView() && senderView != m_currentView) {
        return false;    // none of those features apply -> return
    }

    bool res = false;
    //qCDebug(KONQUEROR_LOG) << senderView->metaObject()->className() << "url=" << url << "serviceType=" << serviceType;
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;
    req.followMode = true;
    req.args = args;
    req.browserArgs = browserArgs;
    // We can't iterate over the map here, and openUrl for each, because the map can get modified
    // (e.g. by part changes). Better copy the views into a list.
    const QList<KonqView *> listViews = m_mapViews.values();

    QObject *senderFrame = lastFrame(senderView);

    foreach (KonqView *view, listViews) {
        if (view == senderView) {
            continue;
        }
        bool followed = false;
        // Views that should follow this URL as both views are linked
        if (view->isLinkedView() && senderView->isLinkedView()) {
            QObject *viewFrame = lastFrame(view);

            // Only views in the same tab of the sender will follow
            if (senderFrame && viewFrame && viewFrame != senderFrame) {
                continue;
            }

            qCDebug(KONQUEROR_LOG) << "sending openUrl to view" << view->part()->metaObject()->className() << "url=" << url;

            // XXX duplicate code from ::openUrl
            if (view == m_currentView) {
                abortLoading();
                setLocationBarURL(url);
            } else {
                view->stop();
            }

            followed = openView(serviceType, url, view, req);
        } else {
            // Make the sidebar follow the URLs opened in the active view
            if (view->isFollowActive() && senderView == m_currentView) {
                followed = openView(serviceType, url, view, req);
            }
        }

        // Ignore return value if the view followed but doesn't really
        // show the file contents. We still want to see that
        // file, e.g. in a separate viewer.
        // This happens in views locked to a directory mode,
        // like sidebar and konsolepart (#52161).
        const bool ignore = view->isLockedViewMode() && view->showsDirectory();
        //qCDebug(KONQUEROR_LOG) << "View " << view->service()->name()
        //              << " supports dirs: " << view->showsDirectory()
        //              << " is locked-view-mode:" << view->isLockedViewMode()
        //              << " ignore=" << ignore;
        if (!ignore) {
            res = followed || res;
        }
    }

    return res;
}

void KonqMainWindow::abortLoading()
{
    if (m_currentView) {
        m_currentView->stop(); // will take care of the statusbar
        stopAnimation();
    }
}

// Are there any indications that this window has a strong popup
// nature and should therefore not be embedded into a tab?
static bool isPopupWindow(const KParts::WindowArgs &windowArgs)
{
    // ### other settings to respect?
    return windowArgs.x() != -1 || windowArgs.y() != -1 ||
           windowArgs.width() != -1 || windowArgs.height() != -1 ||
           !windowArgs.isMenuBarVisible() ||
           !windowArgs.toolBarsVisible() ||
           !windowArgs.isStatusBarVisible();
}

// This is called for the javascript window.open call.
// Also called for MMB on link, target="_blank" link, MMB on folder, etc.
void KonqMainWindow::slotCreateNewWindow(const QUrl &url,
        const KParts::OpenUrlArguments &args,
        const KParts::BrowserArguments &browserArgs,
        const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart **part)
{
    // NOTE: 'part' may be null

    qCDebug(KONQUEROR_LOG) << "url=" << url << "args.mimeType()=" << args.mimeType()
             << "browserArgs.frameName=" << browserArgs.frameName;

    // If we are a popup window, forward the request the proxy window.
    if (m_isPopupWithProxyWindow && m_popupProxyWindow) {
        m_popupProxyWindow->slotCreateNewWindow(url, args, browserArgs, windowArgs, part);
        raiseWindow(m_popupProxyWindow);
        return;
    }

    if (part) {
        *part = nullptr;    // Make sure to be initialized in case of failure...
    }

    KonqMainWindow *mainWindow = nullptr;
    if (!browserArgs.frameName.isEmpty() && browserArgs.frameName.toLower() != QLatin1String("_blank")) {
        KParts::ReadOnlyPart *ro_part = nullptr;
        KParts::BrowserExtension *be = ::qobject_cast<KParts::BrowserExtension *>(sender());
        if (be) {
            ro_part = ::qobject_cast<KParts::ReadOnlyPart *>(be->parent());
        }
        if (findChildView(ro_part, browserArgs.frameName, mainWindow, part)) {
            // Found a view. If url isn't empty, we should open it - but this never happens currently
            // findChildView put the resulting part in 'part', so we can just return now
            //qCDebug(KONQUEROR_LOG) << "frame=" << browserArgs.frameName << "-> found part=" << part << part->name();
            return;
        }
    }

    bool createTab = browserArgs.newTab();
    if (!createTab && !browserArgs.forcesNewWindow() /* explicit "Open in New Window" action, e.g. on frame or history item */) {
        if (args.actionRequestedByUser()) { // MMB or some RMB popupmenu action
            createTab = KonqSettings::mmbOpensTab();
        } else { // Javascript popup
            createTab = KonqSettings::popupsWithinTabs() &&
                        !isPopupWindow(windowArgs);
        }
    }
    qCDebug(KONQUEROR_LOG) << "createTab=" << createTab << "part=" << part;

    if (createTab && !m_isPopupWithProxyWindow) {

        bool newtabsinfront = !windowArgs.lowerWindow();
        if (KonqSettings::newTabsInFront()) {
            newtabsinfront = !newtabsinfront;
        }
        const bool aftercurrentpage = KonqSettings::openAfterCurrentPage();

        KonqOpenURLRequest req;
        req.args = args;
        req.browserArgs = browserArgs;
        // Can we use the standard way (openUrl), or do we need the part pointer immediately?
        if (!part) {
            req.browserArgs.setNewTab(true);
            req.forceAutoEmbed = true; // testcase: MMB on link-to-PDF, when pdf setting is "show file in external browser".
            req.newTabInFront = newtabsinfront;
            req.openAfterCurrentPage = aftercurrentpage;
            openUrl(nullptr, url, args.mimeType(), req);
        } else {
            KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"), QString(), false, aftercurrentpage);
            if (newView == nullptr) {
                return;
            }

            if (newtabsinfront) {
                m_pViewManager->showTab(newView);
            }

            openUrl(newView, url.isEmpty() ? KonqUrl::url(KonqUrl::Type::Blank) : url, QString(), req);
            newView->setViewName(browserArgs.frameName);

            *part = newView->part();
        }

        // Raise the current window if the request to create the tab came from a popup
        // window, e.g. clicking on links with target = "_blank" in popup windows.
        KParts::BrowserExtension *be = qobject_cast<KParts::BrowserExtension *>(sender());
        KonqView *view = (be ? childView(qobject_cast<KParts::ReadOnlyPart *>(be->parent())) : nullptr);
        KonqMainWindow *window = view ? view->mainWindow() : nullptr;
        if (window && window->m_isPopupWithProxyWindow && !m_isPopupWithProxyWindow) {
            raiseWindow(this);
        }

        return;
    }

    KonqOpenURLRequest req;
    req.args = args;
    req.browserArgs = browserArgs;
    req.browserArgs.setNewTab(false); // we got a new window, no need for a new tab in that window
    req.forceAutoEmbed = true;
    req.serviceName = preferredService(m_currentView, args.mimeType());

    mainWindow = KonqMainWindowFactory::createEmptyWindow();
    mainWindow->resetAutoSaveSettings(); // Don't autosave

    // Do we know the mimetype? If not, go to generic openUrl which will use a KonqRun.
    if (args.mimeType().isEmpty()) {
        mainWindow->openUrl(nullptr, url, QString(), req);
    } else {
        if (!mainWindow->openView(args.mimeType(), url, mainWindow->currentView(), req)) {
            // we have problems. abort.
            delete mainWindow;

            if (part) {
                *part = nullptr;
            }
            return;
        }
    }

    qCDebug(KONQUEROR_LOG) << "newWindow" << mainWindow << "currentView" << mainWindow->currentView() << "views" << mainWindow->viewMap().count();

    KonqView *view = nullptr;
    // cannot use activePart/currentView, because the activation through the partmanager
    // is delayed by a singleshot timer (see KonqViewManager::setActivePart)
    // ### TODO: not true anymore
    if (mainWindow->viewMap().count()) {
        MapViews::ConstIterator it = mainWindow->viewMap().begin();
        view = it.value();

        if (part) {
            *part = it.key();
        }
    }

    // activate the view now in order to make the menuBar() hide call work
    if (part && *part) {
        mainWindow->viewManager()->setActivePart(*part);
    }

#if KONQ_HAVE_X11
    // WORKAROUND: Clear the window state information set by KMainWindow::restoreWindowSize
    // so that the size and location settings we set below always take effect.
    KWindowSystem::clearState(mainWindow->winId(), NET::Max);
#endif

    // process the window args
    const int xPos = ((windowArgs.x() == -1) ?  mainWindow->x() : windowArgs.x());
    const int yPos = ((windowArgs.y() == -1) ?  mainWindow->y() : windowArgs.y());
    const int width = ((windowArgs.width() == -1) ?  mainWindow->width() : windowArgs.width());
    const int height = ((windowArgs.height() == -1) ?  mainWindow->height() : windowArgs.height());

    mainWindow->move(xPos, yPos);
    mainWindow->resize(width, height);

    // Make the window open properties configurable. This is equivalent to
    // Firefox's "dom.disable_window_open_feature.*" properties. For now
    // only LocationBar visibility is configurable.
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup cfg(config, "DisableWindowOpenFeatures");

    if (!windowArgs.isMenuBarVisible()) {
        mainWindow->menuBar()->hide();
        mainWindow->m_paShowMenuBar->setChecked(false);
    }

    if (!windowArgs.toolBarsVisible()) {
        // For security reasons the address bar is NOT hidden by default. The
        // user can override the default setup by adding a config option
        // "LocationBar=false" to the [DisableWindowOpenFeatures] section of
        // konquerorrc.
        const bool showLocationBar = cfg.readEntry("LocationBar", true);
        KToolBar *locationToolBar = mainWindow->toolBar(QStringLiteral("locationToolBar"));

        Q_FOREACH (KToolBar *bar, mainWindow->findChildren<KToolBar *>()) {
            if (bar != locationToolBar || !showLocationBar) {
                bar->hide();
            }
        }

        if (locationToolBar && showLocationBar && isPopupWindow(windowArgs)) {
            // Hide all the actions of the popup window
            KActionCollection *collection = mainWindow->actionCollection();
            for (int i = 0, count = collection->count(); i < count; ++i) {
                collection->action(i)->setVisible(false);
            }

            // Show only those actions that are allowed in a popup window
            static const char *const s_allowedActions[] = {
                "go_back", "go_forward", "go_up", "reload", "hard_reload",
                "stop", "cut", "copy", "paste", "print", "fullscreen",
                "add_bookmark", "new_window", nullptr
            };
            for (int i = 0; s_allowedActions[i]; ++i) {
                if (QAction *action = collection->action(QLatin1String(s_allowedActions[i]))) {
                    action->setVisible(true);
                }
            }

            // Make only the address widget available in the location toolbar
            locationToolBar->clear();
            QAction *action = locationToolBar->addWidget(mainWindow->m_combo);
            action->setVisible(true);

            // Make the combo box non editable and clear it of previous history
            QLineEdit *edit = (mainWindow->m_combo ? mainWindow->m_combo->lineEdit() : nullptr);
            if (edit) {
                mainWindow->m_combo->clear();
                mainWindow->m_combo->setCompletionMode(KCompletion::CompletionNone);
                edit->setReadOnly(true);
            }

            // Store the originating window as the popup's proxy window so that
            // new tab requests in the popup window are forwarded to it.
            mainWindow->m_popupProxyWindow = this;
            mainWindow->m_isPopupWithProxyWindow = true;
        }
    }

    if (view) {
        if (!windowArgs.scrollBarsVisible()) {
            view->disableScrolling();
        }
        if (!windowArgs.isStatusBarVisible()) {
            view->frame()->statusbar()->hide();
            mainWindow->m_paShowStatusBar->setChecked(false);
        } else {
            mainWindow->m_paShowStatusBar->setChecked(true);
        }
    }

    if (!windowArgs.isResizable())
        // ### this doesn't seem to work :-(
    {
        mainWindow->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    }

// Trying to show the window initially behind the current window is a bit tricky,
// as this involves the window manager, which may see things differently.
// Many WMs raise and activate new windows, which means without WM support this won't work very
// well. If the WM has support for _NET_WM_USER_TIME, it will be just set to 0 (=don't focus on show),
// and the WM should take care of it itself.
    bool wm_usertime_support = false;

#if KONQ_HAVE_X11
    if (KWindowSystem::platform() == KWindowSystem::Platform::X11) {
        auto saved_last_input_time = QX11Info::appUserTime();
        if (windowArgs.lowerWindow()) {
            NETRootInfo wm_info(QX11Info::connection(), NET::Supported);
            wm_usertime_support = wm_info.isSupported(NET::WM2UserTime);
            if (wm_usertime_support) {
                // *sigh*, and I thought nobody would need QWidget::dontFocusOnShow().
                // Avoid Qt's support for user time by setting it to 0, and
                // set the property ourselves.
                QX11Info::setAppUserTime(0);
            }
            // Put below the current window before showing, in case that actually works with the WM.
            // First do complete lower(), then stackUnder(), because the latter may not work with many WMs.
            mainWindow->lower();
            mainWindow->stackUnder(this);
        }

        mainWindow->show();

        if (windowArgs.lowerWindow()) {
            QX11Info::setAppUserTime(saved_last_input_time);
            if (!wm_usertime_support) {
                // No WM support. Let's try ugly tricks.
                mainWindow->lower();
                mainWindow->stackUnder(this);
                if (this->isActiveWindow()) {
                    this->activateWindow();
                }
            }
        }
    }
#else // KONQ_HAVE_X11
    mainWindow->show();
#endif

    if (windowArgs.isFullScreen()) {
        mainWindow->action("fullscreen")->trigger();
    }
}

void KonqMainWindow::slotNewWindow()
{
    KonqMainWindow *mainWin = KonqMainWindowFactory::createNewWindow();
    mainWin->show();
}

void KonqMainWindow::slotDuplicateWindow()
{
    m_pViewManager->duplicateWindow()->show();
}

void KonqMainWindow::slotSendURL()
{
    const QList<QUrl> lst = currentURLs();
    QString body;
    QString fileNameList;
    for (QList<QUrl>::ConstIterator it = lst.constBegin(); it != lst.constEnd(); ++it) {
        if (!body.isEmpty()) {
            body += '\n';
        }
        body += (*it).toDisplayString();
        if (!fileNameList.isEmpty()) {
            fileNameList += QLatin1String(", ");
        }
        fileNameList += (*it).fileName();
    }
    QString subject;
    if (m_currentView && !m_currentView->showsDirectory()) {
        subject = m_currentView->caption();
    } else { // directory view
        subject = fileNameList;
    }
    QUrl mailtoUrl;
    mailtoUrl.setScheme(QStringLiteral("mailto"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), subject);
    query.addQueryItem(QStringLiteral("body"), body);
    mailtoUrl.setQuery(query);
    QDesktopServices::openUrl(mailtoUrl);
}

void KonqMainWindow::slotSendFile()
{
    const QList<QUrl> lst = currentURLs();
    QStringList urls;
    QString fileNameList;
    for (QList<QUrl>::ConstIterator it = lst.constBegin(); it != lst.constEnd(); ++it) {
        if (!fileNameList.isEmpty()) {
            fileNameList += QLatin1String(", ");
        }
        if ((*it).isLocalFile() && QFileInfo((*it).toLocalFile()).isDir()) {
            // Create a temp dir, so that we can put the ZIP file in it with a proper name
            // Problem: when to delete it?
            QTemporaryDir tempDir;
            tempDir.setAutoRemove(false);
            if (!tempDir.isValid()) {
                qCWarning(KONQUEROR_LOG) << "Could not create temporary dir";
                continue;
            }
            const QString zipFileName = tempDir.path() + '/' + (*it).fileName() + ".zip";
            KZip zip(zipFileName);
            if (!zip.open(QIODevice::WriteOnly)) {
                qCWarning(KONQUEROR_LOG) << "Could not open" << zipFileName << "for writing";
                continue;
            }
            zip.addLocalDirectory((*it).path(), QString());
            zip.close();
            fileNameList += (*it).fileName() + ".zip";
            urls.append(QUrl::fromLocalFile(zipFileName).url());
        } else {
            fileNameList += (*it).fileName();
            urls.append((*it).url());
        }
    }
    QString subject;
    if (m_currentView && !m_currentView->showsDirectory()) {
        subject = m_currentView->caption();
    } else {
        subject = fileNameList;
    }
    QUrl mailtoUrl;
    mailtoUrl.setScheme(QStringLiteral("mailto"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), subject);
    for (const QString& url : urls) {
        query.addQueryItem(QStringLiteral("attachment"), url);
    }
    mailtoUrl.setQuery(query);
    QDesktopServices::openUrl(mailtoUrl);
}

void KonqMainWindow::slotOpenLocation()
{
    focusLocationBar();
    QLineEdit *edit = comboEdit();
    if (edit) {
        edit->selectAll();
    }
}

void KonqMainWindow::slotOpenFile()
{
    QUrl currentUrl;
    if (m_currentView && m_currentView->url().isLocalFile()) {
        currentUrl = m_currentView->url();
    } else {
        currentUrl = QUrl::fromLocalFile(QDir::homePath());
    }

    QUrl url = QFileDialog::getOpenFileUrl(this, i18n("Open File"), currentUrl, QString());
    if (!url.isEmpty()) {
        openFilteredUrl(url.url().trimmed());
    }
}

void KonqMainWindow::slotIconsChanged()
{
    qCDebug(KONQUEROR_LOG);
    if (m_combo) {
        m_combo->updatePixmaps();
    }
    m_pViewManager->updatePixmaps();
    updateWindowIcon();
}

void KonqMainWindow::slotOpenWith()
{
    if (!m_currentView) {
        return;
    }

    const QString serviceName = sender()->objectName();
    const KService::List offers = m_currentView->appServiceOffers();
    for (const KService::Ptr &service : offers) {
        if (service->desktopEntryName() == serviceName) {
            KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service);
            job->setUrls({ m_currentView->url() });
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
            job->start();
            return;
        }
    }
}

void KonqMainWindow::slotViewModeTriggered(QAction *action)
{
    if (!m_currentView) {
        return;
    }

    //TODO port away from query: check that plugin id actually respect these rules
    // Gather data from the action, since the action will be deleted by changePart
    QString modeName = action->objectName();
    Q_ASSERT(modeName.endsWith("-viewmode"));
    modeName.chop(9);
    const QString internalViewMode = action->data().toString();

    if (m_currentView->service().pluginId() != modeName) {
        m_currentView->stop();
        m_currentView->lockHistory();

        // Save those, because changePart will lose them
        const QUrl url = m_currentView->url();
        const QString locationBarURL = m_currentView->locationBarURL();
#if 0
        // Problem: dolphinpart doesn't currently implement it. But we don't need it that much
        // now that it's the main filemanagement part for all standard modes.
        QList<QUrl> filesToSelect = childView->part()->property("filesToSelect").value<QList<QUrl>>();
#endif

        m_currentView->changePart(m_currentView->serviceType(), modeName);
        m_currentView->openUrl(url, locationBarURL);
    }

    if (!internalViewMode.isEmpty() && internalViewMode != m_currentView->internalViewMode()) {
        m_currentView->setInternalViewMode(internalViewMode);
    }
}

void KonqMainWindow::slotLockView()
{
    if (!m_currentView) {
        return;
    }

    m_currentView->setLockedLocation(m_paLockView->isChecked());
}

void KonqMainWindow::slotStop()
{
    abortLoading();
    if (m_currentView) {
        m_currentView->frame()->statusbar()->message(i18n("Canceled."));
    }
}

void KonqMainWindow::slotLinkView()
{
    if (!m_currentView) {
        return;
    }

    // Can't access this action in passive mode anyway
    Q_ASSERT(!m_currentView->isPassiveMode());
    const bool mode = !m_currentView->isLinkedView();

    const QList<KonqView *> linkableViews = KonqLinkableViewsCollector::collect(this);
    if (linkableViews.count() == 2) {
        // Exactly two linkable views : link both
        linkableViews.at(0)->setLinkedView(mode);
        linkableViews.at(1)->setLinkedView(mode);
    } else { // Normal case : just this view
        m_currentView->setLinkedView(mode);
    }
}

void KonqMainWindow::slotReload(KonqView *reloadView, bool softReload)
{
    if (!reloadView) {
        reloadView = m_currentView;
    }

    if (!reloadView || (reloadView->url().isEmpty() && reloadView->locationBarURL().isEmpty())) {
        return;
    }

    if (reloadView->isModified()) {
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("This page contains changes that have not been submitted.\nReloading the page will discard these changes."),
                                               i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("view-refresh")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesreload")) != KMessageBox::Continue) {
            return;
        }
    }

    KonqOpenURLRequest req(reloadView->typedUrl());
    req.userRequestedReload = true;
    if (reloadView->prepareReload(req.args, req.browserArgs, softReload)) {
        reloadView->lockHistory();
        // Reuse current servicetype for local files, but not for remote files (it could have changed, e.g. over HTTP)
        QString serviceType = reloadView->url().isLocalFile() ? reloadView->serviceType() : QString();
        // By using locationBarURL instead of url, we preserve name filters (#54687)
        QUrl reloadUrl = QUrl::fromUserInput(reloadView->locationBarURL(), QString(), QUrl::AssumeLocalFile);
        if (reloadUrl.isEmpty()) { // e.g. initial screen
            reloadUrl = reloadView->url();
        }
        openUrl(reloadView, reloadUrl, serviceType, req);
    }
}

void KonqMainWindow::slotForceReload()
{
    // A forced reload is simply a "hard" (i.e. - not soft!) reload.
    slotReload(nullptr /* Current view */, false /* Not softReload*/);
}

void KonqMainWindow::slotReloadPopup()
{
    KonqFrameBase *tab = m_pViewManager->tabContainer()->tabAt(m_workingTab);
    if (tab) {
        slotReload(tab->activeChildView());
    }
}

void KonqMainWindow::slotHome()
{
    const QString homeURL = m_paHomePopup->data().toString();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = KonqSettings::newTabsInFront();

    Qt::MouseButtons buttons = QApplication::mouseButtons();
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

    if (modifiers & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    if (modifiers & Qt::ControlModifier) { // Ctrl Left/MMB
        openFilteredUrl(homeURL, req);
    } else if (buttons & Qt::MiddleButton) {
        if (KonqSettings::mmbOpensTab()) {
            openFilteredUrl(homeURL, req);
        } else {
            const QUrl finalURL = KonqMisc::konqFilteredURL(this, homeURL);
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(finalURL);
            mw->show();
        }
    } else {
        openFilteredUrl(homeURL, false);
    }
}

void KonqMainWindow::slotHomePopupActivated(QAction *action)
{
    openUrl(nullptr, QUrl(action->data().toString()));
}

void KonqMainWindow::slotGoHistory()
{
    if (!m_historyDialog) {
        m_historyDialog = new KonqHistoryDialog(this);
        m_historyDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_historyDialog->setModal(false);
    }
    m_historyDialog->show();
}

void KonqMainWindow::slotConfigureExtensions()
{
    KonqExtensionManager extensionManager(this, this, m_currentView ? m_currentView->part() : nullptr);
    extensionManager.exec();
}

void KonqMainWindow::slotConfigure(const QString startingModule)
{
    KPageWidgetItem *startingItem = nullptr;
    if (!m_configureDialog) {
        m_configureDialog = new KCMultiDialog(this);
        m_configureDialog->setObjectName(QStringLiteral("configureDialog"));
        m_configureDialog->setFaceType(KPageDialog::Tree);
        connect(m_configureDialog, &KCMultiDialog::finished, this, &KonqMainWindow::slotConfigureDone);

        const char *const toplevelModules[] = {
            "konqueror_kcms/khtml_general",
#ifndef Q_OS_WIN
            "konqueror_kcms/kcm_performance",
#endif
            "konqueror_kcms/kcm_bookmarks",
        };
        for (uint i = 0; i < sizeof(toplevelModules) / sizeof(char *); ++i) {
            const QString kcmName = QString(toplevelModules[i]);
            m_configureDialog->addModule(KPluginMetaData(kcmName));
        }
        m_configureDialog->addModule(KPluginMetaData(QStringLiteral("konqueror_kcms/kcm_konq")));
        const char *const fmModules[] = {
            "dolphin/kcms/kcm_dolphinviewmodes",
            "dolphin/kcms/kcm_dolphinnavigation",
            "dolphin/kcms/kcm_dolphingeneral",
            "kcm_trash",
        };
        for (uint i = 0; i < sizeof(fmModules) / sizeof(char *); ++i) {
            KPageWidgetItem *it = m_configureDialog->addModule(KPluginMetaData(QString(fmModules[i])));
            if (!startingItem && startingModule == fmModules[i]) {
                startingItem = it;
            }
        }
        KPluginMetaData fileTypesData(QStringLiteral("plasma/kcms/systemsettings_qwidgets/kcm_filetypes"));
        if (!fileTypesData.isValid()) {
            QString desktopFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kservices5/filetypes.desktop"));
            fileTypesData = KPluginMetaData::fromDesktopFile(desktopFile, {QStringLiteral("kcmodule.desktop")});
        }
        m_configureDialog->addModule(fileTypesData);

        m_configureDialog->addModule(KPluginMetaData(QStringLiteral("konqueror_kcms/khtml_behavior")));

        const char *const webModules[] = {
            "konqueror_kcms/khtml_appearance",
            "konqueror_kcms/khtml_filter",
            "konqueror_kcms/khtml_cache",
            "kcm_webshortcuts",
            "kcm_proxy",
            "konqueror_kcms/kcm_history",
#if KIO_VERSION >= QT_VERSION_CHECK(5,95,0)

            "plasma/kcms/systemsettings_qwidgets/kcm_cookies",
#else
            "kcm_cookies",
#endif
            "konqueror_kcms/khtml_java_js",
        };
        for (uint i = 0; i < sizeof(webModules) / sizeof(char *); ++i) {
            KPageWidgetItem *it = m_configureDialog->addModule(KPluginMetaData(QString(webModules[i])));
            if (!startingItem && startingModule == webModules[i]) {
                startingItem = it;
            }
        }
    }

    if (startingItem) {
        m_configureDialog->setCurrentPage(startingItem);
    }
    m_configureDialog->show();

}

void KonqMainWindow::slotConfigureDone()
{
    // Cleanup the dialog so other instances can use it..
    if (m_configureDialog) {
        m_configureDialog->deleteLater();
        m_configureDialog = nullptr;
    }
}

void KonqMainWindow::slotConfigureSpellChecking()
{
    Sonnet::ConfigDialog dialog(this);
    dialog.setWindowIcon(QIcon::fromTheme("konqueror"));
    if (dialog.exec() == QDialog::Accepted) {
        updateSpellCheckConfiguration();
    }
}

void KonqMainWindow::updateSpellCheckConfiguration()
{
    //HACK: since Sonnet doesn't allow to find out whether the spell checker should be enabled by default
    //we need to open its config file and read the setting from there. We then store it in our own configuration file
    //so that it can be read from there
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig("KDE/Sonnet.conf");
    KConfigGroup grp = cfg->group("General");
    bool enabled = grp.readEntry("checkerEnabledByDefault", false);
    cfg = KSharedConfig::openConfig();
    grp = cfg->group("General");
    grp.writeEntry("SpellCheckingEnabled", enabled);
    cfg->sync();
    emit KonqSpellCheckingConfigurationDispatcher::self()->spellCheckingConfigurationChanged(enabled);
}

void KonqMainWindow::slotConfigureToolbars()
{
    slotForceSaveMainWindowSettings();
    KEditToolBar dlg(factory(), this);
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KonqMainWindow::slotNewToolbarConfig);
    connect(&dlg, &KEditToolBar::newToolBarConfig, this, &KonqMainWindow::initBookmarkBar);
    dlg.exec();
    checkDisableClearButton();
}

void KonqMainWindow::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
    if (m_toggleViewGUIClient) {
        plugActionList(QStringLiteral("toggleview"), m_toggleViewGUIClient->actions());
    }
    if (m_currentView && m_currentView->appServiceOffers().count() > 0) {
        plugActionList(QStringLiteral("openwith"), m_openWithActions);
    }

    plugViewModeActions();

    KConfigGroup cg = KSharedConfig::openConfig()->group("KonqMainWindow");
    applyMainWindowSettings(cg);
}

void KonqMainWindow::slotUndoAvailable(bool avail)
{
    m_paUndo->setEnabled(avail);
}

void KonqMainWindow::slotPartChanged(KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart)
{
    m_mapViews.remove(oldPart);
    m_mapViews.insert(newPart, childView);

    // Remove the old part, and add the new part to the manager
    const bool wasActive = m_pViewManager->activePart() == oldPart;

    m_pViewManager->replacePart(oldPart, newPart, false);

    // Set active immediately - but only if the old part was the active one (#67956)
    if (wasActive) {
        // Note: this makes the new part active... so it calls slotPartActivated
        m_pViewManager->setActivePart(newPart);
    }

    viewsChanged();
}

void KonqMainWindow::applyKonqMainWindowSettings()
{
    const QStringList toggableViewsShown = KonqSettings::toggableViewsShown();
    QStringList::ConstIterator togIt = toggableViewsShown.begin();
    QStringList::ConstIterator togEnd = toggableViewsShown.end();
    for (; togIt != togEnd; ++togIt) {
        // Find the action by name
        //    QAction * act = m_toggleViewGUIClient->actionCollection()->action( (*togIt).toLatin1() );
        QAction *act = m_toggleViewGUIClient->action(*togIt);
        if (act) {
            act->trigger();
        } else {
            qCWarning(KONQUEROR_LOG) << "Unknown toggable view in ToggableViewsShown " << *togIt;
        }
    }
}

void KonqMainWindow::slotSetStatusBarText(const QString &)
{
    // Reimplemented to disable KParts::MainWindow default behaviour
    // Does nothing here, see KonqFrame
}

void KonqMainWindow::slotViewCompleted(KonqView *view)
{
    Q_ASSERT(view);

    // Need to update the current working directory
    // of the completion object every time the user
    // changes the directory!! (DA)
    if (m_pURLCompletion) {
        m_pURLCompletion->setDir(QUrl::fromUserInput(view->locationBarURL()));
    }
}

void KonqMainWindow::slotPartActivated(KParts::Part *part)
{
    qCDebug(KONQUEROR_LOG) << part << (part ? part->metaData().pluginId() : QString());
    KonqView *newView = nullptr;
    KonqView *oldView = m_currentView;

    //Exit full screen when a new part is activated
    if (m_fullScreenData.currentState == FullScreenState::CompleteFullScreen) {
        if (oldView && oldView->part()) {
            QMetaObject::invokeMethod(oldView->part(), "exitFullScreen");
        } else { //This should never happen
            toggleCompleteFullScreen(false);
        }
    }

    if (part) {
        newView = m_mapViews.value(static_cast<KParts::ReadOnlyPart *>(part));
        Q_ASSERT(newView);
        if (newView->isPassiveMode()) {
            // Passive view. Don't connect anything, don't change m_currentView
            // Another view will become the current view very soon
            //qCDebug(KONQUEROR_LOG) << "Passive mode - return";
            return;
        }
    }

    KParts::BrowserExtension *ext = nullptr;

    if (oldView) {
        ext = oldView->browserExtension();
        if (ext) {
            //qCDebug(KONQUEROR_LOG) << "Disconnecting extension for view" << oldView;
            disconnectExtension(ext);
        }
    }

    qCDebug(KONQUEROR_LOG) << "New current view" << newView;
    m_currentView = newView;
    if (newView) {
        m_paShowStatusBar->setChecked(newView->frame()->statusbar()->isVisible());
    }

    if (!part) {
        //qCDebug(KONQUEROR_LOG) << "No part activated - returning";
        unplugViewModeActions();
        createGUI(nullptr);
        KParts::MainWindow::setCaption(QString());
        return;
    }

    ext = m_currentView->browserExtension();

    if (ext) {
        connectExtension(ext);
    } else {
        qCDebug(KONQUEROR_LOG) << "No Browser Extension for the new part";
        // Disable all browser-extension actions

        KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
        KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
        const KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();
        for (; it != itEnd; ++it) {
            QAction *act = actionCollection()->action(QString::fromLatin1(it.key()));
            Q_ASSERT(act);
            if (act) {
                act->setEnabled(false);
            }
        }

        if (m_paCopyFiles) {
            m_paCopyFiles->setEnabled(false);
        }
        if (m_paMoveFiles) {
            m_paMoveFiles->setEnabled(false);
        }
    }

    m_paShowDeveloperTools->setEnabled(m_currentView && m_currentView->isWebEngineView());

    createGUI(part);

    // View-dependent GUI

    KParts::MainWindow::setCaption(KStringHandler::csqueeze(m_currentView->caption(), 128));
    // This line causes #170470 when removing the current tab, because QTabBar
    // emits currentChanged before calling tabRemoved, so KTabWidget gets confused.
    // I don't see a need for it anyway...
    //m_currentView->frame()->setTitle(m_currentView->caption(), 0);

    updateOpenWithActions();
    updateViewActions(); // undo, lock, link and other view-dependent actions
    updateViewModeActions();

    bool viewShowsDir = m_currentView->showsDirectory();
    bool buttonShowsFolder = m_paHomePopup->text() == i18n("Home Folder");
    if (m_paHomePopup->text() == i18n("Home") || viewShowsDir != buttonShowsFolder) {
        QAction *actHomeFolder = new QAction(this);
        QAction *actHomePage = new QAction(this);

        actHomeFolder->setIcon(QIcon::fromTheme(QStringLiteral("user-home")));
        actHomeFolder->setText(i18n("Home Folder"));
        actHomeFolder->setStatusTip(i18n("Navigate to your 'Home Folder'"));
        actHomeFolder->setWhatsThis(i18n("Navigate to your local 'Home Folder'"));
        actHomeFolder->setData(QUrl::fromLocalFile(QDir::homePath()));
        actHomePage->setIcon(QIcon::fromTheme(QStringLiteral("go-home")));
        actHomePage->setText(i18n("Home Page"));

        actHomePage->setStatusTip(i18n("Navigate to your 'Home Page'"));
        actHomePage->setWhatsThis(i18n("<html>Navigate to your 'Home Page'<br /><br />"
                                       "You can configure the location where this button takes you "
                                       "under <b>Settings -> Configure Konqueror -> General</b>.</html>"));
        actHomePage->setData(KonqSettings::homeURL());

        m_paHome->setIcon(viewShowsDir ? actHomeFolder->icon() : actHomePage->icon());
        m_paHome->setText(viewShowsDir ? actHomeFolder->text() : actHomePage->text());
        m_paHome->setStatusTip(viewShowsDir ? actHomeFolder->statusTip() : actHomePage->statusTip());
        m_paHome->setWhatsThis(viewShowsDir ? actHomeFolder->whatsThis() : actHomePage->whatsThis());
        m_paHomePopup->setIcon(viewShowsDir ? actHomeFolder->icon() : actHomePage->icon());
        m_paHomePopup->setText(viewShowsDir ? actHomeFolder->text() : actHomePage->text());
        m_paHomePopup->setStatusTip(viewShowsDir ? actHomeFolder->statusTip() : actHomePage->statusTip());
        m_paHomePopup->setWhatsThis(viewShowsDir ? actHomeFolder->whatsThis() : actHomePage->whatsThis());
        m_paHomePopup->setData(viewShowsDir ? actHomeFolder->data() : actHomePage->data());
        m_paHomePopup->menu()->clear();
        if (viewShowsDir) {
            m_paHomePopup->menu()->addAction(actHomePage);
            delete actHomeFolder;
        } else {
            m_paHomePopup->menu()->addAction(actHomeFolder);
            delete actHomePage;
        }
    }

    m_currentView->frame()->statusbar()->updateActiveStatus();

    if (oldView && oldView->frame()) {
        oldView->frame()->statusbar()->updateActiveStatus();
    }

    //qCDebug(KONQUEROR_LOG) << "setting location bar url to"
    //         << m_currentView->locationBarURL() << "m_currentView=" << m_currentView;

    // Make sure the location bar gets updated when the view(tab) is changed.
    if (oldView != newView && m_combo) {
        m_combo->lineEdit()->setModified(false);
    }
    m_currentView->setLocationBarURL(m_currentView->locationBarURL());

    updateToolBarActions();
}

void KonqMainWindow::insertChildView(KonqView *childView)
{
    //qCDebug(KONQUEROR_LOG) << childView;
    m_mapViews.insert(childView->part(), childView);

    connect(childView, SIGNAL(viewCompleted(KonqView*)),
            this, SLOT(slotViewCompleted(KonqView*)));

    emit viewAdded(childView);
}

// Called by KonqViewManager, internal
void KonqMainWindow::removeChildView(KonqView *childView)
{
    //qCDebug(KONQUEROR_LOG) << childView;

    disconnect(childView, SIGNAL(viewCompleted(KonqView*)),
               this, SLOT(slotViewCompleted(KonqView*)));

#ifndef NDEBUG
    //dumpViewList();
#endif

    MapViews::Iterator it = m_mapViews.begin();
    const MapViews::Iterator end = m_mapViews.end();

    // find it in the map - can't use the key since childView->part() might be 0

    //qCDebug(KONQUEROR_LOG) << "Searching map";

    while (it != end && it.value() != childView) {
        ++it;
    }

    //qCDebug(KONQUEROR_LOG) << "Verifying search results";

    if (it == m_mapViews.end()) {
        qCWarning(KONQUEROR_LOG) << "KonqMainWindow::removeChildView childView " << childView << " not in map !";
        return;
    }

    //qCDebug(KONQUEROR_LOG) << "Removing view" << childView;

    m_mapViews.erase(it);

    emit viewRemoved(childView);

#ifndef NDEBUG
    //dumpViewList();
#endif

    // KonqViewManager takes care of m_currentView
}

void KonqMainWindow::linkableViewCountChanged()
{
    const QList<KonqView *> linkableViews = KonqLinkableViewsCollector::collect(this);
    const int lvc = linkableViews.count();
    m_paLinkView->setEnabled(lvc > 1);
    // Only one view -> unlink it
    if (lvc == 1) {
        linkableViews.at(0)->setLinkedView(false);
    }
    m_pViewManager->viewCountChanged();
}

void KonqMainWindow::viewCountChanged()
{
    // This is called (by the view manager) when the number of views changes.
    linkableViewCountChanged();
    viewsChanged();
}

void KonqMainWindow::viewsChanged()
{
    // This is called when the number of views changes OR when
    // the type of some view changes.

    updateViewActions(); // undo, lock, link and other view-dependent actions
}

KonqView *KonqMainWindow::childView(KParts::ReadOnlyPart *view)
{
    return m_mapViews.value(view);
}

KonqView *KonqMainWindow::childView(KParts::ReadOnlyPart *callingPart, const QString &name, KParts::ReadOnlyPart **part)
{
    //qCDebug(KONQUEROR_LOG) << "this=" << this << "looking for" << name;
    QList<KonqView *> views = m_mapViews.values();
    KonqView *callingView = m_mapViews.value(callingPart);
    if (callingView) {
        // Move the callingView in front of the list, in case of duplicate frame names (#133967)
        if (views.removeAll(callingView)) {
            views.prepend(callingView);
        }
    }

    for (KonqView *view : qAsConst(views)) {
        QString viewName = view->viewName();
        //qCDebug(KONQUEROR_LOG) << "       - viewName=" << viewName
        //          << "frame names:" << view->frameNames();

        if (!viewName.isEmpty() && viewName == name) {
            qCDebug(KONQUEROR_LOG) << "found existing view by name:" << view;
            if (part) {
                *part = view->part();
            }
            return view;
        }
    }

    return nullptr;
}

int KonqMainWindow::activeViewsNotLockedCount() const
{
    int res = 0;
    MapViews::ConstIterator end = m_mapViews.constEnd();
    for (MapViews::ConstIterator it = m_mapViews.constBegin(); it != end; ++it) {
        if (!it.value()->isPassiveMode() && !it.value()->isLockedLocation()) {
            ++res;
        }
    }

    return res;
}

int KonqMainWindow::linkableViewsCount() const
{
    return KonqLinkableViewsCollector::collect(const_cast<KonqMainWindow *>(this)).count();
}

int KonqMainWindow::mainViewsCount() const
{
    int res = 0;
    MapViews::ConstIterator it = m_mapViews.constBegin();
    const MapViews::ConstIterator end = m_mapViews.constEnd();
    for (; it != end; ++it) {
        if (!it.value()->isPassiveMode() && !it.value()->isToggleView()) {
            //qCDebug(KONQUEROR_LOG) << res << it.value() << it.value()->part()->widget();
            ++res;
        }
    }

    return res;
}

void KonqMainWindow::slotURLEntered(const QString &text, Qt::KeyboardModifiers modifiers)
{
    if (m_bURLEnterLock || text.isEmpty()) {
        return;
    }

    m_bURLEnterLock = true;

    if ((modifiers & Qt::ControlModifier) || (modifiers & Qt::AltModifier)) {
        m_combo->setURL(m_currentView ? m_currentView->url().toDisplayString() : QString());
        const bool inNewTab = !m_isPopupWithProxyWindow; // do not open a new tab in popup window.
        openFilteredUrl(text.trimmed(), inNewTab);
    } else {
        openFilteredUrl(text.trimmed());
    }

    m_bURLEnterLock = false;
}

void KonqMainWindow::splitCurrentView(Qt::Orientation orientation)
{
    if (!m_currentView) {
        return;
    }
    KonqView *oldView = m_currentView;
    KonqView *newView = m_pViewManager->splitView(m_currentView, orientation);
    if (newView == nullptr) {
        return;
    }
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;

    QString mime = oldView->serviceType();
    QUrl url = oldView->url();
    KSharedConfig::Ptr cfg = KSharedConfig::openConfig("konquerorrc");
    const bool alwaysDuplicateView = cfg->group("UserSettings").readEntry("AlwaysDuplicatePageWhenSplittingView", true);
    if (!alwaysDuplicateView && !url.isLocalFile()) {
        url = QUrl(KonqSettings::startURL());
        if (url.isLocalFile()) {
            QMimeDatabase db;
            mime = db.mimeTypeForUrl(url).name();
        } else {
            //We can't know the mimetype
            mime = "text/html";
        }
    }
    openView(mime, url, newView, req);
}

void KonqMainWindow::slotSplitViewHorizontal()
{
    splitCurrentView(Qt::Horizontal);
}

void KonqMainWindow::slotSplitViewVertical()
{
    splitCurrentView(Qt::Vertical);
}

void KonqMainWindow::slotAddTab()
{
    // we can hardcode text/html because this is what konq:blank will use anyway
    KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"),
                        QString(),
                        false,
                        KonqSettings::openAfterCurrentPage());
    if (!newView) {
        return;
    }

    openUrl(newView, KonqUrl::url(KonqUrl::Type::Blank), QString());

    //HACK!! QTabBar likes to steal focus when changing widgets.  This can result
    //in a flicker since we don't want it to get focus we want the combo to get
    //or keep focus...
    // TODO: retest, and replace with the smaller hack from KTabWidget::moveTab
    QWidget *widget = newView->frame() && newView->frame()->part() ?
                      newView->frame()->part()->widget() : nullptr;
    QWidget *origFocusProxy = widget ? widget->focusProxy() : nullptr;
    if (widget) {
        widget->setFocusProxy(m_combo);
    }

    m_pViewManager->showTab(newView);

    if (widget) {
        widget->setFocusProxy(origFocusProxy);
    }

    m_workingTab = 0;
}

void KonqMainWindow::slotDuplicateTab()
{
    m_pViewManager->duplicateTab(m_pViewManager->tabContainer()->currentIndex(), KonqSettings::openAfterCurrentPage());
}

void KonqMainWindow::slotDuplicateTabPopup()
{
    m_pViewManager->duplicateTab(m_workingTab, KonqSettings::openAfterCurrentPage());
}

void KonqMainWindow::slotBreakOffTab()
{
    breakOffTab(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotBreakOffTabPopup()
{
    // Delay the call since it might delete the tabbar
    QMetaObject::invokeMethod(this, "breakOffTab", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::breakOffTab(int tabIndex)
{
    KonqFrameBase *tab = m_pViewManager->tabContainer()->tabAt(tabIndex);
    if (!tab) {
        return;
    }
    const int originalTabIndex = m_pViewManager->tabContainer()->currentIndex();
    // TODO: Why do we warn about breaking off a modified tab, since it seems to show the unsubmitted form data just fine?
    if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
        m_pViewManager->showTab(tabIndex);
        if (KMessageBox::warningContinueCancel(
                    this,
                    i18n("This tab contains changes that have not been submitted.\nDetaching the tab will discard these changes."),
                    i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("tab-detach")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesdetach")) != KMessageBox::Continue) {
            m_pViewManager->showTab(originalTabIndex);
            return;
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->breakOffTab(tabIndex, size());
    updateViewActions();
}

void KonqMainWindow::slotPopupNewWindow()
{
    KFileItemList::const_iterator it = m_popupItems.constBegin();
    const KFileItemList::const_iterator end = m_popupItems.constEnd();
    KonqOpenURLRequest req;
    req.args = m_popupUrlArgs;
    req.browserArgs = m_popupUrlBrowserArgs;
    for (; it != end; ++it) {
        KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow((*it).targetUrl(), req);
        mw->show();
    }
}

void KonqMainWindow::slotPopupThisWindow()
{
    openUrl(nullptr, m_popupItems.first().url());
}

void KonqMainWindow::slotPopupNewTab()
{
    if (m_isPopupWithProxyWindow && !m_popupProxyWindow) {
        slotPopupNewWindow();
        return;
    }
    bool openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    bool newTabsInFront = KonqSettings::newTabsInFront();

    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newTabsInFront = !newTabsInFront;
    }

    popupNewTab(newTabsInFront, openAfterCurrentPage);
}

void KonqMainWindow::popupNewTab(bool infront, bool openAfterCurrentPage)
{
    KonqOpenURLRequest req;
    req.newTabInFront = false;
    req.forceAutoEmbed = true;
    req.openAfterCurrentPage = openAfterCurrentPage;
    req.args = m_popupUrlArgs;
    req.browserArgs = m_popupUrlBrowserArgs;
    req.browserArgs.setNewTab(true);

    KonqMainWindow *mainWindow = (m_popupProxyWindow ? m_popupProxyWindow.data() : this);

    for (int i = 0; i < m_popupItems.count(); ++i) {
        if (infront && i == m_popupItems.count() - 1) {
            req.newTabInFront = true;
        }
        mainWindow->openUrl(nullptr, m_popupItems[i].targetUrl(), QString(), req);
    }

    // Raise this window if the request to create the tab came from a popup window.
    if (m_isPopupWithProxyWindow) {
        raiseWindow(mainWindow);
    }
}

void KonqMainWindow::openMultiURL(const QList<QUrl> &url)
{
    QList<QUrl>::ConstIterator it = url.constBegin();
    const QList<QUrl>::ConstIterator end = url.constEnd();
    for (; it != end; ++it) {
        KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"));
        Q_ASSERT(newView);
        if (newView == nullptr) {
            continue;
        }
        openUrl(newView, *it, QString());
        m_pViewManager->showTab(newView);
    }
}

void KonqMainWindow::slotRemoveView()
{
    if (!m_currentView) {
        return;
    }

    if (m_currentView->isModified()) {
        if (KMessageBox::warningContinueCancel(this,
                                               i18n("This view contains changes that have not been submitted.\nClosing the view will discard these changes."),
                                               i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("view-close")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesclose")) != KMessageBox::Continue) {
            return;
        }
    }

    // takes care of choosing the new active view
    m_pViewManager->removeView(m_currentView);
}

void KonqMainWindow::slotRemoveTab()
{
    removeTab(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotRemoveTabPopup()
{
    // Can't do immediately - may kill the tabbar, and we're in an event path down from it
    QMetaObject::invokeMethod(this, "removeTab", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::removeTab(int tabIndex)
{
    KonqFrameBase *tab = m_pViewManager->tabContainer()->tabAt(tabIndex);
    if (!tab) {
        return;
    }
    const int originalTabIndex = m_pViewManager->tabContainer()->currentIndex();
    if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
        m_pViewManager->showTab(tabIndex);
        if (KMessageBox::warningContinueCancel(
                    this,
                    i18n("This tab contains changes that have not been submitted.\nClosing the tab will discard these changes."),
                    i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("tab-close")), KStandardGuiItem::cancel(), QStringLiteral("discardchangesclose")) != KMessageBox::Continue) {
            m_pViewManager->showTab(originalTabIndex);
            return;
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->removeTab(tab);
    updateViewActions();
}

void KonqMainWindow::slotRemoveOtherTabs()
{
    removeOtherTabs(m_pViewManager->tabContainer()->currentIndex());
}

void KonqMainWindow::slotRemoveOtherTabsPopup()
{
    // Can't do immediately - kills the tabbar, and we're in an event path down from it
    QMetaObject::invokeMethod(this, "removeOtherTabs", Qt::QueuedConnection, Q_ARG(int, m_workingTab));
}

void KonqMainWindow::removeOtherTabs(int tabToKeep)
{
    if (KMessageBox::warningContinueCancel(
                this,
                i18n("Do you really want to close all other tabs?"),
                i18nc("@title:window", "Close Other Tabs Confirmation"), KGuiItem(i18n("Close &Other Tabs"), QStringLiteral("tab-close-other")),
                KStandardGuiItem::cancel(), QStringLiteral("CloseOtherTabConfirm")) != KMessageBox::Continue) {
        return;
    }

    KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
    const int originalTabIndex = tabContainer->currentIndex();
    for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
        if (tabIndex == tabToKeep) {
            continue;
        }
        KonqFrameBase *tab = tabContainer->tabAt(tabIndex);
        if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
            m_pViewManager->showTab(tabIndex);
            if (KMessageBox::warningContinueCancel(
                        this,
                        i18n("This tab contains changes that have not been submitted.\nClosing other tabs will discard these changes."),
                        i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("tab-close")),
                        KStandardGuiItem::cancel(), QStringLiteral("discardchangescloseother")) != KMessageBox::Continue) {
                m_pViewManager->showTab(originalTabIndex);
                return;
            }
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->removeOtherTabs(tabToKeep);
    updateViewActions();
}

void KonqMainWindow::slotReloadAllTabs()
{
    KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
    const int originalTabIndex = tabContainer->currentIndex();
    for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
        KonqFrameBase *tab = tabContainer->tabAt(tabIndex);
        if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
            m_pViewManager->showTab(tabIndex);
            if (KMessageBox::warningContinueCancel(this,
                                                   i18n("This tab contains changes that have not been submitted.\nReloading all tabs will discard these changes."),
                                                   i18nc("@title:window", "Discard Changes?"),
                                                   KGuiItem(i18n("&Discard Changes"), QStringLiteral("view-refresh")),
                                                   KStandardGuiItem::cancel(), QStringLiteral("discardchangesreload")) != KMessageBox::Continue) {
                m_pViewManager->showTab(originalTabIndex);
                return;
            }
        }
    }
    m_pViewManager->showTab(originalTabIndex);
    m_pViewManager->reloadAllTabs();
    updateViewActions();
}

int KonqMainWindow::tabsCount() const
{
    return m_pViewManager->tabsCount();
}

int KonqMainWindow::currentTabIndex() const
{
    return m_pViewManager->currentTabIndex();
}

void KonqMainWindow::activateTab(int index)
{
    m_pViewManager->activateTab(index);
}

void KonqMainWindow::slotActivateNextTab()
{
    m_pViewManager->activateNextTab();
}

void KonqMainWindow::slotActivatePrevTab()
{
    m_pViewManager->activatePrevTab();
}

void KonqMainWindow::slotActivateTab()
{
    m_pViewManager->activateTab(sender()->objectName().rightRef(2).toInt() - 1);
}

void KonqMainWindow::slotDumpDebugInfo()
{
#ifndef NDEBUG
    dumpViewList();
    m_pViewManager->printFullHierarchy();
#endif
}

bool KonqMainWindow::askForTarget(const KLocalizedString &text, QUrl &url)
{
    const QUrl initialUrl = (viewCount() == 2) ? otherView(m_currentView)->url() : m_currentView->url();
    QString label = text.subs(m_currentView->url().toDisplayString(QUrl::PreferLocalFile)).toString();
    KUrlRequesterDialog dlg(initialUrl, label, this);
    dlg.setWindowTitle(i18nc("@title:window", "Enter Target"));
    dlg.urlRequester()->setMode(KFile::File | KFile::ExistingOnly | KFile::Directory);
    if (dlg.exec()) {
        url = dlg.selectedUrl();
        if (url.isValid()) {
            return true;
        } else {
            KMessageBox::error(this, i18n("<qt><b>%1</b> is not valid</qt>", url.url()));
            return false;
        }
    }
    return false;
}

void KonqMainWindow::slotCopyFiles()
{
    QUrl dest;
    if (!askForTarget(ki18n("Copy selected files from %1 to:"), dest)) {
        return;
    }

    KIO::CopyJob *job = KIO::copy(currentURLs(), dest);
    KIO::FileUndoManager::self()->recordCopyJob(job);
    KJobWidgets::setWindow(job, this);
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
}

void KonqMainWindow::slotMoveFiles()
{
    QUrl dest;
    if (!askForTarget(ki18n("Move selected files from %1 to:"), dest)) {
        return;
    }

    KIO::CopyJob *job = KIO::move(currentURLs(), dest);
    KIO::FileUndoManager::self()->recordCopyJob(job);
    KJobWidgets::setWindow(job, this);
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
}

QList<QUrl> KonqMainWindow::currentURLs() const
{
    QList<QUrl> urls;
    if (m_currentView) {
        urls.append(m_currentView->url());
        if (!m_currentView->selectedItems().isEmpty()) { // Return list of selected items only if we have a selection
            urls = m_currentView->selectedItems().urlList();
        }
    }
    return urls;
}

// Only valid if there are one or two views
KonqView *KonqMainWindow::otherView(KonqView *view) const
{
    Q_ASSERT(viewCount() <= 2);
    MapViews::ConstIterator it = m_mapViews.constBegin();
    if ((*it) == view) {
        ++it;
    }
    if (it != m_mapViews.constEnd()) {
        return (*it);
    }
    return nullptr;
}

void KonqMainWindow::slotUpAboutToShow()
{
    if (!m_currentView) {
        return;
    }

    QMenu *popup = m_paUp->menu();
    popup->clear();

    int i = 0;

    // Use the location bar URL, because in case we display a index.html
    // we want to go up from the dir, not from the index.html
    QUrl u(QUrl::fromUserInput(m_currentView->locationBarURL()));
    u = KIO::upUrl(u);
    while (!u.path().isEmpty()) {
        QAction *action = new QAction(QIcon::fromTheme(KonqPixmapProvider::self()->iconNameFor(u)),
                                      u.toDisplayString(QUrl::PreferLocalFile),
                                      popup);
        action->setData(u);
        popup->addAction(action);

        if (u.path() == QLatin1String("/") || ++i > 10) {
            break;
        }

        u = KIO::upUrl(u);
    }
}

void KonqMainWindow::slotUp()
{
    if (!m_currentView) {
        return;
    }

    Qt::MouseButtons goMouseState = QApplication::mouseButtons();
    Qt::KeyboardModifiers goKeyboardState = QApplication::keyboardModifiers();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.forceAutoEmbed = true;

    req.openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    req.newTabInFront = KonqSettings::newTabsInFront();

    if (goKeyboardState & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    const QUrl &url = m_currentView->upUrl();
    if (goKeyboardState & Qt::ControlModifier) {
        openFilteredUrl(url.url(), req);
    } else if (goMouseState & Qt::MiddleButton) {
        if (KonqSettings::mmbOpensTab()) {
            openFilteredUrl(url.url(), req);
        } else {
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(url);
            mw->show();
        }
    } else {
        openFilteredUrl(url.url(), false);
    }
}

void KonqMainWindow::slotUpActivated(QAction *action)
{
    openUrl(nullptr, action->data().value<QUrl>());
}

void KonqMainWindow::slotGoHistoryActivated(int steps)
{
    if (!m_goBuffer) {
        // Only start 1 timer.
        m_goBuffer = steps;
        m_goMouseState = QApplication::mouseButtons();
        m_goKeyboardState = QApplication::keyboardModifiers();
        QTimer::singleShot(0, this, SLOT(slotGoHistoryDelayed()));
    }
}

void KonqMainWindow::slotGoHistoryDelayed()
{
    if (!m_currentView) {
        return;
    }

    bool openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
    bool mmbOpensTab = KonqSettings::mmbOpensTab();
    bool inFront = KonqSettings::newTabsInFront();
    if (m_goKeyboardState & Qt::ShiftModifier) {
        inFront = !inFront;
    }

    if (m_goKeyboardState & Qt::ControlModifier) {
        KonqView *newView = m_pViewManager->addTabFromHistory(m_currentView, m_goBuffer, openAfterCurrentPage);
        if (newView && inFront) {
            m_pViewManager->showTab(newView);
        }
    } else if (m_goMouseState & Qt::MiddleButton) {
        if (mmbOpensTab) {
            KonqView *newView = m_pViewManager->addTabFromHistory(m_currentView, m_goBuffer, openAfterCurrentPage);
            if (newView && inFront) {
                m_pViewManager->showTab(newView);
            }
        } else {
            KonqMisc::newWindowFromHistory(this->currentView(), m_goBuffer);
        }
    } else {
        m_currentView->go(m_goBuffer);
        makeViewsFollow(m_currentView->url(),
                        KParts::OpenUrlArguments(),
                        KParts::BrowserArguments(),
                        m_currentView->serviceType(),
                        m_currentView);
    }

    m_goBuffer = 0;
    m_goMouseState = Qt::LeftButton;
    m_goKeyboardState = Qt::NoModifier;
}

void KonqMainWindow::slotBackAboutToShow()
{
    m_paBack->menu()->clear();
    if (m_currentView) {
        KonqActions::fillHistoryPopup(m_currentView->history(), m_currentView->historyIndex(), m_paBack->menu(), true, false);
    }
}

/**
 * Fill the closed tabs action menu before it's shown
 */
void KonqMainWindow::slotClosedItemsListAboutToShow()
{
    QMenu *popup = m_paClosedItems->menu();
    // Clear the menu and fill it with a maximum of s_closedItemsListLength number of urls
    popup->clear();
    QAction *clearAction = popup->addAction(i18nc("This menu entry empties the closed items history", "Empty Closed Items History"));
    connect(clearAction, &QAction::triggered, m_pUndoManager, &KonqUndoManager::clearClosedItemsList);
    popup->insertSeparator(static_cast<QAction *>(nullptr));

    QList<KonqClosedItem *>::ConstIterator it = m_pUndoManager->closedItemsList().constBegin();
    const QList<KonqClosedItem *>::ConstIterator end = m_pUndoManager->closedItemsList().constEnd();
    for (int i = 0; it != end && i < s_closedItemsListLength; ++it, ++i) {
        const QString text = QString::number(i) + ' ' + (*it)->title();
        QAction *action = popup->addAction((*it)->icon(), text);
        action->setActionGroup(m_closedItemsGroup);
        action->setData(i);
    }
    KAcceleratorManager::manage(popup);
}

/**
 * Fill the sessions list action menu before it's shown
 */
void KonqMainWindow::slotSessionsListAboutToShow()
{
    QMenu *popup = m_paSessions->menu();
    // Clear the menu and fill it with a maximum of s_closedItemsListLength number of urls
    popup->clear();
    QAction *saveSessionAction = popup->addAction(QIcon::fromTheme(QStringLiteral("document-save")), i18n("Save As..."));
    connect(saveSessionAction, &QAction::triggered, this, &KonqMainWindow::saveCurrentSession);
    QAction *manageSessionsAction = popup->addAction(QIcon::fromTheme(QStringLiteral("view-choose")), i18n("Manage..."));
    connect(manageSessionsAction, &QAction::triggered, this, &KonqMainWindow::manageSessions);
    popup->addSeparator();

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + "sessions/";
    QDirIterator it(dir, QDir::Readable | QDir::NoDotAndDotDot | QDir::Dirs);

    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());

        QAction *action = popup->addAction(KIO::decodeFileName(fileInfo.baseName()));
        action->setActionGroup(m_sessionsGroup);
        action->setData(fileInfo.filePath());
    }
    KAcceleratorManager::manage(popup);
}

void KonqMainWindow::saveCurrentSession()
{
    KonqNewSessionDlg dlg(this, this);
    dlg.exec();
}

void KonqMainWindow::manageSessions()
{
    KonqSessionDlg dlg(m_pViewManager, this);
    dlg.exec();
}

void KonqMainWindow::slotSessionActivated(QAction *action)
{
    QString dirpath = action->data().toString();
    KonqSessionManager::self()->restoreSessions(dirpath);
}

void KonqMainWindow::updateClosedItemsAction()
{
    bool available = m_pUndoManager->undoAvailable();
    m_paClosedItems->setEnabled(available);
    m_paUndo->setText(m_pUndoManager->undoText());
}

void KonqMainWindow::slotBack()
{
    slotGoHistoryActivated(-1);
}

void KonqMainWindow::slotBackActivated(QAction *action)
{
    slotGoHistoryActivated(action->data().toInt());
}

void KonqMainWindow::slotForwardAboutToShow()
{
    m_paForward->menu()->clear();
    if (m_currentView) {
        KonqActions::fillHistoryPopup(m_currentView->history(), m_currentView->historyIndex(), m_paForward->menu(), false, true);
    }
}

void KonqMainWindow::slotForward()
{
    slotGoHistoryActivated(1);
}

void KonqMainWindow::slotForwardActivated(QAction *action)
{
    slotGoHistoryActivated(action->data().toInt());
}

void KonqMainWindow::checkDisableClearButton()
{
    // if the location toolbar already has the clear_location action,
    // disable the combobox's embedded clear icon.
    KToolBar *ltb = toolBar(QStringLiteral("locationToolBar"));
    QAction *clearAction = action("clear_location");
    bool enable = true;
    const auto toolbuttons = ltb->findChildren<QToolButton *>();
    for (QToolButton *atb : toolbuttons) {
        if (atb->defaultAction() == clearAction) {
            enable = false;
            break;
        }
    }
    QLineEdit *lineEdit = comboEdit();
    if (lineEdit) {
        lineEdit->setClearButtonEnabled(enable);
    }
}

void KonqMainWindow::initCombo()
{
    m_combo = new KonqCombo(nullptr);

    m_combo->init(s_pCompletion);

    connect(m_combo, SIGNAL(activated(QString,Qt::KeyboardModifiers)),
            this, SLOT(slotURLEntered(QString,Qt::KeyboardModifiers)));
    connect(m_combo, SIGNAL(showPageSecurity()),
            this, SLOT(showPageSecurity()));

    m_pURLCompletion = new KUrlCompletion();
    m_pURLCompletion->setCompletionMode(s_pCompletion->completionMode());

    // This only turns completion off. ~ is still there in the result
    // We do want completion of user names, right?
    //m_pURLCompletion->setReplaceHome( false );  // Leave ~ alone! Will be taken care of by filters!!

    connect(m_combo, SIGNAL(completionModeChanged(KCompletion::CompletionMode)),
            SLOT(slotCompletionModeChanged(KCompletion::CompletionMode)));
    connect(m_combo, SIGNAL(completion(QString)),
            SLOT(slotMakeCompletion(QString)));
    connect(m_combo, SIGNAL(substringCompletion(QString)),
            SLOT(slotSubstringcompletion(QString)));
    connect(m_combo, SIGNAL(textRotation(KCompletionBase::KeyBindingType)),
            SLOT(slotRotation(KCompletionBase::KeyBindingType)));
    connect(m_combo, SIGNAL(cleared()),
            SLOT(slotClearHistory()));
    connect(m_pURLCompletion, SIGNAL(match(QString)),
            SLOT(slotMatch(QString)));

    m_combo->installEventFilter(this);

    static bool bookmarkCompletionInitialized = false;
    if (!bookmarkCompletionInitialized) {
        bookmarkCompletionInitialized = true;
        DelayedInitializer *initializer = new DelayedInitializer(QEvent::KeyPress, m_combo);
        connect(initializer, &DelayedInitializer::initialize, this, &KonqMainWindow::bookmarksIntoCompletion);
    }
}

void KonqMainWindow::bookmarksIntoCompletion()
{
    // add all bookmarks to the completion list for easy access
    addBookmarksIntoCompletion(s_bookmarkManager->root());
}

// the user changed the completion mode in the combo
void KonqMainWindow::slotCompletionModeChanged(KCompletion::CompletionMode m)
{
    s_pCompletion->setCompletionMode(m);

    KonqSettings::setSettingsCompletionMode(int(m_combo->completionMode()));
    KonqSettings::self()->save();

    // tell the other windows too (only this instance currently)
    foreach (KonqMainWindow *window, *s_lstMainWindows) {
        if (window && window->m_combo) {
            window->m_combo->setCompletionMode(m);
            window->m_pURLCompletion->setCompletionMode(m);
        }
    }
}

// at first, try to find a completion in the current view, then use the global
// completion (history)
void KonqMainWindow::slotMakeCompletion(const QString &text)
{
    if (m_pURLCompletion) {
        m_urlCompletionStarted = true; // flag for slotMatch()

        // qCDebug(KONQUEROR_LOG) << "Local Completion object found!";
        QString completion = m_pURLCompletion->makeCompletion(text);
        m_currentDir.clear();

        if (completion.isNull() && !m_pURLCompletion->isRunning()) {
            // No match() signal will come from m_pURLCompletion
            // ask the global one
            // tell the static completion object about the current completion mode
            completion = s_pCompletion->makeCompletion(text);

            // some special handling necessary for CompletionPopup
            if (m_combo->completionMode() == KCompletion::CompletionPopup ||
                    m_combo->completionMode() == KCompletion::CompletionPopupAuto) {
                m_combo->setCompletedItems(historyPopupCompletionItems(text));
            }

            else if (!completion.isNull()) {
                m_combo->setCompletedText(completion);
            }
        } else {
            // To be continued in slotMatch()...
            if (!m_pURLCompletion->dir().isEmpty()) {
                m_currentDir = m_pURLCompletion->dir();
            }
        }
    }
    // qCDebug(KONQUEROR_LOG) << "Current dir:" << m_currentDir << "Current text:" << text;
}

void KonqMainWindow::slotSubstringcompletion(const QString &text)
{
    if (!m_currentView) {
        return;
    }

    QString currentURL = m_currentView->url().toDisplayString();
    bool filesFirst = currentURL.startsWith('/') ||
                      currentURL.startsWith(QLatin1String("file:/"));
    QStringList items;
    if (filesFirst && m_pURLCompletion) {
        items = m_pURLCompletion->substringCompletion(text);
    }

    items += s_pCompletion->substringCompletion(text);
    if (!filesFirst && m_pURLCompletion) {
        items += m_pURLCompletion->substringCompletion(text);
    }

    m_combo->setCompletedItems(items);
}

void KonqMainWindow::slotRotation(KCompletionBase::KeyBindingType type)
{
    // Tell slotMatch() to do nothing
    m_urlCompletionStarted = false;

    bool prev = (type == KCompletionBase::PrevCompletionMatch);
    if (prev || type == KCompletionBase::NextCompletionMatch) {
        QString completion = prev ? m_pURLCompletion->previousMatch() :
                             m_pURLCompletion->nextMatch();

        if (completion.isNull()) {  // try the history KCompletion object
            completion = prev ? s_pCompletion->previousMatch() :
                         s_pCompletion->nextMatch();
        }
        if (completion.isEmpty() || completion == m_combo->currentText()) {
            return;
        }

        m_combo->setCompletedText(completion);
    }
}

// Handle match() from m_pURLCompletion
void KonqMainWindow::slotMatch(const QString &match)
{
    if (match.isEmpty() || !m_combo) {
        return;
    }

    // Check flag to avoid match() raised by rotation
    if (m_urlCompletionStarted) {
        m_urlCompletionStarted = false;

        // some special handling necessary for CompletionPopup
        if (m_combo->completionMode() == KCompletion::CompletionPopup ||
                m_combo->completionMode() == KCompletion::CompletionPopupAuto) {
            QStringList items = m_pURLCompletion->allMatches();
            items += historyPopupCompletionItems(m_combo->currentText());
            items.removeDuplicates();  // when items from completion are also in history
            // items.sort(); // should we?
            m_combo->setCompletedItems(items);
        } else if (!match.isNull()) {
            m_combo->setCompletedText(match);
        }
    }
}

void KonqMainWindow::slotCtrlTabPressed()
{
    KonqView *view = m_pViewManager->chooseNextView(m_currentView);
    //qCDebug(KONQUEROR_LOG) << m_currentView->url() << "->" << view->url();
    if (view) {
        m_pViewManager->setActivePart(view->part());
        KonqFrameTabs *tabs = m_pViewManager->tabContainer();
        m_pViewManager->showTab(tabs->tabIndexContaining(view->frame()));
    }
}

void KonqMainWindow::slotClearHistory()
{
    KonqHistoryManager::kself()->emitClear();
}

void KonqMainWindow::slotClearComboHistory()
{
    if (m_combo && m_combo->count()) {
        m_combo->clearHistory();
    }
}

bool KonqMainWindow::eventFilter(QObject *obj, QEvent *ev)
{
    if ((ev->type() == QEvent::FocusIn || ev->type() == QEvent::FocusOut) &&
            m_combo && m_combo->lineEdit() && m_combo == obj) {
        //qCDebug(KONQUEROR_LOG) << obj << obj->metaObject()->className() << obj->name();

        QFocusEvent *focusEv = static_cast<QFocusEvent *>(ev);
        if (focusEv->reason() == Qt::PopupFocusReason) {
            return KParts::MainWindow::eventFilter(obj, ev);
        }

        KParts::BrowserExtension *ext = nullptr;
        if (m_currentView) {
            ext = m_currentView->browserExtension();
        }

        if (ev->type() == QEvent::FocusIn) {
            //qCDebug(KONQUEROR_LOG) << "ComboBox got the focus...";
            if (m_bLocationBarConnected) {
                //qCDebug(KONQUEROR_LOG) << "Was already connected...";
                return KParts::MainWindow::eventFilter(obj, ev);
            }
            m_bLocationBarConnected = true;

            // Workaround for Qt issue: usually, QLineEdit reacts on Ctrl-D,
            // but the duplicatecurrenttab action also has Ctrl-D as accel and
            // prevents the lineedit from getting this event. IMHO the accel
            // should be disabled in favor of the focus-widget.
            // TODO: decide if the delete-character behaviour of QLineEdit
            // really is useful enough to warrant this workaround
            QAction *duplicate = actionCollection()->action(QStringLiteral("duplicatecurrenttab"));
            if (duplicate->shortcuts().contains(QKeySequence(Qt::CTRL | Qt::Key_D))) {
                duplicate->setEnabled(false);
            }

            connect(m_paCut, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(cut()));
            connect(m_paCopy, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(copy()));
            connect(m_paPaste, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(paste()));
            connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
            connect(m_combo->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotCheckComboSelection()));
            connect(m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()));

            slotClipboardDataChanged();
        } else if (ev->type() == QEvent::FocusOut) {
            //qCDebug(KONQUEROR_LOG) << "ComboBox lost focus...";
            if (!m_bLocationBarConnected) {
                //qCDebug(KONQUEROR_LOG) << "Was already disconnected...";
                return KParts::MainWindow::eventFilter(obj, ev);
            }
            m_bLocationBarConnected = false;

            // see above in FocusIn for explanation
            // action is reenabled if a view exists
            QAction *duplicate = actionCollection()->action(QStringLiteral("duplicatecurrenttab"));
            if (duplicate->shortcuts().contains(QKeySequence(Qt::CTRL | Qt::Key_D))) {
                duplicate->setEnabled(currentView() && currentView()->frame());
            }

            disconnect(m_paCut, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(cut()));
            disconnect(m_paCopy, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(copy()));
            disconnect(m_paPaste, SIGNAL(triggered()), m_combo->lineEdit(), SLOT(paste()));
            disconnect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
            disconnect(m_combo->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(slotCheckComboSelection()));
            disconnect(m_combo->lineEdit(), SIGNAL(selectionChanged()), this, SLOT(slotCheckComboSelection()));

            if (ext) {
                m_paCut->setEnabled(ext->isActionEnabled("cut"));
                m_paCopy->setEnabled(ext->isActionEnabled("copy"));
                m_paPaste->setEnabled(ext->isActionEnabled("paste"));
            } else {
                m_paCut->setEnabled(false);
                m_paCopy->setEnabled(false);
                m_paPaste->setEnabled(false);
            }
        }
    } else if (ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEv = static_cast<QKeyEvent *>(ev);
        if ((keyEv->key() == Qt::Key_Tab) && (keyEv->modifiers() == Qt::ControlModifier)) {
            slotCtrlTabPressed();
            return true; // don't let QTabWidget see the event
        } else if (obj == m_combo && m_currentView && keyEv->key() == Qt::Key_Escape) {
            // reset url to current view's actual url on ESC
            m_combo->setURL(m_currentView->url().QUrl::toDisplayString(QUrl::PreferLocalFile));
            m_combo->lineEdit()->setModified(false);
            return true;
        }
    }
    return KParts::MainWindow::eventFilter(obj, ev);
}

// Only called when m_bLocationBarConnected, i.e. when the combobox has focus.
// The rest of the time, the part handles the cut/copy/paste actions.
void KonqMainWindow::slotClipboardDataChanged()
{
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if (data) {
        m_paPaste->setEnabled(data->hasText());
        slotCheckComboSelection();
    }
}

void KonqMainWindow::slotCheckComboSelection()
{
    QLineEdit *edit = comboEdit();
    if (edit) {
        const bool hasSelection = edit->hasSelectedText();
        //qCDebug(KONQUEROR_LOG) << "m_combo->lineEdit()->hasMarkedText():" << hasSelection;
        m_paCopy->setEnabled(hasSelection);
        m_paCut->setEnabled(hasSelection);
    }
}

void KonqMainWindow::slotClearLocationBar()
{
    slotStop();
    if (m_combo) {
        m_combo->clearTemporary();
    }
    focusLocationBar();
}

void KonqMainWindow::slotForceSaveMainWindowSettings()
{
    if (autoSaveSettings()) {   // don't do it on e.g. JS window.open windows with no toolbars!
        KConfigGroup config = KSharedConfig::openConfig()->group("MainWindow");
        saveMainWindowSettings(config);
    }
}

void KonqMainWindow::slotShowMenuBar()
{
    menuBar()->setVisible(!menuBar()->isVisible());
    slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotShowStatusBar()
{
    if (m_currentView) {
        m_currentView->frame()->statusbar()->setVisible(m_paShowStatusBar->isChecked());
    }

    // An alternative: this will change simultaneously all of the status bars on
    // all of the current views.
    //MapViews::const_iterator end = m_mapViews.constEnd();
    //for (MapViews::const_iterator it = m_mapViews.constBegin(); it != end; ++it) {
    //  KonqView* view = it.value();
    //  view->frame()->statusbar()->setVisible(on);
    //}

    slotForceSaveMainWindowSettings();
}

void KonqMainWindow::slotUpdateFullScreen(bool set)
{
    if (m_fullScreenData.currentState == FullScreenState::CompleteFullScreen) {
        if (m_currentView && m_currentView->part()) {
            QMetaObject::invokeMethod(m_currentView->part(), "exitFullScreen");
        }
        return;
    }

    KToggleFullScreenAction::setFullScreen(this, set);
    if (set) {
        // Create toolbar button for exiting from full-screen mode
        // ...but only if there isn't one already...

        bool haveFullScreenButton = false;

        //Walk over the toolbars and check whether there is a show fullscreen button in any of them
        foreach (KToolBar *bar, findChildren<KToolBar *>()) {
            //Are we plugged here, in a visible toolbar?
            if (bar->isVisible() &&
                    action("fullscreen")->associatedWidgets().contains(bar)) {
                haveFullScreenButton = true;
                break;
            }
        }

        if (!haveFullScreenButton) {
            QList<QAction *> lst;
            lst.append(m_ptaFullScreen);
            plugActionList(QStringLiteral("fullscreen"), lst);
        }

        m_fullScreenData.wasMenuBarVisible = menuBar()->isVisible();
        menuBar()->hide();
        m_paShowMenuBar->setChecked(false);
    } else {
        unplugActionList(QStringLiteral("fullscreen"));

        if (m_fullScreenData.wasMenuBarVisible) {
            menuBar()->show();
            m_paShowMenuBar->setChecked(true);
        }
    }
    m_fullScreenData.switchToState(set ? FullScreenState::OrdinaryFullScreen : FullScreenState::NoFullScreen);
}

void KonqMainWindow::setLocationBarURL(const QUrl &url)
{
    setLocationBarURL(url.toString());
}

void KonqMainWindow::setLocationBarURL(const QString &url)
{
    // Don't set the location bar URL if it hasn't changed
    // or if the user had time to edit the url since the last call to openUrl (#64868)
    QLineEdit *edit = comboEdit();
    if (edit && url != edit->text() && !edit->isModified()) {
        //qCDebug(KONQUEROR_LOG) << "url=" << url;
        m_combo->setURL(url);
        updateWindowIcon();
    }
}

void KonqMainWindow::setPageSecurity(PageSecurity pageSecurity)
{
    if (m_combo) {
        m_combo->setPageSecurity(pageSecurity);
    }
}

void KonqMainWindow::showPageSecurity()
{
    if (m_currentView && m_currentView->part()) {
        QAction *act = m_currentView->part()->action("security");
        if (act) {
            act->trigger();
        }
    }
}

// Called via DBUS from KonquerorApplication
void KonqMainWindow::comboAction(int action, const QString &url, const QString &senderId)
{
    if (!s_lstMainWindows) { // this happens in "konqueror --silent"
        return;
    }

    KonqCombo *combo = nullptr;
    foreach (KonqMainWindow *window, *s_lstMainWindows) {
        if (window && window->m_combo) {
            combo = window->m_combo;

            switch (action) {
            case ComboAdd:
                combo->insertPermanent(url);
                break;
            case ComboClear:
                combo->clearHistory();
                break;
            case ComboRemove:
                combo->removeURL(url);
                break;
            default:
                break;
            }
        }
    }

    // only one instance should save...
    if (combo && senderId == QDBusConnection::sessionBus().baseService()) {
        combo->saveItems();
    }
}

QString KonqMainWindow::locationBarURL() const
{
    return (m_combo ? m_combo->currentText() : QString());
}

void KonqMainWindow::focusLocationBar()
{
    if (m_combo && (m_combo->isVisible() || !isVisible())) {
        m_combo->setFocus();
    }
}

void KonqMainWindow::startAnimation()
{
    m_paAnimatedLogo->start();
    m_paStop->setEnabled(true);
}

void KonqMainWindow::stopAnimation()
{
    m_paAnimatedLogo->stop();
    m_paStop->setEnabled(false);
}

void KonqMainWindow::setUpEnabled(const QUrl &url)
{
    bool bHasUpURL = ((!url.path().isEmpty() && url.path() != QLatin1String("/") && url.path()[0] == '/')
                      || !url.query().isEmpty() /*e.g. lists.kde.org*/);

    m_paUp->setEnabled(bHasUpURL);
}

void KonqMainWindow::initActions()
{
    // Note about this method : don't call setEnabled() on any of the actions.
    // They are all disabled then re-enabled with enableAllActions
    // If any one needs to be initially disabled, put that code in enableAllActions

    // For the popup menu only.
    m_pMenuNew = new KNewFileMenu(actionCollection(), QStringLiteral("new_menu"), this);

    // File menu

    QAction *action = actionCollection()->addAction(QStringLiteral("new_window"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    action->setText(i18n("New &Window"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotNewWindow);
    actionCollection()->setDefaultShortcuts(action, KStandardShortcut::shortcut(KStandardShortcut::New));
    action = actionCollection()->addAction(QStringLiteral("duplicate_window"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("window-duplicate")));
    action->setText(i18n("&Duplicate Window"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotDuplicateWindow);
    actionCollection()->setDefaultShortcut(action, Qt::CTRL+Qt::SHIFT+Qt::Key_D);
    action = actionCollection()->addAction(QStringLiteral("sendURL"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("mail-message-new")));
    action->setText(i18n("Send &Link Address..."));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotSendURL);
    action = actionCollection()->addAction(QStringLiteral("sendPage"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("mail-message-new")));
    action->setText(i18n("S&end File..."));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotSendFile);
    action = actionCollection()->addAction(QStringLiteral("open_location"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open-remote")));
    action->setText(i18n("&Open Location"));
    actionCollection()->setDefaultShortcut(action, Qt::ALT+Qt::Key_O);
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotOpenLocation);

    action = actionCollection()->addAction(QStringLiteral("open_file"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    action->setText(i18n("&Open File..."));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotOpenFile);
    actionCollection()->setDefaultShortcuts(action, KStandardShortcut::shortcut(KStandardShortcut::Open));

#if 0
    m_paFindFiles = new KToggleAction(QIcon::fromTheme("edit-find"), i18n("&Find File..."), this);
    actionCollection()->addAction("findfile", m_paFindFiles);
    connect(m_paFindFiles, &KToggleAction::triggered, this, &KonqMainWindow::slotToolFind);
    actionCollection()->setDefaultShortcuts(m_paFindFiles, KStandardShortcut::shortcut(KStandardShortcut::Find));
#endif

    m_paPrint = actionCollection()->addAction(KStandardAction::Print, QStringLiteral("print"), nullptr, nullptr);
    actionCollection()->addAction(KStandardAction::Quit, QStringLiteral("quit"), this, SLOT(close()));

    m_paLockView = new KToggleAction(i18n("Lock to Current Location"), this);
    actionCollection()->addAction(QStringLiteral("lock"), m_paLockView);
    connect(m_paLockView, &KToggleAction::triggered, this, &KonqMainWindow::slotLockView);
    m_paLinkView = new KToggleAction(i18nc("This option links konqueror views", "Lin&k View"), this);
    actionCollection()->addAction(QStringLiteral("link"), m_paLinkView);
    connect(m_paLinkView, &KToggleAction::triggered, this, &KonqMainWindow::slotLinkView);

    // Go menu
    m_paUp = new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("go-up")), i18n("&Up"), this);
    actionCollection()->addAction(QStringLiteral("go_up"), m_paUp);
    actionCollection()->setDefaultShortcuts(m_paUp, KStandardShortcut::shortcut(KStandardShortcut::Up));
    connect(m_paUp, SIGNAL(triggered()), this, SLOT(slotUp()));
    connect(m_paUp->menu(), SIGNAL(aboutToShow()), this, SLOT(slotUpAboutToShow()));
    connect(m_paUp->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotUpActivated(QAction*)));

    QPair< KGuiItem, KGuiItem > backForward = KStandardGuiItem::backAndForward();

    // Trash bin of closed tabs
    m_paClosedItems = new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("edit-undo-closed-tabs")),  i18n("Closed Items"), this);
    actionCollection()->addAction(QStringLiteral("closeditems"), m_paClosedItems);
    m_closedItemsGroup = new QActionGroup(m_paClosedItems->menu());

    // set the closed tabs list shown
    connect(m_paClosedItems, &KToolBarPopupAction::triggered, m_pUndoManager, &KonqUndoManager::undoLastClosedItem);
    connect(m_paClosedItems->menu(), SIGNAL(aboutToShow()), this, SLOT(slotClosedItemsListAboutToShow()));
    connect(m_closedItemsGroup, &QActionGroup::triggered, m_pUndoManager, &KonqUndoManager::slotClosedItemsActivated);
    connect(m_pViewManager, &KonqViewManager::aboutToRemoveTab, this, &KonqMainWindow::slotAddClosedUrl);
    connect(m_pUndoManager, &KonqUndoManager::openClosedTab, m_pViewManager, &KonqViewManager::openClosedTab);
    connect(m_pUndoManager, &KonqUndoManager::openClosedWindow, m_pViewManager, &KonqViewManager::openClosedWindow);
    connect(m_pUndoManager, &KonqUndoManager::closedItemsListChanged, this, &KonqMainWindow::updateClosedItemsAction);

    m_paSessions = new KActionMenu(i18n("Sessions"), this);
    actionCollection()->addAction(QStringLiteral("sessions"), m_paSessions);
    m_sessionsGroup = new QActionGroup(m_paSessions->menu());
    connect(m_paSessions->menu(), SIGNAL(aboutToShow()), this, SLOT(slotSessionsListAboutToShow()));
    connect(m_sessionsGroup, &QActionGroup::triggered, this, &KonqMainWindow::slotSessionActivated);

    m_paBack = new KToolBarPopupAction(QIcon::fromTheme(backForward.first.iconName()), backForward.first.text(), this);
    actionCollection()->addAction(QStringLiteral("go_back"), m_paBack);
    actionCollection()->setDefaultShortcuts(m_paBack, KStandardShortcut::shortcut(KStandardShortcut::Back));
    connect(m_paBack, SIGNAL(triggered()), this, SLOT(slotBack()));
    connect(m_paBack->menu(), SIGNAL(aboutToShow()), this, SLOT(slotBackAboutToShow()));
    connect(m_paBack->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotBackActivated(QAction*)));

    m_paForward = new KToolBarPopupAction(QIcon::fromTheme(backForward.second.iconName()), backForward.second.text(), this);
    actionCollection()->addAction(QStringLiteral("go_forward"), m_paForward);
    actionCollection()->setDefaultShortcuts(m_paForward, KStandardShortcut::shortcut(KStandardShortcut::Forward));
    connect(m_paForward, SIGNAL(triggered()), this, SLOT(slotForward()));
    connect(m_paForward->menu(), SIGNAL(aboutToShow()), this, SLOT(slotForwardAboutToShow()));
    connect(m_paForward->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotForwardActivated(QAction*)));

    m_paHome = actionCollection()->addAction(KStandardAction::Home);
    connect(m_paHome, SIGNAL(triggered(bool)), this, SLOT(slotHome()));
    m_paHomePopup = new KToolBarPopupAction (QIcon::fromTheme(QStringLiteral("go-home")), i18n("Home"), this);
    actionCollection()->addAction(QStringLiteral("go_home_popup"), m_paHomePopup);
    connect(m_paHomePopup, SIGNAL(triggered()), this, SLOT(slotHome()));
    connect(m_paHomePopup->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotHomePopupActivated(QAction*)));

    KonqMostOftenURLSAction *mostOften = new KonqMostOftenURLSAction(i18nc("@action:inmenu Go", "Most Often Visited"), this);
    actionCollection()->addAction(QStringLiteral("go_most_often"), mostOften);
    connect(mostOften, &KonqMostOftenURLSAction::activated, this, &KonqMainWindow::slotOpenURL);

    KonqHistoryAction *historyAction = new KonqHistoryAction(i18nc("@action:inmenu Go", "Recently Visited"), this);
    actionCollection()->addAction(QStringLiteral("history"), historyAction);
    connect(historyAction, &KonqHistoryAction::activated, this, &KonqMainWindow::slotOpenURL);

    action = actionCollection()->addAction(QStringLiteral("go_history"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-history")));
    // Ctrl+Shift+H, shortcut from firefox
    // TODO: and Ctrl+H should open the sidebar history module
    actionCollection()->setDefaultShortcut(action, Qt::CTRL+Qt::SHIFT+Qt::Key_H);
    action->setText(i18nc("@action:inmenu Go", "Show History"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotGoHistory);

    // Settings menu
    actionCollection()->addAction(KStandardAction::Preferences, this, SLOT(slotConfigure()));


    KStandardAction::keyBindings(guiFactory(), &KXMLGUIFactory::showConfigureShortcutsDialog, actionCollection());

    actionCollection()->addAction(KStandardAction::ConfigureToolbars, this, SLOT(slotConfigureToolbars()));

    m_paConfigureExtensions = actionCollection()->addAction(QStringLiteral("options_configure_extensions"));
    m_paConfigureExtensions->setIcon(QIcon::fromTheme(QStringLiteral("plugins")));
    m_paConfigureExtensions->setText(i18n("Configure Extensions..."));
    connect(m_paConfigureExtensions, &QAction::triggered, this, &KonqMainWindow::slotConfigureExtensions);
    m_paConfigureSpellChecking = actionCollection()->addAction(QStringLiteral("configurespellcheck"));
    m_paConfigureSpellChecking->setIcon(QIcon::fromTheme(QStringLiteral("tools-check-spelling")));
    m_paConfigureSpellChecking->setText(i18n("Configure Spell Checking..."));
    connect(m_paConfigureSpellChecking, &QAction::triggered, this, &KonqMainWindow::slotConfigureSpellChecking);

    // Window menu
    m_paSplitViewHor = actionCollection()->addAction(QStringLiteral("splitviewh"));
    m_paSplitViewHor->setIcon(QIcon::fromTheme(QStringLiteral("view-split-left-right")));
    m_paSplitViewHor->setText(i18n("Split View &Left/Right"));
    connect(m_paSplitViewHor, &QAction::triggered, this, &KonqMainWindow::slotSplitViewHorizontal);
    actionCollection()->setDefaultShortcut(m_paSplitViewHor, Qt::CTRL+Qt::SHIFT+Qt::Key_L);
    m_paSplitViewVer = actionCollection()->addAction(QStringLiteral("splitviewv"));
    m_paSplitViewVer->setIcon(QIcon::fromTheme(QStringLiteral("view-split-top-bottom")));
    m_paSplitViewVer->setText(i18n("Split View &Top/Bottom"));
    connect(m_paSplitViewVer, &QAction::triggered, this, &KonqMainWindow::slotSplitViewVertical);
    actionCollection()->setDefaultShortcut(m_paSplitViewVer, Qt::CTRL+Qt::SHIFT+Qt::Key_T);
    m_paAddTab = actionCollection()->addAction(QStringLiteral("newtab"));
    m_paAddTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    m_paAddTab->setText(i18n("&New Tab"));
    connect(m_paAddTab, &QAction::triggered, this, &KonqMainWindow::slotAddTab);
    QList<QKeySequence> addTabShortcuts;
    addTabShortcuts.append(QKeySequence(Qt::CTRL+Qt::Key_T));
    addTabShortcuts.append(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_N));
    actionCollection()->setDefaultShortcuts(m_paAddTab, addTabShortcuts);

    m_paDuplicateTab = actionCollection()->addAction(QStringLiteral("duplicatecurrenttab"));
    m_paDuplicateTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-duplicate")));
    m_paDuplicateTab->setText(i18n("&Duplicate Current Tab"));
    connect(m_paDuplicateTab, &QAction::triggered, this, &KonqMainWindow::slotDuplicateTab);
    actionCollection()->setDefaultShortcut(m_paDuplicateTab, Qt::CTRL+Qt::Key_D);
    m_paBreakOffTab = actionCollection()->addAction(QStringLiteral("breakoffcurrenttab"));
    m_paBreakOffTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-detach")));
    m_paBreakOffTab->setText(i18n("Detach Current Tab"));
    connect(m_paBreakOffTab, &QAction::triggered, this, &KonqMainWindow::slotBreakOffTab);
    actionCollection()->setDefaultShortcut(m_paBreakOffTab, Qt::CTRL+Qt::SHIFT+Qt::Key_B);
    m_paRemoveView = actionCollection()->addAction(QStringLiteral("removeview"));
    m_paRemoveView->setIcon(QIcon::fromTheme(QStringLiteral("view-close")));
    m_paRemoveView->setText(i18n("&Close Active View"));
    connect(m_paRemoveView, &QAction::triggered, this, &KonqMainWindow::slotRemoveView);
    actionCollection()->setDefaultShortcut(m_paRemoveView, Qt::CTRL+Qt::SHIFT+Qt::Key_W);
    m_paRemoveTab = actionCollection()->addAction(QStringLiteral("removecurrenttab"));
    m_paRemoveTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    m_paRemoveTab->setText(i18n("Close Current Tab"));
    connect(m_paRemoveTab, &QAction::triggered, this, &KonqMainWindow::slotRemoveTab, Qt::QueuedConnection /* exit Ctrl+W handler before deleting */);
    actionCollection()->setDefaultShortcut(m_paRemoveTab, Qt::CTRL+Qt::Key_W);
    m_paRemoveTab->setAutoRepeat(false);
    m_paRemoveOtherTabs = actionCollection()->addAction(QStringLiteral("removeothertabs"));
    m_paRemoveOtherTabs->setIcon(QIcon::fromTheme(QStringLiteral("tab-close-other")));
    m_paRemoveOtherTabs->setText(i18n("Close &Other Tabs"));
    connect(m_paRemoveOtherTabs, &QAction::triggered, this, &KonqMainWindow::slotRemoveOtherTabs);

    m_paActivateNextTab = actionCollection()->addAction(QStringLiteral("activatenexttab"));
    m_paActivateNextTab->setText(i18n("Activate Next Tab"));
    connect(m_paActivateNextTab, &QAction::triggered, this, &KonqMainWindow::slotActivateNextTab);
    actionCollection()->setDefaultShortcuts(m_paActivateNextTab, QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() : KStandardShortcut::tabNext());
    m_paActivatePrevTab = actionCollection()->addAction(QStringLiteral("activateprevtab"));
    m_paActivatePrevTab->setText(i18n("Activate Previous Tab"));
    connect(m_paActivatePrevTab, &QAction::triggered, this, &KonqMainWindow::slotActivatePrevTab);
    actionCollection()->setDefaultShortcuts(m_paActivatePrevTab, QApplication::isRightToLeft() ? KStandardShortcut::tabNext() : KStandardShortcut::tabPrev());

    for (int i = 1; i < 13; i++) {
        const QString actionname = QString::asprintf("activate_tab_%02d", i);
        QAction *action = actionCollection()->addAction(actionname);
        action->setText(i18n("Activate Tab %1", i));
        connect(action, &QAction::triggered, this, &KonqMainWindow::slotActivateTab);
    }

    m_paMoveTabLeft = actionCollection()->addAction(QStringLiteral("tab_move_left"));
    m_paMoveTabLeft->setText(i18n("Move Tab Left"));
    m_paMoveTabLeft->setIcon(QIcon::fromTheme(QStringLiteral("arrow-left")));
    connect(m_paMoveTabLeft, &QAction::triggered, this, &KonqMainWindow::slotMoveTabLeft);
    actionCollection()->setDefaultShortcut(m_paMoveTabLeft, Qt::CTRL+Qt::SHIFT+Qt::Key_Left);
    m_paMoveTabRight = actionCollection()->addAction(QStringLiteral("tab_move_right"));
    m_paMoveTabRight->setText(i18n("Move Tab Right"));
    m_paMoveTabRight->setIcon(QIcon::fromTheme(QStringLiteral("arrow-right")));
    connect(m_paMoveTabRight, &QAction::triggered, this, &KonqMainWindow::slotMoveTabRight);
    actionCollection()->setDefaultShortcut(m_paMoveTabRight, Qt::CTRL+Qt::SHIFT+Qt::Key_Right);

#ifndef NDEBUG
    action = actionCollection()->addAction(QStringLiteral("dumpdebuginfo"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-dump-debug-info")));
    action->setText(i18n("Dump Debug Info"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotDumpDebugInfo);
#endif

    m_ptaFullScreen = KStandardAction::fullScreen(nullptr, nullptr, this, this);
    actionCollection()->addAction(m_ptaFullScreen->objectName(), m_ptaFullScreen);
    QList<QKeySequence> fullScreenShortcut = m_ptaFullScreen->shortcuts();
    fullScreenShortcut.append(Qt::Key_F11);
    actionCollection()->setDefaultShortcuts(m_ptaFullScreen, fullScreenShortcut);
    connect(m_ptaFullScreen, &KToggleFullScreenAction::toggled, this, &KonqMainWindow::slotUpdateFullScreen);

    QList<QKeySequence> reloadShortcut = KStandardShortcut::shortcut(KStandardShortcut::Reload);
    QKeySequence reloadAlternate(Qt::CTRL | Qt::Key_R);
    if (!reloadShortcut.contains(reloadAlternate)) {
        reloadShortcut.append(reloadAlternate);
    }
    m_paReload = actionCollection()->addAction(QStringLiteral("reload"));
    m_paReload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_paReload->setText(i18n("&Reload"));
    connect(m_paReload, SIGNAL(triggered()), SLOT(slotReload()));
    actionCollection()->setDefaultShortcuts(m_paReload, reloadShortcut);
    m_paReloadAllTabs = actionCollection()->addAction(QStringLiteral("reload_all_tabs"));
    m_paReloadAllTabs->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh-all")));
    m_paReloadAllTabs->setText(i18n("&Reload All Tabs"));
    connect(m_paReloadAllTabs, &QAction::triggered, this, &KonqMainWindow::slotReloadAllTabs);
    actionCollection()->setDefaultShortcut(m_paReloadAllTabs, Qt::SHIFT+Qt::Key_F5);
    // "Forced"/ "Hard" reload action - re-downloads all e.g. images even if a cached
    // version already exists.
    m_paForceReload = actionCollection()->addAction(QStringLiteral("hard_reload"));
    // TODO - request new icon? (view-refresh will do for the time being)
    m_paForceReload->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    m_paForceReload->setText(i18n("&Force Reload"));
    connect(m_paForceReload, &QAction::triggered, this, &KonqMainWindow::slotForceReload);
    QList<QKeySequence> forceReloadShortcuts;
    forceReloadShortcuts.append(QKeySequence(Qt::CTRL+Qt::Key_F5));
    forceReloadShortcuts.append(QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::Key_R));
    actionCollection()->setDefaultShortcuts(m_paForceReload, forceReloadShortcuts);

    m_paUndo = KStandardAction::undo(m_pUndoManager, SLOT(undo()), this);
    actionCollection()->addAction(QStringLiteral("undo"), m_paUndo);
    connect(m_pUndoManager, SIGNAL(undoTextChanged(QString)),
            this, SLOT(slotUndoTextChanged(QString)));

    // Those are connected to the browserextension directly
    m_paCut = KStandardAction::cut(nullptr, nullptr, this);
    actionCollection()->addAction(QStringLiteral("cut"), m_paCut);

    QList<QKeySequence> cutShortcuts(m_paCut->shortcuts());
    cutShortcuts.removeAll(QKeySequence(Qt::SHIFT+Qt::Key_Delete)); // used for deleting files
    actionCollection()->setDefaultShortcuts(m_paCut, cutShortcuts);

    m_paCopy = KStandardAction::copy(nullptr, nullptr, this);
    actionCollection()->addAction(QStringLiteral("copy"), m_paCopy);
    m_paPaste = KStandardAction::paste(nullptr, nullptr, this);
    actionCollection()->addAction(QStringLiteral("paste"), m_paPaste);
    m_paStop = actionCollection()->addAction(QStringLiteral("stop"));
    m_paStop->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_paStop->setText(i18n("&Stop"));
    connect(m_paStop, &QAction::triggered, this, &KonqMainWindow::slotStop);
    actionCollection()->setDefaultShortcut(m_paStop, Qt::Key_Escape);

    m_paAnimatedLogo = new KonqAnimatedLogo;
    QWidgetAction *logoAction = new QWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("konq_logo"), logoAction);
    logoAction->setDefaultWidget(m_paAnimatedLogo);
    // Set icon and text so that it's easier to figure out what the action is in the toolbar editor
    logoAction->setText(i18n("Throbber"));
    logoAction->setIcon(QIcon::fromTheme(QStringLiteral("kde")));

    // Location bar
    m_locationLabel = new KonqDraggableLabel(this, i18n("L&ocation: "));
    QWidgetAction *locationAction = new QWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("location_label"), locationAction);
    locationAction->setText(i18n("L&ocation: "));
    connect(locationAction, &QWidgetAction::triggered, this, &KonqMainWindow::slotLocationLabelActivated);
    locationAction->setDefaultWidget(m_locationLabel);
    m_locationLabel->setBuddy(m_combo);

    QWidgetAction *comboAction = new QWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("toolbar_url_combo"), comboAction);
    comboAction->setText(i18n("Location Bar"));
    actionCollection()->setDefaultShortcut(comboAction, Qt::Key_F6);
    connect(comboAction, &QWidgetAction::triggered, this, &KonqMainWindow::slotLocationLabelActivated);
    comboAction->setDefaultWidget(m_combo);
    actionCollection()->setShortcutsConfigurable(comboAction, false);

    m_combo->setWhatsThis(i18n("<html>Location Bar<br /><br />Enter a web address or search term.</html>"));

    QAction *clearLocation = actionCollection()->addAction(QStringLiteral("clear_location"));
    clearLocation->setIcon(QIcon::fromTheme(QApplication::isRightToLeft() ? "edit-clear-locationbar-rtl" : "edit-clear-locationbar-ltr"));
    clearLocation->setText(i18n("Clear Location Bar"));
    actionCollection()->setDefaultShortcut(clearLocation, Qt::CTRL+Qt::Key_L);
    connect(clearLocation, SIGNAL(triggered()),
            SLOT(slotClearLocationBar()));
    clearLocation->setWhatsThis(i18n("<html>Clear Location bar<br /><br />"
                                     "Clears the contents of the location bar.</html>"));

    // Bookmarks menu
    m_pamBookmarks = new KBookmarkActionMenu(s_bookmarkManager->root(),
            i18n("&Bookmarks"), this);
    actionCollection()->addAction(QStringLiteral("bookmarks"), m_pamBookmarks);
    m_pamBookmarks->setPopupMode(QToolButton::InstantPopup);

    // The actual menu needs a different action collection, so that the bookmarks
    // don't appear in kedittoolbar
    m_bookmarksActionCollection = new KActionCollection(static_cast<QWidget *>(this));

    m_pBookmarkMenu = new Konqueror::KonqBookmarkMenu(s_bookmarkManager, m_pBookmarksOwner, m_pamBookmarks, m_bookmarksActionCollection);

    QAction *addBookmark = m_bookmarksActionCollection->action(QStringLiteral("add_bookmark"));
    if (addBookmark) {
        // Keep the "Add bookmark" action visible though (#153835)
        // -> We should think of a way to mark actions as "not configurable in toolbars" and
        // "should not appear in shortcut dialog (!= isShortcutConfigurable)" instead, and use
        // a single actionCollection.
        actionCollection()->addAction(QStringLiteral("add_bookmark"), m_bookmarksActionCollection->takeAction(addBookmark));
    } else {
        qCDebug(KONQUEROR_LOG) << "Action add_bookmark not found!";
    }

    m_paShowMenuBar = KStandardAction::showMenubar(this, SLOT(slotShowMenuBar()), this);
    actionCollection()->addAction(KStandardAction::name(KStandardAction::ShowMenubar), m_paShowMenuBar);

    m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(slotShowStatusBar()), this);
    actionCollection()->addAction(KStandardAction::name(KStandardAction::ShowStatusbar), m_paShowStatusBar);

    action = actionCollection()->addAction(QStringLiteral("konqintro"));
    action->setText(i18n("Kon&queror Introduction"));
    connect(action, &QAction::triggered, this, &KonqMainWindow::slotIntro);

    QAction *goUrl = actionCollection()->addAction(QStringLiteral("go_url"));
    goUrl->setIcon(QIcon::fromTheme(QStringLiteral("go-jump-locationbar")));
    goUrl->setText(i18n("Go"));
    connect(goUrl, &QAction::triggered, this, &KonqMainWindow::goURL);
    goUrl->setWhatsThis(i18n("<html>Go<br /><br />"
                             "Goes to the page that has been entered into the location bar.</html>"));

    KStandardAction::switchApplicationLanguage(nullptr, nullptr, this);

    enableAllActions(false);

    // help stuff
    m_paUp->setWhatsThis(i18n("<html>Enter the parent folder<br /><br />"
                              "For instance, if the current location is file:/home/%1 clicking this "
                              "button will take you to file:/home.</html>",  KUser().loginName()));
    m_paUp->setStatusTip(i18n("Enter the parent folder"));

    m_paBack->setWhatsThis(i18n("Move backwards one step in the browsing history"));
    m_paBack->setStatusTip(i18n("Move backwards one step in the browsing history"));

    m_paForward->setWhatsThis(i18n("Move forward one step in the browsing history"));
    m_paForward->setStatusTip(i18n("Move forward one step in the browsing history"));

    m_paClosedItems->setWhatsThis(i18n("Move backwards one step in the closed tabs history"));
    m_paClosedItems->setStatusTip(i18n("Move backwards one step in the closed tabs history"));

    m_paReload->setWhatsThis(i18n("<html>Reload the currently displayed document<br /><br />"
                                  "This may, for example, be needed to refresh web pages that have been "
                                  "modified since they were loaded, in order to make the changes visible.</html>"));
    m_paReload->setStatusTip(i18n("Reload the currently displayed document"));

    m_paReloadAllTabs->setWhatsThis(i18n("<html>Reload all currently displayed documents in tabs<br /><br />"
                                         "This may, for example, be needed to refresh web pages that have been "
                                         "modified since they were loaded, in order to make the changes visible.</html>"));
    m_paReloadAllTabs->setStatusTip(i18n("Reload all currently displayed document in tabs"));

    m_paStop->setWhatsThis(i18n("<html>Stop loading the document<br /><br />"
                                "All network transfers will be stopped and Konqueror will display the content "
                                "that has been received so far.</html>"));

    m_paForceReload->setWhatsThis(i18n("<html>Reload the currently displayed document<br /><br />"
                                       "This may, for example, be needed to refresh web pages that have been "
                                       "modified since they were loaded, in order to make the changes visible. Any images on the page are downloaded again, even if cached copies exist.</html>"));

    m_paForceReload->setStatusTip(i18n("Force a reload of the currently displayed document and any contained images"));

    m_paStop->setStatusTip(i18n("Stop loading the document"));

    m_paCut->setWhatsThis(i18n("<html>Cut the currently selected text or item(s) and move it "
                               "to the system clipboard<br /><br />"
                               "This makes it available to the <b>Paste</b> command in Konqueror "
                               "and other KDE applications.</html>"));
    m_paCut->setStatusTip(i18n("Move the selected text or item(s) to the clipboard"));

    m_paCopy->setWhatsThis(i18n("<html>Copy the currently selected text or item(s) to the "
                                "system clipboard<br /><br />"
                                "This makes it available to the <b>Paste</b> command in Konqueror "
                                "and other KDE applications.</html>"));
    m_paCopy->setStatusTip(i18n("Copy the selected text or item(s) to the clipboard"));

    m_paPaste->setWhatsThis(i18n("<html>Paste the previously cut or copied clipboard "
                                 "contents<br /><br />"
                                 "This also works for text copied or cut from other KDE applications.</html>"));
    m_paPaste->setStatusTip(i18n("Paste the clipboard contents"));

    m_paPrint->setWhatsThis(i18n("<html>Print the currently displayed document<br /><br />"
                                 "You will be presented with a dialog where you can set various "
                                 "options, such as the number of copies to print and which printer "
                                 "to use.<br /><br />"
                                 "This dialog also provides access to special KDE printing "
                                 "services such as creating a PDF file from the current document.</html>"));
    m_paPrint->setStatusTip(i18n("Print the current document"));

    m_paLockView->setStatusTip(i18n("A locked view cannot change folders. Use in combination with 'link view' to explore many files from one folder"));
    m_paLinkView->setStatusTip(i18n("Sets the view as 'linked'. A linked view follows folder changes made in other linked views."));

    m_paShowDeveloperTools = actionCollection()->addAction("inspect_page", this, &KonqMainWindow::inspectCurrentPage);
    actionCollection()->setDefaultShortcut(m_paShowDeveloperTools, QKeySequence("Ctrl+Shift+I"));
    m_paShowDeveloperTools->setText(i18n("&Inspect Current Page"));
    m_paShowDeveloperTools->setWhatsThis(i18n("<html>Shows the developer tools for the current page<br/><br/>The current view is split in two and the developer tools are displayed in the second half"));
    m_paShowDeveloperTools->setStatusTip(i18n("Shows the developer tools for the current page"));
}

void KonqExtendedBookmarkOwner::openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km)
{
    qCDebug(KONQUEROR_LOG) << bm.url() << km << mb;

    const QString url = bm.url().url();

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = KonqSettings::newTabsInFront();
    req.forceAutoEmbed = true;

    if (km & Qt::ShiftModifier) {
        req.newTabInFront = !req.newTabInFront;
    }

    if (km & Qt::ControlModifier) {  // Ctrl Left/MMB
        m_pKonqMainWindow->openFilteredUrl(url, req);
    } else if (mb & Qt::MiddleButton) {
        if (KonqSettings::mmbOpensTab()) {
            m_pKonqMainWindow->openFilteredUrl(url, req);
        } else {
            const QUrl finalURL = KonqMisc::konqFilteredURL(m_pKonqMainWindow, url);
            KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(finalURL);
            mw->show();
        }
    } else {
        m_pKonqMainWindow->openFilteredUrl(url, false);
    }
}

void KonqMainWindow::slotMoveTabLeft()
{
    if (QApplication::isRightToLeft()) {
        m_pViewManager->moveTabForward();
    } else {
        m_pViewManager->moveTabBackward();
    }

    updateViewActions();
}

void KonqMainWindow::slotMoveTabRight()
{
    if (QApplication::isRightToLeft()) {
        m_pViewManager->moveTabBackward();
    } else {
        m_pViewManager->moveTabForward();
    }

    updateViewActions();
}

void KonqMainWindow::updateHistoryActions()
{
    if (m_currentView) {
        m_paBack->setEnabled(m_currentView->canGoBack());
        m_paForward->setEnabled(m_currentView->canGoForward());
    }
}

bool KonqMainWindow::isPreloaded() const
{
    return !isVisible() && m_mapViews.count() == 1 && m_currentView && KonqUrl::isKonqBlank(m_currentView->url().toString());
}

void KonqMainWindow::updateToolBarActions(bool pendingAction /*=false*/)
{
    if (!m_currentView) {
        return;
    }

    // Enables/disables actions that depend on the current view & url (mostly toolbar)
    // Up, back, forward, the edit extension, stop button, wheel
    setUpEnabled(m_currentView->url());
    m_paBack->setEnabled(m_currentView->canGoBack());
    m_paForward->setEnabled(m_currentView->canGoForward());

    if (m_currentView->isLoading()) {
        startAnimation(); // takes care of m_paStop
    } else {
        m_paAnimatedLogo->stop();
        m_paStop->setEnabled(pendingAction);    //enable/disable based on any pending actions...
    }
}

void KonqMainWindow::updateViewActions()
{
    // Update actions that depend on the current view and its mode, or on the number of views etc.

    // Don't do things in this method that depend on m_currentView->url().
    // When going 'back' in history this will be called before opening the url.
    // Use updateToolBarActions instead.

    bool enable = false;

    if (m_currentView && m_currentView->part()) {
        // Avoid qWarning from QObject::property if it doesn't exist
        if (m_currentView->part()->metaObject()->indexOfProperty("supportsUndo") != -1) {
            QVariant prop = m_currentView->part()->property("supportsUndo");
            if (prop.isValid() && prop.toBool()) {
                enable = true;
            }
        }
    }

    m_pUndoManager->updateSupportsFileUndo(enable);

//   slotUndoAvailable( m_pUndoManager->undoAvailable() );

    m_paLockView->setEnabled(true);
    m_paLockView->setChecked(m_currentView && m_currentView->isLockedLocation());

    // Can remove view if we'll still have a main view after that
    m_paRemoveView->setEnabled(mainViewsCount() > 1 ||
                               (m_currentView && m_currentView->isToggleView()));

    if (!currentView() || !currentView()->frame()) {
        m_paAddTab->setEnabled(false);
        m_paDuplicateTab->setEnabled(false);
        m_paRemoveOtherTabs->setEnabled(false);
        m_paBreakOffTab->setEnabled(false);
        m_paActivateNextTab->setEnabled(false);
        m_paActivatePrevTab->setEnabled(false);
        m_paMoveTabLeft->setEnabled(false);
        m_paMoveTabRight->setEnabled(false);
    } else {
        m_paAddTab->setEnabled(true);
        m_paDuplicateTab->setEnabled(true);
        KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
        bool state = (tabContainer->count() > 1);
        m_paRemoveOtherTabs->setEnabled(state);
        m_paBreakOffTab->setEnabled(state);
        m_paActivateNextTab->setEnabled(state);
        m_paActivatePrevTab->setEnabled(state);

        QList<KonqFrameBase *> childFrameList = tabContainer->childFrameList();
        Q_ASSERT(!childFrameList.isEmpty());
        m_paMoveTabLeft->setEnabled(currentView() ? currentView()->frame() !=
                                    (QApplication::isRightToLeft() ? childFrameList.last() : childFrameList.first()) : false);
        m_paMoveTabRight->setEnabled(currentView() ? currentView()->frame() !=
                                     (QApplication::isRightToLeft() ? childFrameList.first() : childFrameList.last()) : false);
    }

    // Can split a view if it's not a toggle view (because a toggle view can be here only once)
    bool isNotToggle = m_currentView && !m_currentView->isToggleView();
    m_paSplitViewHor->setEnabled(isNotToggle);
    m_paSplitViewVer->setEnabled(isNotToggle);

    m_paLinkView->setChecked(m_currentView && m_currentView->isLinkedView());

#if 0
    if (m_currentView && m_currentView->part() &&
            ::qobject_cast<KonqDirPart *>(m_currentView->part())) {
        KonqDirPart *dirPart = static_cast<KonqDirPart *>(m_currentView->part());
        m_paFindFiles->setEnabled(dirPart->findPart() == 0);

        // Create the copy/move options if not already done
        // TODO: move that stuff to dolphin(part)
        if (!m_paCopyFiles) {
            // F5 is the default key binding for Reload.... a la Windows.
            // mc users want F5 for Copy and F6 for move, but I can't make that default.

            m_paCopyFiles = actionCollection()->addAction("copyfiles");
            m_paCopyFiles->setText(i18n("Copy &Files..."));
            connect(m_paCopyFiles, &QAction::triggered, this, &KonqMainWindow::slotCopyFiles);
            m_paCopyFiles->setShortcut(Qt::Key_F7);
            m_paMoveFiles = actionCollection()->addAction("movefiles");
            m_paMoveFiles->setText(i18n("M&ove Files..."));
            connect(m_paMoveFiles, &QAction::triggered, this, &KonqMainWindow::slotMoveFiles);
            m_paMoveFiles->setShortcut(Qt::Key_F8);

            QList<QAction *> lst;
            lst.append(m_paCopyFiles);
            lst.append(m_paMoveFiles);
            m_paCopyFiles->setEnabled(false);
            m_paMoveFiles->setEnabled(false);
            plugActionList("operations", lst);
        }
    } else {
        m_paFindFiles->setEnabled(false);

        if (m_paCopyFiles) {
            unplugActionList("operations");
            delete m_paCopyFiles;
            m_paCopyFiles = 0;
            delete m_paMoveFiles;
            m_paMoveFiles = 0;
        }
    }
#endif
}

QString KonqMainWindow::findIndexFile(const QString &dir)
{
    QDir d(dir);

    QString f = d.filePath(QStringLiteral("index.html"));
    if (QFile::exists(f)) {
        return f;
    }

    f = d.filePath(QStringLiteral("index.htm"));
    if (QFile::exists(f)) {
        return f;
    }

    f = d.filePath(QStringLiteral("index.HTML"));
    if (QFile::exists(f)) {
        return f;
    }

    return QString();
}

void KonqMainWindow::connectExtension(KParts::BrowserExtension *ext)
{
    KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();

    for (; it != itEnd; ++it) {
        QAction *act = actionCollection()->action(it.key().data());
        //qCDebug(KONQUEROR_LOG) << it.key();
        if (act) {
            // Does the extension have a slot with the name of this action ?
            if (ext->metaObject()->indexOfSlot(QByteArray(it.key() + "()").constData()) != -1) {
                connect(act, SIGNAL(triggered()), ext, it.value() /* SLOT(slot name) */);
                act->setEnabled(ext->isActionEnabled(it.key()));
                const QString text = ext->actionText(it.key());
                if (!text.isEmpty()) {
                    act->setText(text);
                }
                // TODO how to re-set the original action text, when switching to a part that didn't call setAction?
                // Can't test with Paste...
            } else {
                act->setEnabled(false);
            }

        } else {
            qCWarning(KONQUEROR_LOG) << "Error in BrowserExtension::actionSlotMap(), unknown action : " << it.key();
        }
    }

}

void KonqMainWindow::disconnectExtension(KParts::BrowserExtension *ext)
{
    KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator it = actionSlotMap->constBegin();
    KParts::BrowserExtension::ActionSlotMap::ConstIterator itEnd = actionSlotMap->constEnd();

    for (; it != itEnd; ++it) {
        QAction *act = actionCollection()->action(it.key().data());
        //qCDebug(KONQUEROR_LOG) << it.key();
        if (act && ext->metaObject()->indexOfSlot(QByteArray(it.key() + "()").constData()) != -1) {
            //qCDebug(KONQUEROR_LOG) << act << act->name();
            act->disconnect(ext);
        }
    }
}

void KonqMainWindow::enableAction(const char *name, bool enabled)
{
    QAction *act = actionCollection()->action(name);
    if (!act) {
        qCWarning(KONQUEROR_LOG) << "Unknown action " << name << " - can't enable";
    } else {
        if (m_bLocationBarConnected && (
                    act == m_paCopy || act == m_paCut || act == m_paPaste))
            // Don't change action state while the location bar has focus.
        {
            return;
        }
        //qCDebug(KONQUEROR_LOG) << name << enabled;
        act->setEnabled(enabled);
    }

    // Update "copy files" and "move files" accordingly
    if (m_paCopyFiles && !strcmp(name, "copy")) {
        m_paCopyFiles->setEnabled(enabled);
    } else if (m_paMoveFiles && !strcmp(name, "cut")) {
        m_paMoveFiles->setEnabled(enabled);
    }
}

void KonqMainWindow::setActionText(const char *name, const QString &text)
{
    QAction *act = actionCollection()->action(name);
    if (!act) {
        qCWarning(KONQUEROR_LOG) << "Unknown action " << name << "- can't enable";
    } else {
        //qCDebug(KONQUEROR_LOG) << name << " text=" << text;
        act->setText(text);
    }
}

void KonqMainWindow::enableAllActions(bool enable)
{
    //qCDebug(KONQUEROR_LOG) << enable;
    KParts::BrowserExtension::ActionSlotMap *actionSlotMap = KParts::BrowserExtension::actionSlotMapPtr();

    const QList<QAction *> actions = actionCollection()->actions();
    QList<QAction *>::ConstIterator it = actions.constBegin();
    QList<QAction *>::ConstIterator end = actions.constEnd();
    for (; it != end; ++it) {
        QAction *act = *it;
        if (!act->objectName().startsWith(QLatin1String("options_configure"))  /* do not touch the configureblah actions */
                && (!enable || !actionSlotMap->contains(act->objectName().toLatin1()))) {    /* don't enable BE actions */
            act->setEnabled(enable);
        }
    }
    // This method is called with enable=false on startup, and
    // then only once with enable=true when the first view is setup.
    // So the code below is where actions that should initially be disabled are disabled.
    if (enable) {
        setUpEnabled(m_currentView ? m_currentView->url() : QUrl());
        // we surely don't have any history buffers at this time
        m_paBack->setEnabled(false);
        m_paForward->setEnabled(false);

        updateViewActions(); // undo, lock, link and other view-dependent actions
        updateClosedItemsAction();

        m_paStop->setEnabled(m_currentView && m_currentView->isLoading());

        if (m_toggleViewGUIClient) {
            QList<QAction *> actions = m_toggleViewGUIClient->actions();
            for (int i = 0; i < actions.size(); ++i) {
                actions.at(i)->setEnabled(true);
            }
        }

    }
    actionCollection()->action(QStringLiteral("quit"))->setEnabled(true);
    actionCollection()->action(QStringLiteral("link"))->setEnabled(false);
}

void KonqMainWindow::disableActionsNoView()
{
    // No view -> there are some things we can't do
    m_paUp->setEnabled(false);
    m_paReload->setEnabled(false);
    m_paReloadAllTabs->setEnabled(false);
    m_paBack->setEnabled(false);
    m_paForward->setEnabled(false);
    m_paLockView->setEnabled(false);
    m_paLockView->setChecked(false);
    m_paSplitViewVer->setEnabled(false);
    m_paSplitViewHor->setEnabled(false);
    m_paRemoveView->setEnabled(false);
    m_paLinkView->setEnabled(false);
    if (m_toggleViewGUIClient) {
        QList<QAction *> actions = m_toggleViewGUIClient->actions();
        for (int i = 0; i < actions.size(); ++i) {
            actions.at(i)->setEnabled(false);
        }
    }
    // There are things we can do, though : bookmarks, view profile, location bar, new window,
    // settings, etc.
    static const char *const s_enActions[] = { "new_window", "duplicate_window", "open_location",
                                               "toolbar_url_combo", "clear_location", "animated_logo",
                                               "konqintro", "go_most_often", "go_applications",
                                               "go_trash", "go_settings", "go_network_folders", "go_autostart",
                                               "go_url", "go_media", "go_history", "options_configure_extensions", nullptr
                                             };
    for (int i = 0; s_enActions[i]; ++i) {
        QAction *act = action(s_enActions[i]);
        if (act) {
            act->setEnabled(true);
        }
    }
    m_combo->clearTemporary();
}

void KonqMainWindow::setCaption(const QString &caption)
{
    // KParts sends us empty captions when activating a brand new part
    // We can't change it there (in case of apps removing all parts altogether)
    // but here we never do that.
    if (!caption.isEmpty() && m_currentView) {
        //qCDebug(KONQUEROR_LOG) << caption;

        // Keep an unmodified copy of the caption (before squeezing and KComponentData::makeStdCaption are applied)
        m_currentView->setCaption(caption);
        KParts::MainWindow::setCaption(KStringHandler::csqueeze(m_currentView->caption(), 128));
    }
}

void KonqMainWindow::showEvent(QShowEvent *event)
{
    //qCDebug(KONQUEROR_LOG) << QTime::currentTime();
    // We need to check if our toolbars are shown/hidden here, and set
    // our menu items accordingly. We can't do it in the constructor because
    // view profiles store toolbar info, and that info is read after
    // construct time.
    m_paShowMenuBar->setChecked(!menuBar()->isHidden());
    if (m_currentView) {
        m_paShowStatusBar->setChecked(m_currentView->frame()->statusbar()->isVisible());
    }
    updateBookmarkBar(); // hide if empty

    // Call parent method
    KParts::MainWindow::showEvent(event);
}

QUrl KonqExtendedBookmarkOwner::currentUrl() const
{
    const KonqView *view = m_pKonqMainWindow->currentView();
    return view ? view->url() : QUrl();
}

QString KonqMainWindow::currentURL() const
{
    if (!m_currentView) {
        return QString();
    }
    QString url = m_currentView->url().toDisplayString();

#if 0 // do we want this?
    // Add the name filter (*.txt) at the end of the URL again
    if (m_currentView->part()) {
        const QString nameFilter = m_currentView->nameFilter();
        if (!nameFilter.isEmpty()) {
            if (!url.endsWith('/')) {
                url += '/';
            }
            url += nameFilter;
        }
    }
#endif
    return url;
}

bool KonqExtendedBookmarkOwner::supportsTabs() const
{
    return true;
}

QList<KBookmarkOwner::FutureBookmark> KonqExtendedBookmarkOwner::currentBookmarkList() const
{
    QList<KBookmarkOwner::FutureBookmark> list;
    KonqFrameTabs *tabContainer = m_pKonqMainWindow->viewManager()->tabContainer();

    foreach (KonqFrameBase *frame, tabContainer->childFrameList()) {
        if (!frame || !frame->activeChildView()) {
            continue;
        }
        KonqView *view = frame->activeChildView();
        if (view->locationBarURL().isEmpty()) {
            continue;
        }
        list << KBookmarkOwner::FutureBookmark(view->caption(), view->url(), KIO::iconNameForUrl(view->url()));
    }
    return list;
}

QString KonqExtendedBookmarkOwner::currentTitle() const
{
    return m_pKonqMainWindow->currentTitle();
}

void KonqExtendedBookmarkOwner::openInNewTab(const KBookmark &bm)
{
    bool newTabsInFront = KonqSettings::newTabsInFront();
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newTabsInFront = !newTabsInFront;
    }

    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = newTabsInFront;
    req.openAfterCurrentPage = false;
    req.forceAutoEmbed = true;

    m_pKonqMainWindow->openFilteredUrl(bm.url().url(), req);
}

void KonqExtendedBookmarkOwner::openFolderinTabs(const KBookmarkGroup &grp)
{
    bool newTabsInFront = KonqSettings::newTabsInFront();
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        newTabsInFront = !newTabsInFront;
    }
    KonqOpenURLRequest req;
    req.browserArgs.setNewTab(true);
    req.newTabInFront = false;
    req.openAfterCurrentPage = false;
    req.forceAutoEmbed = true;

    const QList<QUrl> list = grp.groupUrlList();
    if (list.isEmpty()) {
        return;
    }

    if (list.size() > 20) {
        if (KMessageBox::questionTwoActions(m_pKonqMainWindow,
                                       i18n("You have requested to open more than 20 bookmarks in tabs. "
                                            "This might take a while. Continue?"),
                                       i18nc("@title:window", "Open bookmarks folder in new tabs"),
                                       KGuiItem(i18nc("@action:button", "Open"), QStringLiteral("tab-new")),
                                       KStandardGuiItem::cancel()) != KMessageBox::PrimaryAction) {
            return;
        }
    }

    QList<QUrl>::ConstIterator it = list.constBegin();
    QList<QUrl>::ConstIterator end = list.constEnd();
    --end;
    for (; it != end; ++it) {
        m_pKonqMainWindow->openFilteredUrl((*it).toString(), req);
    }
    if (newTabsInFront) {
        req.newTabInFront = true;
    }
    m_pKonqMainWindow->openFilteredUrl((*end).toString(), req);
}

void KonqExtendedBookmarkOwner::openInNewWindow(const KBookmark &bm)
{
    const QUrl finalURL(KonqMisc::konqFilteredURL(m_pKonqMainWindow, bm.url().url()));
    KonqMainWindow *mw = KonqMainWindowFactory::createNewWindow(finalURL);
    mw->show();
}

QString KonqMainWindow::currentTitle() const
{
    return m_currentView ? m_currentView->caption() : QString();
}

// Convert between deprecated string-based KParts::BrowserExtension::ActionGroupMap
// to newer enum-based KonqPopupMenu::ActionGroupMap
static KonqPopupMenu::ActionGroupMap convertActionGroups(const KParts::BrowserExtension::ActionGroupMap &input)
{
    KonqPopupMenu::ActionGroupMap agm;
    agm.insert(KonqPopupMenu::TopActions, input.value(QStringLiteral("topactions")));
    agm.insert(KonqPopupMenu::TabHandlingActions, input.value(QStringLiteral("tabhandling")));
    agm.insert(KonqPopupMenu::EditActions, input.value(QStringLiteral("editactions")));
    agm.insert(KonqPopupMenu::PreviewActions, input.value(QStringLiteral("preview")));
    agm.insert(KonqPopupMenu::CustomActions, input.value(QStringLiteral("partactions")));
    agm.insert(KonqPopupMenu::LinkActions, input.value(QStringLiteral("linkactions")));
    return agm;
}

void KonqMainWindow::slotPopupMenu(const QPoint &global, const QUrl &url, mode_t mode, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs, KParts::BrowserExtension::PopupFlags flags, const KParts::BrowserExtension::ActionGroupMap &actionGroups)
{
    KFileItem item(url, args.mimeType(), mode);
    KFileItemList items;
    items.append(item);
    slotPopupMenu(global, items, args, browserArgs, flags, actionGroups);
}

void KonqMainWindow::slotPopupMenu(const QPoint &global, const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs, KParts::BrowserExtension::PopupFlags itemFlags, const KParts::BrowserExtension::ActionGroupMap &actionGroups)
{
    KonqView *m_oldView = m_currentView;
    KonqView *currentView = childView(static_cast<KParts::ReadOnlyPart *>(sender()->parent()));

    //qCDebug(KONQUEROR_LOG) << "m_oldView=" << m_oldView << "new currentView=" << currentView << "passive:" << currentView->isPassiveMode();

    if ((m_oldView != currentView) && currentView->isPassiveMode()) {
        // Make this view active only temporarily (because it's passive)
        m_currentView = currentView;

        if (m_oldView && m_oldView->browserExtension()) {
            disconnectExtension(m_oldView->browserExtension());
        }
        if (m_currentView->browserExtension()) {
            connectExtension(m_currentView->browserExtension());
        }
    }
    // Note that if m_oldView!=currentView and currentView isn't passive,
    // then the KParts mechanism has already noticed the click in it,
    // but KonqViewManager delays the GUI-rebuilding with a single-shot timer.
    // Right after the popup shows up, currentView _will_ be m_currentView.

    //qCDebug(KONQUEROR_LOG) << "current view=" << m_currentView << m_currentView->part()->metaObject()->className();

    // This action collection is used to pass actions to KonqPopupMenu.
    // It has to be a KActionCollection instead of a QList<QAction *> because we need
    // the actionStatusText signal...
    KActionCollection popupMenuCollection(static_cast<QWidget *>(nullptr));

    popupMenuCollection.addAction(QStringLiteral("closeditems"), m_paClosedItems);

#if 0
    popupMenuCollection.addAction("find", m_paFindFiles);
#endif

    popupMenuCollection.addAction(QStringLiteral("undo"), m_paUndo);
    popupMenuCollection.addAction(QStringLiteral("cut"), m_paCut);
    popupMenuCollection.addAction(QStringLiteral("copy"), m_paCopy);
    popupMenuCollection.addAction(QStringLiteral("paste"), m_paPaste);

    // The pasteto action is used when clicking on a dir, to paste into it.
    QAction *actPaste = KStandardAction::paste(this, SLOT(slotPopupPasteTo()), this);
    actPaste->setEnabled(m_paPaste->isEnabled());
    popupMenuCollection.addAction(QStringLiteral("pasteto"), actPaste);

    prepareForPopupMenu(items, args, browserArgs);

    bool sReading = false;
    if (!m_popupUrl.isEmpty()) {
        sReading = KProtocolManager::supportsReading(m_popupUrl);
    }

    QUrl viewURL = currentView->url();
    qCDebug(KONQUEROR_LOG) << "viewURL=" << viewURL;

    bool openedForViewURL = false;
    //bool dirsSelected = false;
    bool devicesFile = false;

    if (items.count() == 1) {
        const QUrl firstURL = items.first().url();
        if (!viewURL.isEmpty()) {
            //firstURL.cleanPath();
            openedForViewURL = firstURL.matches(viewURL, QUrl::StripTrailingSlash);
        }
        devicesFile = firstURL.scheme().indexOf(QLatin1String("device"), 0, Qt::CaseInsensitive) == 0;
        //dirsSelected = S_ISDIR( items.first()->mode() );
    }
    //qCDebug(KONQUEROR_LOG) << "viewURL=" << viewURL;

    QUrl url = viewURL;
    bool isIntoTrash = url.scheme() == QLatin1String("trash") || url.url().startsWith(QLatin1String("system:/trash"));
    const bool doTabHandling = !openedForViewURL && !isIntoTrash && sReading;
    const bool showEmbeddingServices = items.count() == 1 && !m_popupMimeType.isEmpty() &&
                                       !isIntoTrash && !devicesFile &&
                                       (itemFlags & KParts::BrowserExtension::ShowTextSelectionItems) == 0;

    QVector<KPluginMetaData> embeddingServices;
    if (showEmbeddingServices) {
        const QString currentServiceName = currentView->service().pluginId();

        // List of services for the "Preview In" submenu.
        QVector<KPluginMetaData> allEmbeddingServices = KParts::PartLoader::partsForMimeType(m_popupMimeType);
        auto filter = [currentServiceName](const KPluginMetaData &md) {
            return !md.value(QLatin1String("X-KDE-BrowserView-HideFromMenus"),false) && md.pluginId() != currentServiceName;
        };
        std::copy_if(allEmbeddingServices.begin(), allEmbeddingServices.end(), std::back_inserter(embeddingServices), filter);
    }

    // TODO: get rid of KParts::BrowserExtension::PopupFlags
    KonqPopupMenu::Flags popupFlags = static_cast<KonqPopupMenu::Flags>(static_cast<int>(itemFlags));

    KonqPopupMenu::ActionGroupMap popupActionGroups = convertActionGroups(actionGroups);

    PopupMenuGUIClient *konqyMenuClient = new PopupMenuGUIClient(embeddingServices,
            popupActionGroups,
            !menuBar()->isVisible() ? m_paShowMenuBar : nullptr,
            fullScreenMode() ? m_ptaFullScreen : nullptr
                                                                );
    connect(konqyMenuClient, &PopupMenuGUIClient::openEmbedded, this, &KonqMainWindow::slotOpenEmbedded, Qt::QueuedConnection);

    // Those actions go into the PopupMenuGUIClient, since that's the one defining them.
    QList<QAction *> tabHandlingActions;
    if (doTabHandling) {
        if (browserArgs.forcesNewWindow()) {
            QAction *act = konqyMenuClient->actionCollection()->addAction(QStringLiteral("sameview"));
            act->setText(i18n("Open in T&his Window"));
            act->setStatusTip(i18n("Open the document in current window"));
            connect(act, &QAction::triggered, this, &KonqMainWindow::slotPopupThisWindow);
            tabHandlingActions.append(act);
        }
        QAction *actNewWindow = konqyMenuClient->actionCollection()->addAction(QStringLiteral("newview"));
        actNewWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
        actNewWindow->setText(i18n("Open in New &Window"));
        actNewWindow->setStatusTip(i18n("Open the document in a new window"));
        connect(actNewWindow, &QAction::triggered, this, &KonqMainWindow::slotPopupNewWindow);
        tabHandlingActions.append(actNewWindow);

        QAction *actNewTab = konqyMenuClient->actionCollection()->addAction(QStringLiteral("openintab"));
        actNewTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
        actNewTab->setText(i18n("Open in &New Tab"));
        connect(actNewTab, &QAction::triggered, this, &KonqMainWindow::slotPopupNewTab);
        actNewTab->setStatusTip(i18n("Open the document in a new tab"));
        tabHandlingActions.append(actNewTab);

        QAction *separator = new QAction(konqyMenuClient->actionCollection());
        separator->setSeparator(true);
        tabHandlingActions.append(separator);
    }

    if (doTabHandling) {
        popupActionGroups.insert(KonqPopupMenu::TabHandlingActions, tabHandlingActions);
    }

    QPointer<KonqPopupMenu> pPopupMenu = new KonqPopupMenu(
        items,
        viewURL,
        popupMenuCollection,
        popupFlags,
        // This parent ensures that if the part destroys itself (e.g. KHTML redirection),
        // it will close the popupmenu
        currentView->part()->widget());
    pPopupMenu->setNewFileMenu(m_pMenuNew);
    pPopupMenu->setBookmarkManager(s_bookmarkManager);
    pPopupMenu->setActionGroups(popupActionGroups);

    if (openedForViewURL && !viewURL.isLocalFile()) {
        pPopupMenu->setURLTitle(currentView->caption());
    }

    QPointer<KParts::BrowserExtension> be = ::qobject_cast<KParts::BrowserExtension *>(sender());

    if (be) {
        QObject::connect(this, &KonqMainWindow::popupItemsDisturbed, pPopupMenu.data(), &KonqPopupMenu::close);
        QObject::connect(be, SIGNAL(itemsRemoved(KFileItemList)),
                         this, SLOT(slotItemsRemoved(KFileItemList)));
    }

    QPointer<QObject> guard(this);   // #149736, window could be deleted inside popupmenu event loop
    pPopupMenu->exec(global);

    delete pPopupMenu;

    // We're sort of misusing KActionCollection here, but we need it for the actionStatusText signal...
    // Anyway. If the action belonged to the view, and the view got deleted, we don't want ~KActionCollection
    // to iterate over those deleted actions
    /*KActionPtrList lst = popupMenuCollection.actions();
    KActionPtrList::iterator it = lst.begin();
    for ( ; it != lst.end() ; ++it )
        popupMenuCollection.take( *it );*/

    if (guard.isNull()) { // the placement of this test is very important, double-check #149736 if moving stuff around
        return;
    }

    if (be) {
        QObject::disconnect(be, SIGNAL(itemsRemoved(KFileItemList)),
                            this, SLOT(slotItemsRemoved(KFileItemList)));
    }

    delete konqyMenuClient;
    m_popupItems.clear();

    // Deleted by konqyMenuClient's actioncollection
    //delete actNewTab;
    //delete actNewWindow;

    delete actPaste;

    // Restore current view if current is passive
    if ((m_oldView != currentView) && (currentView == m_currentView) && currentView->isPassiveMode()) {
        //qCDebug(KONQUEROR_LOG) << "restoring active view" << m_oldView;
        if (m_currentView->browserExtension()) {
            disconnectExtension(m_currentView->browserExtension());
        }
        if (m_oldView) {
            if (m_oldView->browserExtension()) {
                connectExtension(m_oldView->browserExtension());
                m_currentView = m_oldView;
            }
            // Special case: RMB + renaming in sidebar; setFocus would abort editing.
            QWidget *fw = focusWidget();
            if (!fw || !::qobject_cast<QLineEdit *>(fw)) {
                m_oldView->part()->widget()->setFocus();
                m_pViewManager->setActivePart(m_oldView->part());
            }
        }
    }
}

void KonqMainWindow::prepareForPopupMenu(const KFileItemList &items, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &browserArgs)
{
    if (!items.isEmpty()) {
        m_popupUrl = items.first().url();
        m_popupMimeType = items.first().mimetype();
    } else {
        m_popupUrl = QUrl();
        m_popupMimeType.clear();
    }
    // We will need these if we call the newTab slot
    m_popupItems = items;
    m_popupUrlArgs = args;
    m_popupUrlArgs.setMimeType(QString());   // Reset so that Open in New Window/Tab does mimetype detection
    m_popupUrlBrowserArgs = browserArgs;
}

void KonqMainWindow::slotItemsRemoved(const KFileItemList &items)
{
    QListIterator<KFileItem> it(items);
    while (it.hasNext()) {
        if (m_popupItems.contains(it.next())) {
            emit popupItemsDisturbed();
            return;
        }
    }
}

void KonqMainWindow::slotOpenEmbedded(const KPluginMetaData &part)
{
    if (!m_currentView) {
        return;
    }

    m_currentView->stop();
    m_currentView->setLocationBarURL(m_popupUrl);
    m_currentView->setTypedURL(QString());
    if (m_currentView->changePart(m_popupMimeType,
                                  part.pluginId(), true)) {
        m_currentView->openUrl(m_popupUrl, m_popupUrl.toDisplayString(QUrl::PreferLocalFile));
    }
}

void KonqMainWindow::slotPopupPasteTo()
{
    if (!m_currentView || m_popupUrl.isEmpty()) {
        return;
    }
    m_currentView->callExtensionURLMethod("pasteTo", m_popupUrl);
}

void KonqMainWindow::slotReconfigure()
{
    reparseConfiguration();
}

void KonqMainWindow::reparseConfiguration()
{
    qCDebug(KONQUEROR_LOG);

    KonqSettings::self()->load();
    m_pViewManager->applyConfiguration();
    KonqMouseEventFilter::self()->reparseConfiguration();

    MapViews::ConstIterator it = m_mapViews.constBegin();
    MapViews::ConstIterator end = m_mapViews.constEnd();
    for (; it != end; ++it) {
        (*it)->reparseConfiguration();
    }
}

void KonqMainWindow::saveProperties(KConfigGroup &config)
{
    // Ensure no crash if the sessionmanager timer fires before the ctor is done
    // This can happen via ToggleViewGUIClient -> KServiceTypeTrader::query
    // -> KSycoca running kbuildsycoca -> nested event loop.
    if (m_fullyConstructed) {
        KonqFrameBase::Options flags = KonqFrameBase::SaveHistoryItems;
        m_pViewManager->saveViewConfigToGroup(config, flags);
    }
}

void KonqMainWindow::readProperties(const KConfigGroup &configGroup)
{
    m_pViewManager->loadViewConfigFromGroup(configGroup, QString() /*no profile name*/);
    // read window settings
    applyMainWindowSettings(configGroup);
}

void KonqMainWindow::applyMainWindowSettings(const KConfigGroup &config)
{
    KParts::MainWindow::applyMainWindowSettings(config);
    if (m_currentView) {
        /// @Note status bar isn't direct child to main window
        QString entry = config.readEntry("StatusBar", "Enabled");
        m_currentView->frame()->statusbar()->setVisible(entry != QLatin1String("Disabled"));
    }
}

void KonqMainWindow::saveMainWindowSettings(KConfigGroup &config)
{
    KParts::MainWindow::saveMainWindowSettings(config);
    if (m_currentView) {
        /// @Note status bar isn't direct child to main window
        config.writeEntry("StatusBar", m_currentView->frame()->statusbar()->isHidden() ? "Disabled" : "Enabled");
        config.sync();
    }
}

void KonqMainWindow::setInitialFrameName(const QString &name)
{
    m_initialFrameName = name;
}

void KonqMainWindow::updateOpenWithActions()
{
    unplugActionList(QStringLiteral("openwithbase"));
    unplugActionList(QStringLiteral("openwith"));

    qDeleteAll(m_openWithActions);
    m_openWithActions.clear();

    delete m_openWithMenu;
    m_openWithMenu = nullptr;

    if (!KAuthorized::authorizeAction(QStringLiteral("openwith"))) {
        return;
    }

    m_openWithMenu = new KActionMenu(i18n("&Open With"), this);

    const KService::List &services = m_currentView->appServiceOffers();
    KService::List::ConstIterator it = services.constBegin();
    const KService::List::ConstIterator end = services.constEnd();

    const int baseOpenWithItems = qMax(KonqSettings::openWithItems(), 0);

    int idxService = 0;
    for (; it != end; ++it, ++idxService) {
        QAction *action;

        if (idxService < baseOpenWithItems) {
            action = new QAction(i18n("Open with %1", (*it)->name()), this);
        } else {
            action = new QAction((*it)->name(), this);
        }
        action->setIcon(QIcon::fromTheme((*it)->icon()));

        connect(action, SIGNAL(triggered()),
                this, SLOT(slotOpenWith()));

        actionCollection()->addAction((*it)->desktopEntryName(), action);
        if (idxService < baseOpenWithItems) {
            m_openWithActions.append(action);
        } else {
            m_openWithMenu->addAction(action);
        }
    }

    if (services.count() > 0) {
        plugActionList(QStringLiteral("openwithbase"), m_openWithActions);
        QList<QAction *> openWithActionsMenu;
        if (idxService > baseOpenWithItems) {
            openWithActionsMenu.append(m_openWithMenu);
        }
        QAction *sep = new QAction(this);
        sep->setSeparator(true);
        openWithActionsMenu.append(sep);
        plugActionList(QStringLiteral("openwith"), openWithActionsMenu);
    }
}

void KonqMainWindow::updateViewModeActions()
{
    unplugViewModeActions();
    for (QAction *action : m_viewModesGroup->actions()) {
        for (QWidget *w : action->associatedWidgets()) {
            w->removeAction(action);
        }
        delete action;
    }

    delete m_viewModeMenu;
    m_viewModeMenu = nullptr;

    const QVector<KPluginMetaData> services = m_currentView->partServiceOffers();
    if (services.count() <= 1) {
        return;
    }

    m_viewModeMenu = new KActionMenu(i18nc("@action:inmenu View", "&View Mode"), this);
    actionCollection()->addAction( "viewModeMenu", m_viewModeMenu );

    for (const KPluginMetaData & md : services) {
        const QString id = md.pluginId();
        bool isCurrentView = id == m_currentView->service().pluginId();

        //If a view provide several actions, its metadata contains an X-Konqueror-Actions-File entry
        //with the path of a .desktop file where the actions are described. The contents of this file
        //are the same as the action-related part of the old part .desktop file
        const QString actionDesktopFile = md.value("X-Konqueror-Actions-File");

        if (!actionDesktopFile.isEmpty()) {
            KDesktopFile df(QStandardPaths::DataLocation, actionDesktopFile);
            QStringList actionNames = df.readActions();

            for (const QString &name : actionNames) {
                KConfigGroup grp = df.actionGroup(name);
                QString text = grp.readEntry("Name", QString());
                QString exec = grp.readEntry("Exec", QString());
                if (text.isEmpty()) {
                    qCDebug(KONQUEROR_LOG) << "File" << df.fileName() << "doesn't contain a \"name\" entry";
                    continue;
                }
                KToggleAction *action = new KToggleAction(QIcon::fromTheme(grp.readEntry("Icon", QString())), text, this);
//                 actionCollection()->addAction(id /*not unique!*/, action);
                action->setObjectName(id + QLatin1String("-viewmode"));
                action->setData(name);
                action->setActionGroup(m_viewModesGroup);
                m_viewModeMenu->menu()->addAction(action);
                if (isCurrentView && m_currentView->internalViewMode() == name) {
                    action->setChecked(true);
                }
            }
        } else {
            //TODO port away from query: is there a replacement for KService::genericName?
            QString text = md.name();
            KToggleAction *action = new KToggleAction(QIcon::fromTheme(md.iconName()), text, this);
            // NOTE: "-viewmode" is appended to id to avoid overwriting existing
            // action, e.g. konsolepart added through ToggleViewGUIClient in the ctor will be
            // overwritten by the view mode konsolepart action added here.  #266517.
            actionCollection()->addAction(id + QLatin1String("-viewmode"), action);
            action->setActionGroup(m_viewModesGroup);
            m_viewModeMenu->menu()->addAction(action);
            action->setChecked(isCurrentView);
        }
    }

    // No view mode for actions toggable views
    // (The other way would be to enforce a better servicetype for them, than Browser/View)
    if (!m_currentView->isToggleView()
            /* already tested: && services.count() > 1 */
            && m_viewModeMenu) {
        plugViewModeActions();
    }
}

void KonqMainWindow::slotInternalViewModeChanged()
{
    KParts::ReadOnlyPart *part = static_cast<KParts::ReadOnlyPart *>(sender());
    KonqView *view = m_mapViews.value(part);
    if (view) {
        const QString actionName = view->service().pluginId();
        const QString actionData = view->internalViewMode();
        Q_FOREACH (QAction *action, m_viewModesGroup->actions()) {
            if (action->objectName() == actionName + QLatin1String("-viewmode") &&
                    action->data().toString() == actionData) {
                action->setChecked(true);
                break;
            }
        }
    }
}

void KonqMainWindow::plugViewModeActions()
{
    QList<QAction *> lst;

    if (m_viewModeMenu) {
        lst.append(m_viewModeMenu);
    }

    plugActionList(QStringLiteral("viewmode"), lst);
}

void KonqMainWindow::unplugViewModeActions()
{
    unplugActionList(QStringLiteral("viewmode"));
}

void KonqMainWindow::updateBookmarkBar()
{
    KToolBar *bar = this->findChild<KToolBar *>(QStringLiteral("bookmarkToolBar"));
    if (!bar) {
        return;
    }
    if (m_paBookmarkBar && bar->actions().isEmpty()) {
        bar->hide();
    }

}

void KonqMainWindow::closeEvent(QCloseEvent *e)
{
    // This breaks session management (the window is withdrawn in kwin)
    // so let's do this only when closed by the user.

    if (!qApp->isSavingSession()) {
        KonqFrameTabs *tabContainer = m_pViewManager->tabContainer();
        if (tabContainer->count() > 1) {
            KSharedConfig::Ptr config = KSharedConfig::openConfig();
            KConfigGroup cs(config, QStringLiteral("Notification Messages"));

            if (!cs.hasKey("MultipleTabConfirm")) {
                switch (
                    KMessageBox::warningTwoActionsCancel(
                        this,
                        i18n("You have multiple tabs open in this window, "
                             "are you sure you want to quit?"),
                        i18nc("@title:window", "Confirmation"),
                        KStandardGuiItem::closeWindow(),
                        KGuiItem(i18n("C&lose Current Tab"), QStringLiteral("tab-close")),
                        KStandardGuiItem::cancel(),
                        QStringLiteral("MultipleTabConfirm")
                    )
                ) {
                case KMessageBox::PrimaryAction :
                    break;
                case KMessageBox::SecondaryAction :
                    e->ignore();
                    slotRemoveTab();
                    return;
                case KMessageBox::Cancel :
                    e->ignore();
                    return;
                default:
                    Q_UNREACHABLE();
                }
            }
        }

        const int originalTabIndex = tabContainer->currentIndex();
        for (int tabIndex = 0; tabIndex < tabContainer->count(); ++tabIndex) {
            KonqFrameBase *tab = tabContainer->tabAt(tabIndex);
            if (!KonqModifiedViewsCollector::collect(tab).isEmpty()) {
                m_pViewManager->showTab(tabIndex);
                const QString question = m_pViewManager->isTabBarVisible()
                                         ? i18n("This tab contains changes that have not been submitted.\nClosing the window will discard these changes.")
                                         : i18n("This page contains changes that have not been submitted.\nClosing the window will discard these changes.");
                if (KMessageBox::warningContinueCancel(
                            this, question,
                            i18nc("@title:window", "Discard Changes?"), KGuiItem(i18n("&Discard Changes"), QStringLiteral("application-exit")),
                            KStandardGuiItem::cancel(), QStringLiteral("discardchangesclose")) != KMessageBox::Continue) {
                    e->ignore();
                    m_pViewManager->showTab(originalTabIndex);
                    return;
                }
            }
        }

        if (settingsDirty() && autoSaveSettings()) {
            saveAutoSaveSettings();
        }

        addClosedWindowToUndoList();
    }
    // We're going to close - tell the parts
    MapViews::ConstIterator it = m_mapViews.constBegin();
    MapViews::ConstIterator end = m_mapViews.constEnd();
    for (; it != end; ++it) {
        if ((*it)->part() && (*it)->part()->widget()) {
            QApplication::sendEvent((*it)->part()->widget(), e);
        }
    }
    KParts::MainWindow::closeEvent(e);
}

void KonqMainWindow::addClosedWindowToUndoList()
{
    qCDebug(KONQUEROR_LOG);

    // 1. We get the current title
    int numTabs = m_pViewManager->tabContainer()->childFrameList().count();
    QString title(i18n("no name"));

    if (m_currentView) {
        title = m_currentView->caption();
    }

    // 2. Create the KonqClosedWindowItem and  save its config
    KonqClosedWindowItem *closedWindowItem = new KonqClosedWindowItem(title, KonqClosedWindowsManager::self()->memoryStore(),
                                                                      m_pUndoManager->newCommandSerialNumber(), numTabs);
    saveProperties(closedWindowItem->configGroup());

    // 3. Add the KonqClosedWindowItem to the undo list
    m_paClosedItems->setEnabled(true);
    m_pUndoManager->addClosedWindowItem(closedWindowItem);

    qCDebug(KONQUEROR_LOG) << "done";
}

void KonqMainWindow::updateWindowIcon()
{
    KParts::MainWindow::setWindowIcon(KonqPixmapProvider::self()->iconForUrl(m_combo->currentText()));
}

void KonqMainWindow::slotIntro()
{
    openUrl(nullptr, KonqUrl::url(KonqUrl::Type::NoPath));
}

void KonqMainWindow::goURL()
{
    QLineEdit *lineEdit = comboEdit();
    if (!lineEdit) {
        return;
    }

    QKeyEvent event(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, QChar('\n'));
    QApplication::sendEvent(lineEdit, &event);
}

/**
 * Adds the URL of a KonqView to the closed tabs list.
 * This slot gets called each time a View is closed.
 */
void KonqMainWindow::slotAddClosedUrl(KonqFrameBase *tab)
{
    qCDebug(KONQUEROR_LOG);
    QString title(i18n("no name")), url(KonqUrl::string(KonqUrl::Type::Blank));

    // Did the tab contain a single frame, or a splitter?
    KonqFrame *frame = dynamic_cast<KonqFrame *>(tab);
    if (!frame) {
        KonqFrameContainer *frameContainer = dynamic_cast<KonqFrameContainer *>(tab);
        if (frameContainer->activeChildView()) {
            frame = frameContainer->activeChildView()->frame();
        }
    }

    KParts::ReadOnlyPart *part = frame ? frame->part() : nullptr;
    if (part) {
        url = part->url().url();
    }
    if (frame) {
        title = frame->title().trimmed();
    }
    if (title.isEmpty()) {
        title = url;
    }
    title = KStringHandler::csqueeze(title, 50);

    // Now we get the position of the tab
    const int index =  m_pViewManager->tabContainer()->childFrameList().indexOf(tab);

    KonqClosedTabItem *closedTabItem = new KonqClosedTabItem(url, KonqClosedWindowsManager::self()->memoryStore(),
                                                             title, index, m_pUndoManager->newCommandSerialNumber());

    QString prefix = KonqFrameBase::frameTypeToString(tab->frameType()) + QString::number(0);
    closedTabItem->configGroup().writeEntry("RootItem", prefix);
    prefix.append(QLatin1Char('_'));
    KonqFrameBase::Options flags = KonqFrameBase::SaveHistoryItems;
    tab->saveConfig(closedTabItem->configGroup(), prefix, flags, nullptr, 0, 1);

    m_paClosedItems->setEnabled(true);
    m_pUndoManager->addClosedTabItem(closedTabItem);

    qCDebug(KONQUEROR_LOG) << "done";
}

void KonqMainWindow::slotLocationLabelActivated()
{
    focusLocationBar();
    QLineEdit *edit = comboEdit();
    if (edit) {
        edit->selectAll();
    }
}

void KonqMainWindow::slotOpenURL(const QUrl &url)
{
    openUrl(nullptr, url);
}

bool KonqMainWindow::sidebarVisible() const
{
    QAction *a = m_toggleViewGUIClient->action(QStringLiteral("konq_sidebartng"));
    return (a && static_cast<KToggleAction *>(a)->isChecked());
}

bool KonqMainWindow::fullScreenMode() const
{
    return m_ptaFullScreen->isChecked();
}

void KonqMainWindow::slotAddWebSideBar(const QUrl &url, const QString &name)
{
    if (url.isEmpty() && name.isEmpty()) {
        return;
    }

    qCDebug(KONQUEROR_LOG) << "Requested to add URL" << url << " [" << name << "] to the sidebar!";

    QAction *a = m_toggleViewGUIClient->action(QStringLiteral("konq_sidebartng"));
    if (!a) {
        KMessageBox::error(nullptr, i18n("Your sidebar is not functional or unavailable. A new entry cannot be added."), i18nc("@title:window", "Web Sidebar"));
        return;
    }

    int rc = KMessageBox::questionTwoActions(nullptr,
                                        i18n("Add new web extension \"%1\" to your sidebar?",
                                                name.isEmpty() ? name : url.toDisplayString()),
                                        i18nc("@title:window", "Web Sidebar"), KGuiItem(i18n("Add")), KGuiItem(i18n("Do Not Add")));

    if (rc == KMessageBox::PrimaryAction) {
        // Show the sidebar
        if (!static_cast<KToggleAction *>(a)->isChecked()) {
            a->trigger();
        }

        // Tell it to add a new panel
        MapViews::ConstIterator it;
        for (it = viewMap().constBegin(); it != viewMap().constEnd(); ++it) {
            KonqView *view = it.value();
            if (view) {
                KPluginMetaData svc = view->service();
                if (svc.pluginId() == QLatin1String("konq_sidebartng")) {
                    emit view->browserExtension()->addWebSideBar(url, name);
                    break;
                }
            }
        }
    }
}

void KonqMainWindow::addBookmarksIntoCompletion(const KBookmarkGroup &group)
{
    const QString http = QStringLiteral("http");
    const QString ftp = QStringLiteral("ftp");

    if (group.isNull()) {
        return;
    }

    for (KBookmark bm = group.first();
            !bm.isNull(); bm = group.next(bm)) {
        if (bm.isGroup()) {
            addBookmarksIntoCompletion(bm.toGroup());
            continue;
        }

        QUrl url = bm.url();
        if (!url.isValid()) {
            continue;
        }

        QString u = url.toDisplayString();
        s_pCompletion->addItem(u);

        if (url.isLocalFile()) {
            s_pCompletion->addItem(url.toLocalFile());
        } else if (url.scheme() == http) {
            s_pCompletion->addItem(u.mid(7));
        } else if (url.scheme() == ftp &&
                   url.host().startsWith(ftp)) {
            s_pCompletion->addItem(u.mid(6));
        }
    }
}

//
// the smart popup completion code , <l.lunak@kde.org>
//

// prepend http://www. or http:// if there's no protocol in 's'
// This is used only when there are no completion matches
static QString hp_tryPrepend(const QString &s)
{
    if (s.isEmpty() || s[0] == QLatin1Char('/') || s[0] == QLatin1Char('~')) {
        return QString();
    }

    bool containsSpace = false;

    for (int pos = 0;
            pos < s.length() - 2; // 4 = ://x
            ++pos) {
        if (s[ pos ] == ':' && s[ pos + 1 ] == '/' && s[ pos + 2 ] == '/') {
            return QString();
        }
        if (!s[ pos ].isLetter()) {
            break;
        }
        if (s[pos].isSpace()) {
            containsSpace = true;
            break;
        }
    }

    if (containsSpace || s.at(s.length() - 1).isSpace()) {
        return QString();
    }

    return (s.startsWith(QLatin1String("www.")) ? "http://" : "http://www.") + s;
}

static void hp_removeDupe(KCompletionMatches &l, const QString &dupe,
                          KCompletionMatches::Iterator it_orig)
{
    KCompletionMatches::Iterator it = it_orig + 1;
    while (it != l.end()) {
        if ((*it).value() == dupe) {
            (*it_orig).first = qMax((*it_orig).first, (*it).key());
            it = l.erase(it);
            continue;
        }
        ++it;
    }
}

// remove duplicates like 'http://www.kde.org' and 'http://www.kde.org/'
// (i.e. the trailing slash)
// some duplicates are also created by prepending protocols
static void hp_removeDuplicates(KCompletionMatches &l)
{
    QString http = QStringLiteral("http://");
    QString ftp = QStringLiteral("ftp://ftp.");
    QString file = QStringLiteral("file:");
    QString file2 = QStringLiteral("file://");
    l.removeDuplicates();
    for (KCompletionMatches::Iterator it = l.begin();
            it != l.end();
            ++it) {
        QString str = (*it).value();
        if (str.startsWith(http)) {
            if (str.indexOf('/', 7) < 0) {    // http://something<noslash>
                hp_removeDupe(l, str + '/', it);
                hp_removeDupe(l, str.mid(7) + '/', it);
            } else if (str[ str.length() - 1 ] == '/') {
                hp_removeDupe(l, str.left(str.length() - 1), it);
                hp_removeDupe(l, str.left(str.length() - 1).mid(7), it);
            }
            hp_removeDupe(l, str.mid(7), it);
        } else if (str.startsWith(ftp)) { // ftp://ftp.
            hp_removeDupe(l, str.mid(6), it);    // remove dupes without ftp://
        } else if (str.startsWith(file2)) {
            hp_removeDupe(l, str.mid(7), it);    // remove dupes without file://
        } else if (str.startsWith(file)) {
            hp_removeDupe(l, str.mid(5), it);    // remove dupes without file:
        }
    }
}

static void hp_removeCommonPrefix(KCompletionMatches &l, const QString &prefix)
{
    for (KCompletionMatches::Iterator it = l.begin();
            it != l.end();
        ) {
        if ((*it).value().startsWith(prefix)) {
            it = l.erase(it);
            continue;
        }
        ++it;
    }
}

// don't include common prefixes like 'http://', i.e. when s == 'h', include
// http://hotmail.com but don't include everything just starting with 'http://'
static void hp_checkCommonPrefixes(KCompletionMatches &matches, const QString &s)
{
    static const char *const prefixes[] = {
        "http://",
        "https://",
        "www.",
        "ftp://",
        "http://www.",
        "https://www.",
        "ftp://ftp.",
        "file:",
        "file://",
        nullptr
    };
    for (const char *const *pos = prefixes;
            *pos != nullptr;
            ++pos) {
        QString prefix = *pos;
        if (prefix.startsWith(s)) {
            hp_removeCommonPrefix(matches, prefix);
        }
    }
}

QStringList KonqMainWindow::historyPopupCompletionItems(const QString &s)
{
    const QString http = QStringLiteral("http://");
    const QString https = QStringLiteral("https://");
    const QString www = QStringLiteral("http://www.");
    const QString wwws = QStringLiteral("https://www.");
    const QString ftp = QStringLiteral("ftp://");
    const QString ftpftp = QStringLiteral("ftp://ftp.");
    const QString file = QStringLiteral("file:"); // without /, because people enter /usr etc.
    const QString file2 = QStringLiteral("file://");
    if (s.isEmpty()) {
        return QStringList();
    }
    KCompletionMatches matches = s_pCompletion->allWeightedMatches(s);
    hp_checkCommonPrefixes(matches, s);
    bool checkDuplicates = false;
    if (!s.startsWith(ftp)) {
        matches += s_pCompletion->allWeightedMatches(ftp + s);
        if (QStringLiteral("ftp.").startsWith(s)) {
            hp_removeCommonPrefix(matches, ftpftp);
        }
        checkDuplicates = true;
    }
    if (!s.startsWith(https)) {
        matches += s_pCompletion->allWeightedMatches(https + s);
        if (QStringLiteral("www.").startsWith(s)) {
            hp_removeCommonPrefix(matches, wwws);
        }
        checkDuplicates = true;
    }
    if (!s.startsWith(http)) {
        matches += s_pCompletion->allWeightedMatches(http + s);
        if (QStringLiteral("www.").startsWith(s)) {
            hp_removeCommonPrefix(matches, www);
        }
        checkDuplicates = true;
    }
    if (!s.startsWith(www)) {
        matches += s_pCompletion->allWeightedMatches(www + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(wwws)) {
        matches += s_pCompletion->allWeightedMatches(wwws + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(ftpftp)) {
        matches += s_pCompletion->allWeightedMatches(ftpftp + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(file)) {
        matches += s_pCompletion->allWeightedMatches(file + s);
        checkDuplicates = true;
    }
    if (!s.startsWith(file2)) {
        matches += s_pCompletion->allWeightedMatches(file2 + s);
        checkDuplicates = true;
    }
    if (checkDuplicates) {
        hp_removeDuplicates(matches);
    }
    QStringList items = matches.list();
    if (items.count() == 0
            && !s.contains(':') && !s.isEmpty() && s[ 0 ] != '/') {
        QString pre = hp_tryPrepend(s);
        if (!pre.isNull()) {
            items += pre;
        }
    }
    return items;
}

#ifndef NDEBUG
void KonqMainWindow::dumpViewList()
{
    qCDebug(KONQUEROR_LOG) << m_mapViews.count() << "views:";

    MapViews::Iterator end = m_mapViews.end();
    for (MapViews::Iterator it = m_mapViews.begin(); it != end; ++it) {
        KonqView *view = it.value();
        qCDebug(KONQUEROR_LOG) << view << view->part();
    }
}
#endif

void KonqMainWindow::insertChildFrame(KonqFrameBase *frame, int /*index*/)
{
    m_pChildFrame = frame;
    m_pActiveChild = frame;
    frame->setParentContainer(this);
    if (centralWidget() && centralWidget() != frame->asQWidget()) {
        centralWidget()->setParent(nullptr);   // workaround Qt-4.1.2 crash (reported)
        setCentralWidget(nullptr);
    }
    setCentralWidget(frame->asQWidget());
}

void KonqMainWindow::childFrameRemoved(KonqFrameBase *frame)
{
    Q_ASSERT(frame == m_pChildFrame);
    Q_UNUSED(frame)
    m_pChildFrame = nullptr;
    m_pActiveChild = nullptr;
}

void KonqMainWindow::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id, int depth)
{
    if (m_pChildFrame) {
        m_pChildFrame->saveConfig(config, prefix, options, docContainer, id, depth);
    }
}

void KonqMainWindow::copyHistory(KonqFrameBase *other)
{
    if (m_pChildFrame) {
        m_pChildFrame->copyHistory(other);
    }
}

void KonqMainWindow::setTitle(const QString &/*title*/, QWidget * /*sender*/)
{
}

void KonqMainWindow::setTabIcon(const QUrl &/*url*/, QWidget * /*sender*/)
{
}

QWidget *KonqMainWindow::asQWidget()
{
    return this;
}

KonqFrameBase::FrameType KonqMainWindow::frameType() const
{
    return KonqFrameBase::MainWindow;
}

KonqFrameBase *KonqMainWindow::childFrame()const
{
    return m_pChildFrame;
}

void KonqMainWindow::setActiveChild(KonqFrameBase * /*activeChild*/)
{
}

void KonqMainWindow::setWorkingTab(int index)
{
    m_workingTab = index;
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf(const QString &mimeType)
{
    return isMimeTypeAssociatedWithSelf(mimeType, KApplicationTrader::preferredService(mimeType));
}

bool KonqMainWindow::isMimeTypeAssociatedWithSelf(const QString &/*mimeType*/, const KService::Ptr &offer)
{
    // Prevention against user stupidity : if the associated app for this mimetype
    // is konqueror/kfmclient, then we'll loop forever. So we have to
    // 1) force embedding first, if that works we're ok
    // 2) check what OpenUrlJob is going to do before calling it.
    return (offer && (offer->desktopEntryName() == QLatin1String("konqueror") ||
                      offer->exec().trimmed().startsWith(QLatin1String("kfmclient"))));
}

bool KonqMainWindow::refuseExecutingKonqueror(const QString &mimeType)
{
    if (activeViewsNotLockedCount() > 0) {   // if I lock the only view, then there's no error: open links in a new window
        KMessageBox::error(this, i18n("There appears to be a configuration error. You have associated Konqueror with %1, but it cannot handle this file type.", mimeType));
        return true; // we refuse indeed
    }
    return false; // no error
}

bool KonqMainWindow::event(QEvent *e)
{
    if (e->type() == QEvent::StatusTip) {
        if (m_currentView && m_currentView->frame()->statusbar()) {
            KonqFrameStatusBar *statusBar = m_currentView->frame()->statusbar();
            statusBar->message(static_cast<QStatusTipEvent *>(e)->tip());
        }
    }

    if (KonqFileSelectionEvent::test(e) ||
            KonqFileMouseOverEvent::test(e) ||
            KParts::PartActivateEvent::test(e)) {
        // Forward the event to all views
        MapViews::ConstIterator it = m_mapViews.constBegin();
        MapViews::ConstIterator end = m_mapViews.constEnd();
        for (; it != end; ++it) {
            QApplication::sendEvent((*it)->part(), e);
        }
        return true;
    }

    if (KParts::OpenUrlEvent::test(e)) {
        KParts::OpenUrlEvent *ev = static_cast<KParts::OpenUrlEvent *>(e);

        // Forward the event to all views
        MapViews::ConstIterator it = m_mapViews.constBegin();
        MapViews::ConstIterator end = m_mapViews.constEnd();
        for (; it != end; ++it) {
            // Don't resend to sender
            if (it.key() != ev->part()) {
                //qCDebug(KONQUEROR_LOG) << "Sending event to view" << it.key()->metaObject()->className();
                QApplication::sendEvent(it.key(), e);
            }
        }
    }
    return KParts::MainWindow::event(e);
}

void KonqMainWindow::slotUndoTextChanged(const QString &newText)
{
    m_paUndo->setText(newText);
}

KonqView *KonqMainWindow::currentView() const
{
    return m_currentView;
}

bool KonqMainWindow::accept(KonqFrameVisitor *visitor)
{
    return visitor->visit(this)
           && (!m_pChildFrame || m_pChildFrame->accept(visitor))
           && visitor->endVisit(this);
}

QLineEdit *KonqMainWindow::comboEdit()
{
    return m_combo ? m_combo->lineEdit() : nullptr;
}

void KonqMainWindow::updateProxyForWebEngine(bool updateProtocolManager)
{
    if (updateProtocolManager) {
        KProtocolManager::reparseConfiguration();
    }

    KPluginMetaData part = preferredPart(QStringLiteral("text/html"));
    Q_ASSERT(part.isValid());
    const bool webengineIsDefault = part.pluginId() == QLatin1String("webenginepart");
    if (!webengineIsDefault) {
        return;
    }

    KProtocolManager::ProxyType proxyType = KProtocolManager::proxyType();
    if (proxyType == KProtocolManager::WPADProxy || proxyType == KProtocolManager::PACProxy) {
        QString msg = i18n("Your proxy configuration can't be used with the QtWebEngine HTML engine. "
                           "No proxy will be used\n\n QtWebEngine only support a fixed proxy, so proxy auto-configuration (PAC) "
                           "and Web Proxy Auto-Discovery protocol can't be used with QtWebEngine. If you need a proxy, please select "
                           "the system proxy configuration or specify a proxy URL manually in the settings dialog. Do you want to "
                           "change proxy settings now?");
        KMessageBox::ButtonCode ans = KMessageBox::warningTwoActions(this, msg, i18n("Unsupported proxy configuration"), KGuiItem(i18n("Don't use a proxy")),
                                                                KGuiItem(i18n("Show proxy configuration dialog")), "WebEngineUnsupportedProxyType");
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
        if (ans == KMessageBox::SecondaryAction) {
            slotConfigure("proxy");
            return;
        }
    }
    QString httpProxy = KProtocolManager::proxyForUrl(QUrl("http://fakeurl.test.com"));
    QString httpsProxy = KProtocolManager::proxyForUrl(QUrl("https://fakeurl.test.com"));
    if (httpProxy == "DIRECT" && httpsProxy == "DIRECT") {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    } else {
        QUrl url(httpsProxy);
        if (httpProxy != httpsProxy) {
            QString msg =  i18n("Your proxy configuration can't be used with the QtWebEngine HTML engine because it doesn't support having different proxies for the HTTP and HTTPS protocols. Your current settings are:"
            "<p><b>HTTP proxy:</b> <tt>%1</tt></p><p><b>HTTPS proxy: </b><tt>%2</tt></p>"
            "What do you want to do?", httpProxy, httpsProxy);
            KMessageBox::ButtonCode ans = KMessageBox::questionTwoActionsCancel(this, msg, i18n("Conflicting proxy configuration"),
                KGuiItem(i18n("Use HTTP proxy (only this time)")), KGuiItem(i18n("Use HTTPS proxy (only this time)")), KGuiItem(i18n("Show proxy configuration dialog")), "WebEngineConflictingProxy");
            if (ans == KMessageBox::PrimaryAction) {
                url = QUrl(httpProxy);
            } else if (ans == KMessageBox::Cancel) {
                slotConfigure("proxy");
                return;
            }
        }
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, url.host(), url.port(), url.userName(), url.password()));
    }
}

void KonqMainWindow::toggleCompleteFullScreen(bool on)
{
    //Do nothing if already in complete full screen mode and on is true or not in complete full screen mode and on is false
    if (on == (m_fullScreenData.currentState == FullScreenState::CompleteFullScreen)) {
        return;
    }
    if (on) {
        slotForceSaveMainWindowSettings();
        resetAutoSaveSettings();

        //Hide the menu bar
        menuBar()->setVisible(false);

        //Hide the side bar
        QAction *a = m_toggleViewGUIClient->action(QStringLiteral("konq_sidebartng"));
        if (a) {
            KToggleAction *ta = static_cast<KToggleAction*>(a);
            if (ta){
                m_fullScreenData.wasSidebarVisible = ta->isChecked();
                a->setChecked(false);
            }
        }

        //Hide the tool bars
        const QList<QAction*> actions = toolBarMenuAction()->menu()->actions();
        for (QAction *a : actions) {
            a->setChecked(false);
        }
    } else {
        setAutoSaveSettings();
    }

    //Status bar and side bar are not managed by autoSaveSettings

    //Hide or show the sidebar
    QAction *a = m_toggleViewGUIClient->action(QStringLiteral("konq_sidebartng"));
    KToggleAction *sideBarAction = qobject_cast<KToggleAction*>(a);
    if (sideBarAction) {
        if (on) {
            m_fullScreenData.wasSidebarVisible = sideBarAction ->isChecked();
            sideBarAction ->setChecked(false);
        } else if (m_fullScreenData.wasSidebarVisible) {
            sideBarAction->setChecked(true);
        }
    }

    //Hide or show the status bar
    if (m_currentView) {
        QStatusBar *statusBar = m_currentView->frame()->statusbar();
        if (on) {
            m_fullScreenData.wasStatusBarVisible = statusBar->isVisible();
            statusBar->setVisible(false);
        } else if (m_fullScreenData.wasStatusBarVisible) {
            statusBar->setVisible(true);
        }
    }

    if (on || m_fullScreenData.previousState == FullScreenState::NoFullScreen) {
        disconnect(m_ptaFullScreen, &KToggleAction::toggled, this, &KonqMainWindow::slotUpdateFullScreen);
        KToggleFullScreenAction::setFullScreen(this, on);
        connect(m_ptaFullScreen, &KToggleAction::toggled, this, &KonqMainWindow::slotUpdateFullScreen);
    }
    m_pViewManager->forceHideTabBar(on);

    if (on) {
    QString msg = i18n("You have entered Complete Full Screen mode (the user interface is completely hidden)."
    " You can exit it by pressing the keyboard shortcut for Full Screen Mode (%1)", m_ptaFullScreen->shortcut().toString());
        KMessageBox::information(this, msg, QString(), "Complete Full Screen Warning");
    }

    m_fullScreenData.switchToState(on ? FullScreenState::CompleteFullScreen : m_fullScreenData.previousState);
}

void KonqMainWindow::FullScreenData::switchToState(KonqMainWindow::FullScreenState newState)
{
    if (newState != currentState) {
        previousState = currentState;
        currentState = newState;
    }
}

void KonqMainWindow::inspectCurrentPage()
{
    if (!m_currentView || !m_currentView->isWebEngineView()) {
        return;
    }
    KParts::ReadOnlyPart *partToInspect = m_currentView->part();
    KonqView *devToolsView = m_pViewManager->splitView(m_currentView, Qt::Vertical);
    if (devToolsView == nullptr) {
        return;
    }
    KonqOpenURLRequest req;
    req.forceAutoEmbed = true;

    openView("text/html", QUrl(), devToolsView, req);
    QMetaObject::invokeMethod(devToolsView->part(), "setInspectedPart", Qt::DirectConnection, Q_ARG(KParts::ReadOnlyPart*, partToInspect));
}

void KonqMainWindow::saveGlobalProperties(KConfig* sessionConfig)
{
    QList<int> preloadedNumbers;
    const QList<KMainWindow*> windows = KMainWindow::memberList();
    for (int i = 0; i < windows.length(); ++i) {
        KonqMainWindow *kw = qobject_cast<KonqMainWindow*>(windows.at(i));
        if (kw && kw->isPreloaded()) {
            preloadedNumbers << (i+1); //KMainWindow::restore numbers windows from 1
        }
    }

    KConfigGroup cg = sessionConfig->group(QStringLiteral("PreloadedWindows"));
    cg.writeEntry(QStringLiteral("PreloadedWindowsNumber"), preloadedNumbers);
    cg.sync();
}

void KonqMainWindow::readGlobalProperties(KConfig* sessionConfig)
{
    KConfigGroup cg = sessionConfig->group(QStringLiteral("PreloadedWindows"));
    QList<int> preloadedNumbers = cg.readEntry(QStringLiteral("PreloadedWindowsNumber"), QList<int>{});
    KonqSessionManager::self()->setPreloadedWindowsNumber(preloadedNumbers);
}
