/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konq_kpart_plugin.h"

#include <KConfigGroup>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KService>
#include <KSharedConfig>
#include <KXMLGUIFactory>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace KonqParts
{

class PluginPrivate
{
public:
    QString m_parentInstance;
    QString m_pluginId;
};

Plugin::Plugin(QObject *parent)
    : QObject(parent)
    , d(new PluginPrivate())
{
}

Plugin::~Plugin() = default;

QString Plugin::xmlFile() const
{
    QString path = KXMLGUIClient::xmlFile();

    if (d->m_parentInstance.isEmpty() || (!path.isEmpty() && QDir::isAbsolutePath(path))) {
        return path;
    }

    QString absPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, d->m_parentInstance + QLatin1Char('/') + path);
    Q_ASSERT(!absPath.isEmpty());
    return absPath;
}

QString Plugin::localXMLFile() const
{
    QString path = KXMLGUIClient::xmlFile();

    if (d->m_parentInstance.isEmpty() || (!path.isEmpty() && QDir::isAbsolutePath(path))) {
        return path;
    }

    QString absPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + d->m_parentInstance + QLatin1Char('/') + path;
    return absPath;
}

// static
QList<Plugin::PluginInfo> Plugin::pluginInfos(const QString &componentName)
{
    QList<PluginInfo> plugins;

    QVector<KPluginMetaData> metaDataList = KPluginMetaData::findPlugins(componentName + QStringLiteral("/kpartplugins"));
    std::sort(metaDataList.begin(), metaDataList.end(), [](const KPluginMetaData &d1, const KPluginMetaData &d2) {
        return d1.pluginId() < d2.pluginId();
    });

    for (const KPluginMetaData &data : metaDataList) {
        PluginInfo info;
        info.m_metaData = data;
        QString doc;
        const QString fullComponentName = QStringLiteral("konqueror/partsrcfiles/") + QFileInfo(data.fileName()).baseName();
        const auto files = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, fullComponentName + QLatin1String(".rc"));
        info.m_absXMLFileName = KXMLGUIClient::findMostRecentXMLFile(files, doc);
        doc = KXMLGUIFactory::readConfigFile(info.m_absXMLFileName);
        if (info.m_absXMLFileName.isEmpty()) {
            continue;
        }

        info.m_document.setContent(doc);
        if (info.m_document.documentElement().isNull()) {
            continue;
        }

        const QString tryExec = data.value(QStringLiteral("TryExec"));
        if (!tryExec.isEmpty() && QStandardPaths::findExecutable(tryExec).isEmpty()) {
            continue;
        }

        plugins.append(info);
    }

    return plugins;
}

void Plugin::loadPlugins(QObject *parent, const QList<PluginInfo> &pluginInfos, const QString &componentName)
{
    for (const auto &pluginInfo : pluginInfos) {
        if (hasPlugin(parent, pluginInfo.m_metaData.pluginId())) {
            continue;
        }

        Plugin *plugin = loadPlugin(parent, pluginInfo.m_metaData);

        if (plugin) {
            plugin->d->m_parentInstance = componentName;
            plugin->setXMLFile(pluginInfo.m_absXMLFileName, false, false);
            plugin->setDOMDocument(pluginInfo.m_document);
        }
    }
}

// static
Plugin *Plugin::loadPlugin(QObject *parent, const KPluginMetaData &data)
{
    auto pluginResult = KPluginFactory::instantiatePlugin<Plugin>(data, parent);
    if (pluginResult) {
        pluginResult.plugin->d->m_pluginId = data.pluginId();
    }
    return pluginResult.plugin;
}

QList<KonqParts::Plugin *> Plugin::pluginObjects(QObject *parent)
{
    QList<KonqParts::Plugin *> objects;

    if (!parent) {
        return objects;
    }

    objects = parent->findChildren<Plugin *>(QString(), Qt::FindDirectChildrenOnly);
    return objects;
}

bool Plugin::hasPlugin(QObject *parent, const QString &pluginId)
{
    const QObjectList plugins = parent->children();

    return std::any_of(plugins.begin(), plugins.end(), [&pluginId](QObject *p) {
        Plugin *plugin = qobject_cast<Plugin *>(p);
        return (plugin && plugin->d->m_pluginId == pluginId);
    });
}


void Plugin::setMetaData(const KPluginMetaData &metaData)
{
    // backward-compatible registration, usage deprecated
    KXMLGUIClient::setComponentName(metaData.pluginId(), metaData.name());
}

void Plugin::loadPlugins(QObject *parent, KXMLGUIClient *parentGUIClient, const QString &componentName)
{
    KConfigGroup cfgGroup(KSharedConfig::openConfig(componentName + QLatin1String("rc")), "KParts Plugins");
    const QList<PluginInfo> plugins = pluginInfos(componentName);
    for (const auto &pluginInfo : plugins) {
        QDomElement docElem = pluginInfo.m_document.documentElement();

        bool pluginEnabled = pluginInfo.m_metaData.isEnabled(cfgGroup);
        // search through already present plugins
        const QObjectList pluginList = parent->children();

        bool pluginFound = false;
        for (auto *p : pluginList) {
            Plugin *plugin = qobject_cast<Plugin *>(p);
            if (plugin && plugin->d->m_pluginId == pluginInfo.m_metaData.pluginId()) {
                // delete and unload disabled plugins
                if (!pluginEnabled) {
                    // qDebug() << "remove plugin " << name;
                    KXMLGUIFactory *factory = plugin->factory();
                    if (factory) {
                        factory->removeClient(plugin);
                    }
                    delete plugin;
                }

                pluginFound = true;
                break;
            }
        }

        // if the plugin is already loaded or if it's disabled in the
        // configuration do nothing
        if (pluginFound || !pluginEnabled) {
            continue;
        }

        // qDebug() << "load plugin " << name << " " << library << " " << keyword;
        Plugin *plugin = loadPlugin(parent, pluginInfo.m_metaData);

        if (plugin) {
            plugin->d->m_parentInstance = componentName;
            plugin->setXMLFile(pluginInfo.m_absXMLFileName, false, false);
            plugin->setDOMDocument(pluginInfo.m_document);
            parentGUIClient->insertChildClient(plugin);
        }
    }
}
}

