/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <KPluginMetaData>

#include <QStringList>

class KConfigGroup;

/**
 * The module manager is responsible for discovering the modules (i.e. tabs,
 * i.e. plugins, i.e. desktop files) to use in the sidebar, and for updating them.
 * This class contains no GUI code, so that it can be unit-tested.
 */
class ModuleManager
{
public:
    ModuleManager(KConfigGroup *config);

    /// Returns the filenames of the modules that should be shown in the GUI
    /// Example: "home.desktop" (default module), "dirtree1.desktop" (added by user)...
    QStringList modules() const;

    /// Returns the names of the available plugin libraries
    /// Example: konqsidebar_tree, konqsidebar_web
    QVector<KPluginMetaData> availablePlugins() const;

    /// Returns the paths of all modules that match a given filter, like websidebarplugin*.desktop
    QStringList localModulePaths(const QString &filter) const;

    /// Returns the relative path in the "data" resource, for a given module
    QString moduleDataPath(const QString &fileName) const;
    /// Returns the relative path of the entries directory in the "data" resource
    QString relativeDataPath() const
    {
        return "konqsidebartng/entries/";
    }
    /// Returns the full path for a given module. TEMP HACK, TO BE REMOVED
    QString moduleFullPath(const QString &fileName) const;

    void saveOpenViews(const QStringList &fileName);
    void restoreDeletedButtons();
    void rollbackToDefault();

    void setModuleName(const QString &fileName, const QString &moduleName);
    void setModuleUrl(const QString &fileName, const QUrl &url);
    void setModuleIcon(const QString &fileName, const QString &icon);
    void setShowHiddenFolders(const QString &fileName, const bool &newState);
    int getMaxKDEWeight();
    int getNextAvailableKDEWeight();

    /// Find a unique filename for a new module, based on a template name
    /// like "dirtree%1.desktop".
    /// @return the full path. templ is modified to contain the filename only.
    QString addModuleFromTemplate(QString &templ);

    /// Called when a module was added
    void moduleAdded(const QString &fileName);

    /// Remove a module (deletes the local .desktop file)
    void removeModule(const QString &fileName);

    /**
     * @brief The relative directory where to look for plugins
     *
     * The returned value is suitable to pass as first argument to KPluginMetaData::findPluginById()
     * and KPluginMetaData::findPlugins().
     * @return the relative directory where plugins are stored
     */
    static QString pluginDirectory() {return QStringLiteral("konqueror/sidebar");}

private:
    void sortGlobalEntries(QStringList &fileNames) const;

    KConfigGroup *m_config; // owned by SidebarWidget
    QString m_localPath; // local path
};

#endif
