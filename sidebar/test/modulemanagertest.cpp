/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <module_manager.h>
#include <QTest>
#include <QStandardPaths>
#include <KSharedConfig>

class ModuleManagerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testListModules();
    void testAddLocalModule();
    void testRenameGlobalModule();
    void testRemoveLocalModule();
    void testRemoveGlobalModule();
    void testReAddGlobalModule();
    void testRollback();
    void testAvailablePlugins();

private:
    ModuleManager *m_moduleManager;
    ModuleManager *m_moduleManager2;
    QString m_profile;
    QString m_profile2;
    KConfigGroup *m_configGroup;
    KConfigGroup *m_configGroup2;

    int m_realModules;
    QString m_globalDir;
};


QTEST_GUILESS_MAIN(ModuleManagerTest)

void ModuleManagerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    const QString configFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1Char('/') + "konqsidebartngrc";
    QFile::remove(configFile);
    KSharedConfig::Ptr config = KSharedConfig::openConfig("konqsidebartngrc");

    m_profile = "test_profile";
    m_configGroup = new KConfigGroup(config, m_profile);
    m_moduleManager = new ModuleManager(m_configGroup);

    m_profile2 = "other_profile";
    m_configGroup2 = new KConfigGroup(config, m_profile2);
    m_moduleManager2 = new ModuleManager(m_configGroup2);

    m_realModules = m_moduleManager->modules().count();
    //m_realModules = 0; // because of our modified "global dir", the real ones are not visible anymore.

    // This is needed because, in ModuleManager::modules():
    // "We only list the most-global dir. Other levels use AddedModules."
    m_configGroup->writeEntry("AddedModules", QStringList() << "testModule.desktop");
    m_configGroup2->writeEntry("AddedModules", QStringList() << "testModule.desktop");

    // Create a "global" dir for the (fake) pre-installed modules,
    // which isn't really global of course, but we can register it as such...
    m_globalDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "konqsidebartng/entries/";
    QVERIFY(QDir().mkpath(m_globalDir));
    QFile::remove(m_globalDir + "testModule.desktop");

    // Create a fake pre-installed plugin there.
    KDesktopFile testModule(m_globalDir + "testModule.desktop");
    KConfigGroup scf = testModule.desktopGroup();
    scf.writeEntry("Type", "Link");
    scf.writePathEntry("URL", "http://www.kde.org");
    scf.writeEntry("Icon", "internet-web-browser");
    scf.writeEntry("Name", QString::fromLatin1("SideBar Test Plugin"));
    scf.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_web");
    scf.sync();
    QVERIFY(QFile::exists(m_globalDir + "testModule.desktop"));
}

void ModuleManagerTest::cleanupTestCase()
{
    delete m_moduleManager;
    delete m_moduleManager2;
    delete m_configGroup;
    delete m_configGroup2;
    QFile::remove(m_globalDir + "testModule.desktop");
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "konqsidebartng/entries/testModule.desktop");
}

// This test checks the initial state of things.
// It should remain first. It's called again after the "rollback to defaults".
void ModuleManagerTest::testListModules()
{
    const QStringList modules = m_moduleManager->modules();
    qDebug() << modules;
    QCOMPARE(modules.count(), m_realModules + 1);
    QVERIFY(modules.contains("testModule.desktop"));
    QVERIFY(m_moduleManager->moduleDataPath("testModule.desktop").endsWith("/testModule.desktop"));
    const KDesktopFile df(QStandardPaths::GenericDataLocation, m_moduleManager->moduleDataPath("testModule.desktop"));
    QCOMPARE(df.readName(), QString::fromLatin1("SideBar Test Plugin"));

    const QStringList modules2 = m_moduleManager2->modules();
    QCOMPARE(modules2.count(), m_realModules + 1);
    QVERIFY(modules2.contains("testModule.desktop"));
}

