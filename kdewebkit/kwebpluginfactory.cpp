/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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
    Q_UNUSED(argumentNames);
    Q_UNUSED(argumentValues);
    QWidget* w = new QWidget;
    KParts::ReadOnlyPart* part = KMimeTypeTrader::createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimeType, w, w);
    if (part) {
        part->openUrl(url);
        if (part->widget() != w)
            part->widget()->setParent(w);
    }
    kDebug() << "Asked for plugin, got" << part;
    return w;
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

