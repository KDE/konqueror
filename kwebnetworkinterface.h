/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
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
#ifndef KWEBNETWORKINTERFACE_H
#define KWEBNETWORKINTERFACE_H

#include <qwebnetworkinterface.h>
#include <qhash.h>

#include <KIO/MetaData>

namespace KIO
{
    class Job;
};
class KJob;

class KWebNetworkInterface : public QWebNetworkInterface
{
    Q_OBJECT
public:
    KWebNetworkInterface(QObject *parent);

    virtual void addJob(QWebNetworkJob *job);
    virtual void cancelJob(QWebNetworkJob *job);

    static KIO::MetaData metaDataForRequest(QHttpRequestHeader request);

private slots:
    void forwardJobData(KIO::Job *kioJob, const QByteArray &data);
    void forwardJobResult(KJob *kjob);

private:
    QHash<QWebNetworkJob *, KJob *> m_jobs;
};

#endif // KWEBNETWORKINTERFACE_H
