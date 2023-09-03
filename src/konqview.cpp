/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1998-2005 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqview.h"

#include "KonqViewAdaptor.h"
#include "konqsettingsxt.h"
#include "konqframestatusbar.h"
#include "urlloader.h"
#include <konq_events.h>
#include "konqviewmanager.h"
#include "konqtabs.h"
#include "konqhistorymanager.h"
#include "konqpixmapprovider.h"
#include "implementations/konqbrowserwindowinterface.h"
#include "interfaces/downloaderextension.h"

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include "konqdebug.h"
#include <kcursor.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <ktoggleaction.h>
#include <kjobuidelegate.h>
#include <KUrlMimeData>

#include <QApplication>
#include <QArgument>
#include <QByteRef>
#include <QFile>
#include <QDropEvent>
#include <QDBusConnection>
#include <QMimeData>

#ifdef KActivities_FOUND
#include <KActivities/ResourceInstance>
#endif
#include <kparts_version.h>

#include <KJobWidgets>
#include <KParts/Part>
#include <KParts/StatusBarExtension>
#include <KParts/ReadOnlyPart>
#include <KParts/BrowserArguments>
#include <KParts/OpenUrlEvent>
#include <KParts/OpenUrlArguments>
#include <KParts/BrowserExtension>
#include <KParts/WindowArgs>
#include <QMimeDatabase>

//#define DEBUG_HISTORY

KonqView::KonqView(KonqViewFactory &viewFactory,
                   KonqFrame *viewFrame,
                   KonqMainWindow *mainWindow,
                   const KPluginMetaData &service,
                   const QVector<KPluginMetaData> &partServiceOffers,
                   const KService::List &appServiceOffers,
                   const QString &serviceType,
                   bool passiveMode
                  )
{
    m_pKonqFrame = viewFrame;
    m_pKonqFrame->setView(this);

    m_sLocationBarURL = QLatin1String("");
    m_pageSecurity = KonqMainWindow::NotCrypted;
    m_bLockHistory = false;
    m_doPost = false;
    m_pMainWindow = mainWindow;
    m_loader = nullptr;
    m_pPart = nullptr;

    m_service = service;
    m_partServiceOffers = partServiceOffers;
    m_appServiceOffers = appServiceOffers;
    m_serviceType = serviceType;

    m_lstHistoryIndex = -1;
    m_bLoading = false;
    m_bPendingRedirection = false;
    m_bPassiveMode = passiveMode;
    m_bLockedLocation = false;
    m_bLinkedView = false;
    m_bAborted = false;
    m_bToggleView = false;
    m_bDisableScrolling = false;
    m_bGotIconURL = false;
    m_bPopupMenuEnabled = true;
    m_bFollowActive = false;
    m_bBuiltinView = false;
    m_bURLDropHandling = false;
    m_bErrorURL = false;

#ifdef KActivities_FOUND
    m_activityResourceInstance = new KActivities::ResourceInstance(mainWindow->winId(), this);
#endif

    switchView(viewFactory);
}

KonqView::~KonqView()
{
    //qCDebug(KONQUEROR_LOG) << "part=" << m_pPart;

    // We did so ourselves for passive views
    if (m_pPart != nullptr) {
        finishedWithCurrentURL();
        if (isPassiveMode()) {
            disconnect(m_pPart, SIGNAL(destroyed()), m_pMainWindow->viewManager(), SLOT(slotObjectDestroyed()));
        }

        if (m_pPart->manager()) {
            m_pPart->manager()->removePart(m_pPart);    // ~Part does this, but we have to do it before (#213876, #207173)
        }

        delete m_pPart;
    }

    qDeleteAll(m_lstHistory);
    m_lstHistory.clear();

    setUrlLoader(nullptr);
    //qCDebug(KONQUEROR_LOG) << this << "done";
}

bool KonqView::isWebEngineView() const
{
    return m_service.pluginId() == QLatin1String("webenginepart");
}

void KonqView::openUrl(const QUrl &url, const QString &locationBarURL,
                       const QString &nameFilter, bool tempFile)
{
    qCDebug(KONQUEROR_LOG) << "url=" << url << "locationBarURL=" << locationBarURL;

    setPartMimeType();

    KParts::OpenUrlArguments args;
    if (m_pPart) {
        args = m_pPart->arguments();
    }

    KParts::BrowserExtension *ext = browserExtension();
    KParts::BrowserArguments browserArgs;
    if (ext) {
        browserArgs = ext->browserArguments();
    }

    // Typing "Enter" again after the URL of an aborted view, triggers a reload.
    if (m_bAborted && m_pPart && m_pPart->url() == url && !browserArgs.doPost()) {
        if (!prepareReload(args, browserArgs, false /* not softReload */)) {
            return;
        }
        m_pPart->setArguments(args);
    }

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "m_bLockedLocation=" << m_bLockedLocation << "browserArgs.lockHistory()=" << browserArgs.lockHistory();
#endif
    if (browserArgs.lockHistory()) {
        lockHistory();
    }

    if (!m_bLockHistory) {
        // Store this new URL in the history, removing any existing forward history.
        // We do this first so that everything is ready if a part calls completed().
        createHistoryEntry();
    } else {
        m_bLockHistory = false;
    }

    if (m_pPart) {
        m_pPart->setProperty("nameFilter", nameFilter);
    }

    if (m_bDisableScrolling) {
        callExtensionMethod("disableScrolling");
    }

    // Set location-bar URL, except for error urls, where we know the browser component
    // will set back the url with the error anyway.
    if (url.scheme() != QLatin1String("error")) {
        setLocationBarURL(locationBarURL);
    }

    setPageSecurity(KonqMainWindow::NotCrypted);

    if (!args.reload()) {
        // Save the POST data that is necessary to open this URL
        // (so that reload can re-post it)
        m_doPost = browserArgs.doPost();
        m_postContentType = browserArgs.contentType();
        m_postData = browserArgs.postData;
        // Save the referrer
        m_pageReferrer = args.metaData()[QStringLiteral("referrer")];
    }

    if (tempFile) {
        // Store the path to the tempfile. Yes, we could store a bool only,
        // but this would be more dangerous. If anything goes wrong in the code,
        // we might end up deleting a real file.
        if (url.isLocalFile()) {
            m_tempFile = url.toLocalFile();
        } else {
            qCWarning(KONQUEROR_LOG) << "Tempfile option is set, but URL is remote:" << url;
        }
    }

    aboutToOpenURL(url, args);

    if (args.metaData().contains("urlRequestedByApp") && isWebEngineView()) {
        m_pPart->setProperty("urlRequestedByApp", url);
    }
    m_pPart->openUrl(url);

    updateHistoryEntry(true);
    // add pending history entry
    KonqHistoryManager::kself()->addPending(url, locationBarURL, QString());

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "Current position:" << historyIndex();
#endif

#ifdef KActivities_FOUND
    m_activityResourceInstance->setUri(url);

    if (m_pPart->widget()->hasFocus()) {
        m_activityResourceInstance->notifyFocusedIn();
    }
#endif
}

