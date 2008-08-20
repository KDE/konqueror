/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "webpage.h"
#include "kwebpage.h"
#include "webkitpart.h"
#include "webview.h"
#include "webkitglobal.h"
#include "network/knetworkaccessmanager.h"
#include "settings/webkitsettings.h"

#include <KDE/KParts/GenericFactory>
#include <KDE/KParts/BrowserRun>
#include <KDE/KAboutData>
#include <KDE/KAction>
#include <KDE/KFileDialog>
#include <KDE/KInputDialog>
#include <KDE/KMessageBox>
#include <KDE/KProtocolManager>
#include <KDE/KGlobalSettings>
#include <KDE/KJobUiDelegate>
#include <KDE/KRun>
#include <KDE/KShell>
#include <KDE/KStandardDirs>
#include <KDE/KStandardShortcut>
#include <KIO/Job>

#include <QWebFrame>
#include <QtNetwork/QNetworkReply>

WebPage::WebPage(WebKitPart *wpart, QWidget *parent)
    : KWebPage(parent)
    , m_part(wpart)
{
    connect(this, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(slotGeometryChangeRequested(const QRect &)));
    connect(this, SIGNAL(windowCloseRequested()),
            this, SLOT(slotWindowCloseRequested()));
    connect(this, SIGNAL(statusBarMessage(const QString &)),
            this, SLOT(slotStatusBarMessage(const QString &)));

    const QString host = mainFrame()->url().host();
    settings()->setAttribute(QWebSettings::PluginsEnabled, WebKitGlobal::settings()->isPluginsEnabled(host));
    settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, WebKitGlobal::settings()->isJavaScriptDebugEnabled(host));
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                      NavigationType type)
{
    kDebug() << "acceptNavigationRequest" << request.url();

    return KWebPage::acceptNavigationRequest(frame, request, type);
}

KWebPage *WebPage::createWindow(WebWindowType type)
{
    if (WebKitGlobal::settings()->windowOpenPolicy(mainFrame()->url().host()) != WebKitSettings::KJSWindowOpenDeny)
        return 0;

    kDebug() << type;
    KParts::ReadOnlyPart *part = 0;
    KParts::OpenUrlArguments args;
    //if (type == WebModalDialog) //TODO: correct behavior?
        args.metaData()["forcenewwindow"] = "true";

    emit m_part->browserExtension()->createNewWindow(KUrl("about:blank"), args,
                                                     KParts::BrowserArguments(),
                                                     KParts::WindowArgs(), &part);
    WebKitPart *webKitPart = qobject_cast<WebKitPart*>(part);
    if (!webKitPart) {
        kDebug() << "got NOT a WebKitPart but a" << part->metaObject()->className();
        return 0;
    }
    return webKitPart->view()->page();
}

void WebPage::slotGeometryChangeRequested(const QRect &rect)
{
    const QString host = mainFrame()->url().host();

    if (WebKitGlobal::settings()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {
        emit m_part->browserExtension()->moveTopLevelWidget(rect.x(), rect.y());
    }

    int height = rect.height();
    int width = rect.width();

    // parts of following code are based on kjs_window.cpp
    // Security check: within desktop limits and bigger than 100x100 (per spec)
    if (width < 100 || height < 100) {
        kDebug() << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = KGlobalSettings::desktopGeometry(view());

    if (width > sg.width() || height > sg.height()) {
        kDebug() << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebKitGlobal::settings()->windowResizePolicy(host) == WebKitSettings::KJSWindowResizeAllow) {
        kDebug() << "resizing to " << width << "x" << height;
        emit m_part->browserExtension()->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    int right = view()->x() + view()->frameGeometry().width();
    int bottom = view()->y() + view()->frameGeometry().height();
    int moveByX = 0;
    int moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0
    if ((moveByX || moveByY) &&
      WebKitGlobal::settings()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {
        emit m_part->browserExtension()->moveTopLevelWidget(view()->x() + moveByX, view()->y() + moveByY);
    }
}

void WebPage::slotWindowCloseRequested()
{
    emit m_part->browserExtension()->requestFocus(m_part);
    if (KMessageBox::questionYesNo(view(),
                                   i18n("Close window?"), i18n("Confirmation Required"),
                                   KStandardGuiItem::close(), KStandardGuiItem::cancel())
      == KMessageBox::Yes) {
        m_part->deleteLater();
        m_part = 0;
    }
}

void WebPage::slotStatusBarMessage(const QString &message)
{
    if (WebKitGlobal::settings()->windowStatusPolicy(mainFrame()->url().host()) == WebKitSettings::KJSWindowStatusAllow) {
        m_part->setStatusBarTextProxy(message);
    }
}
