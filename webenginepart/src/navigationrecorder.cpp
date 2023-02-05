/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "navigationrecorder.h"
#include "webenginepage.h"

NavigationRecorder::NavigationRecorder(QObject *parent) : QObject(parent)
{
}

NavigationRecorder::~NavigationRecorder() noexcept
{
}

void NavigationRecorder::registerPage(WebEnginePage* page)
{
    connect(page, &QObject::destroyed, this, &NavigationRecorder::removePage);
    connect(page, &WebEnginePage::mainFrameNavigationRequested, this, &NavigationRecorder::recordNavigation);
    connect(page, &WebEnginePage::loadFinished, this, [this, page](bool){recordNavigationFinished(page, page->url());});
}

void NavigationRecorder::removePage(QObject*)
{
    //NOTE: we cannot use QMultiHash::remove because this is connected to the QObject::destroyed signal,
    //which is emitted *after* the WebEnginePage destructor has been called, so it cannot be cast.
    //The workaround is to remove all nullptr entries, since the destroyed signal is emitted after
    //all instances of QPointer have been notified
    for (const QUrl &url : m_pendingNavigations.keys()) {
        m_pendingNavigations.remove(url, nullptr);
    }
    for (const QUrl &url : m_postNavigations.keys()) {
        m_postNavigations.remove(url, nullptr);
    }
}

void NavigationRecorder::recordNavigation(WebEnginePage* page, const QUrl& url)
{
    m_pendingNavigations.insert(url, page);
}

void NavigationRecorder::recordNavigationFinished(WebEnginePage* page, const QUrl& url)
{
    m_postNavigations.remove(url, page);
}

bool NavigationRecorder::isPostRequest(const QUrl& url, WebEnginePage* page) const
{
    return m_postNavigations.contains(url, page);
}

void NavigationRecorder::recordRequestDetails(const QWebEngineUrlRequestInfo& info)
{
    QUrl url(info.requestUrl());
    QList<QPointer<WebEnginePage>> pages = m_pendingNavigations.values(url);
    WebEnginePage *page = nullptr;
    if (!pages.isEmpty()) {
        //NOTE: QMultiHash::values returns a list where the first element is the one inserted last:
        //pages.last() is the page which first made the navigation request
        page = pages.last();
        m_pendingNavigations.remove(url, page);
    } else {
        return;
    }
    if (info.requestMethod() == QByteArrayLiteral("POST")) {
        m_postNavigations.insert(url, page);
    }
}