void KonqView::switchEmbeddingPart(const QString& newPluginId, const QString& newInternalViewMode)
{
    if (service().pluginId() == newPluginId) {
        return;
    }
    //We don't want to delete the temporary file, otherwise the new part won't have a file to display
    stop(true);
    lockHistory();
    const QUrl origUrl = url();
    const QString locationBarURL = m_sLocationBarURL;
    bool tempFile = !m_tempFile.isEmpty();
    changePart(serviceType(), newPluginId);
    openUrl(origUrl, locationBarURL, {}, tempFile);
    if (!newInternalViewMode.isEmpty() && newInternalViewMode != internalViewMode()){
        setInternalViewMode(newInternalViewMode);
    }
}

void KonqView::switchView(KonqViewFactory &viewFactory)
{
    //qCDebug(KONQUEROR_LOG);
    KParts::ReadOnlyPart *oldPart = m_pPart;
    KParts::ReadOnlyPart *part = m_pKonqFrame->attach(viewFactory);   // creates the part
    if (!part) {
        return;
    }
    
    m_pPart = part;

    // Set the statusbar in the BE asap to avoid a KMainWindow statusbar being created.
    KParts::StatusBarExtension *sbext = statusBarExtension();
    if (sbext) {
        sbext->setStatusBar(frame()->statusbar());
    }

    // Activate the new part
    if (oldPart) {
        m_pPart->setObjectName(oldPart->objectName());
        emit sigPartChanged(this, oldPart, m_pPart);
        delete oldPart;
    }

    connectPart();

    if (m_service.value(QStringLiteral("X-KDE-BrowserView-FollowActive"), false)) {
        //qCDebug(KONQUEROR_LOG) << "X-KDE-BrowserView-FollowActive -> setFollowActive";
        setFollowActive(true);
    }

    m_bBuiltinView = m_service.value(QStringLiteral("X-KDE-BrowserView-Built-Into"), QString()) == QLatin1String("konqueror");

    if (!m_pMainWindow->viewManager()->isLoadingProfile()) {
        // Honor "non-removable passive mode" (like the dirtree)
        if (m_service.value(QStringLiteral("X-KDE-BrowserView-PassiveMode"), false)) {
            qCDebug(KONQUEROR_LOG) << "X-KDE-BrowserView-PassiveMode -> setPassiveMode";
            setPassiveMode(true);   // set as passive
        }

        // Honor "linked view"
        if (m_service.value(QStringLiteral("X-KDE-BrowserView-LinkedView"), false)) {
            setLinkedView(true);   // set as linked
            // Two views : link both
            if (m_pMainWindow->viewCount() <= 2) { // '1' can happen if this view is not yet in the map
                KonqView *otherView = m_pMainWindow->otherView(this);
                if (otherView) {
                    otherView->setLinkedView(true);
                }
            }
        }
    }
}

bool KonqView::ensureViewSupports(const QString &mimeType,
                                  bool forceAutoEmbed)
{
    if (supportsMimeType(mimeType)) {
        // could be more specific, let's store it so that OpenUrlArguments::mimeType is correct
        // testcase: http://acid3.acidtests.org/svg.xml should be opened as image/svg+xml
        m_serviceType = mimeType;
        return true;
    }
    return changePart(mimeType, QString(), forceAutoEmbed);
}

bool KonqView::changePart(const QString &mimeType,
                          const QString &serviceName,
                          bool forceAutoEmbed)
{
    // Caller should call stop first.
    Q_ASSERT(!m_bLoading);

    //qCDebug(KONQUEROR_LOG) << "mimeType=" << mimeType
    //             << "requested serviceName=" << serviceName
    //             << "current service name=" << m_service->desktopEntryName();

    if (serviceName == m_service.pluginId()) {
        m_serviceType = mimeType;
        return true;
    }

    if (isLockedViewMode()) {
        //qCDebug(KONQUEROR_LOG) << "This view's mode is locked - can't change";
        return false; // we can't do that if our view mode is locked
    }

    QVector<KPluginMetaData> partServiceOffers;
    KService::List appServiceOffers;
    KPluginMetaData service;
    KonqFactory konqFactory;
    KonqViewFactory viewFactory = konqFactory.createView(mimeType, serviceName, &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed);

    if (viewFactory.isNull()) {
        return false;
    }

    m_serviceType = mimeType;
    m_partServiceOffers = partServiceOffers;
    m_appServiceOffers = appServiceOffers;

    // Check if that's already the kind of part we have -> no need to recreate it
    // Note: we should have an operator== for KService...
    if (m_service.isValid() && m_service.pluginId() == service.pluginId()) {
        qCDebug(KONQUEROR_LOG) << "Reusing service. Service type set to" << m_serviceType;
        if (m_pMainWindow->currentView() == this) {
            m_pMainWindow->updateViewModeActions();
        }
    } else {
        m_service = service;

        switchView(viewFactory);
    }

    return true;
}

