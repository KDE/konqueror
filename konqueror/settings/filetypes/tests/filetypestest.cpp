/* This file is part of the KDE project
    Copyright 2007 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License or ( at
   your option ) version 3 or, at the discretion of KDE e.V. ( which shall
   act as a proxy as in section 14 of the GPLv3 ), any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   Boston, MA 02110-1301, USA.
*/

#include <kprocess.h>
#include <kservice.h>
#include <qtest_kde.h>

#include <kconfiggroup.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <ksycoca.h>

#include <mimetypedata.h>
#include <mimetypewriter.h>


class FileTypesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_mimeTypeCreatedSuccessfully = false;
        const QString kdehome = QDir::home().canonicalPath() + "/.kde-unit-test";
        // We need a place where we can hack a mimeapps.list without harm, so not ~/.local
        // This test relies on shared-mime-info being installed in /usr/share [or kdedir/share]
        //::setenv("XDG_DATA_DIRS", QFile::encodeName(kdehome + "/share:/usr/share"), 1 );
        ::setenv("XDG_DATA_HOME", QFile::encodeName(kdehome) + "/local", 1);
        QStringList appsDirs = KGlobal::dirs()->resourceDirs("xdgdata-apps");
        //kDebug() << appsDirs;
        m_localApps = kdehome + "/local/applications/";
        QCOMPARE(appsDirs.first(), m_localApps);
        QCOMPARE(KGlobal::dirs()->resourceDirs("xdgdata-mime").first(), kdehome + "/local/mime/");

        QFile::remove(m_localApps + "mimeapps.list");

        // Create fake applications for some tests below.
        bool mustUpdateKSycoca = false;
        fakeApplication = "fakeapplication.desktop";
        if (createDesktopFile(m_localApps + fakeApplication))
            mustUpdateKSycoca = true;
        fakeApplication2 = "fakeapplication2.desktop";
        if (createDesktopFile(m_localApps + fakeApplication2))
            mustUpdateKSycoca = true;

        // Cleanup after testMimeTypePatterns if it failed mid-way
        const QString packageFileName = KStandardDirs::locateLocal( "xdgdata-mime", "packages/text-plain.xml" );
        if (!packageFileName.isEmpty()) {
            QFile::remove(packageFileName);
            MimeTypeWriter::runUpdateMimeDatabase();
            mustUpdateKSycoca = true;
        }

        QFile::remove(KStandardDirs::locateLocal("config", "filetypesrc"));

        if ( mustUpdateKSycoca ) {
            // Update ksycoca in ~/.kde-unit-test after creating the above
            runKBuildSycoca();
        }
        KService::Ptr fakeApplicationService = KService::serviceByStorageId(fakeApplication);
        QVERIFY(fakeApplicationService);
    }

    void testMimeTypeGroupAutoEmbed()
    {
        MimeTypeData data("text");
        QCOMPARE(data.majorType(), QString("text"));
        QCOMPARE(data.name(), QString("text"));
        QVERIFY(data.isMeta());
        QCOMPARE(data.autoEmbed(), MimeTypeData::No); // text doesn't autoembed by default
        QVERIFY(!data.isDirty());
        data.setAutoEmbed(MimeTypeData::Yes);
        QCOMPARE(data.autoEmbed(), MimeTypeData::Yes);
        QVERIFY(data.isDirty());
        QVERIFY(!data.sync()); // save to disk. Should succeed, but return false (no need to run update-mime-database)
        QVERIFY(!data.isDirty());
        // Check what's on disk by creating another MimeTypeData instance
        MimeTypeData data2("text");
        QCOMPARE(data2.autoEmbed(), MimeTypeData::Yes);
        QVERIFY(!data2.isDirty());
        data2.setAutoEmbed(MimeTypeData::No); // revert to default, for next time
        QVERIFY(data2.isDirty());
        QVERIFY(!data2.sync());
        QVERIFY(!data2.isDirty());

        // TODO test askSave after cleaning up the code
    }

    void testMimeTypeAutoEmbed()
    {
        MimeTypeData data(KMimeType::mimeType("text/plain"));
        QCOMPARE(data.majorType(), QString("text"));
        QCOMPARE(data.minorType(), QString("plain"));
        QCOMPARE(data.name(), QString("text/plain"));
        QVERIFY(!data.isMeta());
        QCOMPARE(data.autoEmbed(), MimeTypeData::UseGroupSetting);
        QVERIFY(!data.isDirty());
        data.setAutoEmbed(MimeTypeData::Yes);
        QCOMPARE(data.autoEmbed(), MimeTypeData::Yes);
        QVERIFY(data.isDirty());
        QVERIFY(!data.sync()); // save to disk. Should succeed, but return false (no need to run update-mime-database)
        QVERIFY(!data.isDirty());
        // Check what's on disk by creating another MimeTypeData instance
        MimeTypeData data2(KMimeType::mimeType("text/plain"));
        QCOMPARE(data2.autoEmbed(), MimeTypeData::Yes);
        QVERIFY(!data2.isDirty());
        data2.setAutoEmbed(MimeTypeData::UseGroupSetting); // revert to default, for next time
        QVERIFY(data2.isDirty());
        QVERIFY(!data2.sync());
        QVERIFY(!data2.isDirty());
    }

    void testMimeTypePatterns()
    {
        MimeTypeData data(KMimeType::mimeType("text/plain"));
        QCOMPARE(data.name(), QString("text/plain"));
        QCOMPARE(data.majorType(), QString("text"));
        QCOMPARE(data.minorType(), QString("plain"));
        QVERIFY(!data.isMeta());
        QStringList patterns = data.patterns();
        QVERIFY(patterns.contains("*.txt"));
        QVERIFY(!patterns.contains("*.toto"));
        const QStringList origPatterns = patterns;
        patterns.append("*.toto"); // yes, a french guy wrote this, as you can see
        patterns.sort(); // for future comparisons
        QVERIFY(!data.isDirty());
        data.setPatterns(patterns);
        QVERIFY(data.isDirty());
        bool needUpdateMimeDb = data.sync();
        QVERIFY(needUpdateMimeDb);
        MimeTypeWriter::runUpdateMimeDatabase();
        runKBuildSycoca();
        QCOMPARE(data.patterns(), patterns);
        data.refresh(); // reload from ksycoca
        QCOMPARE(data.patterns(), patterns);
        QVERIFY(!data.isDirty());
        // Check what's in ksycoca
        QStringList newPatterns = KMimeType::mimeType("text/plain")->patterns();
        newPatterns.sort();
        QCOMPARE(newPatterns, patterns);

        // Remove custom file (it's in ~/.local, not in ~/.kde-unit-test, so it messes up the user's configuration!)
        const QString packageFileName = KStandardDirs::locateLocal( "xdgdata-mime", "packages/text-plain.xml" );
        QVERIFY(!packageFileName.isEmpty());
        QFile::remove(packageFileName);
        MimeTypeWriter::runUpdateMimeDatabase();
        runKBuildSycoca();
        // Check what's in ksycoca
        newPatterns = KMimeType::mimeType("text/plain")->patterns();
        newPatterns.sort();
        QCOMPARE(newPatterns, origPatterns);
    }

    void testAddService()
    {
        const char* mimeTypeName = "application/vnd.oasis.opendocument.text";
        MimeTypeData data(KMimeType::mimeType(mimeTypeName));
        QStringList appServices = data.appServices();
        //kDebug() << appServices;
        const QString oldPreferredApp = appServices.first();
        QVERIFY(!appServices.contains(fakeApplication)); // already there? hmm can't really test then
        QVERIFY(!data.isDirty());
        appServices.prepend(fakeApplication);
        data.setAppServices(appServices);
        QVERIFY(data.isDirty());
        QVERIFY(!data.sync()); // success, but no need to run update-mime-database
        runKBuildSycoca();
        QVERIFY(!data.isDirty());
        // Check what's in ksycoca
        checkMimeTypeServices(mimeTypeName, appServices);

        // Now test removing (in the same test, since it's inter-dependent)
        QVERIFY(appServices.removeAll(fakeApplication) > 0);
        data.setAppServices(appServices);
        QVERIFY(data.isDirty());
        QVERIFY(!data.sync()); // success, but no need to run update-mime-database
        runKBuildSycoca();
        // Check what's in ksycoca
        checkMimeTypeServices(mimeTypeName, appServices);
        // Check what's in mimeapps.list
        checkRemovedAssociationsContains(mimeTypeName, fakeApplication);
    }

    void testRemoveTwice()
    {
        // Remove fakeApplication from image/png
        const char* mimeTypeName = "image/png";
        MimeTypeData data(KMimeType::mimeType(mimeTypeName));
        QStringList appServices = data.appServices();
        kDebug() << "initial list for" << mimeTypeName << appServices;
        QVERIFY(appServices.removeAll(fakeApplication) > 0);
        data.setAppServices(appServices);
        QVERIFY(!data.sync()); // success, but no need to run update-mime-database
        runKBuildSycoca();
        // Check what's in ksycoca
        checkMimeTypeServices(mimeTypeName, appServices);
        // Check what's in mimeapps.list
        checkRemovedAssociationsContains(mimeTypeName, fakeApplication);

        // Remove fakeApplication2 from image/png; must keep the previous entry in "Removed Associations"
        QVERIFY(appServices.removeAll(fakeApplication2) > 0);
        data.setAppServices(appServices);
        QVERIFY(!data.sync()); // success, but no need to run update-mime-database
        runKBuildSycoca();
        // Check what's in ksycoca
        checkMimeTypeServices(mimeTypeName, appServices);
        // Check what's in mimeapps.list
        checkRemovedAssociationsContains(mimeTypeName, fakeApplication);
        // Check what's in mimeapps.list
        checkRemovedAssociationsContains(mimeTypeName, fakeApplication2);
    }

    void testCreateMimeType()
    {
        const QString mimeTypeName = "fake/unit-test-fake-mimetype";
        // Clean up after previous runs if necessary
        if (MimeTypeWriter::hasDefinitionFile(mimeTypeName))
            MimeTypeWriter::removeOwnMimeType(mimeTypeName);

        MimeTypeData data(mimeTypeName, true);
        data.setComment("Fake MimeType");
        QStringList patterns = QStringList() << "*.fake";
        data.setPatterns(patterns);
        QVERIFY(data.isDirty());
        QVERIFY(data.sync());
        MimeTypeWriter::runUpdateMimeDatabase();
        runKBuildSycoca();
        KMimeType::Ptr mime = KMimeType::mimeType(mimeTypeName);
        QVERIFY(mime);
        QCOMPARE(mime->comment(), QString("Fake MimeType"));
        QCOMPARE(mime->patterns(), patterns);
        m_mimeTypeCreatedSuccessfully = true;
    }

    void testDeleteMimeType()
    {
        if (!m_mimeTypeCreatedSuccessfully)
            QSKIP("This test relies on testCreateMimeType", SkipAll);
        const QString mimeTypeName = "fake/unit-test-fake-mimetype";
        QVERIFY(MimeTypeWriter::hasDefinitionFile(mimeTypeName));
        MimeTypeWriter::removeOwnMimeType(mimeTypeName);
        MimeTypeWriter::runUpdateMimeDatabase();
        runKBuildSycoca();
        KMimeType::Ptr mime = KMimeType::mimeType(mimeTypeName);
        QVERIFY(!mime);
    }

    void cleanupTestCase()
    {
        // If we remove it, then every run of the unit test has to run kbuildsycoca... slow.
        //QFile::remove(KStandardDirs::locateLocal("xdgdata-apps", "fakeapplication.desktop"));
    }

