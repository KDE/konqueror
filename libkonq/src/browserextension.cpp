/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserextension.h"

#include <QTimer>
#include <QDebug>

struct DelayedRequest {
    QUrl m_delayedURL;
    KParts::OpenUrlArguments m_delayedArgs;
    BrowserArguments m_delayedBrowserArgs;
};

BrowserExtension::BrowserExtension(KParts::ReadOnlyPart *parent)
    : KParts::NavigationExtension(parent)
{
    connect(parent, static_cast<void (KParts::ReadOnlyPart::*)()>(&KParts::ReadOnlyPart::completed), this, &BrowserExtension::slotCompleted);
    connect(this, &BrowserExtension::browserOpenUrlRequest, this, &BrowserExtension::slotOpenUrlRequest);
    connect(this, &BrowserExtension::browserOpenUrlRequestSync, this, &BrowserExtension::browserOpenUrlRequestDelayed);
}

BrowserExtension::~BrowserExtension()
{
}

void BrowserExtension::setBrowserArguments(const BrowserArguments &args)
{
    m_browserArgs = args;
}

BrowserArguments BrowserExtension::browserArguments() const
{
    return m_browserArgs;
}

void BrowserExtension::setBrowserInterface(BrowserInterface *impl)
{
    m_browserInterface = impl;
}

BrowserInterface *BrowserExtension::browserInterface() const
{
    return m_browserInterface;
}

void BrowserExtension::slotCompleted()
{
    // empty the argument stuff, to avoid bogus/invalid values when opening a new url
    setBrowserArguments(BrowserArguments());
}

void BrowserExtension::slotOpenUrlRequest(const QUrl &url, const KParts::OpenUrlArguments &args, const BrowserArguments &browserArgs)
{
    DelayedRequest req;
    req.m_delayedURL = url;
    req.m_delayedArgs = args;
    req.m_delayedBrowserArgs = browserArgs;
    m_requests.append(req);
    QTimer::singleShot(0, this, &BrowserExtension::slotEmitOpenUrlRequestDelayed);
}

void BrowserExtension::slotEmitOpenUrlRequestDelayed()
{
    if (m_requests.isEmpty()) {
        return;
    }
    DelayedRequest req = m_requests.front();
    m_requests.pop_front();
    Q_EMIT browserOpenUrlRequestDelayed(req.m_delayedURL, req.m_delayedArgs, req.m_delayedBrowserArgs);
}

#include "moc_browserextension.cpp"
