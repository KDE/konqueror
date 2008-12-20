/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
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

#include <QWebPluginFactory>
#include <QStringList>

#include <KParts/ReadOnlyPart>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KMimeType>
#include <KDebug>

class KWebPluginFactory::KWebPluginFactoryPrivate
{
public:
    KWebPluginFactoryPrivate() {}
    QList<KWebPluginFactory::Plugin> plugins;
};

KWebPluginFactory::KWebPluginFactory(QObject* parent)
  : QWebPluginFactory(parent)
    , d(new KWebPluginFactory::KWebPluginFactoryPrivate())
{
}

KWebPluginFactory::~KWebPluginFactory()
{
    delete d;
}

QObject* KWebPluginFactory::create(const QString& mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    QVariantList arguments;
    int i = 0;
    Q_FOREACH(const QString &key, argumentNames) {
        arguments << key + "=\"" + argumentValues.at(i) + '\"';
        ++i;
    }
//     arguments << "__KHTML__PLUGINEMBED=\"YES\""; //### following arguments are also set by khtml
//     arguments << "__KHTML__PLUGINBASEURL=\"" + url.scheme() + "://" + url.host() + "/\"";
//     arguments << "Browser/View";

    KParts::ReadOnlyPart* part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimeType, qobject_cast<KWebPage*>(parent())->view(), parent(), QString(), arguments);
    if (part) {
        QMap<QString, QString> metaData = part->arguments().metaData();
        metaData.insert("PropagateHttpHeader", "true");
        metaData.insert("cross-domain", url.scheme() + "://" + url.host() + "/");
        metaData.insert("main_frame_request", "TRUE");
//         metaData.insert("referrer", url.scheme() + "://" + url.host() + "/"); //### following metadata is also set by khtml
//         metaData.insert("ssl_activate_warnings", "TRUE");
//         metaData.insert("ssl_parent_cert", "");
//         metaData.insert("ssl_parent_ip", "");
//         metaData.insert("ssl_was_in_use", "FALSE");

        KParts::OpenUrlArguments openUrlArgs = part->arguments();
        openUrlArgs.metaData() = metaData;
        openUrlArgs.setMimeType(mimeType);
        part->setArguments(openUrlArgs);
        kDebug()<< part->arguments().metaData();
        part->openUrl(url);
    }
    kDebug() << "Asked for plugin, got" << part;
    return part->widget();
}

QList<KWebPluginFactory::Plugin> KWebPluginFactory::plugins() const
{
    if (!d->plugins.isEmpty()) return d->plugins;
    QList<Plugin> plugins;
    KService::List services = KServiceTypeTrader::self()->query("KParts/ReadOnlyPart");
    kDebug() << "Asked for list of plugins. Got:";
    for (int i = 0; i < services.size(); i++) {
        KService::Ptr s = services.at(i);
        Plugin plugin;
        plugin.name = s->desktopEntryName();
        plugin.description = s->comment();
        kDebug() << "- " << plugin.name << "with support for:";
        QList<MimeType> mimes;
        QStringList servicetypes = s->serviceTypes();
        for (int z = 0; z < servicetypes.size(); z++) {
            MimeType mime;
            mime.name = servicetypes.at(z);
            kDebug() << "-- " << mime.name;
            KMimeType::Ptr kmime = KMimeType::mimeType(mime.name);
            if (kmime) {
                mime.fileExtensions = kmime->patterns().replaceInStrings("*.","");
                for (int w = 0; w < mime.fileExtensions.size(); w++)
                    kDebug() << "--- " << mime.fileExtensions.at(w);
            }
            mimes.append(mime);
        }
        plugins.append(plugin);
    }
    kDebug() << "Total:" << plugins.count();
    d->plugins = plugins;
    return plugins;
}

