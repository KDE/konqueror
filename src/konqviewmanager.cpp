/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqviewmanager.h"

#include "konqcloseditem.h"
#include "konqundomanager.h"
#include "konqmisc.h"
#include "konqview.h"
#include "konqframestatusbar.h"
#include "konqtabs.h"
#include "konqsettingsxt.h"
#include "konqframevisitor.h"
#include <konq_events.h>
#include "konqurl.h"

#include <QFileInfo>
#include <QDBusMessage>
#include <QDBusConnection>

#include <KParts/PartActivateEvent>

#include <kactionmenu.h>

#include <kstringhandler.h>
#include "konqdebug.h"
#include <QTemporaryFile>
#include <KLocalizedString>
#include <kmessagebox.h>
#include <QMenu>
#include <QApplication>
#include <QStandardPaths>
#include <KSharedConfig>
#include <KWindowConfig>
#include <QMimeDatabase>
#include <QMimeType>
#include <QScreen>

//#define DEBUG_VIEWMGR

KonqViewManager::KonqViewManager(KonqMainWindow *mainWindow)
    : KParts::PartManager(mainWindow)
{
    m_pMainWindow = mainWindow;

    m_bLoadingProfile = false;
    m_tabContainer = nullptr;

    setIgnoreExplictFocusRequests(true);

    connect(this, SIGNAL(activePartChanged(KParts::Part*)),
            this, SLOT(slotActivePartChanged(KParts::Part*)));
}

KonqView *KonqViewManager::createFirstView(const QString &mimeType, const QString &serviceName)
{
    //qCDebug(KONQUEROR_LOG) << serviceName;
    KPluginMetaData service;
    QVector<KPluginMetaData> partServiceOffers;
    KService::List appServiceOffers;
    KonqViewFactory newViewFactory = createView(mimeType, serviceName, service, partServiceOffers, appServiceOffers, true /*forceAutoEmbed*/);
    if (newViewFactory.isNull()) {
        qCDebug(KONQUEROR_LOG) << "No suitable factory found.";
        return nullptr;
    }

    KonqView *childView = setupView(tabContainer(), newViewFactory, service, partServiceOffers, appServiceOffers, mimeType, false);

    setActivePart(childView->part());

    m_tabContainer->asQWidget()->show();
    return childView;
}

KonqViewManager::~KonqViewManager()
{
    clear();
}

KonqView *KonqViewManager::splitView(KonqView *currentView,
                                     Qt::Orientation orientation,
                                     bool newOneFirst, bool forceAutoEmbed)
{
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG);
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    KonqFrame *splitFrame = currentView->frame();
    const QString serviceType = currentView->serviceType();

    KPluginMetaData service;
    QVector<KPluginMetaData> partServiceOffers;
    KService::List appServiceOffers;

    KonqViewFactory newViewFactory = createView(serviceType, currentView->service().pluginId(), service, partServiceOffers, appServiceOffers, forceAutoEmbed);

    if (newViewFactory.isNull()) {
        return nullptr;    //do not split at all if we can't create the new view
    }

    Q_ASSERT(splitFrame);

    KonqFrameContainerBase *parentContainer = splitFrame->parentContainer();

    // We need the sizes of the views in the parentContainer to restore these after the new container is inserted.
    // To access the sizes via QSplitter::sizes(), a pointer to a KonqFrameContainerBase is not sufficient.
    // We need a pointer to a KonqFrameContainer which is derived from QSplitter.
    KonqFrameContainer *parentKonqFrameContainer = dynamic_cast<KonqFrameContainer *>(parentContainer);
    QList<int> parentSplitterSizes;
    if (parentKonqFrameContainer) {
        parentSplitterSizes = parentKonqFrameContainer->sizes();
    }

    KonqFrameContainer *newContainer = parentContainer->splitChildFrame(splitFrame, orientation);

    //qCDebug(KONQUEROR_LOG) << "Create new child";
    KonqView *newView = setupView(newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, false);

#ifndef DEBUG
    //printSizeInfo( splitFrame, parentContainer, "after child insert" );
#endif

    newContainer->insertWidget(newOneFirst ? 0 : 1, newView->frame());
    if (newOneFirst) {
        newContainer->swapChildren();
    }

    Q_ASSERT(newContainer->count() == 2);

    int width = std::max(newContainer->widget(0)->minimumSizeHint().width(), newContainer->widget(1)->minimumSizeHint().width());
    newContainer->setSizes(QList<int>{width, width});

    splitFrame->show();
    newContainer->show();

    if (parentKonqFrameContainer) {
        parentKonqFrameContainer->setSizes(parentSplitterSizes);
    }

    Q_ASSERT(newView->frame());
    Q_ASSERT(newView->part());
    newContainer->setActiveChild(newView->frame());
    setActivePart(newView->part());

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
    qCDebug(KONQUEROR_LOG) << "done";
#endif

    return newView;
}

KonqView *KonqViewManager::splitMainContainer(KonqView *currentView,
        Qt::Orientation orientation,
        const QString &serviceType, // This can be Browser/View, not necessarily a mimetype
        const QString &serviceName,
        bool newOneFirst)
{
    //qCDebug(KONQUEROR_LOG);

    KPluginMetaData service;
    QVector<KPluginMetaData> partServiceOffers;
    KService::List appServiceOffers;

    KonqViewFactory newViewFactory = createView(serviceType, serviceName, service, partServiceOffers, appServiceOffers);

    if (newViewFactory.isNull()) {
        return nullptr;    //do not split at all if we can't create the new view
    }

    // Get main frame. Note: this is NOT necessarily m_tabContainer!
    // When having tabs plus a konsole, the main frame is a splitter (KonqFrameContainer).
    KonqFrameBase *mainFrame = m_pMainWindow->childFrame();

    KonqFrameContainer *newContainer = m_pMainWindow->splitChildFrame(mainFrame, orientation);

    KonqView *childView = setupView(newContainer, newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, true);

    newContainer->insertWidget(newOneFirst ? 0 : 1, childView->frame());
    if (newOneFirst) {
        newContainer->swapChildren();
    }

    newContainer->show();
    newContainer->setActiveChild(mainFrame);

    childView->openUrl(currentView->url(), currentView->locationBarURL());

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
    qCDebug(KONQUEROR_LOG) << "done";
#endif

    return childView;
}

