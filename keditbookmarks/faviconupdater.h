/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef FAVICONUPDATER_H
#define FAVICONUPDATER_H

#include <kbookmark.h>
#include "favicon_interface.h" // org::kde::FavIcon

#include <kparts/part.h>
#include <kparts/browserinterface.h>

class FavIconWebGrabber : public QObject
{
    Q_OBJECT
public:
    FavIconWebGrabber(KParts::ReadOnlyPart *part, const KUrl &url);
    ~FavIconWebGrabber() {}

Q_SIGNALS:
    void done(bool succeeded, const QString& errorString);

private Q_SLOTS:
    void slotMimetype(KIO::Job *job, const QString &_type);
    void slotFinished(KJob *job);
    void slotCanceled(const QString& errorString);
    void slotCompleted();

private:
    KParts::ReadOnlyPart *m_part;
    KUrl m_url;
};

class FavIconBrowserInterface;

class FavIconUpdater : public QObject
{
    Q_OBJECT

public:
    FavIconUpdater(QObject *parent);
    ~FavIconUpdater();
    void downloadIcon(const KBookmark &bk);
    void downloadIconUsingWebBrowser(const KBookmark &bk, const QString& currentError);

private Q_SLOTS:
    void setIconUrl(const KUrl &iconURL);
    void notifyChange(bool isHost, const QString& hostOrURL, const QString& iconName);
    void slotFavIconError(bool isHost, const QString& hostOrURL, const QString& errorString);

Q_SIGNALS:
    void done(bool succeeded, const QString& error);

private:
    bool isFavIconSignalRelevant(bool isHost, const QString& hostOrURL) const;

private:
    KParts::ReadOnlyPart *m_part;
    FavIconBrowserInterface *m_browserIface;
    FavIconWebGrabber *m_webGrabber;
    KBookmark m_bk;
    bool webupdate;
    org::kde::FavIcon m_favIconModule;
};

class FavIconBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT
public:
    FavIconBrowserInterface(FavIconUpdater *view)
        : KParts::BrowserInterface(view), m_view(view) {
        ;
    }
private:
    FavIconUpdater *m_view;
};

#endif

