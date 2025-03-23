/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "implementations/konqbrowser.h"

#include "interfaces/cookiejar.h"
#include "interfaces/window.h"
#include "konqapplication.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include "konqsettings.h"
#include "konqmainwindowfactory.h"
#include "urlloader.h"

#include <konqdebug.h>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KParts/ReadOnlyPart>

using namespace KonqInterfaces;

#define CHROME_VERSION "108.0.5359.220"

QString KonqBrowser::konquerorUserAgent() {
    QString s_konqUserAgent{QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) QtWebEngine/%1 Chrome/%2 Safari/537.36 Konqueror (WebEngine)").arg(qVersion()).arg(CHROME_VERSION)};
    return s_konqUserAgent;
}

KonqBrowser::KonqBrowser(QObject* parent) : Browser(parent), m_cookieJar(nullptr)
{
    applyConfiguration();
}

KonqBrowser::~KonqBrowser()
{
}

void KonqBrowser::setCookieJar(CookieJar* jar)
{
    m_cookieJar = jar;
}

CookieJar* KonqBrowser::cookieJar() const
{
    return m_cookieJar;
}

QString KonqBrowser::konqUserAgent() const
{
    return konquerorUserAgent();
}

QString KonqBrowser::defaultUserAgent() const
{
    return m_userAgent.defaultUA;
}

QString KonqBrowser::userAgent() const
{
    return m_userAgent.currentUserAgent();
}

void KonqBrowser::readDefaultUserAgent()
{
    const QString oldUA = m_userAgent.currentUserAgent();
    if (Konq::Settings::useDefaultUserAgent()) {
        m_userAgent.defaultUA = konquerorUserAgent();
    } else {
        m_userAgent.defaultUA = Konq::Settings::customUserAgent();
    }
    if (m_userAgent.usingDefaultUA && m_userAgent.defaultUA != oldUA) {
        emit userAgentChanged(m_userAgent.defaultUA);
    }
}

void KonqBrowser::applyConfiguration()
{
    readDefaultUserAgent();
}

void KonqBrowser::setTemporaryUserAgent(const QString& newUA)
{
    const QString oldUA = userAgent();
    m_userAgent.usingDefaultUA = false;
    m_userAgent.temporaryUA = newUA;
    if (oldUA != newUA) {
        emit userAgentChanged(newUA);
    }
}

void KonqBrowser::clearTemporaryUserAgent()
{
    const QString oldUA = m_userAgent.currentUserAgent();
    m_userAgent.usingDefaultUA = true;
    m_userAgent.temporaryUA.clear();
    const QString newUA = m_userAgent.currentUserAgent();
    if (oldUA != newUA) {
        emit userAgentChanged(newUA);
    }
}

bool KonqBrowser::canNavigateTo(KParts::ReadOnlyPart* part, const QUrl &url) const
{
    if (!part || !part->widget()) {
        return true;
    }
    KonqMainWindow *mw = qobject_cast<KonqMainWindow*>(part->widget()->window());
    if (!mw) {
        qCDebug(KONQUEROR_LOG()) << "Part window is not a KonqMainWindow. This shouldn't happen";
        return true;
    }
    KonqView *v = mw->childView(part);
    return v ? v->canNavigateTo(url) : true;
}

KonquerorApplication* KonqBrowser::app() const
{
    return qobject_cast<KonquerorApplication*>(qApp);
}

bool KonqBrowser::openUrl(const QUrl& url, KParts::OpenUrlArguments& args, const BrowserArguments& bargs, QWidget* window)
{
    KonqMainWindow *mainWindow = qobject_cast<KonqMainWindow*>(window);
    if (!mainWindow) {
        mainWindow = KonqMainWindow::mostSuitableWindow();
    }
    if (!mainWindow) {
        mainWindow = KonqMainWindowFactory::createEmptyWindow();
    }
    if (!mainWindow) {
        return false;
    }
    KonqOpenURLRequest req(args, bargs, nullptr);
    mainWindow->slotOpenURLRequest(url, req);
    return true;
}

Window* KonqBrowser::window(QWidget* widget)
{
    QWidget *mainWindow = widget->window();
    return KonqInterfaces::Window::window(mainWindow);
}

void KonqBrowser::setSaveDirForWindow(const QString& saveDir, QWidget* window)
{
    KonqMainWindow *mw = qobject_cast<KonqMainWindow*>(window);
    if (mw) {
        mw->setSaveDir(saveDir);
    }
}

QString KonqBrowser::partForLocalFile(const QString& path, const QString& mimeType)
{
    if (!mimeType.isEmpty()) {
        QString part = UrlLoader::partForMimetype(mimeType);
        if (!part.isEmpty()) {
            return part;
        }
    }
    return UrlLoader::partForLocalFile(path);
}
