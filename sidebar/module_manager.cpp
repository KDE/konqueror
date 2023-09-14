/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "module_manager.h"
#include <kdesktopfile.h>
#include <kio/deletejob.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <QDir>
#include <QUrl>
#include <QStandardPaths>
#include <KSharedConfig>

#include "sidebar_debug.h"

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

ModuleManager::ModuleManager(KConfigGroup *config)
    : m_config(config)
{
    m_localPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + relativeDataPath();
}

QStringList ModuleManager::modules() const
{
    QStringList fileNames;
    const QStringList addedModules = m_config->readEntry("AddedModules", QStringList());
    const QStringList deletedModules = m_config->readEntry("DeletedModules", QStringList());

    const QStringList entries_dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, relativeDataPath(), QStandardPaths::LocateDirectory);
    if (entries_dirs.isEmpty()) { // Ooops
        qCWarning(SIDEBAR_LOG) << "No global directory found for" << relativeDataPath() << "Installation problem!";
        return QStringList();
    }
    // We only list the most-global dir. Other levels use AddedModules.
    QDir globalDir(entries_dirs.last());
    //qCDebug(SIDEBAR_LOG) << "Listing" << entries_dirs.last();
    const QStringList globalDirEntries = globalDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    Q_FOREACH (const QString &globalEntry, globalDirEntries) {
        //qCDebug(SIDEBAR_LOG) << " found" << globalEntry;
        if (!deletedModules.contains(globalEntry)) {
            fileNames.append(globalEntry);
        }
    }
    sortGlobalEntries(fileNames);
    //qCDebug(SIDEBAR_LOG) << "Adding local modules:" << addedModules;
    Q_FOREACH (const QString &module, addedModules) {
        if (!fileNames.contains(module)) {
            fileNames.append(module);
        }
    }

    return fileNames;
}

QVector<KPluginMetaData> ModuleManager::availablePlugins() const
{
    return KPluginMetaData::findPlugins(pluginDirectory());
}

QString ModuleManager::moduleDataPath(const QString &fileName) const
{
    return relativeDataPath() + fileName;
}

QString ModuleManager::moduleFullPath(const QString &fileName) const
{
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, moduleDataPath(fileName));
}

void ModuleManager::saveOpenViews(const QStringList &fileName)
{
    // TODO: this would be best stored per-window, in the session file

    m_config->writeEntry("OpenViews", fileName);
    m_config->sync();
}

void ModuleManager::restoreDeletedButtons()
{
    m_config->writeEntry("DeletedModules", QStringList());
    m_config->sync();
}

void ModuleManager::rollbackToDefault()
{
    const QString loc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/konqsidebartng/";
    QDir dir(loc);
    const QStringList dirEntries = dir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    Q_FOREACH (const QString &subdir, dirEntries) {
        if (subdir != "add") {
            qCDebug(SIDEBAR_LOG) << "Deleting" << (loc + subdir);
            KIO::Job *job = KIO::del(QUrl::fromLocalFile(loc + subdir), KIO::HideProgressInfo);
            job->exec();
        }
    }
    m_config->writeEntry("DeletedModules", QStringList());
    m_config->writeEntry("AddedModules", QStringList());
    m_config->sync();
}

void ModuleManager::setModuleName(const QString &fileName, const QString &moduleName)
{
    // Write the name in the .desktop file of this button.
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writeEntry("Name", moduleName);
    ksc.writeEntry("Name", moduleName, KConfigBase::Persistent | KConfigBase::Localized);
    ksc.sync();
}

void ModuleManager::setModuleUrl(const QString &fileName, const QUrl &url)
{
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writePathEntry("URL", url.toDisplayString());
    ksc.sync();
}

void ModuleManager::setModuleIcon(const QString &fileName, const QString &icon)
{
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writePathEntry("Icon", icon);
    ksc.sync();
}

void ModuleManager::setShowHiddenFolders(const QString &fileName, const bool &newState)
{
    KConfig desktopFile(m_localPath + fileName, KConfig::SimpleConfig);
    KConfigGroup ksc(&desktopFile, "Desktop Entry");
    ksc.writeEntry("ShowHiddenFolders", newState);
    ksc.sync();
}

