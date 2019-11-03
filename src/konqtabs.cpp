/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers <konq-e@kde.org>
    Copyright (C) 2002-2003 Douglas Hanley <douglash@caltech.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/

#include "konqtabs.h"

#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QDrag>
#include <QToolButton>
#include <QIcon>
#include <QMimeData>

#include <kcolorscheme.h>
#include "konqdebug.h"
#include <kiconloader.h>
#include <KLocalizedString>
#include <kstringhandler.h>
#include <KUrlMimeData>

#include "konqview.h"
#include "konqviewmanager.h"
#include "konqmisc.h"
#include "konqsettingsxt.h"
#include "konqframevisitor.h"

#include <kacceleratormanager.h>
#include <konqpixmapprovider.h>
#include <kstandardshortcut.h>
#include "ktabbar.h"

//###################################################################

KonqFrameTabs::KonqFrameTabs(QWidget *parent, KonqFrameContainerBase *parentContainer,
                             KonqViewManager *viewManager)
    : KTabWidget(parent),
      m_pPopupMenu(nullptr),
      m_pSubPopupMenuTab(nullptr),
      m_rightWidget(nullptr), m_leftWidget(nullptr), m_alwaysTabBar(false)
{
    // Set an object name so the widget style can identify this widget.
    setObjectName(QStringLiteral("kde_konq_tabwidget"));
    setDocumentMode(true);

    KAcceleratorManager::setNoAccel(this);

    tabBar()->setWhatsThis(i18n("This bar contains the list of currently open tabs. Click on a tab to make it "
                                "active. You can also use keyboard shortcuts to "
                                "navigate through tabs. The text on the tab shows the content "
                                "currently open in it; place your mouse over the tab to see the full title, in "
                                "case it has been shortened to fit the tab width."));
    //qCDebug(KONQUEROR_LOG) << "KonqFrameTabs::KonqFrameTabs()";

    m_pParentContainer = parentContainer;
    m_pActiveChild = nullptr;
    m_pViewManager = viewManager;

    m_permanentCloseButtons = KonqSettings::permanentCloseButton();
    if (m_permanentCloseButtons) {
        setTabsClosable(true);
    }
    tabBar()->setSelectionBehaviorOnRemove(
        KonqSettings::tabCloseActivatePrevious() ? QTabBar::SelectPreviousTab : QTabBar::SelectRightTab);

    if (KonqSettings::tabPosition() == QLatin1String("Bottom")) {
        setTabPosition(QTabWidget::South);
    }
    connect(this, &KonqFrameTabs::tabCloseRequested, this, &KonqFrameTabs::slotCloseRequest);
    connect(this, SIGNAL(removeTabPopup()),
            m_pViewManager->mainWindow(), SLOT(slotRemoveTabPopup()));

    if (KonqSettings::addTabButton()) {
        m_leftWidget = new NewTabToolButton(this);
        connect(m_leftWidget, SIGNAL(clicked()),
                m_pViewManager->mainWindow(), SLOT(slotAddTab()));
        connect(m_leftWidget, SIGNAL(testCanDecode(const QDragMoveEvent*,bool&)),
                SLOT(slotTestCanDecode(const QDragMoveEvent*,bool&)));
        connect(m_leftWidget, SIGNAL(receivedDropEvent(QDropEvent*)),
                SLOT(slotReceivedDropEvent(QDropEvent*)));
        m_leftWidget->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
        m_leftWidget->adjustSize();
        m_leftWidget->setToolTip(i18n("Open a new tab"));
        setCornerWidget(m_leftWidget, Qt::TopLeftCorner);
    }
    if (KonqSettings::closeTabButton()) {
        m_rightWidget = new QToolButton(this);
        connect(m_rightWidget, SIGNAL(clicked()),
                m_pViewManager->mainWindow(), SLOT(slotRemoveTab()));
        m_rightWidget->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
        m_rightWidget->adjustSize();
        m_rightWidget->setToolTip(i18n("Close the current tab"));
        setCornerWidget(m_rightWidget, Qt::TopRightCorner);
    }

    setAutomaticResizeTabs(true);
    setMovable(true);

    connect(tabBar(), SIGNAL(tabMoved(int,int)),
            SLOT(slotMovedTab(int,int)));
    connect(this, SIGNAL(mouseMiddleClick()),
            SLOT(slotMouseMiddleClick()));
    connect(this, SIGNAL(mouseMiddleClick(QWidget*)),
            SLOT(slotMouseMiddleClick(QWidget*)));
    connect(this, SIGNAL(mouseDoubleClick()),
            m_pViewManager->mainWindow(), SLOT(slotAddTab()));

    connect(this, SIGNAL(testCanDecode(const QDragMoveEvent*,bool&)),
            SLOT(slotTestCanDecode(const QDragMoveEvent*,bool&)));
    connect(this, SIGNAL(receivedDropEvent(QDropEvent*)),
            SLOT(slotReceivedDropEvent(QDropEvent*)));
    connect(this, SIGNAL(receivedDropEvent(QWidget*,QDropEvent*)),
            SLOT(slotReceivedDropEvent(QWidget*,QDropEvent*)));
    connect(this, SIGNAL(initiateDrag(QWidget*)),
            SLOT(slotInitiateDrag(QWidget*)));

    tabBar()->installEventFilter(this);
    initPopupMenu();
}

