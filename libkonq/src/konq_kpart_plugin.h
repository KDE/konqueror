/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef PLUGIN_H
#define PLUGIN_H

#include <libkonq_export.h>

#include <KPluginMetaData>
#include <KXMLGUIClient>
#include <QDomElement>
#include <QObject>
#include <memory>

class KPluginMetaData;

namespace KonqParts
{
class PluginPrivate;

/**
 * @class Plugin plugin.h <KParts/Plugin>
 *
 * @short A plugin is the way to add actions to an existing KParts application,
 * or to a Part.
 *
 * The XML of those plugins looks exactly like of the shell or parts,
 * with one small difference: The document tag should have an additional
 * attribute, named "library", and contain the name of the library implementing
 * the plugin.
 *
 * If you want this plugin to be used by a part, you need to
 * install the rc file under the directory
 * "data" (KDEDIR/share/apps usually)+"/instancename/kpartplugins/"
 * where instancename is the name of the part's instance.
 *
 * You should also install a "plugin info" .desktop file with the same name.
 * \see KPluginInfo
 */
class LIBKONQ_EXPORT Plugin : public QObject, virtual public KXMLGUIClient
{
    Q_OBJECT
public:
    struct PluginInfo {
        KPluginMetaData m_metaData;
        QString m_absXMLFileName; // full path of most recent filename matching the relative
        QDomDocument m_document;
    };

    /**
     * Construct a new KParts plugin.
     */
    explicit Plugin(QObject *parent = nullptr);
    /**
     * Destructor.
     */
    ~Plugin() override;

    /**
     * Reimplemented for internal reasons
     */
    QString xmlFile() const override;

    /**
     * Reimplemented for internal reasons
     */
    QString localXMLFile() const override;

    /**
     * Load the plugin libraries specified by the list @p pluginInfos, make the
     * Plugin objects children of @p parent, and use the given @p instance.
     *
     * It is recommended to use the last loadPlugins method instead,
     * to support enabling and disabling of plugins.
     */
    static void loadPlugins(QObject *parent, const QList<PluginInfo> &pluginInfos, const QString &instance);

    /**
     * Load the plugin libraries for the given @p instance, make the
     * Plugin objects children of @p parent, and insert the plugin as a child GUI client
     * of @p parentGUIClient.
     *
     * This method uses the KConfig object of the given instance, to find out which
     * plugins are enabled and which are disabled.
     * If a disabled plugin is already loaded it will be removed from the GUI
     * factory and deleted.
     *
     *
     * This method is automatically called by KParts::Part and by KParts::MainWindow.
     * @see PartBase::setPluginLoadingMode, PartBase::setPluginInterfaceVersion
     *
     * If you call this method in an already constructed GUI (like when the user
     * has changed which plugins are enabled) you need to add the new plugins to
     * the KXMLGUIFactory:
     * \code
     * if( factory() )
     * {
     *   const QList<KParts::Plugin *> plugins = KParts::Plugin::pluginObjects( this );
     *   for ( KParts::Plugin * plugin : plugins )
     *     factory()->addClient( plugin );
     * }
     * \endcode
     */
    static void loadPlugins(QObject *parent, KXMLGUIClient *parentGUIClient, const QString &instance);

    /**
     * Returns a list of plugin objects loaded for @p parent. This
     * functions basically iterates over the children of the given object
     * and returns those that inherit from KParts::Plugin.
     **/
    static QList<Plugin *> pluginObjects(QObject *parent);

protected:
    /**
     * @since 5.77
     */
    void setMetaData(const KPluginMetaData &metaData);

private:
    /**
     * Look for plugins in the @p instance's "data" directory (+"/kpartplugins")
     *
     * @return A list of QDomDocument s, containing the parsed xml documents returned by plugins.
     */
    static QList<Plugin::PluginInfo> pluginInfos(const QString &instance);

    /**
     * @internal
     * @return The plugin created from the library @p libname
     */
    static Plugin *loadPlugin(QObject *parent, const KPluginMetaData &data);

private:
    static bool hasPlugin(QObject *parent, const QString &pluginId);
    std::unique_ptr<PluginPrivate> const d;
};

}

#endif