KonqView *KonqViewManager::addTab(const QString &serviceType, const QString &serviceName, bool passiveMode, bool openAfterCurrentPage, int pos)
{
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << "------------- KonqViewManager::addTab starting -------------";
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    KPluginMetaData service;
    QVector<KPluginMetaData> partServiceOffers;
    KService::List appServiceOffers;

    Q_ASSERT(!serviceType.isEmpty());

    QString actualServiceName = serviceName;
    if (actualServiceName.isEmpty()) {
        // Use same part as the current view (e.g. khtml/webkit).
        // This is down here in this central method because it should work for
        // MMB-opens-tab, window.open (createNewWindow), and more.
        KonqView *currentView = m_pMainWindow->currentView();
        // Don't use supportsMimeType("text/html"), it's true for katepart too.
        // (Testcase: view text file, ctrl+shift+n, was showing about page in katepart)
        if (currentView) {
            QMimeType mime = currentView->mimeType();
            if (mime.isValid() && mime.inherits(serviceType)) {
                actualServiceName = currentView->service().pluginId();
            }
        }
    }

    KonqViewFactory newViewFactory = createView(serviceType, actualServiceName, service, partServiceOffers, appServiceOffers, true /*forceAutoEmbed*/);

    if (newViewFactory.isNull()) {
        return nullptr;    //do not split at all if we can't create the new view
    }

    KonqView *childView = setupView(tabContainer(), newViewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode, openAfterCurrentPage, pos);

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
    qCDebug(KONQUEROR_LOG) << "------------- KonqViewManager::addTab done -------------";
#endif

    return childView;
}

KonqView *KonqViewManager::addTabFromHistory(KonqView *currentView, int steps, bool openAfterCurrentPage)
{
    int oldPos = currentView->historyIndex();
    int newPos = oldPos + steps;

    const HistoryEntry *he = currentView->historyAt(newPos);
    if (!he) {
        return nullptr;
    }

    KonqView *newView = nullptr;
    newView  = addTab(he->strServiceType, he->strServiceName, false, openAfterCurrentPage);

    if (!newView) {
        return nullptr;
    }

    newView->copyHistory(currentView);
    newView->setHistoryIndex(newPos);
    newView->restoreHistory();

    return newView;
}

void KonqViewManager::duplicateTab(int tabIndex, bool openAfterCurrentPage)
{
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << tabIndex;
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    QTemporaryFile tempFile;
    tempFile.open();
    KConfig config(tempFile.fileName());
    KConfigGroup profileGroup(&config, "Profile");

    KonqFrameBase *tab = tabContainer()->tabAt(tabIndex);
    QString prefix = KonqFrameBase::frameTypeToString(tab->frameType()) + QString::number(0); // always T0
    profileGroup.writeEntry("RootItem", prefix);
    prefix.append(QLatin1Char('_'));
    KonqFrameBase::Options flags = KonqFrameBase::SaveHistoryItems;
    tab->saveConfig(profileGroup, prefix, flags, nullptr, 0, 1);

    loadRootItem(profileGroup, tabContainer(), QUrl(), true, QUrl(), QString(), openAfterCurrentPage);

    if (openAfterCurrentPage) {
        m_tabContainer->setCurrentIndex(m_tabContainer->currentIndex() + 1);
    } else {
        m_tabContainer->setCurrentIndex(m_tabContainer->count() - 1);
    }

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif
}

KonqMainWindow *KonqViewManager::breakOffTab(int tab, const QSize &windowSize)
{
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << "tab=" << tab;
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    QTemporaryFile tempFile;
    tempFile.open();
    KSharedConfigPtr config = KSharedConfig::openConfig(tempFile.fileName());
    KConfigGroup profileGroup(config, "Profile");

    KonqFrameBase *tabFrame = tabContainer()->tabAt(tab);
    QString prefix = KonqFrameBase::frameTypeToString(tabFrame->frameType()) + QString::number(0); // always T0
    profileGroup.writeEntry("RootItem", prefix);
    prefix.append(QLatin1Char('_'));
    KonqFrameBase::Options flags = KonqFrameBase::SaveHistoryItems;
    tabFrame->saveConfig(profileGroup, prefix, flags, nullptr, 0, 1);

    KonqMainWindow *mainWindow = new KonqMainWindow;

    KonqFrameTabs *newTabContainer = mainWindow->viewManager()->tabContainer();
    mainWindow->viewManager()->loadRootItem(profileGroup, newTabContainer, QUrl(), true, QUrl());

    removeTab(tabFrame, false);

    mainWindow->enableAllActions(true);
    mainWindow->resize(windowSize);
    mainWindow->activateChild();
    mainWindow->show();

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    return mainWindow;
}

void KonqViewManager::openClosedWindow(const KonqClosedWindowItem &closedWindowItem)
{
    openSavedWindow(closedWindowItem.configGroup())->show();
}

KonqMainWindow *KonqViewManager::openSavedWindow(const KConfigGroup &configGroup)
{
    // TODO factorize to avoid code duplication with loadViewProfileFromGroup
    KonqMainWindow *mainWindow = new KonqMainWindow;

    if (configGroup.readEntry("FullScreen", false)) {
        // Full screen on
        mainWindow->showFullScreen();
    } else {
        // Full screen off
        if (mainWindow->isFullScreen()) {
            mainWindow->showNormal();
        }
        // Window size comes from the applyMainWindowSettings call below
    }

    mainWindow->viewManager()->loadRootItem(configGroup, mainWindow, QUrl(), true, QUrl());
    mainWindow->applyMainWindowSettings(configGroup);
    mainWindow->activateChild();

#ifdef DEBUG_VIEWMGR
    mainWindow->viewManager()->printFullHierarchy();
#endif
    return mainWindow;
}

KonqMainWindow *KonqViewManager::openSavedWindow(const KConfigGroup &configGroup,
        bool openTabsInsideCurrentWindow)
{
    if (!openTabsInsideCurrentWindow) {
        return KonqViewManager::openSavedWindow(configGroup);
    } else {
        loadRootItem(configGroup, tabContainer(), QUrl(), true, QUrl());
#ifndef NDEBUG
        printFullHierarchy();
#endif
        return m_pMainWindow;
    }
}

void KonqViewManager::removeTab(KonqFrameBase *currentFrame, bool emitAboutToRemoveSignal)
{
    Q_ASSERT(currentFrame);
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << currentFrame;
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    if (m_tabContainer->count() == 1) {
        m_pMainWindow->slotAddTab();    // #214378
    }

    if (emitAboutToRemoveSignal) {
        emit aboutToRemoveTab(currentFrame);
    }

    if (currentFrame->asQWidget() == m_tabContainer->currentWidget()) {
        setActivePart(nullptr);
    }

    const QList<KonqView *> viewList = KonqViewCollector::collect(currentFrame);
    foreach (KonqView *view, viewList) {
        if (view == m_pMainWindow->currentView()) {
            setActivePart(nullptr);
        }
        m_pMainWindow->removeChildView(view);
        delete view;
    }

    m_tabContainer->childFrameRemoved(currentFrame);

    delete currentFrame;

    m_tabContainer->slotCurrentChanged(m_tabContainer->currentIndex());

    m_pMainWindow->viewCountChanged();

#ifdef DEBUG_VIEWMGR
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif
}

