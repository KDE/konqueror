// This file is part of the KDE project
// SPDX-FileCopyrightText: 2025 Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "konqspeeddial.h"
#include "konqsettings.h"

#include <KIO/FavIconRequestJob>
#include <KIO/StoredTransferJob>
#include <KIconLoader>

#include <QPixmap>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>

using namespace KonqInterfaces;
using namespace Qt::Literals::StringLiterals;

KonqSpeedDial::KonqSpeedDial(QObject* parent) : SpeedDial(parent)
{
    QDir dataDir = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    const QString iconSubdir = u"speed_dial_icons"_s;
    m_cacheDir = dataDir.absoluteFilePath(iconSubdir);
    if (!dataDir.exists(iconSubdir)) {
        dataDir.mkpath(iconSubdir);
    }
}

KonqSpeedDial::~KonqSpeedDial() noexcept
{
}

KonqSpeedDial::Entries KonqSpeedDial::entries() const
{
    return Konq::Settings::self()->speedDialEntries();
}

QUrl KonqSpeedDial::fromTheme(const QString& name, int size)
{
    QList<int> allSizes = {
        KIconLoader::NoGroup,
        KIconLoader::Desktop,
        KIconLoader::FirstGroup,
        KIconLoader::Toolbar,
        KIconLoader::MainToolbar,
        KIconLoader::Small,
        KIconLoader::Panel,
        KIconLoader::Dialog,
        KIconLoader::LastGroup,
        KIconLoader::User,
        KIconLoader::SizeSmall,
        KIconLoader::SizeSmallMedium,
        KIconLoader::SizeMedium,
        KIconLoader::SizeLarge,
        KIconLoader::SizeHuge,
        KIconLoader::SizeEnormous
    };
    allSizes.removeOne(size);
    allSizes.prepend(size);
    for (int size : allSizes) {
        QString path = KIconLoader::global()->iconPath(name, size);
        if (!path.isEmpty()) {
            return QUrl::fromLocalFile(path);
        }
    }
    return {};
}

KonqSpeedDial::IconType KonqSpeedDial::iconType(const Entry& entry)
{
    QUrl iconUrl = entry.iconUrl;
    if (iconUrl.isLocalFile()) {
        return LocalUrl;
    }
    if (iconUrl.isEmpty()) {
        return Favicon;
    }
    if (iconUrl.scheme().isEmpty()) {
        return iconUrl.isRelative() ? IconName : LocalFile;
    }
    return RemoteIcon;
}

QUrl KonqSpeedDial::localIconUrlForEntry(const Entry& entry, int size, bool download)
{
    IconType type = iconType(entry);
    QString cachedPath;
    switch (type) {
        case LocalUrl:
            return entry.iconUrl;
        case LocalFile:
            return QUrl::fromLocalFile(entry.iconUrl.path());
        case IconName:
            return fromTheme(entry.iconUrl.path(), size);
        case Favicon:
            cachedPath = KIO::favIconForUrl(entry.url);
            break;
        case RemoteIcon:
            cachedPath = cacheFilePath(entry.iconUrl);
            break;
    }
    if (QFileInfo::exists(cachedPath)) {
        return QUrl::fromLocalFile(cachedPath);
    }

    if (download) {
        type == Favicon ? downloadFavicon(entry) : downloadRemoteIcon(entry);
    }

    return {};
}

void KonqSpeedDial::downloadFavicon(const Entry& entry)
{
    KIO::FavIconRequestJob *job = new KIO::FavIconRequestJob(entry.url);
    auto faviconDownloaded = [job, this, entry] (KJob *) {
        emit iconReady(entry, QUrl::fromLocalFile(job->iconFile()));
    };
    connect(job, &KIO::FavIconRequestJob::result, this, faviconDownloaded);
    job->start();
}

QString KonqSpeedDial::cacheFilePath(const QUrl& iconUrl) const
{
    QFileInfo info(iconUrl.path());
    QString path = m_cacheDir + '/' + iconUrl.host() + u"_"_s + iconUrl.path().replace('/', '_');
    if (info.suffix() != u".png"_s) {
        path.append(u".png"_s);
    }
    return path;
}

void KonqSpeedDial::downloadIcon(const Entry& entry)
{
    switch (iconType(entry)) {
        case Favicon:
            downloadFavicon(entry);
            break;
        case RemoteIcon:
            downloadRemoteIcon(entry);
            break;
        default:
            break;
    }
}

void KonqSpeedDial::downloadRemoteIcon(const Entry& entry)
{
    KIO::StoredTransferJob *job = KIO::storedGet(entry.iconUrl);
    auto iconDownloaded = [job, this, entry] (KJob *) {
        if (!job) {
            return;
        }
        QPixmap pix;
        pix.loadFromData(job->data());
        QString cachedFile = cacheFilePath(entry.iconUrl);
        pix.save(cachedFile, "PNG");
        emit iconReady(entry, QUrl::fromLocalFile(cachedFile));
    };
    connect(job, &KIO::StoredTransferJob::result, this, iconDownloaded);
    job->start();
}

bool KonqSpeedDial::isIconReady(const Entry &entry)
{
    switch (iconType(entry)) {
        case Favicon:
            return !KIO::favIconForUrl(entry.url).isEmpty();
        case RemoteIcon:
            return QFileInfo(cacheFilePath(entry.iconUrl)).exists();
        default:
            return true;
    }
}

void KonqSpeedDial::downloadAllIcons()
{
    Entries allEntries = entries();
    for (const Entry &e : allEntries) {
        if (!isIconReady(e)) {
            downloadIcon(e);
        }
    }
}

void KonqSpeedDial::addEntry(const Entry &entry, QObject *cause)
{
    Entries allEntries = entries();
    allEntries.append(entry);
    Konq::Settings::self()->setSpeedDialEntries(allEntries);
    Konq::Settings::self()->save();
    emit speedDialChanged(cause);
}

void KonqSpeedDial::setEntries(const QList<Entry> &entries, QObject *cause)
{
    Konq::Settings::self()->setSpeedDialEntries(entries);
    Konq::Settings::self()->save();
    emit speedDialChanged(cause);
}
