/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqclosedwindowsmanager.h"
#include "konqsettingsxt.h"
#include "konqmisc.h"
#include "konqcloseditem.h"
#include "konqclosedwindowsmanageradaptor.h"
#include "konqclosedwindowsmanager_interface.h"
#include <kio/fileundomanager.h>
#include <QDirIterator>
#include <QMetaType>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <KLocalizedString>

#include <kconfig.h>
#include <QStandardPaths>
#include <KSharedConfig>

Q_DECLARE_METATYPE(QList<QVariant>)

class KonqClosedWindowsManagerPrivate
{
public:
    KonqClosedWindowsManager instance;
    int m_maxNumClosedItems;
};

static KonqClosedWindowsManagerPrivate *myKonqClosedWindowsManagerPrivate = nullptr;

KonqClosedWindowsManager::KonqClosedWindowsManager() : m_memoryStoreBackend(QStringLiteral("%1/closeditems-XXXXXX").arg(QDir::tempPath()))
{
    new KonqClosedWindowsManagerAdaptor(this);

    KConfigGroup configGroup(KSharedConfig::openConfig(), "Undo");
    m_numUndoClosedItems = configGroup.readEntry("Number of Closed Windows", 0);

    m_konqClosedItemsConfig = nullptr;
    m_blockClosedItems = false;

    m_memoryStoreBackend.open();
    m_konqClosedItemsStore = new KConfig(m_memoryStoreBackend.fileName(), KConfig::SimpleConfig);
}

KonqClosedWindowsManager::~KonqClosedWindowsManager()
{
    qDeleteAll(m_closedWindowItemList); // must be done before deleting the kconfigs
    delete m_konqClosedItemsConfig;
    delete m_konqClosedItemsStore;
}

KConfig *KonqClosedWindowsManager::memoryStore()
{
    return m_konqClosedItemsStore;
}

KonqClosedWindowsManager *KonqClosedWindowsManager::self()
{
    if (!myKonqClosedWindowsManagerPrivate) {
        myKonqClosedWindowsManagerPrivate = new KonqClosedWindowsManagerPrivate;
    }
    return &myKonqClosedWindowsManagerPrivate->instance;
}

void KonqClosedWindowsManager::destroy()
{
    delete myKonqClosedWindowsManagerPrivate;
    myKonqClosedWindowsManagerPrivate = nullptr;
}

void KonqClosedWindowsManager::addClosedWindowItem(KonqUndoManager
        *real_sender, KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    readConfig();
    // If we are off the limit, remove the last closed window item
    if (m_closedWindowItemList.size() >=
            KonqSettings::maxNumClosedItems()) {
        KonqClosedWindowItem *last = m_closedWindowItemList.last();

        emit removeWindowInOtherInstances(nullptr, last);

        m_closedWindowItemList.removeLast();
        delete last;
    }

    if (!m_blockClosedItems) {
        m_numUndoClosedItems++;
        emit addWindowInOtherInstances(real_sender, closedWindowItem);
    }

    // The prepend goes after emit addWindowInOtherInstances() because otherwise
    // the first time addWindowInOtherInstances() is emitted, KonqUndoManager
    // will catch it and it will call to its private populate() function which
    // will add to the undo list closedWindowItem, and then it will add it again
    // but we want it to be added only once.
    m_closedWindowItemList.prepend(closedWindowItem);

    if (propagate) {
        saveConfig();
    }
}

void KonqClosedWindowsManager::removeClosedWindowItem(KonqUndoManager
        *real_sender, const KonqClosedWindowItem *closedWindowItem)
{
    readConfig();
    auto it = std::find(m_closedWindowItemList.begin(), m_closedWindowItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if (it != m_closedWindowItemList.end()) {
        m_closedWindowItemList.erase(it);
        m_numUndoClosedItems--;
    }
    emit removeWindowInOtherInstances(real_sender, closedWindowItem);
}

const QList<KonqClosedWindowItem *> &KonqClosedWindowsManager::closedWindowItemList()
{
    readConfig();
    return m_closedWindowItemList;
}

void KonqClosedWindowsManager::readSettings()
{
    readConfig();
}

KonqClosedWindowItem *KonqClosedWindowsManager::findClosedWindowItem(
    const QString &configFileName,
    const QString &configGroup)
{
    readConfig();
    auto filter = [configFileName, configGroup](KonqClosedWindowItem *wi){
        return wi->configGroup().config()->name() == configFileName && wi->configGroup().name() == configGroup;
    };
    auto it = std::find_if(m_closedWindowItemList.constBegin(), m_closedWindowItemList.constEnd(), filter);
    return *it;
}

void KonqClosedWindowsManager::saveConfig()
{
    readConfig();

    // Create / overwrite the saved closed windows list
    QString filename = QStringLiteral("closeditems_saved");
    QString file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + filename;
    QFile::remove(file);

    KConfig *config = new KConfig(file, KConfig::SimpleConfig);

    // Populate the config file
    KonqClosedWindowItem *closedWindowItem = nullptr;
    uint counter = m_closedWindowItemList.size() - 1;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.constBegin();
            it != m_closedWindowItemList.constEnd(); ++it, --counter) {
        closedWindowItem = *it;
        KConfigGroup configGroup(config, "Closed_Window" + QString::number(counter));
        configGroup.writeEntry("title", closedWindowItem->title());
        configGroup.writeEntry("numTabs", closedWindowItem->numTabs());
        closedWindowItem->configGroup().copyTo(&configGroup);
    }

    KConfigGroup configGroup(KSharedConfig::openConfig(), "Undo");
    configGroup.writeEntry("Number of Closed Windows", m_closedWindowItemList.size());
    configGroup.sync();

    // Finally the most important thing, which is to save the store config
    // so that other konqi processes can reopen windows closed in this process.
    m_konqClosedItemsStore->sync();

    delete config;
}

void KonqClosedWindowsManager ::readConfig()
{
    if (m_konqClosedItemsConfig) {
        return;
    }

    QString filename = QStringLiteral("closeditems_saved");
    QString file = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + filename;

    m_konqClosedItemsConfig = new KConfig(file, KConfig::SimpleConfig);

    // If the config file doesn't exists, there's nothing to read
    if (!QFile::exists(file)) {
        return;
    }

    m_blockClosedItems = true;
    for (int i = 0; i < m_numUndoClosedItems; i++) {
        // For each item, create a new ClosedWindowItem
        KConfigGroup configGroup(m_konqClosedItemsConfig, "Closed_Window" +
                                 QString::number(i));

        // The number of closed items was not correctly set, fix it and save the
        // correct number.
        if (!configGroup.exists()) {
            m_numUndoClosedItems = i;
            KConfigGroup configGroup(KSharedConfig::openConfig(), "Undo");
            configGroup.writeEntry("Number of Closed Windows",
                                   m_closedWindowItemList.size());
            configGroup.sync();
            break;
        }

        QString title = configGroup.readEntry("title", i18n("no name"));
        int numTabs = configGroup.readEntry("numTabs", 0);

        KonqClosedWindowItem *closedWindowItem = new KonqClosedWindowItem(
            title, memoryStore(), i, numTabs);
        configGroup.copyTo(&closedWindowItem->configGroup());
        configGroup.writeEntry("foo", 0);

        // Add the item only to this window
        addClosedWindowItem(nullptr, closedWindowItem, false);
    }

    m_blockClosedItems = false;
}

bool KonqClosedWindowsManager::undoAvailable() const
{
    return m_numUndoClosedItems > 0;
}
