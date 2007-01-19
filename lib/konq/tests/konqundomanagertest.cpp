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

static QString homeTmpDir()
{
    return QFile::decodeName( getenv( "KDEHOME" ) ) + "/jobtest/";
}

static void createTestFile( const QString& path )
{
    QFile f( path );
    if ( !f.open( QIODevice::WriteOnly ) )
        kFatal() << "Can't create " << path << endl;
    f.write( QByteArray( "Hello world" ) );
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

static void createTestDirectory( const QString& path )
{
    QDir dir;
    bool ok = dir.mkdir( path );
    if ( !ok && !dir.exists() )
        kFatal() << "couldn't create " << path << endl;
    createTestFile( path + "/testfile" );
#ifndef Q_WS_WIN
    createTestSymlink( path + "/testlink" );
    QVERIFY( QFileInfo( path + "/testlink" ).isSymLink() );
#endif
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

    QVERIFY( !KonqUndoManager::self()->undoAvailable() );
}

void KonqUndomanagerTest::cleanupTestCase()
{
    KIO::NetAccess::del( KUrl::fromPath( homeTmpDir() ), 0 );
}

void KonqUndomanagerTest::testCopyFiles()
{
    kDebug() << k_funcinfo << endl;
    // See JobTest::copyFileToSamePartition()
    const QString filePath = homeTmpDir() + "fileFromHome";
    createTestFile( filePath );
#ifndef Q_WS_WIN
    const QString linkPath = homeTmpDir() + "symlink";
    createTestSymlink( linkPath );
#endif
    const QString destdir = homeTmpDir() + "destdir";
    QDir().mkdir( destdir );
    KUrl::List lst;
    lst << KUrl( filePath );
#ifndef Q_WS_WIN
    lst << KUrl( linkPath );
#endif
    const KUrl d( destdir );
    KIO::CopyJob* job = KIO::copy( lst, d, 0 );
    KonqUndoManager::self()->recordJob( KonqUndoManager::COPY, lst, d, job );

    QSignalSpy spyUndoAvailable( KonqUndoManager::self(), SIGNAL(undoAvailable(bool)) );
    QVERIFY( spyUndoAvailable.isValid() );
    QSignalSpy spyTextChanged( KonqUndoManager::self(), SIGNAL(undoTextChanged(QString)) );
    QVERIFY( spyTextChanged.isValid() );

    bool ok = KIO::NetAccess::synchronousRun( job, 0 );
    QVERIFY( ok );
    QVERIFY( QFile::exists( destdir + "/fileFromHome" ) );
#ifndef Q_WS_WIN
    // Don't use QFile::exists, it's a broken symlink...
    QVERIFY( QFileInfo( destdir + "/symlink" ).isSymLink() );
#endif

    // might have to wait for dbus signal here... but this is currently disabled.
    //QTest::qWait( 20 );

    QVERIFY( KonqUndoManager::self()->undoAvailable() );
    QCOMPARE( spyUndoAvailable.count(), 1 );
    QCOMPARE( spyTextChanged.count(), 1 );

    ok = connect( KonqUndoManager::self(), SIGNAL( undoJobFinished() ),
                  &m_eventLoop, SLOT( quit() ) );
    QVERIFY( ok );

    // Now undo
    KonqUndoManager::self()->undo();
    m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents); // wait for undo job to finish

    QVERIFY( !KonqUndoManager::self()->undoAvailable() );
    QVERIFY( spyUndoAvailable.count() >= 2 ); // it's in fact 3, due to lock/unlock emitting it as well
    QCOMPARE( spyTextChanged.count(), 2 );

    // Check that undo worked
    QVERIFY( !QFile::exists( destdir + "/fileFromHome" ) );
#ifndef Q_WS_WIN
    QVERIFY( !QFile::exists( destdir + "/symlink" ) );
    QVERIFY( !QFileInfo( destdir + "/symlink" ).isSymLink() );
#endif
}
