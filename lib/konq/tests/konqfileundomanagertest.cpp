/* This file is part of KDE
    Copyright (c) 2006 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

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

#include "konqfileundomanagertest.h"
#include <konq_fileundomanager.h>

#include <kio/copyjob.h>
#include <kio/job.h>
#include <kio/deletejob.h>
#include <kio/netaccess.h>
#include <kprotocolinfo.h>

#include <kde_file.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <errno.h>
#include <utime.h>
#include <time.h>
#include <sys/time.h>

#include "konqfileundomanagertest.moc"

QTEST_KDEMAIN( KonqFileUndoManagerTest, NoGUI )

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
        kFatal() << "Can't create " << path ;
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
            kFatal() << "couldn't create symlink: " << strerror( errno ) ;
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
        kFatal() << "couldn't create " << path ;
    createTestFile( path + "/fileindir", "File in dir" );
#ifndef Q_WS_WIN
    createTestSymlink( path + "/testlink" );
#endif
    ok = dir.mkdir( path + "/dirindir" );
    if ( !ok )
        kFatal() << "couldn't create " << path ;
    createTestFile( path + "/dirindir/nested", "Nested" );
    checkTestDirectory( path );
}

class TestUiInterface : public KonqFileUndoManager::UiInterface
{
public:
    TestUiInterface() : KonqFileUndoManager::UiInterface(), m_nextReplyToConfirmDeletion(true) {
        setShowProgressInfo( false );
    }
    virtual void jobError( KIO::Job* job ) {
        kFatal() << job->errorString() ;
    }
    virtual bool copiedFileWasModified( const KUrl& src, const KUrl& dest, const KDateTime& srcTime, const KDateTime& destTime ) {
        Q_UNUSED( src );
        m_dest = dest;
        Q_UNUSED( srcTime );
        Q_UNUSED( destTime );
        return true;
    }
    virtual bool confirmDeletion( const KUrl::List& files ) {
        m_files = files;
        return m_nextReplyToConfirmDeletion;
    }
    void setNextReplyToConfirmDeletion( bool b ) {
        m_nextReplyToConfirmDeletion = b;
    }
    KUrl::List files() const { return m_files; }
    KUrl dest() const { return m_dest; }
    void clear() {
        m_dest = KUrl();
        m_files.clear();
    }
private:
    bool m_nextReplyToConfirmDeletion;
    KUrl m_dest;
    KUrl::List m_files;
};

void KonqFileUndoManagerTest::initTestCase()
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
            kFatal() << "Couldn't create " << homeTmpDir() ;
    }

    createTestFile( srcFile(), "Hello world" );
#ifndef Q_WS_WIN
    createTestSymlink( srcLink() );
#endif
    createTestDirectory( srcSubDir() );

    QDir().mkdir( destDir() );
    QVERIFY( QFileInfo( destDir() ).isDir() );

    QVERIFY( !KonqFileUndoManager::self()->undoAvailable() );
    m_uiInterface = new TestUiInterface; // owned by KonqFileUndoManager
    KonqFileUndoManager::self()->setUiInterface( m_uiInterface );
}

void KonqFileUndoManagerTest::cleanupTestCase()
{
    KIO::Job* job = KIO::del( KUrl::fromPath( homeTmpDir() ), KIO::HideProgressInfo );
    KIO::NetAccess::synchronousRun( job, 0 );
}

void KonqFileUndoManagerTest::doUndo()
{
    QEventLoop eventLoop;
    bool ok = connect( KonqFileUndoManager::self(), SIGNAL( undoJobFinished() ),
                  &eventLoop, SLOT( quit() ) );
    QVERIFY( ok );

    KonqFileUndoManager::self()->undo();
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents); // wait for undo job to finish
}

void KonqFileUndoManagerTest::testCopyFiles()
{
    kDebug() ;
    // Initially inspired from JobTest::copyFileToSamePartition()
    const QString destdir = destDir();
    KUrl::List lst = sourceList();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::copy( lst, d, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::COPY, lst, d, job );

    QSignalSpy spyUndoAvailable( KonqFileUndoManager::self(), SIGNAL(undoAvailable(bool)) );
    QVERIFY( spyUndoAvailable.isValid() );
    QSignalSpy spyTextChanged( KonqFileUndoManager::self(), SIGNAL(undoTextChanged(QString)) );
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
    QVERIFY( KonqFileUndoManager::self()->undoAvailable() );
    QCOMPARE( spyUndoAvailable.count(), 1 );
    QCOMPARE( spyTextChanged.count(), 1 );
    m_uiInterface->clear();

    m_uiInterface->setNextReplyToConfirmDeletion( false ); // act like the user didn't confirm
    KonqFileUndoManager::self()->undo();
    QCOMPARE( m_uiInterface->files().count(), 1 ); // confirmDeletion was called
    QCOMPARE( m_uiInterface->files()[0].url(), KUrl(destFile()).url() );
    QVERIFY( QFile::exists( destFile() ) ); // nothing happened yet

    // OK, now do it
    m_uiInterface->clear();
    m_uiInterface->setNextReplyToConfirmDeletion( true );
    doUndo();

    QVERIFY( !KonqFileUndoManager::self()->undoAvailable() );
    QVERIFY( spyUndoAvailable.count() >= 2 ); // it's in fact 3, due to lock/unlock emitting it as well
    QCOMPARE( spyTextChanged.count(), 2 );
    QCOMPARE( m_uiInterface->files().count(), 1 ); // confirmDeletion was called
    QCOMPARE( m_uiInterface->files()[0].url(), KUrl(destFile()).url() );

    // Check that undo worked
    QVERIFY( !QFile::exists( destFile() ) );
#ifndef Q_WS_WIN
    QVERIFY( !QFile::exists( destLink() ) );
    QVERIFY( !QFileInfo( destLink() ).isSymLink() );
#endif
}

void KonqFileUndoManagerTest::testMoveFiles()
{
    kDebug() ;
    const QString destdir = destDir();
    KUrl::List lst = sourceList();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::move( lst, d, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::MOVE, lst, d, job );

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
void KonqFileUndoManagerTest::testCopyFilesOverwrite()
{
    kDebug() ;
    // Create a different file in the destdir
    createTestFile( destFile(), "An old file already in the destdir" );

    testCopyFiles();
}
#endif

void KonqFileUndoManagerTest::testCopyDirectory()
{
    const QString destdir = destDir();
    KUrl::List lst; lst << srcSubDir();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::copy( lst, d, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::COPY, lst, d, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    checkTestDirectory( srcSubDir() ); // src untouched
    checkTestDirectory( destSubDir() );

    doUndo();

    checkTestDirectory( srcSubDir() );
    QVERIFY( !QFile::exists( destSubDir() ) );
}

void KonqFileUndoManagerTest::testMoveDirectory()
{
    const QString destdir = destDir();
    KUrl::List lst; lst << srcSubDir();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::move( lst, d, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::MOVE, lst, d, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcSubDir() ) );
    checkTestDirectory( destSubDir() );

    doUndo();

    checkTestDirectory( srcSubDir() );
    QVERIFY( !QFile::exists( destSubDir() ) );
}

void KonqFileUndoManagerTest::testRenameFile()
{
    const KUrl oldUrl( srcFile() );
    const KUrl newUrl( srcFile() + ".new" );
    KUrl::List lst;
    lst.append(oldUrl);
    KIO::Job* job = KIO::moveAs( oldUrl, newUrl, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::RENAME, lst, newUrl, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcFile() ) );
    QVERIFY( QFileInfo( newUrl.path() ).isFile() );

    doUndo();

    QVERIFY( QFile::exists( srcFile() ) );
    QVERIFY( !QFileInfo( newUrl.path() ).isFile() );
}

void KonqFileUndoManagerTest::testRenameDir()
{
    const KUrl oldUrl( srcSubDir() );
    const KUrl newUrl( srcSubDir() + ".new" );
    KUrl::List lst;
    lst.append(oldUrl);
    KIO::Job* job = KIO::moveAs( oldUrl, newUrl, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::RENAME, lst, newUrl, job );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );

    QVERIFY( !QFile::exists( srcSubDir() ) );
    QVERIFY( QFileInfo( newUrl.path() ).isDir() );

    doUndo();

    QVERIFY( QFile::exists( srcSubDir() ) );
    QVERIFY( !QFileInfo( newUrl.path() ).isDir() );
}

void KonqFileUndoManagerTest::testCreateDir()
{
    const KUrl url( srcSubDir() + ".mkdir" );
    const QString path = url.path();
    QVERIFY( !QFile::exists(path) );

    KIO::SimpleJob* job = KIO::mkdir(url);
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::MKDIR, KUrl(), url, job );
    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    QVERIFY( QFile::exists(path) );
    QVERIFY( QFileInfo(path).isDir() );

    m_uiInterface->clear();
    m_uiInterface->setNextReplyToConfirmDeletion( false ); // act like the user didn't confirm
    KonqFileUndoManager::self()->undo();
    QCOMPARE( m_uiInterface->files().count(), 1 ); // confirmDeletion was called
    QCOMPARE( m_uiInterface->files()[0].url(), url.url() );
    QVERIFY( QFile::exists(path) ); // nothing happened yet

    // OK, now do it
    m_uiInterface->clear();
    m_uiInterface->setNextReplyToConfirmDeletion( true );
    doUndo();

    QVERIFY( !QFile::exists(path) );
}

void KonqFileUndoManagerTest::testTrashFiles()
{
    if ( !KProtocolInfo::isKnownProtocol( "trash" ) )
        QSKIP( "kio_trash not installed", SkipAll );

    // Trash it all at once: the file, the symlink, the subdir.
    KUrl::List lst = sourceList();
    lst.append( srcSubDir() );
    KIO::Job* job = KIO::trash( lst, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::TRASH, lst, KUrl("trash:/"), job );

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
    KConfig cfg( "trashrc", KConfig::SimpleConfig );
    QVERIFY( cfg.hasGroup( "Status" ) );
    QCOMPARE( cfg.group("Status").readEntry( "Empty", true ), false );

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

void KonqFileUndoManagerTest::testModifyFileBeforeUndo()
{
    // based on testCopyDirectory (so that we check that it works for files in subdirs too)
    const QString destdir = destDir();
    KUrl::List lst; lst << srcSubDir();
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::copy( lst, d, KIO::HideProgressInfo );
    job->setUiDelegate( 0 );
    KonqFileUndoManager::self()->recordJob( KonqFileUndoManager::COPY, lst, d, job );

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

// TODO: add test (and fix bug) for  DND of remote urls / "Link here" (creates .desktop files) // Undo (doesn't do anything)
// TODO: add test for interrupting a moving operation and then using Undo - bug:91579