private: // helper methods

    void checkRemovedAssociationsContains(const QString& mimeTypeName, const QString& application)
    {
        const KConfig config(m_localApps + "mimeapps.list", KConfig::NoGlobals);
        const KConfigGroup group(&config, "Removed Associations");
        const QStringList removedEntries = group.readXdgListEntry(mimeTypeName);
        if (!removedEntries.contains(application)) {
            kWarning() << removedEntries << "does not contain" << application;
            QVERIFY(removedEntries.contains(application));
        }
    }

    void runKBuildSycoca()
    {
        // Wait for notifyDatabaseChanged DBus signal
        // (The real KCM code simply does the refresh in a slot, asynchronously)
        QEventLoop loop;
        QObject::connect(KSycoca::self(), SIGNAL(databaseChanged()), &loop, SLOT(quit()));
        KProcess proc;
        proc << KStandardDirs::findExe(KBUILDSYCOCA_EXENAME);
        proc.setOutputChannelMode(KProcess::MergedChannels); // silence kbuildsycoca output
        proc.execute();
        loop.exec();
    }

    bool createDesktopFile(const QString& path)
    {
        if (!QFile::exists(path)) {
            KDesktopFile file(path);
            KConfigGroup group = file.desktopGroup();
            group.writeEntry("Name", "FakeApplication");
            group.writeEntry("Type", "Application");
            group.writeEntry("Exec", "ls");
            group.writeEntry("MimeType", "image/png");
            return true;
        }
        return false;
    }

    void checkMimeTypeServices(const QString& mimeTypeName, const QStringList& expectedServices)
    {
        MimeTypeData data2(KMimeType::mimeType(mimeTypeName));
        kDebug() << data2.appServices();
        QCOMPARE(data2.appServices(), expectedServices);
    }

    QString fakeApplication; // storage id of the fake application
    QString fakeApplication2; // storage id of the fake application2
    QString m_localApps;
    bool m_mimeTypeCreatedSuccessfully;
};

QTEST_KDEMAIN( FileTypesTest, NoGUI )

#include "filetypestest.moc"