void KonqView::connectPart()
{
    //qCDebug(KONQUEROR_LOG);
    connect(m_pPart, SIGNAL(started(KIO::Job*)),
            this, SLOT(slotStarted(KIO::Job*)));
    connect(m_pPart, SIGNAL(completed()),
            this, SLOT(slotCompleted()));
    connect(m_pPart, &KParts::ReadOnlyPart::completedWithPendingAction, this, [this](){slotCompleted(true);});
    connect(m_pPart, SIGNAL(canceled(QString)),
            this, SLOT(slotCanceled(QString)));
    connect(m_pPart, SIGNAL(setWindowCaption(QString)),
            this, SLOT(setCaption(QString)));
    if (!internalViewMode().isEmpty()) {
        // Update checked action in "View Mode" menu when switching view mode in dolphin
        connect(m_pPart, SIGNAL(viewModeChanged()),
                m_pMainWindow, SLOT(slotInternalViewModeChanged()));
    }

    if (m_pPart) {
        KonqInterfaces::DownloaderExtension *dext = KonqInterfaces::DownloaderExtension::downloader(m_pPart);
        if (dext) {
            connect(dext, &KonqInterfaces::DownloaderExtension::downloadAndOpenUrl, m_pMainWindow, [dext, this](const QUrl &url, int id, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &bargs, bool temp){
                    KonqOpenURLRequest req(args, bargs, dext->part(), true, id);
                    req.tempFile = temp;
                    m_pMainWindow->slotOpenURLRequest(url, req);
                });
        }
    }

    KParts::BrowserExtension *ext = browserExtension();

    if (ext) {
        
        KonqBrowserWindowInterface *bi = new KonqBrowserWindowInterface(mainWindow(), m_pPart);
        ext->setBrowserInterface(bi);
        
        connect(ext, &KParts::BrowserExtension::openUrlRequestDelayed, ext,
                [ext, this](const QUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &bargs){
                    KonqOpenURLRequest req(args, bargs, qobject_cast<KParts::ReadOnlyPart*>(ext->parent()));
                    m_pMainWindow->slotOpenURLRequest(url, req);
                });

        if (m_bPopupMenuEnabled) {
            m_bPopupMenuEnabled = false; // force
            enablePopupMenu(true);
        }

        connect(ext, SIGNAL(setLocationBarUrl(QString)),
                this, SLOT(setLocationBarURL(QString)));

        connect(ext, SIGNAL(setIconUrl(QUrl)),
                this, SLOT(setIconURL(QUrl)));

        connect(ext, SIGNAL(setPageSecurity(int)),
                this, SLOT(setPageSecurity(int)));

        connect(ext, &KParts::BrowserExtension::createNewWindow, ext,
                [ext, this] (const QUrl &url, const KParts::OpenUrlArguments &args, const KParts::BrowserArguments &bargs, const KParts::WindowArgs &wargs ,KParts::ReadOnlyPart** part) {
                    KonqOpenURLRequest req(args, bargs, qobject_cast<KParts::ReadOnlyPart*>(ext->parent()));
                    m_pMainWindow->slotCreateNewWindow(url, req, wargs, part);
                });

        connect(ext, SIGNAL(loadingProgress(int)),
                m_pKonqFrame->statusbar(), SLOT(slotLoadingProgress(int)));

        connect(ext, SIGNAL(speedProgress(int)),
                m_pKonqFrame->statusbar(), SLOT(slotSpeedProgress(int)));

        connect(ext, SIGNAL(selectionInfo(KFileItemList)),
                this, SLOT(slotSelectionInfo(KFileItemList)));

        connect(ext, SIGNAL(mouseOverInfo(KFileItem)),
                this, SLOT(slotMouseOverInfo(KFileItem)));

        connect(ext, SIGNAL(openUrlNotify()),
                this, SLOT(slotOpenURLNotify()));

        connect(ext, SIGNAL(enableAction(const char*,bool)),
                this, SLOT(slotEnableAction(const char*,bool)));

        connect(ext, SIGNAL(setActionText(const char*,QString)),
                this, SLOT(slotSetActionText(const char*,QString)));

        connect(ext, SIGNAL(moveTopLevelWidget(int,int)),
                this, SLOT(slotMoveTopLevelWidget(int,int)));

        connect(ext, SIGNAL(resizeTopLevelWidget(int,int)),
                this, SLOT(slotResizeTopLevelWidget(int,int)));

        connect(ext, SIGNAL(requestFocus(KParts::ReadOnlyPart*)),
                this, SLOT(slotRequestFocus(KParts::ReadOnlyPart*)));

        if (service().pluginId() != QLatin1String("konq_sidebartng")) {
            connect(ext, &KParts::BrowserExtension::infoMessage, m_pKonqFrame->statusbar(), &KonqFrameStatusBar::message);

            //Must use this form of connect because slotAddWebSideBar is private
            connect(ext, SIGNAL(addWebSideBar(QUrl,QString)), m_pMainWindow, SLOT(slotAddWebSideBar(QUrl,QString)));
        }
    }

    QVariant urlDropHandling;

    if (ext) {
        urlDropHandling = ext->property("urlDropHandling");
    } else {
        urlDropHandling = QVariant(true);
    }

    // Handle url drops if
    //  a) either the property says "ok"
    //  or
    //  b) the part is a plain krop (no BE)
    m_bURLDropHandling = (urlDropHandling.type() == QVariant::Bool &&
                          urlDropHandling.toBool());
    if (m_bURLDropHandling) {
        m_pPart->widget()->setAcceptDrops(true);
    }

    m_pPart->widget()->installEventFilter(this);
}

void KonqView::slotEnableAction(const char *name, bool enabled)
{
    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->enableAction(name, enabled);
    }
    // Otherwise, we don't have to do anything, the state of the action is
    // stored inside the browser-extension.
}

void KonqView::slotSetActionText(const char *name, const QString &text)
{
    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->setActionText(name, text);
    }
    // Otherwise, we don't have to do anything, the state of the action is
    // stored inside the browser-extension.
}

void KonqView::slotMoveTopLevelWidget(int x, int y)
{
    KonqFrameContainerBase *container = frame()->parentContainer();
    // If tabs are shown, only accept to move the whole window if there's only one tab.
    if (container->frameType() != KonqFrameBase::Tabs || static_cast<KonqFrameTabs *>(container)->count() == 1) {
        m_pMainWindow->move(x, y);
    }
}

void KonqView::slotResizeTopLevelWidget(int w, int h)
{
    KonqFrameContainerBase *container = frame()->parentContainer();
    // If tabs are shown, only accept to resize the whole window if there's only one tab.
    // ### Maybe we could store the size requested by each tab and resize the window to the biggest one.
    if (container->frameType() != KonqFrameBase::Tabs || static_cast<KonqFrameTabs *>(container)->count() == 1) {
        m_pMainWindow->resize(w, h);
    }
}