void KonqViewManager::reloadAllTabs()
{
    foreach (KonqFrameBase *frame, tabContainer()->childFrameList()) {
        if (frame && frame->activeChildView()) {
            if (!frame->activeChildView()->locationBarURL().isEmpty()) {
                frame->activeChildView()->openUrl(frame->activeChildView()->url(), frame->activeChildView()->locationBarURL());
            }
        }
    }
}

void KonqViewManager::removeOtherTabs(int tabIndex)
{
    QList<KonqFrameBase *> tabs = m_tabContainer->childFrameList();
    for (int i = 0; i < tabs.count(); ++i) {
        if (i != tabIndex) {
            removeTab(tabs.at(i));
        }
    }
}

void KonqViewManager::moveTabBackward()
{
    if (m_tabContainer->count() == 1) {
        return;
    }

    int iTab = m_tabContainer->currentIndex();
    m_tabContainer->moveTabBackward(iTab);
}

void KonqViewManager::moveTabForward()
{
    if (m_tabContainer->count() == 1) {
        return;
    }

    int iTab = m_tabContainer->currentIndex();
    m_tabContainer->moveTabForward(iTab);
}

void KonqViewManager::activateNextTab()
{
    if (m_tabContainer->count() == 1) {
        return;
    }

    int iTab = m_tabContainer->currentIndex();

    iTab++;

    if (iTab == m_tabContainer->count()) {
        iTab = 0;
    }

    m_tabContainer->setCurrentIndex(iTab);
}

void KonqViewManager::activatePrevTab()
{
    if (m_tabContainer->count() == 1) {
        return;
    }

    int iTab = m_tabContainer->currentIndex();

    iTab--;

    if (iTab == -1) {
        iTab = m_tabContainer->count() - 1;
    }

    m_tabContainer->setCurrentIndex(iTab);
}

int KonqViewManager::currentTabIndex() const
{
    return m_tabContainer->currentIndex();
}

int KonqViewManager::tabsCount() const
{
    return m_tabContainer->count();
}

void KonqViewManager::activateTab(int position)
{
    if (position < 0 || m_tabContainer->count() == 1 || position >= m_tabContainer->count()) {
        return;
    }

    m_tabContainer->setCurrentIndex(position);
}

void KonqViewManager::showTab(KonqView *view)
{
    if (m_tabContainer->currentWidget() != view->frame()) {
        m_tabContainer->setCurrentIndex(m_tabContainer->indexOf(view->frame()));
    }
}

void KonqViewManager::showTab(int tabIndex)
{
    if (m_tabContainer->currentIndex() != tabIndex) {
        m_tabContainer->setCurrentIndex(tabIndex);
    }
}

void KonqViewManager::updatePixmaps()
{
    const QList<KonqView *> viewList = KonqViewCollector::collect(tabContainer());
    foreach (KonqView *view, viewList) {
        view->setTabIcon(QUrl::fromUserInput(view->locationBarURL()));
    }
}

void KonqViewManager::openClosedTab(const KonqClosedTabItem &closedTab)
{
    qCDebug(KONQUEROR_LOG);
    loadRootItem(closedTab.configGroup(), m_tabContainer, QUrl(), true, QUrl(), QString(), false, closedTab.pos());

    int pos = (closedTab.pos() < m_tabContainer->count()) ? closedTab.pos() : m_tabContainer->count() - 1;
    qCDebug(KONQUEROR_LOG) << "pos, m_tabContainer->count():" << pos << m_tabContainer->count() - 1;

    m_tabContainer->setCurrentIndex(pos);
}

void KonqViewManager::removeView(KonqView *view)
{
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << view;
    m_pMainWindow->dumpViewList();
    printFullHierarchy();
#endif

    if (!view) {
        return;
    }

    KonqFrame *frame = view->frame();
    KonqFrameContainerBase *parentContainer = frame->parentContainer();

    qCDebug(KONQUEROR_LOG) << "view=" << view << "frame=" << frame << "parentContainer=" << parentContainer;

    if (parentContainer->frameType() == KonqFrameBase::Container) {
        setActivePart(nullptr);

        qCDebug(KONQUEROR_LOG) << "parentContainer is a KonqFrameContainer";

        KonqFrameContainerBase *grandParentContainer = parentContainer->parentContainer();
        qCDebug(KONQUEROR_LOG) << "grandParentContainer=" << grandParentContainer;

        KonqFrameBase *otherFrame = static_cast<KonqFrameContainer *>(parentContainer)->otherChild(frame);
        if (!otherFrame) {
            qCWarning(KONQUEROR_LOG) << "This shouldn't happen!";
            return;
        }

        static_cast<KonqFrameContainer *>(parentContainer)->setAboutToBeDeleted();

        // If the grand parent is a KonqFrameContainer, we need the sizes of the views inside it to restore these after
        // the parent is replaced. To access the sizes via QSplitter::sizes(), a pointer to a KonqFrameContainerBase
        //  is not sufficient. We need a pointer to a KonqFrameContainer which is derived from QSplitter.
        KonqFrameContainer *grandParentKonqFrameContainer = dynamic_cast<KonqFrameContainer *>(grandParentContainer);
        QList<int> grandParentSplitterSizes;
        if (grandParentKonqFrameContainer) {
            grandParentSplitterSizes = grandParentKonqFrameContainer->sizes();
        }

        m_pMainWindow->removeChildView(view);

        //qCDebug(KONQUEROR_LOG) << "--- Deleting view" << view;
        grandParentContainer->replaceChildFrame(parentContainer, otherFrame);

        //qCDebug(KONQUEROR_LOG) << "--- Removing otherFrame from parentContainer";
        parentContainer->childFrameRemoved(otherFrame);

        delete view; // This deletes the view, which deletes the part, which deletes its widget

        delete parentContainer;

        if (grandParentKonqFrameContainer) {
            grandParentKonqFrameContainer->setSizes(grandParentSplitterSizes);
        }

        grandParentContainer->setActiveChild(otherFrame);
        grandParentContainer->activateChild();
        m_pMainWindow->viewCountChanged();
    } else if (parentContainer->frameType() == KonqFrameBase::Tabs) {
        qCDebug(KONQUEROR_LOG) << "parentContainer" << parentContainer << "is a KonqFrameTabs";

        removeTab(frame);
    } else if (parentContainer->frameType() == KonqFrameBase::MainWindow) {
        qCDebug(KONQUEROR_LOG) << "parentContainer is a KonqMainWindow.  This shouldn't be removable, not removing.";
    } else {
        qCDebug(KONQUEROR_LOG) << "Unrecognized frame type, not removing.";
    }

#ifdef DEBUG_VIEWMGR
    printFullHierarchy();
    m_pMainWindow->dumpViewList();

    qCDebug(KONQUEROR_LOG) << "done";
#endif
}