KonqFrameTabs::~KonqFrameTabs()
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrameTabs::~KonqFrameTabs() " << this << " - " << className();
    qDeleteAll(m_childFrameList);
    m_childFrameList.clear();
}

void KonqFrameTabs::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options,
                               KonqFrameBase *docContainer, int id, int depth)
{
    //write children
    QStringList strlst;
    int i = 0;
    QString newPrefix;
    foreach (KonqFrameBase *frame, m_childFrameList) {
        newPrefix = KonqFrameBase::frameTypeToString(frame->frameType()) + 'T' + QString::number(i);
        strlst.append(newPrefix);
        newPrefix.append(QLatin1Char('_'));
        frame->saveConfig(config, newPrefix, options, docContainer, id, depth + i);
        i++;
    }

    config.writeEntry(QStringLiteral("Children").prepend(prefix), strlst);

    config.writeEntry(QStringLiteral("activeChildIndex").prepend(prefix),
                      currentIndex());
}

void KonqFrameTabs::copyHistory(KonqFrameBase *other)
{

    if (!other) {
        qCDebug(KONQUEROR_LOG) << "The Frame does not exist";
        return;
    }

    if (other->frameType() != KonqFrameBase::Tabs) {
        qCDebug(KONQUEROR_LOG) << "Frame types are not the same";
        return;
    }

    for (int i = 0; i < m_childFrameList.count(); i++) {
        m_childFrameList.at(i)->copyHistory(static_cast<KonqFrameTabs *>(other)->m_childFrameList.at(i));
    }
}

void KonqFrameTabs::setTitle(const QString &title, QWidget *sender)
{
    // qCDebug(KONQUEROR_LOG) << "KonqFrameTabs::setTitle( " << title << " , " << sender << " )";
    // Make sure that '&' is displayed correctly
    QString tabText(title);
    setTabText(indexOf(sender), tabText.replace('&', QLatin1String("&&")));
}

void KonqFrameTabs::setTabIcon(const QUrl &url, QWidget *sender)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrameTabs::setTabIcon( " << url << " , " << sender << " )";
    QIcon iconSet = QIcon::fromTheme(KonqPixmapProvider::self()->iconNameFor(url));
    const int pos = indexOf(sender);
    KTabWidget::setTabIcon(pos, iconSet);
}

void KonqFrameTabs::activateChild()
{
    if (m_pActiveChild) {
        setCurrentIndex(indexOf(m_pActiveChild->asQWidget()));
        m_pActiveChild->activateChild();
    }
}

