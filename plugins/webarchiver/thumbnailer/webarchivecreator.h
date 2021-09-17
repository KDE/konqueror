/*
    SPDX-FileCopyrightText: 2001 Malte Starostik <malte@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Marten <jjm@keelhaul.me.uk>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, see
    <http://www.gnu.org/licenses/>.
*/

#ifndef WEBARCHIVECREATOR_H
#define WEBARCHIVECREATOR_H

#include <qobject.h>
#ifdef THUMBNAIL_USE_WEBKIT
#include <qnetworkcookiejar.h>
#endif // THUMBNAIL_USE_WEBKIT

#include <kio/thumbcreator.h>


class QTemporaryDir;


class WebArchiveCreator : public QObject, public ThumbCreator
{
    Q_OBJECT

public:
    WebArchiveCreator();
    ~WebArchiveCreator() override;

    bool create(const QString &path, int width, int height, QImage &img) override;
    ThumbCreator::Flags flags() const override;

private slots:
    void slotLoadFinished(bool ok);

    void slotProcessingTimeout();
    void slotRenderTimer();

private:
    QTemporaryDir *m_tempDir;

    bool m_rendered;
    bool m_error;
};


#ifdef THUMBNAIL_USE_WEBKIT

class WebArchiveCreatorCookieJar : public QNetworkCookieJar
{
    Q_OBJECT

public:
    WebArchiveCreatorCookieJar(QObject *parent = nullptr);
    ~WebArchiveCreatorCookieJar() override = default;

    QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const override;
    bool insertCookie(const QNetworkCookie & cookie) override;
    bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url) override;
};

#endif // THUMBNAIL_USE_WEBKIT

#endif // WEBARCHIVECREATOR_H