// reimplemented from PartManager
void KonqViewManager::removePart(KParts::Part *part)
{
    //qCDebug(KONQUEROR_LOG) << part;
    // This is called when a part auto-deletes itself (case 1), or when
    // the "delete view" above deletes, in turn, the part (case 2)

    KParts::PartManager::removePart(part);

    // If we were called by PartManager::slotObjectDestroyed, then the inheritance has
    // been deleted already... Can't use inherits().

    KonqView *view = m_pMainWindow->childView(static_cast<KParts::ReadOnlyPart *>(part));
    if (view) { // the child view still exists, so we are in case 1
        qCDebug(KONQUEROR_LOG) << "Found a child view";

        // Make sure that deleting the frame won't delete the part's widget;
        // that's already taken care of by the part.
        view->part()->widget()->hide();
        view->part()->widget()->setParent(nullptr);

        view->partDeleted(); // tell the child view that the part auto-deletes itself

        if (m_pMainWindow->mainViewsCount() == 1) {
            qCDebug(KONQUEROR_LOG) << "Deleting last view -> closing the window";
            clear();
            qCDebug(KONQUEROR_LOG) << "Closing m_pMainWindow" << m_pMainWindow;
            m_pMainWindow->close(); // will delete it
            return;
        } else { // normal case
            removeView(view);
        }
    }

    //qCDebug(KONQUEROR_LOG) << part << "done";
}

void KonqViewManager::slotPassiveModePartDeleted()
{
    // Passive mode parts aren't registered to the part manager,
    // so we have to handle suicidal ones ourselves
    KParts::ReadOnlyPart *part = const_cast<KParts::ReadOnlyPart *>(static_cast<const KParts::ReadOnlyPart *>(sender()));
    disconnect(part, SIGNAL(destroyed()), this, SLOT(slotPassiveModePartDeleted()));
    qCDebug(KONQUEROR_LOG) << "part=" << part;
    KonqView *view = m_pMainWindow->childView(part);
    qCDebug(KONQUEROR_LOG) << "view=" << view;
    if (view != nullptr) { // the child view still exists, so the part suicided
        view->partDeleted(); // tell the child view that the part deleted itself
        removeView(view);
    }
}

void KonqViewManager::viewCountChanged()
{
    bool bShowActiveViewIndicator = (m_pMainWindow->viewCount() > 1);
    bool bShowLinkedViewIndicator = (m_pMainWindow->linkableViewsCount() > 1);

    const KonqMainWindow::MapViews mapViews = m_pMainWindow->viewMap();
    KonqMainWindow::MapViews::ConstIterator it = mapViews.begin();
    KonqMainWindow::MapViews::ConstIterator end = mapViews.end();
    for (; it != end; ++it) {
        KonqFrameStatusBar *sb = it.value()->frame()->statusbar();
        sb->showActiveViewIndicator(bShowActiveViewIndicator && !it.value()->isPassiveMode());
        sb->showLinkedViewIndicator(bShowLinkedViewIndicator && !it.value()->isFollowActive());
    }
}

void KonqViewManager::clear()
{
    //qCDebug(KONQUEROR_LOG);
    setActivePart(nullptr);

    if (m_pMainWindow->childFrame() == nullptr) {
        return;
    }

    const QList<KonqView *> viewList = KonqViewCollector::collect(m_pMainWindow);
    if (!viewList.isEmpty()) {
        //qCDebug(KONQUEROR_LOG) << viewList.count() << "items";

        foreach (KonqView *view, viewList) {
            m_pMainWindow->removeChildView(view);
            //qCDebug(KONQUEROR_LOG) << "Deleting" << view;
            delete view;
        }
    }

    KonqFrameBase *frame = m_pMainWindow->childFrame();
    Q_ASSERT(frame);
    //qCDebug(KONQUEROR_LOG) << "deleting mainFrame ";
    m_pMainWindow->childFrameRemoved(frame);   // will set childFrame() to NULL
    delete frame;
    // tab container was deleted by the above
    m_tabContainer = nullptr;
    m_pMainWindow->viewCountChanged();
}

KonqView *KonqViewManager::chooseNextView(KonqView *view)
{
    //qCDebug(KONQUEROR_LOG) << view;

    int it = 0;
    const QList<KonqView *> viewList = KonqViewCollector::collect(m_pMainWindow);
    if (viewList.isEmpty()) {
        return nullptr; // We have no view at all - this used to happen with totally-empty-profiles
    }

    if (view) { // find it in the list
        it = viewList.indexOf(view);
    }

    // the view should always be in the list
    if (it == -1) {
        qCWarning(KONQUEROR_LOG) << "View" << view << "is not in list!";
        it = 0;
    }

    bool rewinded = false;
    const int startIndex = it;
    const int end = viewList.count();

    //qCDebug(KONQUEROR_LOG) << "count=" << end;
    while (true) {
        //qCDebug(KONQUEROR_LOG) << "going next";
        if (++it == end) { // move to next
            // end reached: restart from begin (but only once)
            if (!rewinded) {
                it = 0;
                rewinded = true;
            } else {
                break; // nothing found, probably buggy profile
            }
        }

        if (it == startIndex && view) {
            break;    // no next view found
        }

        KonqView *nextView = viewList.at(it);;
        if (nextView && !nextView->isPassiveMode()) {
            return nextView;
        }
        //qCDebug(KONQUEROR_LOG) << "nextView=" << nextView << "passive=" << nextView->isPassiveMode();
    }

    //qCDebug(KONQUEROR_LOG) << "returning 0";
    return nullptr; // no next view found
}

