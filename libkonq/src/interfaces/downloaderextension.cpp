/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "downloaderextension.h"
#include "common.h"

#include <KIO/JobUiDelegateFactory>
#include <KJobTrackerInterface>
#include <KIO/JobTracker>
#include <KJobWidgets>

using namespace KonqInterfaces;

DownloaderExtension::DownloaderExtension(QObject* parent) : QObject(parent)
{
}

DownloaderExtension::~DownloaderExtension()
{
}

DownloaderExtension * DownloaderExtension::downloader(QObject* obj)
{
    return as<DownloaderExtension>(obj);
}

DownloaderJob::DownloaderJob(QObject* parent) : KJob(parent)
{
}

void KonqInterfaces::DownloaderJob::prepareDownloadJob(QWidget* widget, const QString& destPath)
{
    if (!destPath.isEmpty()) {
        setDownloadPath(destPath);
    }
    setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, widget));
    KJobWidgets::setWindow(this, widget);
    KJobTrackerInterface *t = KIO::getJobTracker();
    if (t) {
        t->registerJob(this);
    }
}