void KonqFrameTabs::insertChildFrame(KonqFrameBase *frame, int index)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrameTabs " << this << ": insertChildFrame " << frame;

    if (!frame) {
        qCWarning(KONQUEROR_LOG) << "KonqFrameTabs " << this << ": insertChildFrame(0) !";
        return;
    }

    //qCDebug(KONQUEROR_LOG) << "Adding frame";

    //QTabWidget docs say that inserting tabs while already shown causes
    //flicker...
    setUpdatesEnabled(false);

    frame->setParentContainer(this);
    if (index == -1) {
        m_childFrameList.append(frame);
    } else {
        m_childFrameList.insert(index, frame);
    }

    // note that this can call slotCurrentChanged (e.g. when inserting/replacing the first tab)
    insertTab(index, frame->asQWidget(), QLatin1String(""));

    // Connect to currentChanged only after inserting the first tab,
    // otherwise insertTab() can call slotCurrentChanged, which we don't expect
    // (the part isn't in the partmanager yet; better let konqviewmanager take care
    // of setting the active part)
    connect(this, SIGNAL(currentChanged(int)),
            this, SLOT(slotCurrentChanged(int)), Qt::UniqueConnection);

    if (KonqView *activeChildView = frame->activeChildView()) {
        activeChildView->setCaption(activeChildView->caption());
        activeChildView->setTabIcon(activeChildView->url());
    }

    updateTabBarVisibility();
    setUpdatesEnabled(true);
}

void KonqFrameTabs::childFrameRemoved(KonqFrameBase *frame)
{
    //qCDebug(KONQUEROR_LOG) << "KonqFrameTabs::RemoveChildFrame " << this << ". Child " << frame << " removed";
    if (frame) {
        removeTab(indexOf(frame->asQWidget()));
        m_childFrameList.removeAll(frame);
        if (count() == 1) {
            updateTabBarVisibility();
        }
    } else {
        qCWarning(KONQUEROR_LOG) << "KonqFrameTabs " << this << ": childFrameRemoved(0L) !";
    }

    //qCDebug(KONQUEROR_LOG) << "KonqFrameTabs::RemoveChildFrame finished";
}

void KonqFrameTabs::moveTabBackward(int index)
{
    if (index == 0) {
        return;
    }
    tabBar()->moveTab(index, index - 1);
}

void KonqFrameTabs::moveTabForward(int index)
{
    if (index == count() - 1) {
        return;
    }
    tabBar()->moveTab(index, index + 1);
}

void KonqFrameTabs::slotMovedTab(int from, int to)
{
    KonqFrameBase *fromFrame = m_childFrameList.at(from);
    m_childFrameList.removeAll(fromFrame);
    m_childFrameList.insert(to, fromFrame);

    KonqFrameBase *currentFrame = dynamic_cast<KonqFrameBase *>(currentWidget());
    if (currentFrame && !m_pViewManager->isLoadingProfile()) {
        m_pActiveChild = currentFrame;
        currentFrame->activateChild();
    }
}

void KonqFrameTabs::slotContextMenu(const QPoint &p)
{
    refreshSubPopupMenuTab();
    m_popupActions[QStringLiteral("reload")]->setEnabled(false);
    m_popupActions[QStringLiteral("duplicatecurrenttab")]->setEnabled(false);
    m_popupActions[QStringLiteral("breakoffcurrenttab")]->setEnabled(false);
    m_popupActions[QStringLiteral("removecurrenttab")]->setEnabled(false);
    m_popupActions[QStringLiteral("othertabs")]->setEnabled(true);
    m_popupActions[QStringLiteral("closeothertabs")]->setEnabled(false);

    m_pPopupMenu->exec(p);
}

void KonqFrameTabs::slotContextMenu(QWidget *w, const QPoint &p)
{
    refreshSubPopupMenuTab();
    uint tabCount = m_childFrameList.count();
    m_popupActions[QStringLiteral("reload")]->setEnabled(true);
    m_popupActions[QStringLiteral("duplicatecurrenttab")]->setEnabled(true);
    m_popupActions[QStringLiteral("breakoffcurrenttab")]->setEnabled(tabCount > 1);
    m_popupActions[QStringLiteral("removecurrenttab")]->setEnabled(true);
    m_popupActions[QStringLiteral("othertabs")]->setEnabled(true);
    m_popupActions[QStringLiteral("closeothertabs")]->setEnabled(true);

    m_pViewManager->mainWindow()->setWorkingTab(indexOf(w));
    m_pPopupMenu->exec(p);
}

