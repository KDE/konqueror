/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "downloaderinterface.h"

QString DownloaderInterface::requestDownloadByPartKey()
{
    static QString s_requestDownloadByPartKey = QStringLiteral("DownloadWillBePerformedByPart");
    return s_requestDownloadByPartKey;
}

QString DownloaderInterface::jobIDKey()
{
    static QString s_jobIDKey = QStringLiteral("JobID");
    return s_jobIDKey;
}

DownloaderInterface::~DownloaderInterface()
{
}

DownloaderInterface * DownloaderInterface::interface(QObject* obj)
{
    if (!obj) {
        return nullptr;
    }
    DownloaderInterface *iface = dynamic_cast<DownloaderInterface*>(obj);
    if (iface) {
        return iface;
    }
    QList<QObject*> children = obj->findChildren<QObject*>();
    for (auto c : children) {
        iface = dynamic_cast<DownloaderInterface*>(c);
        if (iface) {
            return iface;
        }
    }
    return nullptr;
}

DownloaderJob::DownloaderJob(QObject* parent) : KJob(parent)
{
}
