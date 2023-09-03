/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "downloaderextension.h"
#include "common.h"

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