void KonqFrameTabs::refreshSubPopupMenuTab()
{
    m_pSubPopupMenuTab->clear();
    int i = 0;
    m_pSubPopupMenuTab->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")),
                                  i18n("&Reload All Tabs"),
                                  m_pViewManager->mainWindow(),
                                  SLOT(slotReloadAllTabs()),
                                  m_pViewManager->mainWindow()->action("reload_all_tabs")->shortcut());
    m_pSubPopupMenuTab->addSeparator();
    foreach (KonqFrameBase *frameBase, m_childFrameList) {
        KonqFrame *frame = dynamic_cast<KonqFrame *>(frameBase);
        if (frame && frame->activeChildView()) {
            QString title = frame->title().trimmed();
            const QUrl url = frame->activeChildView()->url();
            if (title.isEmpty()) {
                title = url.toDisplayString();
            }
            title = KStringHandler::csqueeze(title, 50);
            QAction *action = m_pSubPopupMenuTab->addAction(QIcon::fromTheme(KonqPixmapProvider::self()->iconNameFor(url)), title);
            action->setData(i);
        }
        ++i;
    }
    m_pSubPopupMenuTab->addSeparator();
    m_popupActions[QStringLiteral("closeothertabs")] =
        m_pSubPopupMenuTab->addAction(QIcon::fromTheme(QStringLiteral("tab-close-other")),
                                      i18n("Close &Other Tabs"),
                                      m_pViewManager->mainWindow(),
                                      SLOT(slotRemoveOtherTabsPopup()),
                                      m_pViewManager->mainWindow()->action("removeothertabs")->shortcut());
}

void KonqFrameTabs::slotCloseRequest(int idx)
{
    m_pViewManager->mainWindow()->setWorkingTab(idx);
    emit removeTabPopup();
}

void KonqFrameTabs::slotSubPopupMenuTabActivated(QAction *action)
{
    setCurrentIndex(action->data().toInt());
}

void KonqFrameTabs::slotMouseMiddleClick()
{
    KonqMainWindow *mainWindow = m_pViewManager->mainWindow();
    QUrl filteredURL(KonqMisc::konqFilteredURL(mainWindow, QApplication::clipboard()->text(QClipboard::Selection)));
    if (filteredURL.isValid() && filteredURL.scheme() != QLatin1String("error")) {
        KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"), QString(), false, false);
        if (newView == nullptr) {
            return;
        }
        mainWindow->openUrl(newView, filteredURL, QString());
        m_pViewManager->showTab(newView);
        mainWindow->focusLocationBar();
    }
}

void KonqFrameTabs::slotMouseMiddleClick(QWidget *w)
{
    QUrl filteredURL(KonqMisc::konqFilteredURL(m_pViewManager->mainWindow(), QApplication::clipboard()->text(QClipboard::Selection)));
    if (filteredURL.isValid() && filteredURL.scheme() != QLatin1String("error")) {
        KonqFrameBase *frame = dynamic_cast<KonqFrameBase *>(w);
        if (frame) {
            m_pViewManager->mainWindow()->openUrl(frame->activeChildView(), filteredURL);
        }
    }
}

void KonqFrameTabs::slotTestCanDecode(const QDragMoveEvent *e, bool &accept /* result */)
{
    accept = e->mimeData()->hasUrls();
}

void KonqFrameTabs::slotReceivedDropEvent(QDropEvent *e)
{
    QList<QUrl> lstDragURLs = KUrlMimeData::urlsFromMimeData(e->mimeData());
    if (!lstDragURLs.isEmpty()) {
        KonqView *newView = m_pViewManager->addTab(QStringLiteral("text/html"), QString(), false, false);
        if (newView == nullptr) {
            return;
        }
        m_pViewManager->mainWindow()->openUrl(newView, lstDragURLs.first(), QString());
        m_pViewManager->showTab(newView);
        m_pViewManager->mainWindow()->focusLocationBar();
    }
}

