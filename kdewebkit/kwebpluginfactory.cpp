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
#include <QWebView>
#include <QStringList>

#include <KParts/ReadOnlyPart>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KMimeType>
#include <KDebug>

class KWebPluginFactory::KWebPluginFactoryPrivate
{
public:
    KWebPluginFactoryPrivate(QWebPluginFactory *del) : delegate(del) {}
    QList<KWebPluginFactory::Plugin> plugins;
    QWebPluginFactory *delegate;
};

KWebPluginFactory::KWebPluginFactory(QObject *parent)
  : QWebPluginFactory(parent)
    , d(new KWebPluginFactory::KWebPluginFactoryPrivate(0))
{
}

KWebPluginFactory::KWebPluginFactory(QWebPluginFactory *delegate, QObject *parent)
  : QWebPluginFactory(parent)
    , d(new KWebPluginFactory::KWebPluginFactoryPrivate(delegate))
{
}

KWebPluginFactory::~KWebPluginFactory()
{
    delete d;
}

QObject* KWebPluginFactory::create(const QString& mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    if (d->delegate) {
        QObject* q = d->delegate->create(mimeType, url, argumentNames, argumentValues);
        if (q) return q;
    }
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
    kDebug() << "Asked for" << mimeType << "plugin, got" << part;
    if (!part) {
        kDebug() << "No plugins found for" << mimeType;
        kDebug() << "Trying a QWebView (known work-around for QtWebKit's built-in flash support).";
        QWebView* webView = new QWebView;
        webView->load(url);
        return webView;
    }
    return part->widget();
}

QList<KWebPluginFactory::Plugin> KWebPluginFactory::plugins() const
{
    if (!d->plugins.isEmpty()) return d->plugins;
    QList<Plugin> plugins;
    if (d->delegate) plugins = d->delegate->plugins();
    KService::List services = KServiceTypeTrader::self()->query("KParts/ReadOnlyPart");
    for (int i = 0; i < services.size(); i++) {
        KService::Ptr s = services.at(i);
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
                mime.fileExtensions = kmime->patterns().replaceInStrings("*.","");
            }
            mimes.append(mime);
        }
        plugins.append(plugin);
    }
    d->plugins = plugins;
    return plugins;
}

