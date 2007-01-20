/* This file is part of KDE
    Copyright (c) 2006 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <qtest_kde.h>

#include <kdebug.h>
#include <konq_undo.h>
#include <kio/job.h>
#include <kde_file.h>
#include "konqundomanagertest.h"
#include <kio/netaccess.h>
#include <errno.h>

#include "konqundomanagertest.moc"

QTEST_KDEMAIN( KonqUndomanagerTest, NoGUI )

static QString homeTmpDir() { return QFile::decodeName( getenv( "KDEHOME" ) ) + "/jobtest/"; }
static QString destDir() { return homeTmpDir() + "destdir/"; }

static QString srcFile() { return homeTmpDir() + "testfile"; }
static QString destFile() { return destDir() + "testfile"; }

#ifndef Q_WS_WIN
static QString srcLink() { return homeTmpDir() + "symlink"; }
static QString destLink() { return destDir() + "symlink"; }
#endif

static QString srcSubDir() { return homeTmpDir() + "subdir"; }
static QString destSubDir() { return destDir() + "subdir"; }

static KUrl::List sourceList()
{
    KUrl::List lst;
    lst << KUrl( srcFile() );
#ifndef Q_WS_WIN
    lst << KUrl( srcLink() );
#endif
    return lst;
}

static void createTestFile( const QString& path, const char* contents )
{
    QFile f( path );
    if ( !f.open( QIODevice::WriteOnly ) )
        kFatal() << "Can't create " << path << endl;
    f.write( QByteArray( contents ) );
    f.close();
}

static void createTestSymlink( const QString& path )
{
    // Create symlink if it doesn't exist yet
    KDE_struct_stat buf;
    if ( KDE_lstat( QFile::encodeName( path ), &buf ) != 0 ) {
        bool ok = symlink( "/IDontExist", QFile::encodeName( path ) ) == 0; // broken symlink
        if ( !ok )
            kFatal() << "couldn't create symlink: " << strerror( errno ) << endl;
        QVERIFY( KDE_lstat( QFile::encodeName( path ), &buf ) == 0 );
        QVERIFY( S_ISLNK( buf.st_mode ) );
    } else {
        QVERIFY( S_ISLNK( buf.st_mode ) );
    }
    qDebug( "symlink %s created", qPrintable( path ) );
    QVERIFY( QFileInfo( path ).isSymLink() );
}

static void checkTestDirectory( const QString& path )
{
    QVERIFY( QFileInfo( path ).isDir() );
    QVERIFY( QFileInfo( path + "/fileindir" ).isFile() );
#ifndef Q_WS_WIN
    QVERIFY( QFileInfo( path + "/testlink" ).isSymLink() );
#endif
    QVERIFY( QFileInfo( path + "/dirindir" ).isDir() );
    QVERIFY( QFileInfo( path + "/dirindir/nested" ).isFile() );
}

static void createTestDirectory( const QString& path )
{
    QDir dir;
    bool ok = dir.mkdir( path );
    if ( !ok )
        kFatal() << "couldn't create " << path << endl;
    createTestFile( path + "/fileindir", "File in dir" );
#ifndef Q_WS_WIN
    createTestSymlink( path + "/testlink" );
#endif
    ok = dir.mkdir( path + "/dirindir" );
    if ( !ok )
        kFatal() << "couldn't create " << path << endl;
    createTestFile( path + "/dirindir/nested", "Nested" );
    checkTestDirectory( path );
}

void KonqUndomanagerTest::initTestCase()
{
    qDebug( "initTestCase" );
    // Start with a clean base dir
    cleanupTestCase();

    QDir dir; // TT: why not a static method?
    if ( !QFile::exists( homeTmpDir() ) ) {
        bool ok = dir.mkdir( homeTmpDir() );
        if ( !ok )
            kFatal() << "Couldn't create " << homeTmpDir() << endl;
    }

    createTestFile( srcFile(), "Hello world" );
#ifndef Q_WS_WIN
    createTestSymlink( srcLink() );
#endif
    createTestDirectory( srcSubDir() );

    QDir().mkdir( destDir() );
    QVERIFY( QFileInfo( destDir() ).isDir() );

    QVERIFY( !KonqUndoManager::self()->undoAvailable() );
}

void KonqUndomanagerTest::cleanupTestCase()
{
    KIO::NetAccess::del( KUrl::fromPath( homeTmpDir() ), 0 );
}

void KonqUndomanagerTest::doUndo()
{
    bool ok = connect( KonqUndoManager::self(), SIGNAL( undoJobFinished() ),
                  &m_eventLoop, SLOT( quit() ) );
    QVERIFY( ok );

    KonqUndoManager::self()->undo();
    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents); // wait for undo job to finish
}

void KonqUndomanagerTest::testCopyFiles()
{
    kDebug() << k_funcinfo << endl;
    // Initially inspired from JobTest::copyFileToSamePartition()
    const QString destdir = destDir();
    KUrl::List lst = sourceList();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::copy( lst, d, 0 );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::COPY, lst, d, job );

    QSignalSpy spyUndoAvailable( KonqUndoManager::self(), SIGNAL(undoAvailable(bool)) );
    QVERIFY( spyUndoAvailable.isValid() );
    QSignalSpy spyTextChanged( KonqUndoManager::self(), SIGNAL(undoTextChanged(QString)) );
    QVERIFY( spyTextChanged.isValid() );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( QFile::exists( destFile() ) );
#ifndef Q_WS_WIN
    // Don't use QFile::exists, it's a broken symlink...
    QVERIFY( QFileInfo( destLink() ).isSymLink() );
#endif

    // might have to wait for dbus signal here... but this is currently disabled.
    //QTest::qWait( 20 );
    QVERIFY( KonqUndoManager::self()->undoAvailable() );
    QCOMPARE( spyUndoAvailable.count(), 1 );
    QCOMPARE( spyTextChanged.count(), 1 );

    doUndo();

    QVERIFY( !KonqUndoManager::self()->undoAvailable() );
    QVERIFY( spyUndoAvailable.count() >= 2 ); // it's in fact 3, due to lock/unlock emitting it as well
    QCOMPARE( spyTextChanged.count(), 2 );

    // Check that undo worked
    QVERIFY( !QFile::exists( destFile() ) );
#ifndef Q_WS_WIN
    QVERIFY( !QFile::exists( destLink() ) );
    QVERIFY( !QFileInfo( destLink() ).isSymLink() );
#endif
}

void KonqUndomanagerTest::testMoveFiles()
{
    kDebug() << k_funcinfo << endl;
    const QString destdir = destDir();
    KUrl::List lst = sourceList();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::move( lst, d, 0 );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::MOVE, lst, d, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcFile() ) ); // the source moved
    QVERIFY( QFile::exists( destFile() ) );
#ifndef Q_WS_WIN
    QVERIFY( !QFileInfo( srcLink() ).isSymLink() );
    // Don't use QFile::exists, it's a broken symlink...
    QVERIFY( QFileInfo( destLink() ).isSymLink() );
#endif

    doUndo();

    QVERIFY( QFile::exists( srcFile() ) ); // the source is back
    QVERIFY( !QFile::exists( destFile() ) );
#ifndef Q_WS_WIN
    QVERIFY( QFileInfo( srcLink() ).isSymLink() );
    QVERIFY( !QFileInfo( destLink() ).isSymLink() );
#endif
}

// Testing for overwrite isn't possible, because non-interactive jobs never overwrite.
// And nothing different happens anyway, the dest is removed...
#if 0
void KonqUndomanagerTest::testCopyFilesOverwrite()
{
    kDebug() << k_funcinfo << endl;
    // Create a different file in the destdir
    createTestFile( destFile(), "An old file already in the destdir" );

    testCopyFiles();
}
#endif

void KonqUndomanagerTest::testCopyDirectory()
{
    const QString destdir = destDir();
    KUrl::List lst; lst << srcSubDir();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::copy( lst, d, 0 );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::COPY, lst, d, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    checkTestDirectory( srcSubDir() ); // src untouched
    checkTestDirectory( destSubDir() );

    doUndo();

    checkTestDirectory( srcSubDir() );
    QVERIFY( !QFile::exists( destSubDir() ) );
}

void KonqUndomanagerTest::testMoveDirectory()
{
    const QString destdir = destDir();
    KUrl::List lst; lst << srcSubDir();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::move( lst, d, 0 );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::MOVE, lst, d, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcSubDir() ) );
    checkTestDirectory( destSubDir() );

    doUndo();

    checkTestDirectory( srcSubDir() );
    QVERIFY( !QFile::exists( destSubDir() ) );
}

// TODO: add test for undoing after a partial move (http://bugs.kde.org/show_bug.cgi?id=91579)
