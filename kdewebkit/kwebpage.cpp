/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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
 *
 */

// Own
#include "kwebpage.h"
#include "kwebwallet.h"

// Local
#include "networkaccessmanager_p.h"
#include "networkcookiejar_p.h"
#include "kwebpluginfactory.h"

// KDE
#include <kaction.h>
#include <kfiledialog.h>
#include <kprotocolmanager.h>
#include <kjobuidelegate.h>
#include <krun.h>
#include <kstandarddirs.h>
#include <kstandardshortcut.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kio/accessmanager.h>
#include <kio/job.h>
#include <kio/renamedlg.h>

// Qt
#include <QtCore/QPointer>
#include <QtCore/QFileInfo>
#include <QtWebKit/QWebFrame>


#define QL1S(x)  QLatin1String(x)


class KWebPage::KWebPagePrivate
{
public:
    QPointer<KWebWallet> wallet;
};


KWebPage::KWebPage(QObject *parent, Integration flags)
         :QWebPage(parent), d(new KWebPagePrivate)
{
  // KDE KParts integration for <embed> tag...
  if (!flags || (flags & KPartsIntegration))
      setPluginFactory(new KWebPluginFactory(this));

  // KDE IO (KIO) integration...
  if (!flags || (flags & KIOIntegration)) {
      KDEPrivate::NetworkAccessManager *manager = new KDEPrivate::NetworkAccessManager(this);

      QWidget *widget = qobject_cast<QWidget*>(parent);
      if (widget && widget->window())
          manager->setCookieJarWindowId(widget->window()->winId());
      setNetworkAccessManager(manager);
  }

  // KWallet integration...
  if (!flags || (flags & KWalletIntegration))
      setWallet(new KWebWallet);

  action(Back)->setIcon(KIcon("go-previous"));
  action(Forward)->setIcon(KIcon("go-next"));
  action(Reload)->setIcon(KIcon("view-refresh"));
  action(Stop)->setIcon(KIcon("process-stop"));
  action(Cut)->setIcon(KIcon("edit-cut"));
  action(Copy)->setIcon(KIcon("edit-copy"));
  action(Paste)->setIcon(KIcon("edit-paste"));
  action(Undo)->setIcon(KIcon("edit-undo"));
  action(Redo)->setIcon(KIcon("edit-redo"));
  action(InspectElement)->setIcon(KIcon("view-process-all"));
  action(OpenLinkInNewWindow)->setIcon(KIcon("window-new"));
  action(OpenFrameInNewWindow)->setIcon(KIcon("window-new"));
  action(OpenImageInNewWindow)->setIcon(KIcon("window-new"));
  action(CopyLinkToClipboard)->setIcon(KIcon("edit-copy"));
  action(CopyImageToClipboard)->setIcon(KIcon("edit-copy"));
  action(ToggleBold)->setIcon(KIcon("format-text-bold"));
  action(ToggleItalic)->setIcon(KIcon("format-text-italic"));
  action(ToggleUnderline)->setIcon(KIcon("format-text-underline"));
  action(DownloadLinkToDisk)->setIcon(KIcon("document-save"));
  action(DownloadImageToDisk)->setIcon(KIcon("document-save"));

  settings()->setWebGraphic(QWebSettings::MissingPluginGraphic, KIcon("preferences-plugin").pixmap(32, 32));
  settings()->setWebGraphic(QWebSettings::MissingImageGraphic, KIcon("image-missing").pixmap(32, 32));
  settings()->setWebGraphic(QWebSettings::DefaultFrameIconGraphic, KIcon("applications-internet").pixmap(32, 32));

  action(Back)->setShortcut(KStandardShortcut::back().primary());
  action(Forward)->setShortcut(KStandardShortcut::forward().primary());
  action(Reload)->setShortcut(KStandardShortcut::reload().primary());
  action(Stop)->setShortcut(Qt::Key_Escape);
  action(Cut)->setShortcut(KStandardShortcut::cut().primary());
  action(Copy)->setShortcut(KStandardShortcut::copy().primary());
  action(Paste)->setShortcut(KStandardShortcut::paste().primary());
  action(Undo)->setShortcut(KStandardShortcut::undo().primary());
  action(Redo)->setShortcut(KStandardShortcut::redo().primary());
  action(SelectAll)->setShortcut(KStandardShortcut::selectAll().primary());
}