KonqViewFactory KonqViewManager::createView(const QString &serviceType,
        const QString &serviceName,
        KPluginMetaData &service,
        QVector<KPluginMetaData> &partServiceOffers,
        KService::List &appServiceOffers,
        bool forceAutoEmbed)
{
    KonqViewFactory viewFactory;

    QString _serviceType(serviceType);
    QString _serviceName(serviceName);

    if (serviceType.isEmpty() && m_pMainWindow->currentView()) {
        //clone current view
        KonqView *cv = m_pMainWindow->currentView();
        if (cv->service().pluginId() == QLatin1String("konq_sidebartng")) {
            _serviceType = QStringLiteral("text/html");
            _serviceName.clear();
        } else {
            _serviceType = cv->serviceType();
            _serviceName = cv->service().pluginId();
        }
    }

    KonqFactory konqFactory;
    viewFactory = konqFactory.createView(_serviceType, _serviceName, &service, &partServiceOffers, &appServiceOffers, forceAutoEmbed);

    return viewFactory;
}

KonqView *KonqViewManager::setupView(KonqFrameContainerBase *parentContainer,
                                     KonqViewFactory &viewFactory,
                                     const KPluginMetaData &service,
                                     const QVector<KPluginMetaData> &partServiceOffers,
                                     const KService::List &appServiceOffers,
                                     const QString &serviceType,
                                     bool passiveMode,
                                     bool openAfterCurrentPage,
                                     int pos)
{
    //qCDebug(KONQUEROR_LOG) << "passiveMode=" << passiveMode;

    QString sType = serviceType;

    if (sType.isEmpty()) { // TODO remove this -- after checking all callers; splitMainContainer seems to need this logic
        sType = m_pMainWindow->currentView()->serviceType();
    }

    //qCDebug(KONQUEROR_LOG) << "creating KonqFrame with parent=" << parentContainer;
    KonqFrame *newViewFrame = new KonqFrame(parentContainer->asQWidget(), parentContainer);
    newViewFrame->setGeometry(0, 0, m_pMainWindow->width(), m_pMainWindow->height());

    //qCDebug(KONQUEROR_LOG) << "Creating KonqView";
    KonqView *v = new KonqView(viewFactory, newViewFrame,
                               m_pMainWindow, service, partServiceOffers, appServiceOffers, sType, passiveMode);
    //qCDebug(KONQUEROR_LOG) << "KonqView created - v=" << v << "v->part()=" << v->part();

    connect(v, &KonqView::sigPartChanged, m_pMainWindow, &KonqMainWindow::slotPartChanged);
//     QObject::connect(v, SIGNAL(sigPartChanged(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)),
//                      m_pMainWindow, SLOT(slotPartChanged(KonqView*,KParts::ReadOnlyPart*,KParts::ReadOnlyPart*)));

    m_pMainWindow->insertChildView(v);

    int index = -1;
    if (openAfterCurrentPage) {
        index = m_tabContainer->currentIndex() + 1;
    } else if (pos > -1) {
        index = pos;
    }

    parentContainer->insertChildFrame(newViewFrame, index);

    if (parentContainer->frameType() != KonqFrameBase::Tabs) {
        newViewFrame->show();
    }

    // Don't register passive views to the part manager
    if (!v->isPassiveMode()) { // note that KonqView's constructor could set this to true even if passiveMode is false
        addPart(v->part(), false);
    } else {
        // Passive views aren't registered, but we still want to detect the suicidal ones
        connect(v->part(), SIGNAL(destroyed()), this, SLOT(slotPassiveModePartDeleted()));
    }

    if (!m_bLoadingProfile) {
        m_pMainWindow->viewCountChanged();
    }

    //qCDebug(KONQUEROR_LOG) << "done";
    return v;
}


void KonqViewManager::saveViewConfigToGroup(KConfigGroup &profileGroup, KonqFrameBase::Options options)
{
    if (m_pMainWindow->childFrame()) {
        QString prefix = KonqFrameBase::frameTypeToString(m_pMainWindow->childFrame()->frameType())
                         + QString::number(0);
        profileGroup.writeEntry("RootItem", prefix);
        prefix.append(QLatin1Char('_'));
        m_pMainWindow->saveConfig(profileGroup, prefix, options, tabContainer(), 0, 1);
    }

    profileGroup.writeEntry("FullScreen", m_pMainWindow->fullScreenMode());

    m_pMainWindow->saveMainWindowSettings(profileGroup);
}

void KonqViewManager::loadViewConfigFromGroup(const KConfigGroup &profileGroup, const QString &filename,
        const QUrl &forcedUrl, const KonqOpenURLRequest &req,
        bool openUrl)
{
    Q_UNUSED(filename); // could be useful in case of error messages

    QUrl defaultURL;
    if (m_pMainWindow->currentView()) {
        defaultURL = m_pMainWindow->currentView()->url();
    }

    clear();

    if (!KonqUrl::isKonqBlank(forcedUrl)) {
        loadRootItem(profileGroup, m_pMainWindow, defaultURL, openUrl && forcedUrl.isEmpty(), forcedUrl, req.serviceName);
    } else {
        // ## in this case we won't resize the window, so bool resetWindow could be useful after all?
        m_pMainWindow->disableActionsNoView();
        m_pMainWindow->action("clear_location")->trigger();
    }

    //qCDebug(KONQUEROR_LOG) << "after loadRootItem";

    // Set an active part first so that we open the URL in the current view
    // (to set the location bar correctly and asap)
    KonqView *nextChildView = nullptr;
    nextChildView = m_pMainWindow->activeChildView();
    if (nextChildView == nullptr) {
        nextChildView = chooseNextView(nullptr);
    }
    setActivePart(nextChildView ? nextChildView->part() : nullptr);

    // #71164
    if (!req.browserArgs.frameName.isEmpty() && nextChildView) {
        nextChildView->setViewName(req.browserArgs.frameName);
    }

    if (openUrl && !forcedUrl.isEmpty()) {
        KonqOpenURLRequest _req(req);
        _req.openAfterCurrentPage = KonqSettings::openAfterCurrentPage();
        _req.forceAutoEmbed = true; // it's a new window, let's use it

        m_pMainWindow->openUrl(nextChildView /* can be 0 for an empty profile */,
                               forcedUrl, _req.args.mimeType(), _req, _req.browserArgs.trustedSource);

        // TODO choose a linked view if any (instead of just the first one),
        // then open the same URL in any non-linked one
    } else {
        if (forcedUrl.isEmpty() && m_pMainWindow->locationBarURL().isEmpty()) {
            // No URL -> the user will want to type one
            m_pMainWindow->focusLocationBar();
        }
    }

    // Window size
    if (profileGroup.readEntry("FullScreen", false)) {
        // Full screen on
        m_pMainWindow->setWindowState(m_pMainWindow->windowState() | Qt::WindowFullScreen);
    } else {
        // Full screen off
        m_pMainWindow->setWindowState(m_pMainWindow->windowState() & ~Qt::WindowFullScreen);
        applyWindowSize(profileGroup);
    }

    //qCDebug(KONQUEROR_LOG) << "done";
}

