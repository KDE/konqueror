/* This file is part of the KDE project
   Copyright (C) 2001 Malte Starostik <malte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "favicons.h"
#include "favicons_adaptor.h"

#include <kicontheme.h>
#include <kconfig.h>
#include <klocale.h>
#include <kde_file.h>
#include <kstandarddirs.h>
#include <kio/job.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QCache>
#include <QtCore/QTimer>
#include <QImage>
#include <QImageReader>

#include <ctime>

static int dfavi() { static int s_area = KDebug::registerArea("FavIcons", false); return s_area; }

K_PLUGIN_FACTORY(FavIconsFactory,
                 registerPlugin<FavIconsModule>();
    )
K_EXPORT_PLUGIN(FavIconsFactory("favicons"))

static QString portForUrl(const KUrl& url)
{
    if (url.port() > 0) {
        return (QString(QLatin1Char('_')) + QString::number(url.port()));
    }
    return QString();
}

static QString simplifyURL(const KUrl &url)
{
    // splat any = in the URL so it can be safely used as a config key
    QString result = url.host() + portForUrl(url) + url.path();
    for (int i = 0; i < result.length(); ++i)
        if (result[i] == '=')
            result[i] = '_';
    return result;
}

static QString iconNameFromURL(const KUrl &iconURL)
{
    if (iconURL.path() == QLatin1String("/favicon.ico"))
       return iconURL.host() + portForUrl(iconURL);

    QString result = simplifyURL(iconURL);
    // splat / so it can be safely used as a file name
    for (int i = 0; i < result.length(); ++i)
        if (result[i] == '/')
            result[i] = '_';

    QString ext = result.right(4);
    if (ext == QLatin1String(".ico") || ext == QLatin1String(".png") || ext == QLatin1String(".xpm"))
        result.remove(result.length() - 4, 4);

    return result;
}

struct FavIconsModulePrivate
{
    virtual ~FavIconsModulePrivate() { delete config; }

    struct DownloadInfo
    {
        QString hostOrURL;
        bool isHost;
        QByteArray iconData;
    };
    QString makeIconName(const DownloadInfo& download, const KUrl& iconURL)
    {
        QString iconName (QLatin1String("favicons/"));
        iconName += (download.isHost ? download.hostOrURL : iconNameFromURL(iconURL));
        return iconName;
    }

    QMap<KJob *, DownloadInfo> downloads;
    KUrl::List failedDownloads;
    KConfig *config;
    QList<KIO::Job*> killJobs;
    KIO::MetaData metaData;
    QString faviconsDir;
    QCache<QString,QString> faviconsCache;
};

FavIconsModule::FavIconsModule(QObject* parent, const QList<QVariant>&)
    : KDEDModule(parent)
{
    // create our favicons folder so that KIconLoader knows about it
    d = new FavIconsModulePrivate;
    d->faviconsDir = KGlobal::dirs()->saveLocation( "cache", QLatin1String("favicons/"));
    d->faviconsDir.truncate(d->faviconsDir.length()-9); // Strip off "favicons/"
    d->metaData.insert(QLatin1String("ssl_no_client_cert"), QLatin1String("true"));
    d->metaData.insert(QLatin1String("ssl_no_ui"), QLatin1String("true"));
    d->metaData.insert(QLatin1String("UseCache"), "false");
    d->metaData.insert(QLatin1String("cookies"), "none");
    d->metaData.insert(QLatin1String("no-www-auth"), QLatin1String("true"));
    d->metaData.insert(QLatin1String("errorPage"), QLatin1String("false"));
    d->config = new KConfig(KStandardDirs::locateLocal("data", QLatin1String("konqueror/faviconrc")));

    new FavIconsAdaptor( this );
}

FavIconsModule::~FavIconsModule()
{
    delete d;
}

static QString removeSlash(QString result)
{
    for (unsigned int i = result.length() - 1; i > 0; --i) {
        if (result[i] != '/') {
            result.truncate(i + 1);
            break;
        }
    }

    return result;
}

QString FavIconsModule::iconForUrl(const KUrl &url)
{
    if (url.host().isEmpty())
        return QString();

    //kDebug(dfavi()) << url;

    const QString simplifiedURL = removeSlash(simplifyURL(url));
    QString *iconURL = d->faviconsCache[simplifiedURL];
    QString icon = (iconURL ? *iconURL : d->config->group(QString()).readEntry(simplifiedURL, QString()));

    if (!icon.isEmpty())
        icon = iconNameFromURL(KUrl(icon));
    else
        icon = url.host();

    icon = QLatin1String("favicons/") + icon;

    kDebug(dfavi()) << "URL:" << url << "ICON:" << icon;

    if (QFile::exists(d->faviconsDir+icon+QLatin1String(".png")))
        return icon;

    return QString();
}

bool FavIconsModule::isIconOld(const QString &icon)
{
    KDE_struct_stat st;
    if (KDE::stat(QFile::encodeName(icon), &st) != 0) {
        //kDebug(dfavi()) << "isIconOld" << icon << "yes, no such file";
        return true; // Trigger a new download on error
    }

    //kDebug(dfavi()) << "isIconOld" << icon << "?";
    return (time(0) - st.st_mtime) > 604800; // arbitrary value (one week)
}

void FavIconsModule::setIconForUrl(const KUrl &url, const KUrl &iconURL)
{
    //kDebug(dfavi()) << url << iconURL;
    const QString simplifiedURL = simplifyURL(url);

    d->faviconsCache.insert(removeSlash(simplifiedURL), new QString(iconURL.url()) );

    const QString iconName = QLatin1String("favicons/") + iconNameFromURL(iconURL);
    const QString iconFile = d->faviconsDir + iconName + QLatin1String(".png");

    if (!isIconOld(iconFile)) {
        //kDebug(dfavi()) << "emit iconChanged" << false << url << iconName;
        emit iconChanged(false, url.url(), iconName);
        return;
    }

    startDownload(url.url(), false, iconURL);
}

void FavIconsModule::downloadHostIcon(const KUrl &url)
{
    //kDebug(dfavi()) << url;
    const QString iconFile = d->faviconsDir + QLatin1String("favicons/") + url.host() + QLatin1String(".png");
    if (!isIconOld(iconFile)) {
        //kDebug(dfavi()) << "not old -> doing nothing";
        return;
    }
    startDownload(url.host(), true, KUrl(url, QLatin1String("/favicon.ico")));
}

void FavIconsModule::forceDownloadHostIcon(const KUrl &url)
{
    //kDebug(dfavi()) << url;
    KUrl iconURL = KUrl(url, QLatin1String("/favicon.ico"));
    d->failedDownloads.removeAll(iconURL); // force a download to happen
    startDownload(url.host(), true, iconURL);
}

void FavIconsModule::startDownload(const QString &hostOrURL, bool isHost, const KUrl &iconURL)
{
    if (d->failedDownloads.contains(iconURL)) {
        //kDebug(dfavi()) << iconURL << "already in failedDownloads, emitting error";
        emit error(isHost, hostOrURL, i18n("No favicon found"));
        return;
    }

    //kDebug(dfavi()) << iconURL;
    KIO::Job *job = KIO::get(iconURL, KIO::NoReload, KIO::HideProgressInfo);
    job->addMetaData(d->metaData);
    connect(job, SIGNAL(data(KIO::Job*,QByteArray)), SLOT(slotData(KIO::Job*,QByteArray)));
    connect(job, SIGNAL(result(KJob*)), SLOT(slotResult(KJob*)));
    connect(job, SIGNAL(infoMessage(KJob*,QString,QString)), SLOT(slotInfoMessage(KJob*,QString)));
    FavIconsModulePrivate::DownloadInfo download;
    download.hostOrURL = hostOrURL;
    download.isHost = isHost;
    d->downloads.insert(job, download);
}

void FavIconsModule::slotData(KIO::Job *job, const QByteArray &data)
{
    KIO::TransferJob* tjob = static_cast<KIO::TransferJob*>(job);
    FavIconsModulePrivate::DownloadInfo &download = d->downloads[job];
    unsigned int oldSize = download.iconData.size();
    // Size limit. Stop downloading if the file is huge.
    // Testcase (as of june 2008, at least): http://planet-soc.com/favicon.ico, 136K and strange format.
    if (oldSize > 0x10000) {
        kDebug(dfavi()) << "Favicon too big, aborting download of" << tjob->url();
        d->killJobs.append(job);
        QTimer::singleShot(0, this, SLOT(slotKill()));
        const KUrl iconURL = tjob->url();
        d->failedDownloads.append(iconURL);
    }
    download.iconData.resize(oldSize + data.size());
    memcpy(download.iconData.data() + oldSize, data.data(), data.size());
}

void FavIconsModule::slotResult(KJob *job)
{
    KIO::TransferJob* tjob = static_cast<KIO::TransferJob*>(job);
    FavIconsModulePrivate::DownloadInfo download = d->downloads[job];
    d->killJobs.removeAll(tjob);
    d->downloads.remove(job);
    const KUrl iconURL = tjob->url();
    QString iconName;
    QString errorMessage;
    if (!job->error())
    {
        QBuffer buffer(&download.iconData);
        buffer.open(QIODevice::ReadOnly);
        QImageReader ir( &buffer );
        QSize desired( 16,16 );
        if( ir.canRead() ) {
            while( ir.imageCount() > 1
              && ir.currentImageRect() != QRect(0, 0, desired.width(), desired.height())) {
                if (!ir.jumpToNextImage()) {
                    break;
                }
            }
            ir.setScaledSize( desired );
            const QImage img = ir.read();
            if( !img.isNull() ) {
                iconName = d->makeIconName(download, iconURL);
                const QString localPath = d->faviconsDir + iconName + QLatin1String(".png");
                if( !img.save(localPath, "PNG") ) {
                    iconName.clear();
                    errorMessage = i18n("Error saving image to %1", localPath);
                } else if (!download.isHost)
                    d->config->group(QString()).writeEntry(removeSlash(download.hostOrURL), iconURL.url());
            }
        }
    } else {
        errorMessage = job->errorString();
    }
    if (iconName.isEmpty()) {
        //kDebug(dfavi()) << "adding" << iconURL << "to failed downloads";
        d->failedDownloads.append(iconURL);
        emit error(download.isHost, download.hostOrURL, errorMessage);
    } else {
        //kDebug(dfavi()) << "emit iconChanged" << download.isHost << download.hostOrURL << iconName;
        emit iconChanged(download.isHost, download.hostOrURL, iconName);
    }
}

void FavIconsModule::slotInfoMessage(KJob *job, const QString &msg)
{
    emit infoMessage(static_cast<KIO::TransferJob *>( job )->url().url(), msg);
}

void FavIconsModule::slotKill()
{
    //kDebug(dfavi());
    Q_FOREACH(KIO::Job* job, d->killJobs) {
        job->kill();
    }
    d->killJobs.clear();
}

#include "favicons.moc"
