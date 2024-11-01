//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "fakewindow.h"

#include <QWidget>

using namespace KonqInterfaces;

FakeWindow::FakeWindow(QObject *parent) : Window(parent), m_widget{new QWidget(nullptr)}
{
}

QWidget* FakeWindow::widget()
{
    return m_widget;
}

void FakeWindow::activateTab(int idx)
{
    m_activeTab = idx;
    emit currentTabChanged(idx);
}

int FakeWindow::activeTab() const
{
    return m_activeTab;
}

QIcon FakeWindow::tabFavicon(int idx)
{
    return idx < m_tabs.count() ? m_tabs.at(idx).favicon : QIcon();
}

int FakeWindow::tabsCount() const
{
    return m_tabs.count();
}

QString FakeWindow::tabTitle(int idx) const
{
    return idx < m_tabs.count() ? m_tabs.at(idx).title : QString();
}

QUrl FakeWindow::tabUrl(int idx) const
{
    return idx < m_tabs.count() ? m_tabs.at(idx).url: QUrl();
}

void FakeWindow::moveTab(int fromIdx, int toIdx)
{
    if (fromIdx < 0 || fromIdx >= m_tabs.count() || fromIdx == toIdx) {
        return;
    }
    Tab tab = m_tabs.takeAt(fromIdx);
    // if (toIdx > fromIdx) {
    //     toIdx--;
    // }
    if (toIdx > m_tabs.count()) {
        toIdx = m_tabs.count();
    }
    m_tabs.insert(toIdx, tab);
    emit tabMoved(fromIdx, toIdx);
}

void FakeWindow::addTab(int idx, const Tab& newTab)
{
    m_tabs.insert(idx, newTab);
    emit tabAdded(idx);
}

// void FakeWindow::addTab(int idx, const QString& title, const QUrl& url, const QIcon& favicon)
// {
//     m_tabs.insert(idx, {title, url, favicon});
//     emit tabAdded(idx);
// }

void FakeWindow::removeTab(int idx)
{
    m_tabs.removeAt(idx);
    emit tabRemoved(idx);
}

void FakeWindow::changeTitle(int idx, const QString& newTitle)
{
    m_tabs[idx].title = newTitle;
    emit tabTitleChanged(idx, newTitle);
}

void FakeWindow::changeUrl(int idx, const QUrl& newUrl)
{
    m_tabs[idx].url = newUrl;
    emit tabUrlChanged(idx, newUrl);
}