void KonqViewManager::setActivePart(KParts::Part *part, QWidget *)
{
    doSetActivePart(static_cast<KParts::ReadOnlyPart *>(part));
}

void KonqViewManager::doSetActivePart(KParts::ReadOnlyPart *part)
{
    if (part) {
        qCDebug(KONQUEROR_LOG) << part << part->url();
    }

    KParts::Part *mainWindowActivePart = m_pMainWindow->currentView()
                                         ? m_pMainWindow->currentView()->part() : nullptr;
    if (part == activePart() && mainWindowActivePart == part) {
        //qCDebug(KONQUEROR_LOG) << "Part is already active!";
        return;
    }

    // ## is this the right currentView() already?
    if (m_pMainWindow->currentView()) {
        m_pMainWindow->currentView()->setLocationBarURL(m_pMainWindow->locationBarURL());
    }

    KParts::PartManager::setActivePart(part);

    if (part && part->widget()) {
        part->widget()->setFocus();

        // However in case of an error URL we want to make it possible for the user to fix it
        KonqView *view = m_pMainWindow->viewMap().value(part);
        if (view && view->isErrorUrl()) {
            m_pMainWindow->focusLocationBar();
        }
    }

    emitActivePartChanged(); // This is what triggers KonqMainWindow::slotPartActivated
}

void KonqViewManager::slotActivePartChanged(KParts::Part *newPart)
{
    //qCDebug(KONQUEROR_LOG) << newPart;
    if (newPart == nullptr) {
        //qCDebug(KONQUEROR_LOG) << "newPart = 0L , returning";
        return;
    }
    // Send event to mainwindow - this is useful for plugins (like searchbar)
    KParts::PartActivateEvent ev(true, newPart, newPart->widget());
    QApplication::sendEvent(m_pMainWindow, &ev);
    
    KonqView *view = m_pMainWindow->childView(static_cast<KParts::ReadOnlyPart *>(newPart));
    if (view == nullptr) {
        qCDebug(KONQUEROR_LOG) << "No view associated with this part";
        return;
    }
    if (view->frame()->parentContainer() == nullptr) {
        return;
    }
    if (!m_bLoadingProfile)  {
        view->frame()->statusbar()->updateActiveStatus();
        view->frame()->parentContainer()->setActiveChild(view->frame());
    }
    //qCDebug(KONQUEROR_LOG) << "done";
}

void KonqViewManager::emitActivePartChanged()
{
    m_pMainWindow->slotPartActivated(activePart());
}

// Read default size from profile (e.g. Width=80%)
static QSize readDefaultSize(const KConfigGroup &cfg, QWidget *widget)
{
    QString widthStr = cfg.readEntry("Width");
    QString heightStr = cfg.readEntry("Height");
    int width = -1;
    int height = -1;
    const QRect geom = widget->screen()->geometry();

    bool ok;
    if (widthStr.endsWith('%')) {
        widthStr.truncate(widthStr.length() - 1);
        const int relativeWidth = widthStr.toInt(&ok);
        if (ok) {
            width = relativeWidth * geom.width() / 100;
        }
    } else {
        width = widthStr.toInt(&ok);
        if (!ok) {
            width = -1;
        }
    }

    if (heightStr.endsWith('%')) {
        heightStr.truncate(heightStr.length() - 1);
        int relativeHeight = heightStr.toInt(&ok);
        if (ok) {
            height = relativeHeight * geom.height() / 100;
        }
    } else {
        height = heightStr.toInt(&ok);
        if (!ok) {
            height = -1;
        }
    }

    return QSize(width, height);
}

void KonqViewManager::applyWindowSize(const KConfigGroup &profileGroup)
{
    const QSize size = readDefaultSize(profileGroup, m_pMainWindow); // example: "Width=80%"
    if (size.isValid()) {
        m_pMainWindow->resize(size);
    }
    KWindowConfig::restoreWindowSize(m_pMainWindow->windowHandle(), profileGroup); // example: "Width 1400=1120"
}

void KonqViewManager::loadRootItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                                   const QUrl &defaultURL, bool openUrl,
                                   const QUrl &forcedUrl, const QString &forcedService,
                                   bool openAfterCurrentPage,
                                   int pos)
{
    const QString rootItem = cfg.readEntry("RootItem", "empty");

    // This flag is used by KonqView, to distinguish manual view creation
    // from profile loading (e.g. in switchView)
    m_bLoadingProfile = true;

    loadItem(cfg, parent, rootItem, defaultURL, openUrl, forcedUrl, forcedService, openAfterCurrentPage, pos);

    m_bLoadingProfile = false;

    m_pMainWindow->enableAllActions(true);

    // This flag disables calls to viewCountChanged while creating the views,
    // so we do it once at the end:
    viewCountChanged();
}

