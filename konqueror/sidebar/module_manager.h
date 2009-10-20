/*
    Copyright (c) 2009 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QStringList>
#include <kservice.h>
class KConfigGroup;
class KUrl;

/**
 * The module manager is responsible for discovering the modules (i.e. tabs,
 * i.e. plugins, i.e. desktop files) to use in the sidebar, and for updating them.
 * This class contains no GUI code, so that it can be unit-tested.
 */
class ModuleManager
{
public:
    ModuleManager(KConfigGroup* config);

    /// Returns the filenames of the modules that should be shown in the GUI
    /// Example: "home.desktop" (default module), "dirtree1.desktop" (added by user)...
    QStringList modules() const;

    /// Returns the names of the available plugin libraries
    /// Example: konqsidebar_tree, konqsidebar_web
    KService::List availablePlugins() const;

    /// Returns the paths of all modules that match a given filter, like websidebarplugin*.desktop
    QStringList localModulePaths(const QString& filter) const;

    /// Returns the relative path in the "data" resource, for a given module
    QString moduleDataPath(const QString& fileName) const;
    /// Returns the relative path of the entries directory in the "data" resource
    QString relativeDataPath() const { return "konqsidebartng/entries/"; }
    /// Returns the full path for a given module. TEMP HACK, TO BE REMOVED
    QString moduleFullPath(const QString& fileName) const;

    void rollbackToDefault(QWidget* parent);

    void setModuleName(const QString& fileName, const QString& moduleName);
    void setModuleUrl(const QString& fileName, const KUrl& url);
    void setModuleIcon(const QString& fileName, const QString& icon);

    /// Find a unique filename for a new module, based on a template name
    /// like "dirtree%1.desktop".
    /// @return the full path. templ is modified to contain the filename only.
    QString addModuleFromTemplate(QString& templ);

    /// Called when a module was added
    void moduleAdded(const QString& fileName);

    /// Remove a module (deletes the local .desktop file)
    void removeModule(const QString& fileName);

private:

    KConfigGroup *m_config; // owned by SidebarWidget
    QString m_localPath; // local path
};
