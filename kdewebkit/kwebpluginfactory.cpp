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

#include <kparts/part.h>
#include <kmimetypetrader.h>
#include <kservicetypetrader.h>
#include <kmimetype.h>
#include <kdebug.h>

#include <QWebPluginFactory>
#include <QWebView>
#include <QWebFrame>
#include <QStringList>
#include <QList>
#include <QListIterator>

#define QL1(x)  QLatin1String(x)


class KWebPluginFactory::KWebPluginFactoryPrivate
{
public:
    QStringList supportedMimeTypes;
    QList<KWebPluginFactory::Plugin> plugins;
};

KWebPluginFactory::KWebPluginFactory(QObject *parent)
                  :QWebPluginFactory(parent),
                   d(new KWebPluginFactory::KWebPluginFactoryPrivate)
{
}

KWebPluginFactory::~KWebPluginFactory()
{
  delete d;
}

QObject* KWebPluginFactory::create(const QString& _mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    QString mimeType (_mimeType.trimmed());

    /*
       HACK: This is a big time hack to determine the mime-type from the url
       when no mime-type is provided. Since we do not want to make async calls,
       (e.g. KIO::mimeType) here, we resort to the hack below to determine the
       mime-type it from the request's filename.

       NOTE: This hack is not full proof and might not always work. See the
       KMimeType::findByPath docs for details. It is however the best option
       to properly handle documents, images, and other resources embedded
       into html content with the <embed> tag when they lack the "type"
       attribute that specifies their mime-type.

       See the sample file "embed_tag_test.html" in the tests folder.
    */
    if (mimeType.isEmpty()) {
       KMimeType::Ptr ptr = KMimeType::findByPath(url.path());
       mimeType = ptr->name();
       kDebug() << "Changed mimetype from " << _mimeType << " to " << mimeType;
    }

    KParts::ReadOnlyPart* part = 0;

    // Only attempt to find a KPart for the supported mime types...
    if (d->supportedMimeTypes.contains(mimeType)) {

        QVariantList arguments;
        const int count = argumentNames.count();

        for(int i = 0; i < count; ++i) {
            arguments << argumentNames.at(i) + "=\"" + argumentValues.at(i) + '\"';
            ++i;
        }

        KWebPage *page = qobject_cast<KWebPage*>(parent());
        QWidget *view = 0;
        if (page)
          view = page->view();

        part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimeType, view, parent(), QString(), arguments);

        if (part) {
            QMap<QString, QString> metaData = part->arguments().metaData();
            QString urlStr = url.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);
            metaData.insert("PropagateHttpHeader", "true");
            metaData.insert("referrer", urlStr);
            metaData.insert("cross-domain", urlStr);
            metaData.insert("main_frame_request", "TRUE");
            metaData.insert("ssl_activate_warnings", "TRUE");

            const QString scheme = page->mainFrame()->url().scheme();
            if (page && (QString::compare(scheme, QL1("https"), Qt::CaseInsensitive) == 0 ||
                         QString::compare(scheme, QL1("webdavs"), Qt::CaseInsensitive) == 0))
              metaData.insert("ssl_was_in_use", "TRUE");
            else
              metaData.insert("ssl_was_in_use", "FALSE");

            KParts::OpenUrlArguments openUrlArgs = part->arguments();
            openUrlArgs.metaData() = metaData;
            openUrlArgs.setMimeType(mimeType);
            part->setArguments(openUrlArgs);

            kDebug() << part->arguments().metaData();
            part->openUrl(url);
        }
    }

    kDebug() << "Asked for" << mimeType << "plugin, got" << part;

    if (part)
      return part->widget();

    return part;
}

QList<KWebPluginFactory::Plugin> KWebPluginFactory::plugins() const
{
    if (!d->plugins.isEmpty())
      return d->plugins;

    QList<Plugin> plugins;
    QStringList supportedMimeTypes;
    KService::List services = KServiceTypeTrader::self()->query("KParts/ReadOnlyPart");

    for (int i = 0; i < services.size(); i++) {
        KService::Ptr s = services.at(i);
        /*
          NOTE: We skip over the part that handles Adobe Flash (nspluginpart)
          here because it has issues when embeded into QtWebKit. Hence we defer
          the handling of flash content to QtWebKit's own builtin flash viewer.
        */
        if (s->hasMimeType(KMimeType::mimeType("application/x-shockwave-flash").data()))
          continue;
        Plugin plugin;
        plugin.name = s->desktopEntryName();
        plugin.description = s->comment();
        QList<MimeType> mimes;
        QStringList servicetypes = s->serviceTypes();
        for (int z = 0; z < servicetypes.size(); z++) {
            MimeType mime;
            mime.name = servicetypes.at(z);
            KMimeType::Ptr kmime = KMimeType::mimeType(mime.name);
            if (kmime) {
                mime.fileExtensions = kmime->patterns().replaceInStrings("*.", "");
            }
            mimes.append(mime);
            supportedMimeTypes << mime.name;
        }
        //kDebug() << "Adding plugin: " << s->desktopEntryName() << servicetypes;
        plugins.append(plugin);
    }

    d->plugins = plugins;
    d->supportedMimeTypes = supportedMimeTypes;

    return plugins;
}