void KonqFrameTabs::slotReceivedDropEvent(QWidget *w, QDropEvent *e)
{
    QList<QUrl> lstDragURLs = KUrlMimeData::urlsFromMimeData(e->mimeData());
    KonqFrameBase *frame = dynamic_cast<KonqFrameBase *>(w);
    if (lstDragURLs.count() && frame) {
        const QUrl dragUrl = lstDragURLs.first();
        if (dragUrl != frame->activeChildView()->url()) {
            emit openUrl(frame->activeChildView(), dragUrl);
        }
    }
}

void KonqFrameTabs::slotInitiateDrag(QWidget *w)
{
    KonqFrameBase *frame = dynamic_cast<KonqFrameBase *>(w);
    if (frame) {
        QDrag *d = new QDrag(this);
        QMimeData *md = new QMimeData;
        md->setUrls(QList<QUrl>() << frame->activeChildView()->url());
        d->setMimeData(md);
        QString iconName = KIO::iconNameForUrl(frame->activeChildView()->url());
        d->setPixmap(KIconLoader::global()->loadIcon(iconName, KIconLoader::Small, 0));
        d->exec();
    }
}

void KonqFrameTabs::updateTabBarVisibility()
{
    if (m_alwaysTabBar) {
        tabBar()->show();
    } else {
        tabBar()->setVisible(count() > 1);
    }
}

void KonqFrameTabs::setAlwaysTabbedMode(bool enable)
{
    const bool update = (enable != m_alwaysTabBar);
    m_alwaysTabBar = enable;
    if (update) {
        updateTabBarVisibility();
    }
}

void KonqFrameTabs::initPopupMenu()
{
    m_pPopupMenu = new QMenu(this);
    m_popupActions[QStringLiteral("newtab")] = m_pPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("tab-new")),
                               i18n("&New Tab"),
                               m_pViewManager->mainWindow(),
                               SLOT(slotAddTab()),
                               m_pViewManager->mainWindow()->action("newtab")->shortcut());
    m_popupActions[QStringLiteral("duplicatecurrenttab")] = m_pPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("tab-duplicate")),
                                            i18n("&Duplicate Tab"),
                                            m_pViewManager->mainWindow(),
                                            SLOT(slotDuplicateTabPopup()),
                                            m_pViewManager->mainWindow()->action("duplicatecurrenttab")->shortcut());
    m_popupActions[QStringLiteral("reload")] = m_pPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")),
                               i18n("&Reload Tab"),
                               m_pViewManager->mainWindow(),
                               SLOT(slotReloadPopup()),
                               m_pViewManager->mainWindow()->action("reload")->shortcut());
    m_pPopupMenu->addSeparator();
    m_pSubPopupMenuTab = new QMenu(this);
    m_popupActions[QStringLiteral("othertabs")] = m_pPopupMenu->addMenu(m_pSubPopupMenuTab);
    m_popupActions[QStringLiteral("othertabs")]->setText(i18n("Other Tabs"));
    connect(m_pSubPopupMenuTab, SIGNAL(triggered(QAction*)),
            this, SLOT(slotSubPopupMenuTabActivated(QAction*)));
    m_pPopupMenu->addSeparator();
    m_popupActions[QStringLiteral("breakoffcurrenttab")] = m_pPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("tab-detach")),
                                           i18n("D&etach Tab"),
                                           m_pViewManager->mainWindow(),
                                           SLOT(slotBreakOffTabPopup()),
                                           m_pViewManager->mainWindow()->action("breakoffcurrenttab")->shortcut());
    m_pPopupMenu->addSeparator();
    m_popupActions[QStringLiteral("removecurrenttab")] = m_pPopupMenu->addAction(QIcon::fromTheme(QStringLiteral("tab-close")),
                                         i18n("&Close Tab"),
                                         m_pViewManager->mainWindow(),
                                         SLOT(slotRemoveTabPopup()),
                                         m_pViewManager->mainWindow()->action("removecurrenttab")->shortcut());
    connect(this, SIGNAL(contextMenu(QWidget*,QPoint)),
            SLOT(slotContextMenu(QWidget*,QPoint)));
    connect(this, SIGNAL(contextMenu(QPoint)),
            SLOT(slotContextMenu(QPoint)));

}