void KonqViewManager::loadItem(const KConfigGroup &cfg, KonqFrameContainerBase *parent,
                               const QString &name, const QUrl &defaultURL, bool openUrl,
                               const QUrl &forcedUrl,
                               const QString &forcedService,
                               bool openAfterCurrentPage, int pos)
{
    QString prefix;
    if (name != QLatin1String("InitialView")) { // InitialView is old stuff, not in use anymore
        prefix = name + QLatin1Char('_');
    }

#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << "begin name=" << name << "openUrl=" << openUrl;
#endif

    if (name.startsWith(QLatin1String("View")) || name == QLatin1String("empty")) {
        // load view config

        QString serviceType;
        QString serviceName;
        if (name == QLatin1String("empty")) {
            // An empty profile is an empty KHTML part. Makes all KHTML actions available, avoids crashes,
            // makes it easy to DND a URL onto it, and makes it fast to load a website from there.
            serviceType = QStringLiteral("text/html");
            serviceName = forcedService; // coming e.g. from the cmdline, otherwise empty
        } else {
            serviceType = cfg.readEntry(QStringLiteral("ServiceType").prepend(prefix), QStringLiteral("inode/directory"));
            serviceName = cfg.readEntry(QStringLiteral("ServiceName").prepend(prefix), QString());
            if (serviceName == QLatin1String("konq_aboutpage")) {
                if ((!forcedUrl.isEmpty() && !KonqUrl::hasKonqScheme(forcedUrl)) ||
                        (forcedUrl.isEmpty() && openUrl == false)) { // e.g. window.open
                    // No point in loading the about page if we're going to replace it with a KHTML part right away
                    serviceType = QStringLiteral("text/html");
                    serviceName = forcedService; // coming e.g. from the cmdline, otherwise empty
                }
            }
        }
        //qCDebug(KONQUEROR_LOG) << "serviceType" << serviceType << serviceName;

        KPluginMetaData service;
        QVector<KPluginMetaData> partServiceOffers;
        KService::List appServiceOffers;

        KonqFactory konqFactory;
        KonqViewFactory viewFactory = konqFactory.createView(serviceType, serviceName, &service, &partServiceOffers, &appServiceOffers, true /*forceAutoEmbed*/);
        if (viewFactory.isNull()) {
            qCWarning(KONQUEROR_LOG) << "Profile Loading Error: View creation failed";
            return; //ugh..
        }

        bool passiveMode = cfg.readEntry(QStringLiteral("PassiveMode").prepend(prefix), false);

        //qCDebug(KONQUEROR_LOG) << "Creating View Stuff; parent=" << parent;
        if (parent == m_pMainWindow) {
            parent = tabContainer();
        }
        KonqView *childView = setupView(parent, viewFactory, service, partServiceOffers, appServiceOffers, serviceType, passiveMode, openAfterCurrentPage, pos);

        if (!childView->isFollowActive()) {
            childView->setLinkedView(cfg.readEntry(QStringLiteral("LinkedView").prepend(prefix), false));
        }
        const bool isToggleView = cfg.readEntry(QStringLiteral("ToggleView").prepend(prefix), false);
        childView->setToggleView(isToggleView);
        if (isToggleView /*100373*/ || !cfg.readEntry(QStringLiteral("ShowStatusBar").prepend(prefix), true)) {
            childView->frame()->statusbar()->hide();
        }

        if (parent == m_tabContainer && m_tabContainer->count() == 1) {
            // First tab, make it the active one
            parent->setActiveChild(childView->frame());
        }

        if (openUrl) {
            const QString keyHistoryItems = QStringLiteral("NumberOfHistoryItems").prepend(prefix);
            if (cfg.hasKey(keyHistoryItems)) {
                childView->loadHistoryConfig(cfg, prefix);
                m_pMainWindow->updateHistoryActions();
            } else {
                // determine URL
                const QString urlKey = QStringLiteral("URL").prepend(prefix);
                QUrl url;
                if (cfg.hasKey(urlKey)) {
                    url = QUrl(cfg.readPathEntry(urlKey, KonqUrl::string(KonqUrl::Type::Blank)));
                } else if (urlKey == QLatin1String("empty_URL")) { // old stuff, not in use anymore
                    url = KonqUrl::url(KonqUrl::Type::Blank);
                } else {
                    url = defaultURL;
                }

                if (!url.isEmpty()) {
                    //qCDebug(KONQUEROR_LOG) << "calling openUrl" << url;
                    //childView->openUrl( url, url.toDisplayString() );
                    // We need view-follows-view (for the dirtree, for instance)
                    KonqOpenURLRequest req;
                    if (!KonqUrl::hasKonqScheme(url)) {
                        req.typedUrl = url.toDisplayString();
                    }
                    m_pMainWindow->openView(serviceType, url, childView, req);
                }
                //else qCDebug(KONQUEROR_LOG) << "url is empty";
            }
        }
        // Do this after opening the URL, so that it's actually possible to open it :)
        childView->setLockedLocation(cfg.readEntry(QStringLiteral("LockedLocation").prepend(prefix), false));
    } else if (name.startsWith(QLatin1String("Container"))) {
        //qCDebug(KONQUEROR_LOG) << "Item is Container";

        //load container config
        QString ostr = cfg.readEntry(QStringLiteral("Orientation").prepend(prefix), QString());
        //qCDebug(KONQUEROR_LOG) << "Orientation:" << ostr;
        Qt::Orientation o;
        if (ostr == QLatin1String("Vertical")) {
            o = Qt::Vertical;
        } else if (ostr == QLatin1String("Horizontal")) {
            o = Qt::Horizontal;
        } else {
            qCWarning(KONQUEROR_LOG) << "Profile Loading Error: No orientation specified in" << name;
            o = Qt::Horizontal;
        }

        QList<int> sizes =
            cfg.readEntry(QStringLiteral("SplitterSizes").prepend(prefix), QList<int>());

        int index = cfg.readEntry(QStringLiteral("activeChildIndex").prepend(prefix), -1);

        QStringList childList = cfg.readEntry(QStringLiteral("Children").prepend(prefix), QStringList());
        if (childList.count() < 2) {
            qCWarning(KONQUEROR_LOG) << "Profile Loading Error: Less than two children in" << name;
            // fallback to defaults
            loadItem(cfg, parent, QStringLiteral("InitialView"), defaultURL, openUrl, forcedUrl, forcedService);
        } else {
            KonqFrameContainer *newContainer = new KonqFrameContainer(o, parent->asQWidget(), parent);

            int tabindex = pos;
            if (openAfterCurrentPage && parent->frameType() == KonqFrameBase::Tabs) { // Need to honor it, if possible
                tabindex = static_cast<KonqFrameTabs *>(parent)->currentIndex() + 1;
            }
            parent->insertChildFrame(newContainer, tabindex);

            loadItem(cfg, newContainer, childList.at(0), defaultURL, openUrl, forcedUrl, forcedService);
            loadItem(cfg, newContainer, childList.at(1), defaultURL, openUrl, forcedUrl, forcedService);

            //qCDebug(KONQUEROR_LOG) << "setSizes" << sizes;
            newContainer->setSizes(sizes);

            if (index == 1) {
                newContainer->setActiveChild(newContainer->secondChild());
            } else if (index == 0) {
                newContainer->setActiveChild(newContainer->firstChild());
            }

            newContainer->show();
        }
    } else if (name.startsWith(QLatin1String("Tabs"))) {
        //qCDebug(KONQUEROR_LOG) << "Item is a Tabs";

        int index = cfg.readEntry(QStringLiteral("activeChildIndex").prepend(prefix), 0);
        if (!m_tabContainer) {
            createTabContainer(parent->asQWidget(), parent);
            parent->insertChildFrame(m_tabContainer);
        }

        const QStringList childList = cfg.readEntry(QStringLiteral("Children").prepend(prefix), QStringList());
        for (QStringList::const_iterator it = childList.begin(); it != childList.end(); ++it) {
            loadItem(cfg, tabContainer(), *it, defaultURL, openUrl, forcedUrl, forcedService);
            QWidget *currentPage = m_tabContainer->currentWidget();
            if (currentPage != nullptr) {
                KonqView *activeChildView = dynamic_cast<KonqFrameBase *>(currentPage)->activeChildView();
                if (activeChildView != nullptr) {
                    activeChildView->setCaption(activeChildView->caption());
                    activeChildView->setTabIcon(activeChildView->url());
                }
            }
        }

        QWidget *w = m_tabContainer->widget(index);
        if (w) {
            m_tabContainer->setActiveChild(dynamic_cast<KonqFrameBase *>(w));
            m_tabContainer->setCurrentIndex(index);
            m_tabContainer->show();
        } else {
            qCWarning(KONQUEROR_LOG) << "Profile Loading Error: Unknown current item index" << index;
        }

    } else {
        qCWarning(KONQUEROR_LOG) << "Profile Loading Error: Unknown item" << name;
    }

    //qCDebug(KONQUEROR_LOG) << "end" << name;
}

