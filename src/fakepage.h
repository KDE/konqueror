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

#ifndef FAKEPAGE_H
#define FAKEPAGE_H

#include <QtCore/QPointer>
#include <QtWebKit/QWebPage>

class QRect;
namespace KParts {
    class ReadOnlyPart;
}

/**
 * This fake QWebPage implementation is used as a workaround for the limitations
 * of QWebPage::createWindow API. Specifically the fact that the request url is
 * not provided when it is invoked. That impeads our ability to provide the
 * request url to then newly created window when it is a non KWebKitPart part.
 *
 * See the WebPage::createWindow implementation in webpage.cpp for further details.
 */
class FakePage : public QWebPage
{
    Q_OBJECT
public:
    FakePage(KParts::ReadOnlyPart* part, WebWindowType type, QObject* parent = 0);
    virtual ~FakePage();

protected:
    virtual bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);
    
private Q_SLOTS:
    void slotGeometryChangeRequested(const QRect& rect);
    void slotMenuBarVisibilityChangeRequested(bool visible);
    void slotStatusBarVisibilityChangeRequested(bool visible);
    void slotToolBarVisibilityChangeRequested(bool visible);

private:
    QPointer<KParts::ReadOnlyPart> m_part;
    WebWindowType m_type;
};

#endif
