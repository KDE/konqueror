// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "faviconupdater.h"

#include "bookmarkiterator.h"
#include "toplevel.h"

#include <kdebug.h>
#include <klocale.h>

#include <kio/job.h>

#include <kmimetype.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kmimetypetrader.h>

FavIconUpdater::FavIconUpdater(QObject *parent)
    : QObject(parent),
      m_favIconModule("org.kde.kded", "/modules/favicons", QDBusConnection::sessionBus())
{
    connect(&m_favIconModule, SIGNAL(iconChanged(bool,QString,QString)),
            this, SLOT(notifyChange(bool,QString,QString)) );
    connect(&m_favIconModule, SIGNAL(error(bool,QString,QString)),
            this, SLOT(slotFavIconError(bool,QString,QString)) );
    m_part = 0;
    m_webGrabber = 0;
    m_browserIface = 0;
}

void FavIconUpdater::downloadIcon(const KBookmark &bk)
{
    m_bk = bk;
    const QString url = bk.url().url();
    const QString favicon = KMimeType::favIconForUrl(url);
    if (!favicon.isEmpty()) {
        kDebug() << "got favicon" << favicon;
        m_bk.setIcon(favicon);
        KEBApp::self()->notifyCommandExecuted();
        // kDebug() << "emit done(true)";
        emit done(true, QString());

    } else {
        kDebug() << "no favicon found";
        webupdate = false;
        m_favIconModule.forceDownloadHostIcon(url);
    }
}

FavIconUpdater::~FavIconUpdater()
{
    delete m_browserIface;
    delete m_webGrabber;
    delete m_part;
}

void FavIconUpdater::downloadIconUsingWebBrowser(const KBookmark &bk, const QString& currentError)
{
    kDebug();
    m_bk = bk;
    webupdate = true;

    if (!m_part) {
        QString partLoadingError;
        KParts::ReadOnlyPart *part
            = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>("text/html", 0, this, QString(), QVariantList(), &partLoadingError);
        if (!part) {
            emit done(false, i18n("%1; no HTML component found (%2)", currentError, partLoadingError));
            return;
        }

        part->setProperty("pluginsEnabled", QVariant(false));
        part->setProperty("javaScriptEnabled", QVariant(false));
        part->setProperty("javaEnabled", QVariant(false));
        part->setProperty("autoloadImages", QVariant(false));

        KParts::BrowserExtension *ext = KParts::BrowserExtension::childObject(part);
        Q_ASSERT(ext);

        // TODO: what is this useful for?
        m_browserIface = new FavIconBrowserInterface(this);
        ext->setBrowserInterface(m_browserIface);

        connect(ext, SIGNAL(setIconUrl(KUrl)),
                this, SLOT(setIconUrl(KUrl)));

        m_part = part;
    }

    // The part isn't created by the webgrabber so that we can create the part
    // only once.
    delete m_webGrabber;
    m_webGrabber = new FavIconWebGrabber(m_part, bk.url());
    connect(m_webGrabber, SIGNAL(done(bool,QString)), this, SIGNAL(done(bool,QString)));
}

// khtml callback
void FavIconUpdater::setIconUrl(const KUrl &iconURL)
{
    m_favIconModule.setIconForUrl(m_bk.url().url(), iconURL.url());
    // The above call will make the kded module start the download and emit iconChanged or error.

    delete m_webGrabber;
    m_webGrabber = 0;
}

bool FavIconUpdater::isFavIconSignalRelevant(bool isHost, const QString& hostOrURL) const
{
    // Is this signal interesting to us? (Don't react on an unrelated favicon)
    return (isHost && hostOrURL == m_bk.url().host()) ||
        (!isHost && hostOrURL == m_bk.url().url()); // should we use the api that ignores trailing slashes?
}

void FavIconUpdater::notifyChange(bool isHost,
                                  const QString& hostOrURL,
                                  const QString& iconName)
{
    kDebug() << hostOrURL << iconName;
    if (isFavIconSignalRelevant(isHost, hostOrURL)) {
        if (iconName.isEmpty()) { // old version of the kded module could emit with an empty iconName on error
            slotFavIconError(isHost, hostOrURL, QString());
        } else {
            m_bk.setIcon(iconName);
            emit done(true, QString());
        }
    }
}

void FavIconUpdater::slotFavIconError(bool isHost, const QString& hostOrURL, const QString& errorString)
{
    kDebug() << hostOrURL << errorString;
    if (isFavIconSignalRelevant(isHost, hostOrURL)) {
        if (!webupdate) {
            // no icon found, try webupdater
            downloadIconUsingWebBrowser(m_bk, errorString);
        } else {
            // already tried webupdater
            emit done(false, errorString);
        }
    }
}

/* -------------------------- */

FavIconWebGrabber::FavIconWebGrabber(KParts::ReadOnlyPart *part, const KUrl &url)
    : m_part(part), m_url(url) {

    //FIXME only connect to result?
//  connect(part, SIGNAL(result(KIO::Job*job)),
//          this, SLOT(slotCompleted()));
    connect(part, SIGNAL(canceled(QString)),
            this, SLOT(slotCanceled(QString)));
    connect(part, SIGNAL(completed(bool)),
            this, SLOT(slotCompleted()));

    // the use of KIO rather than directly using KHTML is to allow silently abort on error
    // TODO: an alternative would be to derive from KHTMLPart and reimplement showError(KJob*).

    kDebug() << "starting KIO::get() on" << m_url;
    KIO::Job *job = KIO::get(m_url, KIO::NoReload, KIO::HideProgressInfo);
    job->addMetaData( QString("cookies"), QString("none") );
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(slotFinished(KJob*)));
    connect(job, SIGNAL(mimetype(KIO::Job*,QString)),
            this, SLOT(slotMimetype(KIO::Job*,QString)));
}

void FavIconWebGrabber::slotMimetype(KIO::Job *job, const QString & /*type*/) {
    KIO::SimpleJob *sjob = static_cast<KIO::SimpleJob *>(job);
    m_url = sjob->url(); // allow for redirection
    sjob->putOnHold();

    // QString typeLocal = typeUncopied; // local copy
    // kDebug() << "slotMimetype : " << typeLocal;
    // TODO - what to do if typeLocal is not text/html ??

    m_part->openUrl(m_url);
}

void FavIconWebGrabber::slotFinished(KJob *job)
{
    if (job->error()) {
        kDebug() << job->errorString();
        emit done(false, job->errorString());
    }
    // On success mimetype was emitted, so no need to do anything.
}

void FavIconWebGrabber::slotCompleted()
{
    kDebug();
    emit done(true, QString());
}

void FavIconWebGrabber::slotCanceled(const QString& errorString)
{
    kDebug() << errorString;
    emit done(false, errorString);
}

#include "faviconupdater.moc"