int ModuleManager::getMaxKDEWeight()
{
    int curMax = 1; // 0 is reserved for the treeModule
    for (const QString &fileName : modules()) {
        const QString path = moduleDataPath(fileName);
        if (! QStandardPaths::locate(QStandardPaths::GenericDataLocation, path).isEmpty()) {
            KSharedConfig::Ptr config = KSharedConfig::openConfig(path,
                                        KConfig::NoGlobals,
                                        QStandardPaths::GenericDataLocation);
            KConfigGroup configGroup(config, "Desktop Entry");
            const int weight = configGroup.readEntry("X-KDE-Weight", 0);
            if (curMax < weight) {
                curMax = weight;
            }
        }
    }
    return curMax;
}

int ModuleManager::getNextAvailableKDEWeight()
{
    return getMaxKDEWeight() + 1;
}

void ModuleManager::removeModule(const QString &fileName)
{
    // Remove the local file (if it exists)
    QFile f(m_localPath + fileName);
    f.remove();

    // Mark module as deleted (so that we skip global file, if there's one)
    QStringList deletedModules = m_config->readEntry("DeletedModules", QStringList());
    QStringList addedModules = m_config->readEntry("AddedModules", QStringList());
    if ( !addedModules.contains(fileName) && !deletedModules.contains(fileName)) { // only add it to the "deletedModules" list if it is a global module
        deletedModules.append(fileName);
    }

    addedModules.removeAll(fileName);
    m_config->writeEntry("DeletedModules", deletedModules);
    m_config->writeEntry("AddedModules", addedModules);
    m_config->sync();
}

void ModuleManager::moduleAdded(const QString &fileName)
{
    qCDebug(SIDEBAR_LOG) << fileName;
    QStringList deletedModules = m_config->readEntry("DeletedModules", QStringList());
    QStringList addedModules = m_config->readEntry("AddedModules", QStringList());
    if (!addedModules.contains(fileName)) {
        addedModules.append(fileName);
    }
    deletedModules.removeAll(fileName);
    m_config->writeEntry("DeletedModules", deletedModules);
    m_config->writeEntry("AddedModules", addedModules);
    m_config->sync();
}

QString ModuleManager::addModuleFromTemplate(QString &templ)
{
    if (!templ.contains("%1")) {
        qCWarning(SIDEBAR_LOG) << "Template filename should contain %1";
    }

    QString filename = templ.arg(QString());
    QString myFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + moduleDataPath(filename);

    if (QFile::exists(myFile)) {
        for (ulong l = 1; l < ULONG_MAX; l++) {
            filename = templ.arg(l);
            myFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + moduleDataPath(filename);
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

QStringList ModuleManager::localModulePaths(const QString &filter) const
{
    return QDir(m_localPath).entryList(QStringList() << filter);
}

void ModuleManager::sortGlobalEntries(QStringList &fileNames) const
{
    QMap<int, QString> sorter;
    Q_FOREACH (const QString &fileName, fileNames) {
        const QString path = moduleDataPath(fileName);
        if (QStandardPaths::locate(QStandardPaths::GenericDataLocation, path).isEmpty()) {
            // doesn't exist anymore, skip it
            qCDebug(SIDEBAR_LOG) << "Skipping" << path;
        } else {
            KSharedConfig::Ptr config = KSharedConfig::openConfig(path,
                                        KConfig::NoGlobals,
                                        QStandardPaths::GenericDataLocation);
            KConfigGroup configGroup(config, "Desktop Entry");
            const int weight = configGroup.readEntry("X-KDE-Weight", 0);
            sorter.insert(weight, fileName);

            // While we have the config file open, fix migration issue between old and new history sidebar
            if (configGroup.readEntry("X-KDE-TreeModule") == "History") {
                // Old local file still there; remove it
                const QString localFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + path;
                QFile::rename(localFile, localFile + ".orig");
                qCDebug(SIDEBAR_LOG) << "Migration: moving" << localFile << "out of the way";
            }
        }
    }
    fileNames = sorter.values();
    qCDebug(SIDEBAR_LOG) << fileNames;
}