void KonqView::slotStarted(KIO::Job *job)
{
    //qCDebug(KONQUEROR_LOG) << job;
    setLoading(true);

    if (job) {
        // Manage passwords properly...
        //qCDebug(KONQUEROR_LOG) << "Window ID =" << m_pMainWindow->window()->winId();
        KJobWidgets::setWindow(job, m_pMainWindow->window());

        connect(job, SIGNAL(percent(KJob*,ulong)), this, SLOT(slotPercent(KJob*,ulong)));
        connect(job, SIGNAL(speed(KJob*,ulong)), this, SLOT(slotSpeed(KJob*,ulong)));
        connect(job, SIGNAL(infoMessage(KJob*,QString,QString)), this, SLOT(slotInfoMessage(KJob*,QString)));
    }
}

void KonqView::slotRequestFocus(KParts::ReadOnlyPart *)
{
    m_pMainWindow->viewManager()->showTab(this);
}

void KonqView::setLoading(bool loading, bool hasPending /*= false*/)
{
    //qCDebug(KONQUEROR_LOG) << "loading=" << loading << "hasPending=" << hasPending;
    m_bLoading = loading;
    m_bPendingRedirection = hasPending;
    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->updateToolBarActions(hasPending);
        // Make sure the focus is restored on the part's widget and not the combo
        // box if it starts loading a request. See #304933.
        if (loading) {
            QWidget *partWidget = (m_pPart ? m_pPart->widget() : nullptr);
            if (partWidget && !partWidget->hasFocus()) {
                //qCDebug(KONQUEROR_LOG) << "SET FOCUS on the widget";
                partWidget->setFocus();
            }
        }
    }

    m_pMainWindow->viewManager()->setLoading(this, loading || hasPending);
}

void KonqView::slotPercent(KJob *, unsigned long percent)
{
    m_pKonqFrame->statusbar()->slotLoadingProgress(percent);
}

void KonqView::slotSpeed(KJob *, unsigned long bytesPerSecond)
{
    m_pKonqFrame->statusbar()->slotSpeedProgress(bytesPerSecond);
}

void KonqView::slotInfoMessage(KJob *, const QString &msg)
{
    m_pKonqFrame->statusbar()->message(msg);
}

void KonqView::slotCompleted()
{
    slotCompleted(false);
}

void KonqView::slotCompleted(bool hasPending)
{
    //qCDebug(KONQUEROR_LOG) << "hasPending=" << hasPending;
    m_pKonqFrame->statusbar()->slotLoadingProgress(-1);

    if (! m_bLockHistory) {
        // Success... update history entry
        updateHistoryEntry(false);

        if (m_bAborted) { // remove the pending entry on error
            KonqHistoryManager::kself()->removePending(url());
        } else if (currentHistoryEntry()) // register as proper history entry
            KonqHistoryManager::kself()->confirmPending(url(), typedUrl(),
                    currentHistoryEntry()->title);

        emit viewCompleted(this);
    }
    setLoading(false, hasPending);

    if (!m_bGotIconURL && !m_bAborted) {
        if (KonqSettings::enableFavicon() == true) {
            // Try to get /favicon.ico. KonqPixmapProvider::downloadHostIcon does nothing if the URL is not http(s)
            if (supportsMimeType(QStringLiteral("text/html"))) {
                KonqPixmapProvider::self()->downloadHostIcon(url());
            }
        }
    }
}

void KonqView::slotCanceled(const QString &errorMsg)
{
    //qCDebug(KONQUEROR_LOG);
    // The errorMsg comes from the ReadOnlyPart (usually from its kio job, but not necessarily).
    // It should probably be used in a KMessageBox
    // Let's use the statusbar for now
    m_pKonqFrame->statusbar()->setMessage(errorMsg, KonqStatusBarMessageLabel::Error);
    m_bAborted = true;
    slotCompleted();
}

void KonqView::slotSelectionInfo(const KFileItemList &items)
{
    m_selectedItems = items;
    KonqFileSelectionEvent ev(items, m_pPart);
    QApplication::sendEvent(m_pMainWindow, &ev);
}

void KonqView::slotMouseOverInfo(const KFileItem &item)
{
    KonqFileMouseOverEvent ev(item, m_pPart);
    QApplication::sendEvent(m_pMainWindow, &ev);
}

void KonqView::setLocationBarURL(const QUrl &locationBarURL)
{
    setLocationBarURL(locationBarURL.url(QUrl::PreferLocalFile));
}

void KonqView::setLocationBarURL(const QString &locationBarURL)
{
    //qCDebug(KONQUEROR_LOG) << locationBarURL << "this=" << this;
    m_sLocationBarURL = locationBarURL;
    if (m_pMainWindow->currentView() == this) {
        //qCDebug(KONQUEROR_LOG) << "is current view" << this;
        m_pMainWindow->setLocationBarURL(m_sLocationBarURL);
        m_pMainWindow->setPageSecurity(m_pageSecurity);
    }
    if (!m_bPassiveMode) {
        setTabIcon(QUrl::fromUserInput(m_sLocationBarURL));
    }
}

void KonqView::setIconURL(const QUrl &iconURL)
// This function sets the favIcon in konqui's window if enabled,
// thus it is responsible for the icon in the taskbar.
// It does not set the tab's favIcon.
{
    if (KonqSettings::enableFavicon()) {
        KonqPixmapProvider::self()->setIconForUrl(QUrl(m_sLocationBarURL), iconURL);
        m_bGotIconURL = true;
    }
}

void KonqView::setPageSecurity(int pageSecurity)
{
    m_pageSecurity = static_cast<KonqMainWindow::PageSecurity>(pageSecurity);

    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->setPageSecurity(m_pageSecurity);
    }
}

void KonqView::setTabIcon(const QUrl &url)
{
    if (!m_bPassiveMode && url.isValid()) {
        frame()->setTabIcon(url, nullptr);
    }
}