bool KonqFrameTabs::accept(KonqFrameVisitor *visitor)
{
    if (!visitor->visit(this)) {
        return false;
    }
    if (visitor->visitAllTabs()) {
        foreach (KonqFrameBase *frame, m_childFrameList) {
            Q_ASSERT(frame);
            if (!frame->accept(visitor)) {
                return false;
            }
        }
    } else {
        // visit only current tab
        if (m_pActiveChild) {
            if (!m_pActiveChild->accept(visitor)) {
                return false;
            }
        }
    }
    if (!visitor->endVisit(this)) {
        return false;
    }
    return true;
}

void KonqFrameTabs::slotCurrentChanged(int index)
{
    const KColorScheme colorScheme(QPalette::Active, KColorScheme::Window);
    tabBar()->setTabTextColor(index, colorScheme.foreground(KColorScheme::NormalText).color());

    KonqFrameBase *currentFrame = tabAt(index);
    if (currentFrame && !m_pViewManager->isLoadingProfile()) {
        m_pActiveChild = currentFrame;
        currentFrame->activateChild();
    }

    m_pViewManager->mainWindow()->linkableViewCountChanged();
}

int KonqFrameTabs::tabIndexContaining(KonqFrameBase *frame) const
{
    KonqFrameBase *frameBase = frame;
    while (frameBase && frameBase->parentContainer() != this) {
        frameBase = frameBase->parentContainer();
    }
    if (frameBase) {
        return indexOf(frameBase->asQWidget());
    } else {
        return -1;
    }
}

int KonqFrameTabs::tabWhereActive(KonqFrameBase *frame) const
{
    for (int i = 0; i < m_childFrameList.count(); i++) {
        KonqFrameBase *f = m_childFrameList.at(i);
        while (f && f != frame) {
            f = f->isContainer() ? static_cast<KonqFrameContainerBase *>(f)->activeChild() : nullptr;
        }
        if (f == frame) {
            return i;
        }
    }
    return -1;
}

void KonqFrameTabs::setLoading(KonqFrameBase *frame, bool loading)
{
    const int pos = tabWhereActive(frame);
    if (pos == -1) {
        return;
    }

    const KColorScheme colorScheme(QPalette::Active, KColorScheme::Window);
    QColor color;
    if (loading) {
        color = colorScheme.foreground(KColorScheme::NeutralText).color(); // a tab is currently loading
    } else {
        if (currentIndex() != pos) {
            // another tab has newly loaded contents. Use "link" because you can click on it to read it.
            color = colorScheme.foreground(KColorScheme::LinkText).color();
        } else {
            // the current tab has finished loading.
            color = colorScheme.foreground(KColorScheme::NormalText).color();
        }
    }
    tabBar()->setTabTextColor(pos, color);
}

void KonqFrameTabs::replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame)
{
    const int index = indexOf(oldFrame->asQWidget());
    childFrameRemoved(oldFrame);
    insertChildFrame(newFrame, index);
    setCurrentIndex(index);
}

KonqFrameBase *KonqFrameTabs::tabAt(int index) const
{
    return dynamic_cast<KonqFrameBase *>(widget(index));
}

KonqFrameBase *KonqFrameTabs::currentTab() const
{
    return tabAt(currentIndex());
}

bool KonqFrameTabs::eventFilter(QObject *watched, QEvent *event)
{
    if (KonqSettings::mouseMiddleClickClosesTab()) {
        QTabBar *bar = tabBar();
        if (watched == bar &&
                (event->type() == QEvent::MouseButtonPress ||
                 event->type() == QEvent::MouseButtonRelease)) {
            QMouseEvent *e = static_cast<QMouseEvent *>(event);
            if (e->button() == Qt::MidButton) {
                if (event->type() == QEvent::MouseButtonRelease) {
                    const int index = bar->tabAt(e->pos());
                    slotCloseRequest(index);
                }
                e->accept();
                return true;
            }
        }
    }
    return KTabWidget::eventFilter(watched, event);
}

