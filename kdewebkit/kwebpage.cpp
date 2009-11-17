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
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>
#include <kjobuidelegate.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kstandardshortcut.h>
#include <kurl.h>
#include <kdebug.h>
#include <klocalizedstring.h>
#include <kio/accessmanager.h>
#include <kio/job.h>

// Qt
#include <QtCore/QPointer>
#include <QtGui/QTextDocument>
#include <QtGui/QPaintEngine>
#include <QtWebKit/QWebFrame>
#include <QtUiTools/QUiLoader>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkCookieJar>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusReply>

#define QL1(x)    QLatin1String(x)


class KWebPage::KWebPagePrivate
{
public:
    QPointer<KWebWallet> wallet;
};

KWebPage::KWebPage(QObject *parent, qlonglong windowId)
         :QWebPage(parent), d(new KWebPage::KWebPagePrivate)
{  
    // KDE KParts integration for <embed> tag...
    setPluginFactory(new KWebPluginFactory(this));

    // KDE IO (KIO) integration...
    setNetworkAccessManager(new KDEPrivate::NetworkAccessManager(this));

    // KDE Cookiejar (KCookieJar) integration...
    KDEPrivate::NetworkCookieJar *cookiejar = new KDEPrivate::NetworkCookieJar;

    // If windowid is 0, the default, make a best effort attempt to try and
    // determine that value from the parent object.
    if (!windowId) {
        QWidget *widget = qobject_cast<QWidget*>(parent);
        if (widget)
            windowId = widget->window()->winId();
    }

    if (windowId) {
      cookiejar->setWindowId(windowId);
      setSessionMetaData(QL1("window-id"), QString::number(windowId));
    }

    networkAccessManager()->setCookieJar(cookiejar);

    // Create a wallet...
    setWallet(new KWebWallet);


#if QT_VERSION >= 0x040600
    action(Back)->setIcon(QIcon::fromTheme("go-previous"));
    action(Forward)->setIcon(QIcon::fromTheme("go-next"));
    action(Reload)->setIcon(QIcon::fromTheme("view-refresh"));
    action(Stop)->setIcon(QIcon::fromTheme("process-stop"));
    action(Cut)->setIcon(QIcon::fromTheme("edit-cut"));
    action(Copy)->setIcon(QIcon::fromTheme("edit-copy"));
    action(Paste)->setIcon(QIcon::fromTheme("edit-paste"));
    action(Undo)->setIcon(QIcon::fromTheme("edit-undo"));
    action(Redo)->setIcon(QIcon::fromTheme("edit-redo"));
    action(InspectElement)->setIcon(QIcon::fromTheme("view-process-all"));
    action(OpenLinkInNewWindow)->setIcon(QIcon::fromTheme("window-new"));
    action(OpenFrameInNewWindow)->setIcon(QIcon::fromTheme("window-new"));
    action(OpenImageInNewWindow)->setIcon(QIcon::fromTheme("window-new"));
    action(CopyLinkToClipboard)->setIcon(QIcon::fromTheme("edit-copy"));
    action(CopyImageToClipboard)->setIcon(QIcon::fromTheme("edit-copy"));
    action(ToggleBold)->setIcon(QIcon::fromTheme("format-text-bold"));
    action(ToggleItalic)->setIcon(QIcon::fromTheme("format-text-italic"));
    action(ToggleUnderline)->setIcon(QIcon::fromTheme("format-text-underline"));
    action(DownloadLinkToDisk)->setIcon(QIcon::fromTheme("document-save"));
    action(DownloadImageToDisk)->setIcon(QIcon::fromTheme("document-save"));

    settings()->setWebGraphic(QWebSettings::MissingPluginGraphic, QIcon::fromTheme("preferences-plugin").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::MissingImageGraphic, QIcon::fromTheme("image-missing").pixmap(32, 32));
    settings()->setWebGraphic(QWebSettings::DefaultFrameIconGraphic, QIcon::fromTheme("applications-internet").pixmap(32, 32));
#else
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
#endif

    action(Back)->setShortcut(KStandardShortcut::back().primary());
    action(Forward)->setShortcut(KStandardShortcut::forward().primary());
    action(Reload)->setShortcut(KStandardShortcut::reload().primary());
    action(Stop)->setShortcut(Qt::Key_Escape);
    action(Cut)->setShortcut(KStandardShortcut::cut().primary());
    action(Copy)->setShortcut(KStandardShortcut::copy().primary());
    action(Paste)->setShortcut(KStandardShortcut::paste().primary());
    action(Undo)->setShortcut(KStandardShortcut::undo().primary());
    action(Redo)->setShortcut(KStandardShortcut::redo().primary());
}

