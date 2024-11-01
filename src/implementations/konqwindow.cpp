//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "konqwindow.h"

#include "konqmainwindow.h"
#include "konqtabs.h"
#include "konqviewmanager.h"
#include "konqview.h"

using namespace KonqImplementations;

KonqWindow::KonqWindow(KonqMainWindow *window, KonqViewManager *viewManager) : KonqInterfaces::Window(window),
    m_mainWindow(window),
    m_viewManager(viewManager)
{
    connect(m_viewManager, &KonqViewManager::tabContainerChanged, this, &KonqWindow::tabsContainerChanged);
    connect(m_viewManager, &KonqViewManager::viewCreated, this, &KonqWindow::connectToView);
}

KonqWindow::~KonqWindow() noexcept
{
}

void KonqWindow::tabsContainerChanged(KonqFrameTabs* container)
{
    if (m_tabsContainer) {
        disconnect(m_tabsContainer, nullptr, this, nullptr);
        disconnect(container->tabBar(), &QTabBar::tabMoved, this, &Window::tabMoved);
    }
    m_tabsContainer = container;
    if (container) {
        connect(container, &KonqFrameTabs::tabHasBeenAdded, this, &KonqWindow::slotTabAdded);
        connect(container, &KonqFrameTabs::tabHasBeenRemoved, this, &Window::tabRemoved);
        connect(container, &KonqFrameTabs::currentChanged, this, &Window::currentTabChanged);
        connect(container->tabBar(), &QTabBar::tabMoved, this, &Window::tabMoved);
    }
}

QWidget* KonqWindow::widget()
{
    return m_mainWindow;
}

int KonqImplementations::KonqWindow::tabsCount() const
{
    if (!m_tabsContainer) {
        return 0;
    }
    return m_tabsContainer->count();
}

int KonqWindow::activeTab() const
{
    if (!m_tabsContainer) {
        return -1;
    }
    return m_tabsContainer->currentIndex();
}

QIcon KonqWindow::tabFavicon(int idx)
{
    if (!m_tabsContainer) {
        return {};
    }
    return m_tabsContainer->tabIcon(idx);
}

QString KonqWindow::tabTitle(int idx) const
{
    return m_tabsContainer ? m_tabsContainer->tabText(idx) : QString();
}

QUrl KonqWindow::tabUrl(int idx) const
{
    if (!m_tabsContainer) {
        return {};
    }
    KonqFrameBase *frm = m_tabsContainer->tabAt(idx);
    KonqView *view = frm ? frm->activeChildView() : nullptr;
    return view ? view->url() : QUrl();
}

void KonqWindow::activateTab(int idx)
{
    if (m_mainWindow) {
        m_mainWindow->activateTab(idx);
    }
}

void KonqImplementations::KonqWindow::slotTabAdded(int idx)
{
    emit tabAdded(idx);
}

void KonqWindow::slotTabRemoved(int idx)
{
    emit tabRemoved(idx);
}

void KonqWindow::connectToView(KonqView *view)
{
    int idx = m_tabsContainer->tabIndexContaining(view->frame());
    if (idx < 0) {
        return;
    }
    connect(view, &KonqView::urlChanged, this, [this, view](const QUrl &url){viewUrlChanged(view, url);});
    connect(view, &KonqView::captionChanged, this, [this, view](const QString &caption){viewCaptionChanged(view, caption);});
}

bool KonqWindow::isViewActive(KonqView* view, int idx) const
{
    if (idx < 0) {
        return false;
    }
    KonqFrameBase *tab = m_tabsContainer->tabAt(idx);
    switch (tab->frameType()) {
        case KonqFrameBase::View:
            return true;
        case KonqFrameBase::Container:
        case KonqFrameBase::ContainerBase:
            return dynamic_cast<KonqFrameContainerBase*>(tab)->activeChild() == view->frame();
        default:
            return false;
    }
}

void KonqWindow::viewUrlChanged(KonqView *view, const QUrl& url)
{
    int idx = m_tabsContainer->tabIndexContaining(view->frame());
    if (isViewActive(view, idx)) {
        emit tabUrlChanged(idx, url);
    }
}

void KonqWindow::viewCaptionChanged(KonqView* view, const QString& caption)
{
    int idx = m_tabsContainer->tabIndexContaining(view->frame());
    if (isViewActive(view, idx)) {
        emit tabTitleChanged(idx, caption);
    }
}

void KonqWindow::moveTab(int fromIdx, int toIdx)
{
    if (m_tabsContainer) {
        m_tabsContainer->moveTab(fromIdx, toIdx);
    }
}

KonqInterfaces::TabBarContextMenu* KonqWindow::tabBarContextMenu(QWidget* parent) const
{
    return new TabBarContextMenu(m_tabsContainer, parent);
}