void KonqView::setCaption(const QString &caption)
{
    if (caption.isEmpty()) {
        return;
    }

    QString adjustedCaption = caption;
    // For local URLs we prefer to use only the directory name
    if (url().isLocalFile()) {
        // Is the caption a URL?  If so, is it local?  If so, only display the filename!
        const QUrl captionUrl(QUrl::fromUserInput(caption));
        if (captionUrl.isValid() && captionUrl.isLocalFile() && captionUrl.path() == url().path()) {
            adjustedCaption = captionUrl.adjusted(QUrl::StripTrailingSlash).fileName();
            if (adjustedCaption.isEmpty()) {
                adjustedCaption = QLatin1Char('/');
            }
        }
    }

    m_caption = adjustedCaption;
    if (!m_bPassiveMode) {
        frame()->setTitle(adjustedCaption, nullptr);
    }
}

void KonqView::slotOpenURLNotify()
{
#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG);
#endif
    updateHistoryEntry(false);
    createHistoryEntry();
    updateHistoryEntry(true);
    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->updateToolBarActions();
    }
}

void KonqView::createHistoryEntry()
{
    // First, remove any forward history
    HistoryEntry *current = currentHistoryEntry();
    if (current) {
#ifdef DEBUG_HISTORY
        qCDebug(KONQUEROR_LOG) << "Truncating history";
#endif
        while (current != m_lstHistory.last()) {
            delete m_lstHistory.takeLast();
        }
    }
    // Append a new entry
#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "Append a new entry";
#endif
    appendHistoryEntry(new HistoryEntry);
    setHistoryIndex(m_lstHistory.count() - 1); // made current
#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "at=" << historyIndex() << "count=" << m_lstHistory.count();
#endif
}

void KonqView::appendHistoryEntry(HistoryEntry *historyEntry)
{
    // If there are too many HistoryEntries remove old ones
    while (m_lstHistory.count() > 0 && m_lstHistory.count() >= KonqSettings::maximumHistoryEntriesPerView()) {
        delete m_lstHistory.takeFirst();
    }

    m_lstHistory.append(historyEntry);
}

void KonqView::updateHistoryEntry(bool needsReload)
{
    Q_ASSERT(!m_bLockHistory);   // should never happen

    HistoryEntry *current = currentHistoryEntry();
    if (!current) {
        return;
    }

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "Saving part URL:" << m_pPart->url() << "in history position" << historyIndex();
#endif

    current->reload = needsReload; // We have a state for it now.
    if (!needsReload && browserExtension()) {
        current->buffer = QByteArray(); // Start with empty buffer.
        QDataStream stream(&current->buffer, QIODevice::WriteOnly);

        browserExtension()->saveState(stream);
    }

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "Saving part URL:" << m_pPart->url() << "in history position" << historyIndex();
#endif
    current->url = m_pPart->url();

    if (!needsReload) {
#ifdef DEBUG_HISTORY
        qCDebug(KONQUEROR_LOG) << "Saving location bar URL:" << m_sLocationBarURL << "in history position" << historyIndex();
#endif
        current->locationBarURL = m_sLocationBarURL;
        current->pageSecurity = m_pageSecurity;
    }
#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "Saving title:" << m_caption << "in history position" << historyIndex();
#endif
    current->title = m_caption;
    current->strServiceType = m_serviceType;
    current->strServiceName = m_service.pluginId();

    current->doPost = m_doPost;
    current->postData = m_doPost ? m_postData : QByteArray();
    current->postContentType = m_doPost ? m_postContentType : QString();
    current->pageReferrer = m_pageReferrer;
}

void KonqView::go(int steps)
{
    if (!steps) { // [WildFox] i bet there are sites on the net with stupid devs who do that :)
#ifdef DEBUG_HISTORY
        qCDebug(KONQUEROR_LOG) << "go(0) -> reload";
#endif
        // [David] and you're right. And they expect that it reloads, apparently.
        // [George] I'm going to make nspluginviewer rely on this too. :-)
        m_pMainWindow->slotReload();
        return;
    }

    int newPos = historyIndex() + steps;
#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "steps=" << steps
             << "newPos=" << newPos
             << "m_lstHistory.count()=" << m_lstHistory.count();
#endif
    if (newPos < 0 || newPos >= m_lstHistory.count()) {
        return;
    }

    stop();

    setHistoryIndex(newPos);   // sets current item

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "New position" << historyIndex();
#endif

    restoreHistory();
}

void KonqView::restoreHistory()
{
    HistoryEntry h(*currentHistoryEntry());   // make a copy of the current history entry, as the data
    // the pointer points to will change with the following calls

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "Restoring servicetype/name, and location bar URL from history:" << h.locationBarURL;
#endif
    setLocationBarURL(h.locationBarURL);
    setPageSecurity(h.pageSecurity);
    m_sTypedURL.clear();

    if (!changePart(h.strServiceType, h.strServiceName)) {
        qCWarning(KONQUEROR_LOG) << "Couldn't change view mode to" << h.strServiceType << h.strServiceName;
        return /*false*/;
    }

    setPartMimeType();

    aboutToOpenURL(h.url);

    if (h.reload == false && browserExtension() && historyIndex() > 0) {
        //qCDebug(KONQUEROR_LOG) << "Restoring view from stream";
        QDataStream stream(h.buffer);

        browserExtension()->restoreState(stream);

        m_doPost = h.doPost;
        m_postContentType = h.postContentType;
        m_postData = h.postData;
        m_pageReferrer = h.pageReferrer;
    } else {
        m_pPart->openUrl(h.url);
    }

    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->updateToolBarActions();
    }

#ifdef DEBUG_HISTORY
    qCDebug(KONQUEROR_LOG) << "New position (2)" << historyIndex();
#endif
}

const HistoryEntry *KonqView::historyAt(int pos)
{
    return m_lstHistory.value(pos);
}

void KonqView::copyHistory(KonqView *other)
{
    if (!other) {
        return;
    }

    qDeleteAll(m_lstHistory);
    m_lstHistory.clear();

    foreach (HistoryEntry *he, other->m_lstHistory) {
        appendHistoryEntry(new HistoryEntry(*he));
    }
    setHistoryIndex(other->historyIndex());
}

QUrl KonqView::url() const
{
    Q_ASSERT(m_pPart);
    return m_pPart->url();
}

QUrl KonqView::upUrl() const
{
    QUrl currentURL;
    if (m_loader) {
        currentURL = m_loader->url();
    } else {
        currentURL = QUrl::fromUserInput(m_sLocationBarURL);
    }
    return KIO::upUrl(currentURL);
}