KWebPage::~KWebPage()
{
    delete d;
}

void KWebPage::setAllowExternalContent(bool allow)
{
    KIO::AccessManager *manager = qobject_cast<KIO::AccessManager*>(networkAccessManager());
    if (manager)
        manager->setExternalContentAllowed(allow);
}

bool KWebPage::isExternalContentAllowed() const
{
    KIO::AccessManager *manager = qobject_cast<KIO::AccessManager*>(networkAccessManager());
    if (manager)
        return manager->isExternalContentAllowed();
    return true;
}

QString KWebPage::sessionMetaData(const QString &key) const
{
    QString value;

    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        value = manager->sessionMetaData().value(key);

    return value;
}

QString KWebPage::requestMetaData(const QString &key) const
{
    QString value;

    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        value = manager->requestMetaData().value(key);

    return value;
}

void KWebPage::setSessionMetaData(const QString &key, const QString &value)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        manager->sessionMetaData()[key] = value;
}

void KWebPage::setRequestMetaData(const QString &key, const QString &value)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        manager->requestMetaData()[key] = value;
}

QString KWebPage::userAgentForUrl(const QUrl& _url) const
{
    const KUrl url(_url);
    QString userAgent = KProtocolManager::userAgentForHost((url.isLocalFile() ? "localhost" : url.host()));

    if (userAgent == KProtocolManager::defaultUserAgent())
        return QWebPage::userAgentForUrl(_url);

    return userAgent;
}

void KWebPage::removeSessionMetaData(const QString &key)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        manager->sessionMetaData().remove(key);
}

void KWebPage::removeRequestMetaData(const QString &key)
{
    KDEPrivate::NetworkAccessManager *manager = qobject_cast<KDEPrivate::NetworkAccessManager*>(networkAccessManager());
    if (manager)
        manager->requestMetaData().remove(key);
}

bool KWebPage::acceptNavigationRequest(QWebFrame * frame, const QNetworkRequest & request, NavigationType type)
{
    kDebug() << "url: " << request.url() << ", type: " << type << ", frame: " << frame;   

    if (d->wallet && (type == QWebPage::NavigationTypeFormSubmitted ||
        type == QWebPage::NavigationTypeFormResubmitted)) {
        d->wallet->saveFormData(frame);
    }
    /*
      If the navigation request is from the main frame, set the cross-domain
      meta-data value to the current url for proper integration with KCookieJar...
    */
    if (frame == mainFrame() && type != QWebPage::NavigationTypeReload) {
        setSessionMetaData(QL1("cross-domain"), request.url().toString());
    }



    return QWebPage::acceptNavigationRequest(frame, request, type);
}

bool KWebPage::authorizedRequest(const QUrl &url) const
{
    Q_UNUSED(url);
    return true;
}

void KWebPage::downloadRequest(const QNetworkRequest &request)
{
    KUrl url (request.url());

    const QString destUrl = KFileDialog::getSaveFileName(url.fileName(), QString(), view());

    if (destUrl.isEmpty())
        return;

    KIO::Job *job = KIO::file_copy(url, KUrl(destUrl), -1, KIO::Overwrite);
    QVariant attr = request.attribute(static_cast<QNetworkRequest::Attribute>(KIO::AccessManager::MetaData));
    if (attr.isValid() && attr.type() == QVariant::Map)
        job->setMetaData(KIO::MetaData(attr.toMap()));

    job->addMetaData(QL1("MaxCacheSize"), QL1("0")); // Don't store in http cache.
    job->addMetaData(QL1("cache"), QL1("cache")); // Use entry from cache if available.
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    downloadRequest(request.url());
}

void KWebPage::downloadRequest(const KUrl &url)
{
    QNetworkRequest request (url);
    downloadRequest(request);
}

KWebWallet *KWebPage::wallet() const
{
    return d->wallet;
}

void KWebPage::setWallet(KWebWallet* wallet)
{
    if (d->wallet) {
      // Only delete if we have ownership of the wallet...
      if (this == d->wallet->parent())
          delete d->wallet;
      else
          disconnect(this, 0, d->wallet, 0);
    }

    d->wallet = wallet;

    if (d->wallet) {
        d->wallet->setParent(this);
        connect(this, SIGNAL(restoreFrameStateRequested(QWebFrame*)),
                d->wallet, SLOT(restoreFormData(QWebFrame*)));
    }
}

#include "kwebpage.moc"
