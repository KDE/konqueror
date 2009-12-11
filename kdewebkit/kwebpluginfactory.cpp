/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit @ kde.org>
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

#include "kwebpluginfactory.h"
#include "kwebpage.h"

#include <kmimetypetrader.h>
#include <kservicetypetrader.h>
#include <kmimetype.h>
#include <kdebug.h>

#include <kparts/part.h>

#include <QtCore/QListIterator>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QTimer>

#include <QtWebKit/QWebPluginFactory>
#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebView>

#define QL1S(x)  QLatin1String(x)

static bool excludedMimeType(const QString &type)
{
    // Let QtWebKit handle flash and java applets...
    return (type == QL1S("application/x-shockwave-flash") ||
            type == QL1S("application/futuresplash") ||
            type.startsWith(QL1S("application/x-java")));
}


KWebPluginFactory::KWebPluginFactory(QObject *parent)
                  :QWebPluginFactory(parent),
                   d(0)
{
}

KWebPluginFactory::~KWebPluginFactory()
{
}

QObject* KWebPluginFactory::create(const QString& _mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    // Only attempt to find a KPart for the supported mime types...
    QVariantList arguments;
    const int count = argumentNames.count();

    for (int i = 0; i < count; ++i) {
        arguments << argumentNames.at(i) + "=\"" + argumentValues.at(i) + '\"';
    }

    /*
       HACK: This is a big time hack to determine the mime-type from the url
       when no mime-type is provided. Since we do not want to make async calls
       through KIO::mimeType here, we resort to the hack below to determine
       mime-type from the request's filename.

       This hack is not full proof and might not always work. See the
       KMimeType::findByPath docs for details. It is however the best option
       to properly handle documents, images, and other resources embedded
       into html content with the <embed> tag when they lack the "type"
       attribute that specifies their mime-type.

       See the sample file "embed_tag_test.html" in the tests folder.
    */
    QString mimeType (_mimeType.trimmed());
    if (mimeType.isEmpty()) {
       KMimeType::Ptr ptr = KMimeType::findByPath(url.path());
       mimeType = ptr->name();
       kDebug() << "Updated mimetype to" << mimeType;
    }

    KParts::ReadOnlyPart* part = 0;
    // NOTE: To use the KDE nspluginviewer, comment out the if statement below.
    // We are still relying on the QtWebKit's version, because url navigation
    // from flash content does not work. However, the KDE version is out of
    // process and as such will not take down the browser when it crashes.
    if (!excludedMimeType(mimeType))
        part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimeType, 0, parent(), QString(), arguments);

    kDebug() << "Asked for" << mimeType << "plugin, got" << part;

    if (part) {
        QMap<QString, QString> metaData = part->arguments().metaData();
        QString urlStr = url.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);
        metaData.insert("PropagateHttpHeader", "true");
        metaData.insert("referrer", urlStr);
        metaData.insert("cross-domain", urlStr);
        metaData.insert("main_frame_request", "TRUE");
        metaData.insert("ssl_activate_warnings", "TRUE");

        KWebPage *page = qobject_cast<KWebPage *>(parent());

        if (page) {
            const QString scheme = page->mainFrame()->url().scheme();
            if (page && (QString::compare(scheme, QL1S("https"), Qt::CaseInsensitive) == 0 ||
                         QString::compare(scheme, QL1S("webdavs"), Qt::CaseInsensitive) == 0))
              metaData.insert("ssl_was_in_use", "TRUE");
            else
              metaData.insert("ssl_was_in_use", "FALSE");
        }

        KParts::OpenUrlArguments openUrlArgs = part->arguments();
        openUrlArgs.metaData() = metaData;
        openUrlArgs.setMimeType(mimeType);
        part->setArguments(openUrlArgs);
        part->openUrl(url);

        return part->widget();
    }

    return 0;
}

QList<KWebPluginFactory::Plugin> KWebPluginFactory::plugins() const
{
    QList<Plugin> plugins;
    return plugins;
}