void KonqView::setUrlLoader(UrlLoader *run)
{
    if (m_loader) {
        // Tell the UrlLoader to abort, but don't delete it ourselves.
        // It could be showing a message box right now. It will delete itself anyway.
        m_loader->abort();
        // finish() will be emitted later (when back to event loop)
        // and we don't want it to call slotRunFinished (which stops the animation and stop button).
        m_loader->disconnect(m_pMainWindow);
        if (!run) {
            frame()->unsetCursor();
        }
    } else if (run) {
        frame()->setCursor(Qt::BusyCursor);
    }
    m_loader = run;
}

void KonqView::stop(bool keepTemporaryFile)
{
    //qCDebug(KONQUEROR_LOG);
    m_bAborted = false;

    //Don't remove the temporary file if we're just changing the part used to display it
    if (!keepTemporaryFile) {
        finishedWithCurrentURL();
    }
    if (m_bLoading || m_bPendingRedirection) {
        // aborted -> confirm the pending url. We might as well remove it, but
        // we decided to keep it :)
        KonqHistoryManager::kself()->confirmPending(url(), m_sTypedURL);

        //qCDebug(KONQUEROR_LOG) << "m_pPart->closeUrl()";
        m_pPart->closeUrl();
        m_bAborted = true;
        m_pKonqFrame->statusbar()->slotLoadingProgress(-1);
        setLoading(false, false);
    }
    if (m_loader) {
        // Revert to working URL - unless the URL was typed manually
        // This is duplicated with KonqMainWindow::slotRunFinished, but we can't call it
        //   since it relies on sender()...
        if (currentHistoryEntry() && m_loader->request().typedUrl.isEmpty()) {   // not typed
            setLocationBarURL(currentHistoryEntry()->locationBarURL);
            setPageSecurity(currentHistoryEntry()->pageSecurity);
        }

        setUrlLoader(nullptr);
        m_pKonqFrame->statusbar()->slotLoadingProgress(-1);
    }
    if (!m_bLockHistory && m_lstHistory.count() > 0) {
        updateHistoryEntry(false);
    }
}

void KonqView::finishedWithCurrentURL()
{
    if (!m_tempFile.isEmpty()) {
        qCDebug(KONQUEROR_LOG) << "######### Deleting tempfile after use:" << m_tempFile;
        QFile::remove(m_tempFile);
        m_tempFile.clear();
    }
}

void KonqView::setPassiveMode(bool mode)
{
    // In theory, if m_bPassiveMode is true and mode is false,
    // the part should be removed from the part manager,
    // and if the other way round, it should be readded to the part manager...
    m_bPassiveMode = mode;

    if (mode && m_pMainWindow->viewCount() > 1 && m_pMainWindow->currentView() == this) {
        KParts::Part *part = m_pMainWindow->viewManager()->chooseNextView(this)->part();    // switch active part
        m_pMainWindow->viewManager()->setActivePart(part);
    }

    // Update statusbar stuff
    m_pMainWindow->viewManager()->viewCountChanged();
}

void KonqView::setLinkedView(bool mode)
{
    m_bLinkedView = mode;
    if (m_pMainWindow->currentView() == this) {
        m_pMainWindow->linkViewAction()->setChecked(mode);
    }
    frame()->statusbar()->setLinkedView(mode);
}

void KonqView::setLockedLocation(bool b)
{
    m_bLockedLocation = b;
}

void KonqView::aboutToOpenURL(const QUrl &url, const KParts::OpenUrlArguments &args)
{
    m_bErrorURL = url.scheme() == QLatin1String("error");

    KParts::OpenUrlEvent ev(m_pPart, url, args);
    QApplication::sendEvent(m_pMainWindow, &ev);

    m_bGotIconURL = false;
    m_bAborted = false;
}

void KonqView::setPartMimeType()
{
    KParts::OpenUrlArguments args(m_pPart->arguments());
    args.setMimeType(m_serviceType);
    m_pPart->setArguments(args);
}

bool KonqView::callExtensionMethod(const char *methodName)
{
    QObject *obj = KParts::BrowserExtension::childObject(m_pPart);
    if (!obj) { // not all views have a browser extension !
        return false;
    }

    return QMetaObject::invokeMethod(obj, methodName,  Qt::DirectConnection);
}

bool KonqView::callExtensionBoolMethod(const char *methodName, bool value)
{
    QObject *obj = KParts::BrowserExtension::childObject(m_pPart);
    if (!obj) { // not all views have a browser extension !
        return false;
    }

    return QMetaObject::invokeMethod(obj, methodName,  Qt::DirectConnection, Q_ARG(bool, value));
}

bool KonqView::callExtensionURLMethod(const char *methodName, const QUrl &value)
{
    QObject *obj = KParts::BrowserExtension::childObject(m_pPart);
    if (!obj) { // not all views have a browser extension !
        return false;
    }

    return QMetaObject::invokeMethod(obj, methodName,  Qt::DirectConnection, Q_ARG(QUrl, value));
}

void KonqView::setViewName(const QString &name)
{
    //qCDebug(KONQUEROR_LOG) << this << "name=" << name;
    if (m_pPart) {
        m_pPart->setObjectName(name);
    }
}

QString KonqView::viewName() const
{
    return m_pPart ? m_pPart->objectName() : QString();
}

