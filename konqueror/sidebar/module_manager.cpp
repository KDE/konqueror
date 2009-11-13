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

#include "module_manager.h"
#include <kdesktopfile.h>
#include <kio/deletejob.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <QDir>

// Input data:
// Global dir: list of desktop files.
// Local dir: list of desktop files, for modules added or modified
//      (When the user modifies something, we make a local copy)
// Local KConfig file: config file with list (per profile) of "added" and "removed" desktop files

// Algorithm:
// Then we create buttons:
// * desktop files from the most-global directory (pre-installed modules),
//   skipping those listed as "removed"
// * desktop files from other (more local) directories, if listed as "added" (for the current profile).
//
// (module not present globally and not "added" are either modified copies of
// not-installed-anymore modules, or modules that were only added for another profile).


ModuleManager::ModuleManager(KConfigGroup* config)
    : m_config(config)
{
    m_localPath = KGlobal::dirs()->saveLocation("data", relativeDataPath(), true);
}

QStringList ModuleManager::modules() const
{
    QStringList fileNames;
    const QStringList addedModules = m_config->readEntry("AddedModules", QStringList());
    const QStringList deletedModules = m_config->readEntry("DeletedModules", QStringList());

    const QStringList entries_dirs = KGlobal::dirs()->findDirs("data", relativeDataPath());
    if (entries_dirs.isEmpty()) { // Ooops
        kWarning() << "No global directory found for apps/konqsidebartng/entries. Installation problem!";
        return QStringList();
    }
    // We only list the most-global dir. Other levels use AddedModules.
    QDir globalDir(entries_dirs.last());
    //kDebug() << "Listing" << entries_dirs.last();
    const QStringList globalDirEntries = globalDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    Q_FOREACH(const QString& globalEntry, globalDirEntries) {
        //kDebug() << " found" << globalEntry;
        if (!deletedModules.contains(globalEntry)) {
            fileNames.append(globalEntry);
        }
    }
    sortGlobalEntries(fileNames);
    //kDebug() << "Adding local modules:" << addedModules;
    Q_FOREACH(const QString& module, addedModules) {
        if (!fileNames.contains(module))
            fileNames.append(module);
    }

    return fileNames;
}

KService::List ModuleManager::availablePlugins() const
{
    // For the "add" menu, we need all available plugins.
    // We could use KServiceTypeTrader for that; not sure 2 files make a big performance difference though.
    const QStringList files = KGlobal::dirs()->findAllResources("data", "konqsidebartng/plugins/*.desktop");
    KService::List services;
    Q_FOREACH(const QString& path, files) {
        KDesktopFile df(path); // no merging. KService warns, and we don't need it.
        services.append(KService::Ptr(new KService(&df)));
    }
    return services;
}

QString ModuleManager::moduleDataPath(const QString& fileName) const
{
    return relativeDataPath() + fileName;
}

QString ModuleManager::moduleFullPath(const QString& fileName) const
{
    return KGlobal::dirs()->locate("data", moduleDataPath(fileName));
}

void ModuleManager::rollbackToDefault()
{
    const QString loc = KGlobal::dirs()->saveLocation("data", "konqsidebartng/");
    QDir dir(loc);
    const QStringList dirEntries = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    Q_FOREACH(const QString& subdir, dirEntries) {
        if (subdir != "add") {
            kDebug() << "Deleting" << (loc+subdir);
            KIO::Job* job = KIO::del(KUrl(loc+subdir), KIO::HideProgressInfo);
            job->exec();
        }
    }
    m_config->writeEntry("DeletedModules", QStringList());
    m_config->writeEntry("AddedModules", QStringList());
}

void ModuleManager::setModuleName(const QString& fileName, const QString& moduleName)
{
    // Write the name in the .desktop file of this button.
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writeEntry("Name", moduleName);
    ksc.writeEntry("Name", moduleName, KConfigBase::Persistent|KConfigBase::Localized);
    ksc.sync();
}

void ModuleManager::setModuleUrl(const QString& fileName, const KUrl& url)
{
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writePathEntry("URL", url.prettyUrl());
    ksc.sync();
}

void ModuleManager::setModuleIcon(const QString& fileName, const QString& icon)
{
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writePathEntry("Icon", icon);
    ksc.sync();
}

void ModuleManager::removeModule(const QString& fileName)
{
    // Remove the local file (if it exists)
    QFile f(m_localPath + fileName);
    f.remove();

    // Mark module as deleted (so that we skip global file, if there's one)
    QStringList deletedModules = m_config->readEntry("DeletedModules", QStringList());
    QStringList addedModules = m_config->readEntry("AddedModules", QStringList());
    if (!deletedModules.contains(fileName))
        deletedModules.append(fileName);
    addedModules.removeAll(fileName);
    m_config->writeEntry("DeletedModules", deletedModules);
    m_config->writeEntry("AddedModules", addedModules);
}

void ModuleManager::moduleAdded(const QString& fileName)
{
    kDebug() << fileName;
    QStringList deletedModules = m_config->readEntry("DeletedModules", QStringList());
    QStringList addedModules = m_config->readEntry("AddedModules", QStringList());
    if (!addedModules.contains(fileName))
        addedModules.append(fileName);
    deletedModules.removeAll(fileName);
    m_config->writeEntry("DeletedModules", deletedModules);
    m_config->writeEntry("AddedModules", addedModules);
}

QString ModuleManager::addModuleFromTemplate(QString& templ)
{
    if (!templ.contains("%1"))
        kWarning() << "Template filename should contain %1";

    QString filename = templ.arg(QString());
    QString myFile = KStandardDirs::locateLocal("data", moduleDataPath(filename));

    if (QFile::exists(myFile)) {
        for (ulong l = 1; l < ULONG_MAX; l++) {
            filename = templ.arg(l);
            myFile = KStandardDirs::locateLocal("data", moduleDataPath(filename));
            if (!QFile::exists(myFile)) {
                break;
            } else {
                filename.clear();
                myFile.clear();
            }
        }
    }
    templ = filename;
    return myFile;
}

QStringList ModuleManager::localModulePaths(const QString& filter) const
{
    return QDir(m_localPath).entryList(QStringList() << filter);
}

void ModuleManager::sortGlobalEntries(QStringList& fileNames) const
{
    QMap<int, QString> sorter;
    Q_FOREACH(const QString& fileName, fileNames) {
        const QString path = moduleDataPath(fileName);
        if (KStandardDirs::locate("data", path).isEmpty()) {
            // doesn't exist anymore, skip it
            kDebug() << "Skipping" << path;
        } else {
            KSharedConfig::Ptr config = KSharedConfig::openConfig(path,
                                                                  KConfig::NoGlobals,
                                                                  "data");
            KConfigGroup configGroup(config, "Desktop Entry");
            const int weight = configGroup.readEntry("X-KDE-Weight", 0);
            sorter.insert(weight, fileName);
        }
    }
    fileNames = sorter.values();
    kDebug() << fileNames;
}
