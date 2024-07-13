/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2000, 2006 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "KonqViewAdaptor.h"
#include "konqview.h"

KonqViewAdaptor::KonqViewAdaptor(KonqView *view)
    : m_pView(view)
{
}

KonqViewAdaptor::~KonqViewAdaptor()
{
}

void KonqViewAdaptor::openUrl(const QString &url, const QString &locationBarURL, const QString &nameFilter)
{
    m_pView->openUrl(QUrl::fromUserInput(url), locationBarURL, nameFilter);
}

bool KonqViewAdaptor::changeViewMode(const QString &mimeType,
                                     const QString &serviceName)
{
    return m_pView->changePart({mimeType}, serviceName);
}

void KonqViewAdaptor::lockHistory()

{
    m_pView->lockHistory();
}

void KonqViewAdaptor::stop()
{
    m_pView->stop();
}

QString KonqViewAdaptor::url()
{
    return m_pView->url().url();
}

QString KonqViewAdaptor::locationBarURL()
{
    return m_pView->locationBarURL();
}

QString KonqViewAdaptor::type()
{
    return m_pView->type().toString();
}

QStringList KonqViewAdaptor::serviceTypes()
{
    KParts::PartCapabilities capabilities = m_pView->partCapabilities();
    QStringList serviceTypes;
    if (capabilities & KParts::PartCapability::ReadOnly) {
        serviceTypes << QLatin1String("KParts/ReadOnlyPart");
    }
    if (capabilities & KParts::PartCapability::ReadWrite) {
        serviceTypes <<  QStringLiteral("KParts/ReadWritePart");
    }
    if (capabilities & KParts::PartCapability::BrowserView) {
        serviceTypes << QStringLiteral("Browser/View");
    }
    return serviceTypes;
}

QDBusObjectPath KonqViewAdaptor::part()
{
    return QDBusObjectPath(m_pView->partObjectPath());
}

void KonqViewAdaptor::enablePopupMenu(bool b)
{
    m_pView->enablePopupMenu(b);
}

uint KonqViewAdaptor::historyLength()const
{
    return m_pView->historyLength();
}

void KonqViewAdaptor::goForward()
{
    m_pView->go(-1);
}

void KonqViewAdaptor::goBack()
{
    m_pView->go(+1);
}

bool KonqViewAdaptor::isPopupMenuEnabled() const
{
    return m_pView->isPopupMenuEnabled();
}

bool KonqViewAdaptor::canGoBack()const
{
    return m_pView->canGoBack();
}

bool KonqViewAdaptor::canGoForward()const
{
    return m_pView->canGoForward();
}

void KonqViewAdaptor::reload()
{
    return m_pView->mainWindow()->slotReload(m_pView);
}

