/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
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
 */

#include "fakepage.h"

#include "settings/webkitsettings.h"

#include <KDE/KDebug>
#include <KDE/KMenuBar>
#include <KDE/KStatusBar>
#include <KDE/KToolBar>
#include <KDE/KMainWindow>
#include <KDE/KGlobalSettings>
#include <KDE/KParts/Part>
#include <KDE/KParts/BrowserExtension>

#include <QtCore/QRect>
#include <QtNetwork/QNetworkRequest>
#include <QtWebKit/QWebFrame>

static KMainWindow* topLevelWindowForPart(KParts::ReadOnlyPart* part)
{
    const QWidget* mainWidget = part ? part->widget() : 0;
    return (mainWidget ? qobject_cast<KMainWindow*>(mainWidget->window()) : 0);
}

FakePage::FakePage(KParts::ReadOnlyPart* part, WebWindowType type, QObject* parent)
         : QWebPage(parent), m_part(part), m_type(type)
{
    connect(this, SIGNAL(geometryChangeRequested(const QRect&)),
            this, SLOT(slotGeometryChangeRequested(const QRect&)));
    connect(this, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            this, SLOT(slotMenuBarVisibilityChangeRequested(bool)));
    connect(this, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            this, SLOT(slotToolBarVisibilityChangeRequested(bool)));
    connect(this, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            this, SLOT(slotStatusBarVisibilityChangeRequested(bool)));
    connect(part, SIGNAL(completed()), this, SLOT(deleteLater()));
}

FakePage::~FakePage()
{
    //kDebug();  
}

bool FakePage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    //kDebug() << "url:" << request.url() << ",type:" << type << ",frame:" << frame;

    if (m_part && frame == mainFrame() && type == QWebPage::NavigationTypeOther)
        m_part->openUrl(request.url());
    return true;
}

void FakePage::slotGeometryChangeRequested(const QRect & rect)
{
    // Do nothing if the new window type is WebBrowserWindow...
    if (m_type == WebBrowserWindow)
        return;
    
    //kDebug() << "rect:" << rect << ",part:" << m_part << ",part widget:" << m_part->widget();
    
    if (!m_part)
        return;

    QWidget* widget = m_part->widget();
    if (!widget)
        return;
    
    KParts::BrowserExtension* extension = m_part->browserExtension();
    if (!extension)
        return;

    const QString host = mainFrame()->url().host();   

    // NOTE: If a new window was created from another window which is in
    // maximized mode and its width and/or height were not specified at the
    // time of its creation, which is always the case in QWebPage::createWindow,
    // then any move operation will seem not to work. That is because the new
    // window will be in maximized mode where moving it will not be possible...
    if (WebKitSettings::self()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow ||
        (widget->x() != rect.x() || widget->y() != rect.y()))
        emit extension->moveTopLevelWidget(rect.x(), rect.y());

    const int height = rect.height();
    const int width = rect.width();

    // parts of following code are based on kjs_window.cpp
    // Security check: within desktop limits and bigger than 100x100 (per spec)
    if (width < 100 || height < 100) {
        //kDebug() << "Window resize refused, window would be too small (" << width << "," << height << ")";
        return;
    }

    QRect sg = KGlobalSettings::desktopGeometry(m_part->widget());

    if (width > sg.width() || height > sg.height()) {
        //kDebug() << "Window resize refused, window would be too big (" << width << "," << height << ")";
        return;
    }

    if (WebKitSettings::self()->windowResizePolicy(host) == WebKitSettings::KJSWindowResizeAllow) {
        //kDebug() << "resizing to " << width << "x" << height;
        emit extension->resizeTopLevelWidget(width, height);
    }

    // If the window is out of the desktop, move it up/left
    // (maybe we should use workarea instead of sg, otherwise the window ends up below kicker)
    const int right = widget->x() + m_part->widget()->frameGeometry().width();
    const int bottom = widget->y() + m_part->widget()->frameGeometry().height();
    int moveByX = 0;
    int moveByY = 0;
    if (right > sg.right())
        moveByX = - right + sg.right(); // always <0
    if (bottom > sg.bottom())
        moveByY = - bottom + sg.bottom(); // always <0
    if ((moveByX || moveByY) &&
        WebKitSettings::self()->windowMovePolicy(host) == WebKitSettings::KJSWindowMoveAllow) {
        emit extension->moveTopLevelWidget(widget->x() + moveByX, widget->y() + moveByY);
    }
}

void FakePage::slotMenuBarVisibilityChangeRequested(bool visible)
{
    // Do nothing if the new window type is WebBrowserWindow...
    if (m_type == WebBrowserWindow)
        return;

    KMainWindow* window = topLevelWindowForPart(m_part);
    if (window)
        window->menuBar()->setVisible(visible);
}

void FakePage::slotStatusBarVisibilityChangeRequested(bool visible)
{
    // Do nothing if the new window type is WebBrowserWindow...
    if (m_type == WebBrowserWindow)
        return;

    KMainWindow* window = topLevelWindowForPart(m_part);
    if (window)
        window->statusBar()->setVisible(visible);
}

void FakePage::slotToolBarVisibilityChangeRequested(bool visible)
{
    // Do nothing if the new window type is WebBrowserWindow...
    if (m_type == WebBrowserWindow)
        return;

    KMainWindow* window = topLevelWindowForPart(m_part);
    if (window) {
        QList<KToolBar*> toolBars = window->toolBars();
        Q_FOREACH(KToolBar* bar, toolBars)
            bar->setVisible(visible);
    }
}