KWebPage::~KWebPage()
{
    delete d;
}

bool KWebPage::isExternalContentAllowed() const
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        return manager->isExternalContentAllowed();
    return true;
}

KWebWallet *KWebPage::wallet() const
{
    return d->wallet;
}

void KWebPage::setAllowExternalContent(bool allow)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        manager->setExternalContentAllowed(allow);
}

void KWebPage::setWallet(KWebWallet* wallet)
{
    // Delete the current wallet if this object is its parent...
    if (d->wallet && this == d->wallet->parent())
        delete d->wallet;

    d->wallet = wallet;

    if (d->wallet)
        d->wallet->setParent(this);
}

void KWebPage::downloadRequest(const QNetworkRequest &request)
{
    KUrl destUrl;
    KUrl srcUrl (request.url());
    int result = KIO::R_OVERWRITE;

    do {
        destUrl = KFileDialog::getSaveFileName(srcUrl.fileName(), QString(), view());

        if (destUrl.isLocalFile()) {
            QFileInfo finfo (destUrl.toLocalFile());
            if (finfo.exists()) {
                QDateTime now = QDateTime::currentDateTime();
                KIO::RenameDialog dlg (view(), i18n("Overwrite File?"), srcUrl, destUrl,
                                       KIO::RenameDialog_Mode(KIO::M_OVERWRITE | KIO::M_SKIP),
                                       -1, finfo.size(),
                                       now.toTime_t(), finfo.created().toTime_t(),
                                       now.toTime_t(), finfo.lastModified().toTime_t());
                result = dlg.exec();
            }
        }
    } while (result == KIO::R_CANCEL && destUrl.isValid());

    if (result == KIO::R_OVERWRITE && destUrl.isValid()) {
        KIO::Job *job = KIO::file_copy(srcUrl, destUrl, -1, KIO::Overwrite);
        QVariant attr = request.attribute(static_cast<QNetworkRequest::Attribute>(KDEPrivate::NetworkAccessManager::MetaData));
        if (attr.isValid() && attr.type() == QVariant::Map)
            job->setMetaData(KIO::MetaData(attr.toMap()));

        job->addMetaData(QL1S("MaxCacheSize"), QL1S("0")); // Don't store in http cache.
        job->addMetaData(QL1S("cache"), QL1S("cache")); // Use entry from cache if available.
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void KWebPage::downloadUrl(const KUrl &url)
{
    QNetworkRequest request (url);
    downloadRequest(request);
}

QString KWebPage::sessionMetaData(const QString &key) const
{
    QString value;

    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        value = manager->sessionMetaData().value(key);

    return value;
}

QString KWebPage::requestMetaData(const QString &key) const
{
    QString value;

    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        value = manager->requestMetaData().value(key);

    return value;
}

void KWebPage::setSessionMetaData(const QString &key, const QString &value)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        manager->sessionMetaData()[key] = value;
}

void KWebPage::setRequestMetaData(const QString &key, const QString &value)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        manager->requestMetaData()[key] = value;
}

void KWebPage::removeSessionMetaData(const QString &key)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        manager->sessionMetaData().remove(key);
}

void KWebPage::removeRequestMetaData(const QString &key)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager *>(networkAccessManager());
    if (manager)
        manager->requestMetaData().remove(key);
}

QString KWebPage::userAgentForUrl(const QUrl& _url) const
{
    const KUrl url(_url);
    QString userAgent = KProtocolManager::userAgentForHost((url.isLocalFile() ? "localhost" : url.host()));

    if (userAgent == KProtocolManager::defaultUserAgent())
        return QWebPage::userAgentForUrl(_url);

    return userAgent;
}

bool KWebPage::acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type)
{
    //kDebug() << "url:" << request.url() << ", type:" << type << ", frame:" << frame;

    if (frame && d->wallet && type == QWebPage::NavigationTypeFormSubmitted) {
        d->wallet->saveFormData(frame);
    }

    /*
      If the navigation request is from the main frame, set the cross-domain
      meta-data value to the current url for proper integration with KDE's
      cookieJar...
    */
    if (frame == mainFrame() && type != QWebPage::NavigationTypeReload) {
        setSessionMetaData(QL1S("cross-domain"), request.url().toString());
    }

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

#include "kwebpage.moc"
