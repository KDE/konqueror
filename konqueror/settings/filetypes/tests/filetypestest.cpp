/* This file is part of the KDE project
   Copyright (C) 2007 David Faure <faure@kde.org>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   Boston, MA 02110-1301, USA.
*/

#include <qtest_kde.h>

#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <kstandarddirs.h>
#include <ksycoca.h>

#include <mimetypedata.h>


class FileTypesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        const QString profilerc = KStandardDirs::locateLocal( "config", "profilerc" );
        if ( !profilerc.isEmpty() )
            QFile::remove( profilerc );

        // Create fake application for some tests below.
        bool mustUpdateKSycoca = false;
        const QString fakeApplication = KStandardDirs::locateLocal("xdgdata-apps", "fakeapplication.desktop");
        const bool mustCreateFakeService = !QFile::exists(fakeApplication);
        if (mustCreateFakeService) {
            mustUpdateKSycoca = true;
            KDesktopFile file(fakeApplication);
            KConfigGroup group = file.desktopGroup();
            group.writeEntry("Name", "FakeApplication");
            group.writeEntry("Type", "Application");
            group.writeEntry("Exec", "ls");
        }

        // Cleanup after testMimeTypePatterns if it failed mid-way
        const QString packageFileName = KStandardDirs::locateLocal( "xdgdata-mime", "packages/text-plain.xml" );
        if (!packageFileName.isEmpty()) {
            QFile::remove(packageFileName);
            MimeTypeData::runUpdateMimeDatabase();
            mustUpdateKSycoca = true;
        }


        if ( mustUpdateKSycoca ) {
            // Update ksycoca in ~/.kde-unit-test after creating the above
            QProcess::execute( KGlobal::dirs()->findExe(KBUILDSYCOCA_EXENAME) );
        }
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
        data.sync(); // save to disk
        QVERIFY(!data.isDirty());
        // Check what's on disk by creating another MimeTypeData instance
        MimeTypeData data2("text");
        QCOMPARE(data2.autoEmbed(), MimeTypeData::Yes);
        QVERIFY(!data2.isDirty());
        data2.setAutoEmbed(MimeTypeData::No); // revert to default, for next time
        QVERIFY(data2.isDirty());
        data2.sync();
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
        data.sync(); // save to disk
        QVERIFY(!data.isDirty());
        // Check what's on disk by creating another MimeTypeData instance
        MimeTypeData data2(KMimeType::mimeType("text/plain"));
        QCOMPARE(data2.autoEmbed(), MimeTypeData::Yes);
        QVERIFY(!data2.isDirty());
        data2.setAutoEmbed(MimeTypeData::UseGroupSetting); // revert to default, for next time
        QVERIFY(data2.isDirty());
        data2.sync();
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
        MimeTypeData::runUpdateMimeDatabase();
        QProcess::execute( KGlobal::dirs()->findExe(KBUILDSYCOCA_EXENAME) );
        // Wait for notifyDatabaseChanged DBus signal
        // (The real KCM code simply does the refresh in a slot, asynchronously)
        QEventLoop loop;
        QObject::connect(KSycoca::self(), SIGNAL(databaseChanged()), &loop, SLOT(quit()));
        loop.exec();
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
        MimeTypeData::runUpdateMimeDatabase();
        QProcess::execute( KGlobal::dirs()->findExe(KBUILDSYCOCA_EXENAME) );
        loop.exec(); // need to process DBUS signal
        // Check what's in ksycoca
        newPatterns = KMimeType::mimeType("text/plain")->patterns();
        newPatterns.sort();
        QCOMPARE(newPatterns, origPatterns);
    }

    // TODO see TODO in filetypesview
    //void testDeleteMimeType()
    //{
    //}

    void cleanupTestCase()
    {
        // If we remove it, then every run of the unit test has to run kbuildsycoca... slow.
        //QFile::remove(KStandardDirs::locateLocal("xdgdata-apps", "fakeapplication.desktop"));
    }
};

QTEST_KDEMAIN( FileTypesTest, NoGUI )

#include "filetypestest.moc"