void KonqView::enablePopupMenu(bool b)
{
    Q_ASSERT(m_pMainWindow);

    KParts::BrowserExtension *ext = browserExtension();

    if (!ext) {
        return;
    }

    if (m_bPopupMenuEnabled == b) {
        return;
    }

    // enable context popup
    if (b) {
        m_bPopupMenuEnabled = true;

        connect(ext, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
                m_pMainWindow, SLOT(slotPopupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));

        connect(ext, SIGNAL(popupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
                m_pMainWindow, SLOT(slotPopupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
    } else { // disable context popup
        m_bPopupMenuEnabled = false;

        disconnect(ext, SIGNAL(popupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
                   m_pMainWindow, SLOT(slotPopupMenu(QPoint,KFileItemList,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));

        disconnect(ext, SIGNAL(popupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)),
                   m_pMainWindow, SLOT(slotPopupMenu(QPoint,QUrl,mode_t,KParts::OpenUrlArguments,KParts::BrowserArguments,KParts::BrowserExtension::PopupFlags,KParts::BrowserExtension::ActionGroupMap)));
    }
}

void KonqView::reparseConfiguration()
{
    callExtensionMethod("reparseConfiguration");
}

void KonqView::disableScrolling()
{
    m_bDisableScrolling = true;
    callExtensionMethod("disableScrolling");
}

QString KonqView::dbusObjectPath()
{
    // TODO maybe this can be improved?
    // E.g. using the part's name, but we'd have to update the name in setViewName maybe?
    // And to make sure it's a valid dbus object path like in kmainwindow...
    static int s_viewNumber = 0;
    if (m_dbusObjectPath.isEmpty()) {
        m_dbusObjectPath = m_pMainWindow->dbusName() + '/' + QString::number(++s_viewNumber);
        new KonqViewAdaptor(this);
        QDBusConnection::sessionBus().registerObject(m_dbusObjectPath, this);
    }
    return m_dbusObjectPath;
}

QString KonqView::partObjectPath()
{
    if (!m_pPart) {
        return QString();
    }

    const QVariant dcopProperty = m_pPart->property("dbusObjectPath");
    return dcopProperty.toString();
}

bool KonqView::eventFilter(QObject *obj, QEvent *e)
{
    if (!m_pPart) {
        return false;
    }
//  qCDebug(KONQUEROR_LOG) << "--" << obj->className() << "--" << e->type() << "--" ;
    if (e->type() == QEvent::DragEnter && m_bURLDropHandling && obj == m_pPart->widget()) {
        QDragEnterEvent *ev = static_cast<QDragEnterEvent *>(e);
        const QMimeData *mimeData = ev->mimeData();
        if (mimeData->hasUrls()) {
            QList<QUrl> lstDragURLs = KUrlMimeData::urlsFromMimeData(mimeData);

            const QList<QObject *> children = m_pPart->widget()->findChildren<QObject *>();   // ### slow, better write a isChildOf with a loop...

            if (!lstDragURLs.isEmpty()
                    && !lstDragURLs.first().url().startsWith(QLatin1String("javascript:"), Qt::CaseInsensitive) &&   // ### this looks like a hack to me
                    ev->source() != m_pPart->widget() &&
                    !children.contains(ev->source())) {
                ev->acceptProposedAction();
            }
        }
    } else if (e->type() == QEvent::Drop && m_bURLDropHandling && obj == m_pPart->widget()) {
        QDropEvent *ev = static_cast<QDropEvent *>(e);
        const QMimeData *mimeData = ev->mimeData();

        QList<QUrl> lstDragURLs = KUrlMimeData::urlsFromMimeData(mimeData);
        KParts::BrowserExtension *ext = browserExtension();
        if (!lstDragURLs.isEmpty() && ext && lstDragURLs.first().isValid()) {
            emit ext->openUrlRequest(lstDragURLs.first());    // this will call m_pMainWindow::slotOpenURLRequest delayed
        }
    }

#ifdef KActivities_FOUND
    if (e->type() == QEvent::FocusIn) {
        m_activityResourceInstance->notifyFocusedIn();
    }

    if (e->type() == QEvent::FocusOut) {
        m_activityResourceInstance->notifyFocusedOut();
    }
#endif

    return false;
}

bool KonqView::prepareReload(KParts::OpenUrlArguments &args, KParts::BrowserArguments &browserArgs, bool softReload)
{
    args.setReload(true);
    if (softReload) {
        browserArgs.softReload = true;
    }

    // Repost form data if this URL is the result of a POST HTML form.
    if (m_doPost && !browserArgs.redirectedRequest()) {
        if (KMessageBox::warningContinueCancel(nullptr, i18n(
                "The page you are trying to view is the result of posted form data. "
                "If you resend the data, any action the form carried out (such as search or online purchase) will be repeated. "),
                                               i18nc("@title:window", "Warning"), KGuiItem(i18n("Resend"))) == KMessageBox::Continue) {
            browserArgs.setDoPost(true);
            browserArgs.setContentType(m_postContentType);
            browserArgs.postData = m_postData;
        } else {
            return false;
        }
    }
    // Re-set referrer
    args.metaData()[QStringLiteral("referrer")] = m_pageReferrer;

    return true;
}

KParts::BrowserExtension *KonqView::browserExtension() const
{
    return m_pPart ? KParts::BrowserExtension::childObject(m_pPart) : nullptr;
}

KParts::StatusBarExtension *KonqView::statusBarExtension() const
{
    return KParts::StatusBarExtension::childObject(m_pPart);
}

QMimeType KonqView::mimeType() const
{
    QMimeDatabase db;
    return db.mimeTypeForName(serviceType());
}

bool KonqView::supportsMimeType(const QString &mimeType) const
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    if (!mime.isValid()) {
        return false;
    }
    const QStringList lst = m_service.mimeTypes();
    QStringList::const_iterator found = std::find_if(lst.constBegin(), lst.constEnd(), [mime](const QString &name){return mime.inherits(name);});
    return found != lst.constEnd();
}

bool KonqView::isWebBrowsingPart() const
{
    if (!m_pPart) {
        return false;
    }
    const QString partName = m_pPart->componentName();
    return partName == QLatin1String("webenginepart") || partName == QLatin1String("khtml") || partName == QLatin1String("kwebkitpart");
}


void HistoryEntry::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options)
{
    if (options & KonqFrameBase::SaveUrls) {
        config.writeEntry(QStringLiteral("Url").prepend(prefix), url.url());
        config.writeEntry(QStringLiteral("LocationBarURL").prepend(prefix), locationBarURL);
        config.writeEntry(QStringLiteral("Title").prepend(prefix), title);
        config.writeEntry(QStringLiteral("StrServiceType").prepend(prefix), strServiceType);
        config.writeEntry(QStringLiteral("StrServiceName").prepend(prefix), strServiceName);
    } else if (options & KonqFrameBase::SaveHistoryItems) {
        config.writeEntry(QStringLiteral("Url").prepend(prefix), url.url());
        config.writeEntry(QStringLiteral("LocationBarURL").prepend(prefix), locationBarURL);
        config.writeEntry(QStringLiteral("Title").prepend(prefix), title);
        config.writeEntry(QStringLiteral("Buffer").prepend(prefix), buffer);
        config.writeEntry(QStringLiteral("StrServiceType").prepend(prefix), strServiceType);
        config.writeEntry(QStringLiteral("StrServiceName").prepend(prefix), strServiceName);
        config.writeEntry(QStringLiteral("PostData").prepend(prefix), postData);
        config.writeEntry(QStringLiteral("PostContentType").prepend(prefix), postContentType);
        config.writeEntry(QStringLiteral("DoPost").prepend(prefix), doPost);
        config.writeEntry(QStringLiteral("PageReferrer").prepend(prefix), pageReferrer);
        config.writeEntry(QStringLiteral("PageSecurity").prepend(prefix), static_cast<int>(pageSecurity));
    }
}

void HistoryEntry::loadItem(const KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options)
{
    if (options & (KonqFrameBase::SaveUrls|KonqFrameBase::SaveHistoryItems)) { // either one
        url = QUrl(config.readEntry(QStringLiteral("Url").prepend(prefix), ""));
        locationBarURL = config.readEntry(QStringLiteral("LocationBarURL").prepend(prefix), "");
        title = config.readEntry(QStringLiteral("Title").prepend(prefix), "");
        strServiceType = config.readEntry(QStringLiteral("StrServiceType").prepend(prefix), "");
        strServiceName = config.readEntry(QStringLiteral("StrServiceName").prepend(prefix), "");
    }
    if (options & KonqFrameBase::SaveUrls) {
        reload = true;
    } else if (options & KonqFrameBase::SaveHistoryItems) {
        buffer = config.readEntry(QStringLiteral("Buffer").prepend(prefix), QByteArray());
        postData = config.readEntry(QStringLiteral("PostData").prepend(prefix), QByteArray());
        postContentType = config.readEntry(QStringLiteral("PostContentType").prepend(prefix), "");
        doPost = config.readEntry(QStringLiteral("DoPost").prepend(prefix), false);
        pageReferrer = config.readEntry(QStringLiteral("PageReferrer").prepend(prefix), "");
        pageSecurity = static_cast<KonqMainWindow::PageSecurity>(config.readEntry(
                           QStringLiteral("PageSecurity").prepend(prefix), 0));
        reload = false;
    }
}

void KonqView::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options)
{
    config.writeEntry(QStringLiteral("ServiceType").prepend(prefix), serviceType());
    config.writeEntry(QStringLiteral("ServiceName").prepend(prefix), service().pluginId());
    config.writeEntry(QStringLiteral("PassiveMode").prepend(prefix), isPassiveMode());
    config.writeEntry(QStringLiteral("LinkedView").prepend(prefix), isLinkedView());
    config.writeEntry(QStringLiteral("ToggleView").prepend(prefix), isToggleView());
    config.writeEntry(QStringLiteral("LockedLocation").prepend(prefix), isLockedLocation());

    if (options & KonqFrameBase::SaveUrls) {
        config.writePathEntry(QStringLiteral("URL").prepend(prefix), url().url());
    } else if (options & KonqFrameBase::SaveHistoryItems) {
        QList<HistoryEntry *>::Iterator it = m_lstHistory.begin();
        for (int i = 0; it != m_lstHistory.end(); ++it, ++i) {
            // In order to not end up with a huge config file, we only save full
            // history for current history item
            KonqFrameBase::Options options;
            if (i == m_lstHistoryIndex) {
                options = KonqFrameBase::SaveHistoryItems;
            } else {
                options = KonqFrameBase::SaveUrls;
            }

            (*it)->saveConfig(config, QLatin1String("HistoryItem")
                              + QString::number(i).prepend(prefix), options);
        }
        config.writeEntry(QStringLiteral("CurrentHistoryItem").prepend(prefix), m_lstHistoryIndex);
        config.writeEntry(QStringLiteral("NumberOfHistoryItems").prepend(prefix), historyLength());
    }
}

void KonqView::loadHistoryConfig(const KConfigGroup &config, const QString &prefix)
{
    // First, remove any history
    qDeleteAll(m_lstHistory);
    m_lstHistory.clear();

    int historySize = config.readEntry(QStringLiteral("NumberOfHistoryItems").prepend(prefix), 0);
    int currentIndex = config.readEntry(QStringLiteral("CurrentHistoryItem").prepend(prefix), historySize - 1);

    // No history to restore..
    if (historySize == 0) {
        createHistoryEntry();
        return;
    }

    // restore history list
    for (int i = 0; i < historySize; ++i) {
        HistoryEntry *historyEntry = new HistoryEntry;

        // Only current history item saves completely its HistoryEntry
        KonqFrameBase::Options options;
        if (i == currentIndex) {
            options = KonqFrameBase::SaveHistoryItems;
        } else {
            options = KonqFrameBase::SaveUrls;
        }

        historyEntry->loadItem(config, QLatin1String("HistoryItem") + QString::number(i).prepend(prefix), options);

        appendHistoryEntry(historyEntry);
    }

    // Shouldn't happen, but just in case..
    if (currentIndex >= historyLength()) {
        currentIndex = historyLength() - 1;
    }

    // set and load the correct history index
    setHistoryIndex(currentIndex);
    restoreHistory();
}

QString KonqView::internalViewMode() const
{
    const QVariant viewModeProperty = m_pPart->property("currentViewMode");
    return viewModeProperty.toString();
}

void KonqView::setInternalViewMode(const QString &viewMode)
{
    m_pPart->setProperty("currentViewMode", viewMode);
}

QString KonqView::nameFilter() const
{
    const QVariant nameFilterProperty = m_pPart->property("nameFilter");
    return nameFilterProperty.toString();
}

bool KonqView::showsDirectory() const
{
    return supportsMimeType(QStringLiteral("inode/directory"));
}

bool KonqView::isModified() const
{
    if (m_pPart && (m_pPart->metaObject()->indexOfProperty("modified") != -1)) {
        const QVariant prop = m_pPart->property("modified");
        return prop.isValid() && prop.toBool();
    }
    return false;
}

void KonqView::setFocus()
{
    if (m_pPart && m_pPart->widget() && !isErrorUrl()) {
        m_pPart->widget()->setFocus();
    }
}

bool KonqView::isErrorUrl() const
{
    return m_bErrorURL;
}

