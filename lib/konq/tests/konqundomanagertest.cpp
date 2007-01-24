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

#include "konqundomanagertest.h"
#include <konq_undo.h>

#include <kio/job.h>
#include <kio/netaccess.h>

#include <kde_file.h>
#include <kdebug.h>
#include <ksimpleconfig.h>

#include <errno.h>
#include <utime.h>
#include <time.h>
#include <sys/time.h>

#include "konqundomanagertest.moc"

QTEST_KDEMAIN( KonqUndoManagerTest, NoGUI )

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

class TestUiInterface : public KonqUndoManager::UiInterface
{
public:
    TestUiInterface() {}
    virtual void jobError( KIO::Job* job ) {
        kFatal() << job->errorString() << endl;
    }
    virtual bool copiedFileWasModified( const KUrl& src, const KUrl& dest, time_t srcTime, time_t destTime ) {
        Q_UNUSED( src );
        m_dest = dest;
        Q_UNUSED( srcTime );
        Q_UNUSED( destTime );
        return true;
    }
    KUrl dest() const { return m_dest; }
private:
    KUrl m_dest;
};

void KonqUndoManagerTest::initTestCase()
{
    qDebug( "initTestCase" );

    // Get kio_trash to share our environment so that it writes trashrc to the right kdehome
    setenv( "KDE_FORK_SLAVES", "yes", true );

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
    m_uiInterface = new TestUiInterface; // owned by KonqUndoManager
    KonqUndoManager::self()->setUiInterface( m_uiInterface );
}

void KonqUndoManagerTest::cleanupTestCase()
{
    KIO::NetAccess::del( KUrl::fromPath( homeTmpDir() ), 0 );
}

void KonqUndoManagerTest::doUndo()
{
    bool ok = connect( KonqUndoManager::self(), SIGNAL( undoJobFinished() ),
                  &m_eventLoop, SLOT( quit() ) );
    QVERIFY( ok );

    KonqUndoManager::self()->undo();
    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents); // wait for undo job to finish
}

void KonqUndoManagerTest::testCopyFiles()
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

void KonqUndoManagerTest::testMoveFiles()
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
void KonqUndoManagerTest::testCopyFilesOverwrite()
{
    kDebug() << k_funcinfo << endl;
    // Create a different file in the destdir
    createTestFile( destFile(), "An old file already in the destdir" );

    testCopyFiles();
}
#endif

void KonqUndoManagerTest::testCopyDirectory()
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

void KonqUndoManagerTest::testMoveDirectory()
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

void KonqUndoManagerTest::testRenameFile()
{
    const KUrl oldUrl( srcFile() );
    const KUrl newUrl( srcFile() + ".new" );
    KUrl::List lst;
    lst.append(oldUrl);
    KIO::Job* job = KIO::moveAs( oldUrl, newUrl );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::RENAME, lst, newUrl, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcFile() ) );
    QVERIFY( QFileInfo( newUrl.path() ).isFile() );

    doUndo();

    QVERIFY( QFile::exists( srcFile() ) );
    QVERIFY( !QFileInfo( newUrl.path() ).isFile() );
}

void KonqUndoManagerTest::testRenameDir()
{
    const KUrl oldUrl( srcSubDir() );
    const KUrl newUrl( srcSubDir() + ".new" );
    KUrl::List lst;
    lst.append(oldUrl);
    KIO::Job* job = KIO::moveAs( oldUrl, newUrl );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::RENAME, lst, newUrl, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcSubDir() ) );
    QVERIFY( QFileInfo( newUrl.path() ).isDir() );

    doUndo();

    QVERIFY( QFile::exists( srcSubDir() ) );
    QVERIFY( !QFileInfo( newUrl.path() ).isDir() );
}

void KonqUndoManagerTest::testTrashFiles()
{
    // Trash it all at once: the file, the symlink, the subdir.
    KUrl::List lst = sourceList();
    lst.append( srcSubDir() );
    KIO::Job* job = KIO::trash( lst );
    job->setUiDelegate( 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::TRASH, lst, KUrl("trash:/"), job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    // Check that things got removed
    QVERIFY( !QFile::exists( srcFile() ) );
#ifndef Q_WS_WIN
    QVERIFY( !QFileInfo( srcLink() ).isSymLink() );
#endif
    QVERIFY( !QFile::exists( srcSubDir() ) );

    // check trash?
    // Let's just check that it's not empty. kio_trash has its own unit tests anyway.
    KSimpleConfig cfg( "trashrc", true );
    QVERIFY( cfg.hasGroup( "Status" ) );
    cfg.setGroup( "Status" );
    QCOMPARE( cfg.readEntry( "Empty", true ), false );

    doUndo();

    QVERIFY( QFile::exists( srcFile() ) );
#ifndef Q_WS_WIN
    QVERIFY( QFileInfo( srcLink() ).isSymLink() );
#endif
    QVERIFY( QFile::exists( srcSubDir() ) );

    // We can't check that the trash is empty; other partitions might have their own trash
}

static void setTimeStamp( const QString& path )
{
#ifdef Q_OS_UNIX
    // Put timestamp in the past so that we can check that the
    // copy actually preserves it.
    struct timeval tp;
    gettimeofday( &tp, 0 );
    struct utimbuf utbuf;
    utbuf.actime = tp.tv_sec + 30; // 30 seconds in the future
    utbuf.modtime = tp.tv_sec + 60; // 60 second in the future
    utime( QFile::encodeName( path ), &utbuf );
    qDebug( "Time changed for %s", qPrintable( path ) );
#endif
}

void KonqUndoManagerTest::testModifyFileBeforeUndo()
{
    // based on testCopyDirectory (so that we check that it works for files in subdirs too)
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
    const QString destFile =  destSubDir() + "/fileindir";
    setTimeStamp( destFile ); // simulate a modification of the file

    doUndo();

    // Check that TestUiInterface::copiedFileWasModified got called
    QCOMPARE( m_uiInterface->dest().path(), destFile );

    checkTestDirectory( srcSubDir() );
    QVERIFY( !QFile::exists( destSubDir() ) );

}