void ModuleManagerTest::testAddLocalModule()
{
    const QString expectedFileName = "local.desktop";
    QString fileName = "local%1.desktop";
    const QString path = m_moduleManager->addModuleFromTemplate(fileName);
    QCOMPARE(fileName, expectedFileName);
    QVERIFY(path.endsWith(fileName));
    KDesktopFile testModule(path);
    KConfigGroup scf = testModule.desktopGroup();
    scf.writeEntry("Type", "Link");
    scf.writePathEntry("URL", "/tmp");
    scf.writeEntry("Icon", "home");
    scf.sync();
    m_moduleManager->moduleAdded(fileName);
    QVERIFY(QFile::exists(path));

    const QStringList modules = m_moduleManager->modules();
    if (modules.count() != m_realModules + 2) {
        qDebug() << modules;
    }
    QCOMPARE(modules.count(), m_realModules + 2);
    QVERIFY(modules.contains("testModule.desktop"));
    QVERIFY(modules.contains(fileName));

    // Check that this didn't affect the other profile
    const QStringList modules2 = m_moduleManager2->modules();
    QCOMPARE(modules2.count(), m_realModules + 1);
    QVERIFY(modules2.contains("testModule.desktop"));

    fileName = "local%1.desktop";
    const QString secondPath = m_moduleManager->addModuleFromTemplate(fileName);
    QCOMPARE(fileName, QString("local1.desktop"));
    QVERIFY(secondPath.endsWith("local1.desktop"));
}

void ModuleManagerTest::testRenameGlobalModule()
{
    m_moduleManager->setModuleName("testModule.desktop", "new name");
    const QStringList modules = m_moduleManager->modules();
    if (modules.count() != m_realModules + 2) {
        qDebug() << modules;
    }
    QCOMPARE(modules.count(), m_realModules + 2);
    QVERIFY(modules.contains("testModule.desktop"));
    // A local copy was made
    const QString localCopy = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1Char('/') + "konqsidebartng/entries/testModule.desktop";
    qDebug() << localCopy;
    QVERIFY(QFile(localCopy).exists());
    // We didn't lose the icon (e.g. due to lack of merging)
    // Well, this code does the merging ;)
    const QString icon = KDesktopFile(QStandardPaths::GenericDataLocation, "konqsidebartng/entries/testModule.desktop").readIcon();
    QVERIFY(!icon.isEmpty());
}


void ModuleManagerTest::testRemoveLocalModule()
{
    m_moduleManager->removeModule("local.desktop");
    QCOMPARE(m_moduleManager->modules().count(), m_realModules + 1);
    QCOMPARE(m_moduleManager2->modules().count(), m_realModules + 1);
}

void ModuleManagerTest::testRemoveGlobalModule()
{
    m_moduleManager->removeModule("testModule.desktop");
    QCOMPARE(m_moduleManager->modules().count(), m_realModules + 0);
    QCOMPARE(m_moduleManager2->modules().count(), m_realModules + 1); // not affected.
}

void ModuleManagerTest::testReAddGlobalModule()
{
    m_moduleManager->moduleAdded("testModule.desktop");
    // It should re-appear once, not twice [was: 1 because global desktop file and 1 because in AddedModules]
    QCOMPARE(m_moduleManager->modules().count(), m_realModules + 1);
    QCOMPARE(m_moduleManager2->modules().count(), m_realModules + 1); // not affected.
}

void ModuleManagerTest::testRollback()
{
    m_moduleManager->rollbackToDefault();

    // FIXME: This will fail because testModule.desktop will have been removed
    // by testRemoveGlobalModule() above.
    //testListModules();
}

void ModuleManagerTest::testAvailablePlugins()
{
    KService::List availablePlugins = m_moduleManager->availablePlugins();
    QVERIFY(availablePlugins.count() >= 2);
    QStringList libs;
    Q_FOREACH (KService::Ptr service, availablePlugins) {
        libs.append(service->library());
    }
    qDebug() << libs;
    QVERIFY(libs.contains("konqsidebar_tree"));
    // konqsidebar_web is not built because it depends on KHTML
    //QVERIFY(libs.contains("konqsidebar_web"));
}

#include "modulemanagertest.moc"
