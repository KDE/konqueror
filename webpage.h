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
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "kwebpage.h"

#include "webkitkde_export.h"

class QWebFrame;
class WebKitPart;

class WEBKITKDE_EXPORT WebPage : public KWebPage
{
    Q_OBJECT
public:
    WebPage(WebKitPart *wpart, QWidget *parent);

protected:
    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request,
                                         NavigationType type);

    virtual KWebPage *newWindow(WebWindowType type);

protected Q_SLOTS:
    void slotGeometryChangeRequested(const QRect &rect);
    void slotWindowCloseRequested();
    void slotStatusBarMessage(const QString &message);

private:
    WebKitPart *m_part;
};

#endif // WEBPAGE_H