void KonqViewManager::setLoading(KonqView *view, bool loading)
{
    tabContainer()->setLoading(view->frame(), loading);
}

///////////////// Debug stuff ////////////////

#ifndef NDEBUG
void KonqViewManager::printSizeInfo(KonqFrameBase *frame,
                                    KonqFrameContainerBase *parent,
                                    const char *msg)
{
    const QRect r = frame->asQWidget()->geometry();
    qCDebug(KONQUEROR_LOG) << "Child size" << msg << r;
    if (parent->frameType() == KonqFrameBase::Container) {
        const QList<int> sizes = static_cast<KonqFrameContainer *>(parent)->sizes();
        printf("Parent sizes %s :", msg);
        foreach (int i, sizes) {
            printf(" %d", i);
        }
        printf("\n");
    }
}

class KonqDebugFrameVisitor : public KonqFrameVisitor
{
public:
    KonqDebugFrameVisitor() {}
    bool visit(KonqFrame *frame) override
    {
        QString className;
        if (!frame->part()) {
            className = QStringLiteral("NoPart!");
        } else if (!frame->part()->widget()) {
            className = QStringLiteral("NoWidget!");
        } else {
            className = frame->part()->widget()->metaObject()->className();
        }
        qCDebug(KONQUEROR_LOG) << m_spaces << frame
                 << "parent=" << frame->parentContainer()
                 << (frame->isHidden() ? "hidden" : "shown")
                 << "containing view" << frame->childView()
                 << "and part" << frame->part()
                 << "whose widget is a" << className;
        return true;
    }
    bool visit(KonqFrameContainer *container) override
    {
        qCDebug(KONQUEROR_LOG) << m_spaces << container
                 << (container->isHidden() ? "hidden" : "shown")
                 << (container->orientation() == Qt::Horizontal ? "horizontal" : "vertical")
                 << "sizes=" << container->sizes()
                 << "parent=" << container->parentContainer()
                 << "activeChild=" << container->activeChild();

        if (!container->activeChild()) {
            qCDebug(KONQUEROR_LOG) << "WARNING:" << container << "has a null active child!";
        }

        m_spaces += QLatin1String("  ");
        return true;
    }
    bool visit(KonqFrameTabs *tabs) override
    {
        qCDebug(KONQUEROR_LOG) << m_spaces << "KonqFrameTabs" << tabs
                 << "visible=" << tabs->isVisible()
                 << "activeChild=" << tabs->activeChild();
        if (!tabs->activeChild()) {
            qCDebug(KONQUEROR_LOG) << "WARNING:" << tabs << "has a null active child!";
        }
        m_spaces += QLatin1String("  ");
        return true;
    }
    bool visit(KonqMainWindow *) override
    {
        return true;
    }
    bool endVisit(KonqFrameTabs *) override
    {
        m_spaces.resize(m_spaces.size() - 2);
        return true;
    }
    bool endVisit(KonqFrameContainer *) override
    {
        m_spaces.resize(m_spaces.size() - 2);
        return true;
    }
    bool endVisit(KonqMainWindow *) override
    {
        return true;
    }
private:
    QString m_spaces;
};

void KonqViewManager::printFullHierarchy()
{
    qCDebug(KONQUEROR_LOG) << "currentView=" << m_pMainWindow->currentView();
    KonqDebugFrameVisitor visitor;
    m_pMainWindow->accept(&visitor);
}
#endif

KonqFrameTabs *KonqViewManager::tabContainer()
{
    if (!m_tabContainer) {
        createTabContainer(m_pMainWindow /*as widget*/, m_pMainWindow /*as container*/);
        m_pMainWindow->insertChildFrame(m_tabContainer);
    }
    return m_tabContainer;
}

bool KonqViewManager::isTabBarVisible() const
{
    if (!m_tabContainer) {
        return false;
    }
    return !m_tabContainer->tabBar()->isHidden();
}

void KonqViewManager::forceHideTabBar(bool force)
{
    if (m_tabContainer) {
        m_tabContainer->forceHideTabBar(force);
    }
}


void KonqViewManager::createTabContainer(QWidget *parent, KonqFrameContainerBase *parentContainer)
{
#ifdef DEBUG_VIEWMGR
    qCDebug(KONQUEROR_LOG) << "createTabContainer" << parent << parentContainer;
#endif
    m_tabContainer = new KonqFrameTabs(parent, parentContainer, this);
    // Delay the opening of the URL for #106641
    bool ok = connect(m_tabContainer, SIGNAL(openUrl(KonqView*,QUrl)), m_pMainWindow, SLOT(openUrl(KonqView*,QUrl)), Qt::QueuedConnection);
    Q_ASSERT(ok);
    Q_UNUSED(ok);
    applyConfiguration();
}

void KonqViewManager::applyConfiguration()
{
    tabContainer()->setAlwaysTabbedMode(KonqSettings::alwaysTabbedMode());
    tabContainer()->setTabsClosable(KonqSettings::permanentCloseButton());
}

KonqMainWindow *KonqViewManager::duplicateWindow()
{
    QTemporaryFile tempFile;
    tempFile.open();
    KConfig config(tempFile.fileName());
    KConfigGroup group(&config, "Profile");
    KonqFrameBase::Options flags = KonqFrameBase::SaveHistoryItems;
    saveViewConfigToGroup(group, flags);

    KonqMainWindow *mainWindow = openSavedWindow(group);
#ifndef NDEBUG
    mainWindow->viewManager()->printFullHierarchy();
#endif
    return mainWindow;
}


