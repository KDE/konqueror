/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2006 David Faure <faure@kde.org>

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

#include "konq_undo.h"
#include "konq_undo_p.h"
#include "undomanageradaptor.h"
#include "konq_operations.h"

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <QtDBus/QtDBus>
#include <kdirnotify.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <kuiserverjobtracker.h>

#include <QDateTime>

#include <assert.h>


/**
 * checklist:
 * copy dir -> overwrite -> works
 * move dir -> overwrite -> works
 * copy dir -> rename -> works
 * move dir -> rename -> works
 *
 * copy dir -> works
 * move dir -> works
 *
 * copy files -> works
 * move files -> works (TODO: optimize (change FileCopyJob to use the renamed arg for copyingDone)
 *
 * copy files -> overwrite -> works (sorry for your overwritten file...)
 * move files -> overwrite -> works (sorry for your overwritten file...)
 *
 * copy files -> rename -> works
 * move files -> rename -> works
 *
 * -> see also konqundomanagertest, which tests some of the above (but not renaming).
 *
 */

enum UndoState { MAKINGDIRS = 0, MOVINGFILES, STATINGFILE, REMOVINGDIRS, REMOVINGLINKS };
static const char* undoStateToString( UndoState state ) {
    static const char* s_undoStateToString[] = { "MAKINGDIRS", "MOVINGFILES", "STATINGFILE", "REMOVINGDIRS", "REMOVINGLINKS" };
    return s_undoStateToString[state];
}

class KonqUndoJob : public KIO::Job
{
public:
    KonqUndoJob() : KIO::Job( ) { KIO::getJobTracker()->registerJob(this); KonqUndoManager::incRef(); }
    virtual ~KonqUndoJob() { KonqUndoManager::decRef(); }

    virtual void kill( bool ) { KonqUndoManager::self()->stopUndo( true ); KIO::Job::doKill(); }

    void emitCreatingDir(const KUrl &dir)
    { emit description(this, i18n("Creating directory"),
                       qMakePair(i18n("Directory"), dir.prettyUrl())); }
    void emitMoving(const KUrl &src, const KUrl &dest)
    { emit description(this, i18n("Moving"),
                       qMakePair(i18n("Source"), src.prettyUrl()),
                       qMakePair(i18n("Destination"), dest.prettyUrl())); }
    void emitDeleting(const KUrl &url)
    { emit description(this, i18n("Deleting"),
                       qMakePair(i18n("File"), url.prettyUrl())); }
    void emitResult() { KIO::Job::emitResult(); }
};

KonqCommandRecorder::KonqCommandRecorder( KonqUndoManager::CommandType op, const KUrl::List &src, const KUrl &dst, KIO::Job *job )
  : QObject( job )
{
  setObjectName( "konqcmdrecorder" );

  m_cmd.m_type = op;
  m_cmd.m_valid = true;
  m_cmd.m_src = src;
  m_cmd.m_dst = dst;
  connect( job, SIGNAL( result( KJob * ) ),
           this, SLOT( slotResult( KJob * ) ) );

  if ( op != KonqUndoManager::MKDIR ) {
      connect( job, SIGNAL( copyingDone(KIO::Job*,const KUrl&,const KUrl&,time_t,bool,bool) ),
               this, SLOT( slotCopyingDone(KIO::Job*,const KUrl&,const KUrl&,time_t,bool,bool) ) );
      connect( job, SIGNAL( copyingLinkDone( KIO::Job *, const KUrl &, const QString &, const KUrl & ) ),
               this, SLOT( slotCopyingLinkDone( KIO::Job *, const KUrl &, const QString &, const KUrl & ) ) );
  }

  KonqUndoManager::incRef();
}

KonqCommandRecorder::~KonqCommandRecorder()
{
  KonqUndoManager::decRef();
}

void KonqCommandRecorder::slotResult( KJob *job )
{
  if ( job->error() )
    return;

  KonqUndoManager::self()->addCommand( m_cmd );
}

void KonqCommandRecorder::slotCopyingDone( KIO::Job *job, const KUrl &from, const KUrl &to, time_t mtime, bool directory, bool renamed )
{
  KonqBasicOperation op;
  op.m_valid = true;
  op.m_type = directory ? KonqBasicOperation::Directory : KonqBasicOperation::File;
  op.m_renamed = renamed;
  op.m_src = from;
  op.m_dst = to;
  op.m_mtime = mtime;

  if ( m_cmd.m_type == KonqUndoManager::TRASH )
  {
      Q_ASSERT( to.protocol() == "trash" );
      QMap<QString, QString> metaData = job->metaData();
      QMap<QString, QString>::ConstIterator it = metaData.find( "trashURL-" + from.path() );
      if ( it != metaData.end() ) {
          // Update URL
          op.m_dst = it.value();
      }
  }

  m_cmd.m_opStack.prepend( op );
}

// TODO merge the signals?
void KonqCommandRecorder::slotCopyingLinkDone( KIO::Job *, const KUrl &from, const QString &target, const KUrl &to )
{
  KonqBasicOperation op;
  op.m_valid = true;
  op.m_type = KonqBasicOperation::Link;
  op.m_renamed = false;
  op.m_src = from;
  op.m_target = target;
  op.m_dst = to;
  op.m_mtime = -1;
  m_cmd.m_opStack.prepend( op );
}

KonqUndoManager *KonqUndoManager::s_self = 0;
static unsigned long s_undoManagerRefCnt = 0;

class KonqUndoManager::KonqUndoManagerPrivate
{
public:
    KonqUndoManagerPrivate()
        : m_uiInterface( new KonqUndoManager::UiInterface( 0 /*TODO*/ ) ),
          m_undoJob( 0 )
    {
    }
    ~KonqUndoManagerPrivate()
    {
        delete m_uiInterface;
    }

    bool m_syncronized;
    bool m_lock;

    KonqCommand::Stack m_commands;

    KonqCommand m_current;
    KIO::Job *m_currentJob;
    UndoState m_undoState;
    QStack<KUrl> m_dirStack;
    QStack<KUrl> m_dirCleanupStack;
    QStack<KUrl> m_linkCleanupStack;
    QList<KUrl> m_dirsToUpdate;
    KonqUndoManager::UiInterface* m_uiInterface;

    KonqUndoJob *m_undoJob;
};

KonqUndoManager::KonqUndoManager()
{
    (void) new UndoManagerAdaptor( this );
    const QString dbusPath = "/KonqUndoManager";
    const QString dbusInterface = "org.kde.libkonq.UndoManager";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    dbus.connect(QString(), dbusPath, dbusInterface, "lock", this, SLOT(slotLock()));
    dbus.connect(QString(), dbusPath, dbusInterface, "pop", this, SLOT(slotPop()));
    dbus.connect(QString(), dbusPath, dbusInterface, "push", this, SLOT(slotPush(QByteArray)) );
    dbus.connect(QString(), dbusPath, dbusInterface, "unlock", this, SLOT(slotUnlock()) );

    d = new KonqUndoManagerPrivate;
    d->m_syncronized = initializeFromKDesky();
    d->m_lock = false;
    d->m_currentJob = 0;
}

KonqUndoManager::~KonqUndoManager()
{
    delete d;
}

void KonqUndoManager::incRef()
{
    s_undoManagerRefCnt++;
}

void KonqUndoManager::decRef()
{
    s_undoManagerRefCnt--;
    if ( s_undoManagerRefCnt == 0 && s_self )
    {
        delete s_self;
        s_self = 0;
    }
}

KonqUndoManager *KonqUndoManager::self()
{
    if ( !s_self )
    {
        if ( s_undoManagerRefCnt == 0 )
            s_undoManagerRefCnt++; // someone forgot to call incRef
        s_self = new KonqUndoManager;
    }
    return s_self;
}

void KonqUndoManager::recordJob( CommandType op, const KUrl::List &src, const KUrl &dst, KIO::Job *job )
{
    // This records what the job does and calls addCommand when done
    (void) new KonqCommandRecorder( op, src, dst, job );
}

void KonqUndoManager::addCommand( const KonqCommand &cmd )
{
    broadcastPush( cmd );
}

bool KonqUndoManager::undoAvailable() const
{
    return ( d->m_commands.count() > 0 ) && !d->m_lock;
}

QString KonqUndoManager::undoText() const
{
    if ( d->m_commands.isEmpty() )
        return i18n( "Und&o" );

    KonqUndoManager::CommandType t = d->m_commands.top().m_type;
    if ( t == KonqUndoManager::COPY )
        return i18n( "Und&o: Copy" );
    else if ( t == KonqUndoManager::LINK )
        return i18n( "Und&o: Link" );
    else if ( t == KonqUndoManager::MOVE )
        return i18n( "Und&o: Move" );
    else if ( t == KonqUndoManager::RENAME )
        return i18n( "Und&o: Rename" );
    else if ( t == KonqUndoManager::TRASH )
        return i18n( "Und&o: Trash" );
    else if ( t == KonqUndoManager::MKDIR )
        return i18n( "Und&o: Create Folder" );
    else
        assert( false );
    /* NOTREACHED */
    return QString();
}

void KonqUndoManager::undo()
{
    // Make a copy of the command to undo before broadcastPop() pops it.
    KonqCommand cmd = d->m_commands.top();
    assert( cmd.m_valid );
    d->m_current = cmd;

    KonqBasicOperation::Stack& opStack = d->m_current.m_opStack;
    assert( !opStack.isEmpty() );

    // Let's first ask for confirmation if we need to delete any file (#99898)
    KUrl::List fileCleanupStack;
    QStack<KonqBasicOperation>::Iterator it = opStack.begin();
    for ( ; it != opStack.end() ; ++it ) {
        KonqBasicOperation::Type type = (*it).m_type;
        if ( type == KonqBasicOperation::File && d->m_current.m_type == KonqUndoManager::COPY ) {
            fileCleanupStack.append( (*it).m_dst );
        }
    }
    if ( !fileCleanupStack.isEmpty() ) {
        if ( !d->m_uiInterface->confirmDeletion( fileCleanupStack ) ) {
            return;
        }
    }

    broadcastPop();
    broadcastLock();

    d->m_dirCleanupStack.clear();
    d->m_dirStack.clear();
    d->m_dirsToUpdate.clear();

    d->m_undoState = MOVINGFILES;

    // Let's have a look at the basic operations we need to undo.
    // While we're at it, collect all links that should be deleted.

    it = opStack.begin();
    while ( it != opStack.end() ) // don't cache end() here, erase modifies it
    {
        bool removeBasicOperation = false;
        KonqBasicOperation::Type type = (*it).m_type;
        if ( type == KonqBasicOperation::Directory && !(*it).m_renamed )
        {
            // If any directory has to be created/deleted, we'll start with that
            d->m_undoState = MAKINGDIRS;
            // Collect all the dirs that have to be created in case of a move undo.
            if ( d->m_current.isMoveCommand() )
                d->m_dirStack.push( (*it).m_src );
            // Collect all dirs that have to be deleted
            // from the destination in both cases (copy and move).
            d->m_dirCleanupStack.prepend( (*it).m_dst );
            removeBasicOperation = true;
        }
        else if ( type == KonqBasicOperation::Link )
        {
            d->m_linkCleanupStack.prepend( (*it).m_dst );

            removeBasicOperation = !d->m_current.isMoveCommand();
        }

        if ( removeBasicOperation )
            it = opStack.erase( it );
        else
            ++it;
    }

    kDebug(1203) << "KonqUndoManager::undo starting with " << undoStateToString(d->m_undoState) << endl;
    d->m_undoJob = new KonqUndoJob;
    undoStep();
}

void KonqUndoManager::stopUndo( bool step )
{
    d->m_current.m_opStack.clear();
    d->m_dirCleanupStack.clear();
    d->m_linkCleanupStack.clear();
    d->m_undoState = REMOVINGDIRS;
    d->m_undoJob = 0;

    if ( d->m_currentJob )
        d->m_currentJob->kill();

    d->m_currentJob = 0;

    if ( step )
        undoStep();
}

void KonqUndoManager::slotResult( KJob *job )
{
    d->m_currentJob = 0;
    if ( job->error() )
    {
        d->m_uiInterface->jobError( static_cast<KIO::Job*>( job ) );
        delete d->m_undoJob;
        stopUndo( false );
    }
    else if ( d->m_undoState == STATINGFILE )
    {
        KonqBasicOperation op = d->m_current.m_opStack.top();
        //kDebug(1203) << "KonqUndoManager::slotResult stat result for " << op.m_dst << endl;
        KIO::StatJob* statJob = static_cast<KIO::StatJob*>( job );
        time_t mtime = statJob->statResult().numberValue( KIO::UDSEntry::UDS_MODIFICATION_TIME, -1 );
        if ( mtime != op.m_mtime ) {
            kDebug(1203) << op.m_dst << " was modified after being copied!" << endl;
            if ( !d->m_uiInterface->copiedFileWasModified( op.m_src, op.m_dst, op.m_mtime, mtime ) ) {
                stopUndo( false );
            }
        }
    }

    undoStep();
}


void KonqUndoManager::addDirToUpdate( const KUrl& url )
{
    if ( !d->m_dirsToUpdate.contains( url ) )
        d->m_dirsToUpdate.prepend( url );
}

void KonqUndoManager::undoStep()
{
    d->m_currentJob = 0;

    if ( d->m_undoState == MAKINGDIRS )
        stepMakingDirectories();

    if ( d->m_undoState == MOVINGFILES || d->m_undoState == STATINGFILE )
        stepMovingFiles();

    if ( d->m_undoState == REMOVINGLINKS )
        stepRemovingLinks();

    if ( d->m_undoState == REMOVINGDIRS )
        stepRemovingDirectories();

    if ( d->m_currentJob ) {
        //TODO d->m_currentJob->setWindow(...);
        connect( d->m_currentJob, SIGNAL( result( KJob * ) ),
                 this, SLOT( slotResult( KJob * ) ) );
    }
}

void KonqUndoManager::stepMakingDirectories()
{
    if ( !d->m_dirStack.isEmpty() ) {
        KUrl dir = d->m_dirStack.pop();
        kDebug(1203) << "KonqUndoManager::stepMakingDirectories creatingDir " << dir << endl;
        d->m_currentJob = KIO::mkdir( dir );
        d->m_undoJob->emitCreatingDir( dir );
    }
    else
        d->m_undoState = MOVINGFILES;
}

// Misnamed method: It moves files back, but it also
// renames directories back, recreates symlinks,
// deletes copied files, and restores trashed files.
void KonqUndoManager::stepMovingFiles()
{
    if ( !d->m_current.m_opStack.isEmpty() )
    {
        KonqBasicOperation op = d->m_current.m_opStack.top();
        KonqBasicOperation::Type type = op.m_type;

        assert( op.m_valid );
        if ( type == KonqBasicOperation::Directory )
        {
            if ( op.m_renamed )
            {
                kDebug(1203) << "KonqUndoManager::stepMovingFiles rename " << op.m_dst << " " << op.m_src << endl;
                d->m_currentJob = KIO::rename( op.m_dst, op.m_src, false );
                d->m_undoJob->emitMoving( op.m_dst, op.m_src );
            }
            else
                assert( 0 ); // this should not happen!
        }
        else if ( type == KonqBasicOperation::Link )
        {
            kDebug(1203) << "KonqUndoManager::stepMovingFiles symlink " << op.m_target << " " << op.m_src << endl;
            d->m_currentJob = KIO::symlink( op.m_target, op.m_src, true, false );
        }
        else if ( d->m_current.m_type == KonqUndoManager::COPY )
        {
            if ( d->m_undoState == MOVINGFILES ) // dest not stat'ed yet
            {
                // Before we delete op.m_dst, let's check if it was modified (#20532)
                kDebug(1203) << "KonqUndoManager::stepMovingFiles stat " << op.m_dst << endl;
                d->m_currentJob = KIO::stat( op.m_dst );
                d->m_undoState = STATINGFILE; // temporarily
                return; // no pop() yet, we'll finish the work in slotResult
            }
            else // dest was stat'ed, and the deletion was approved in slotResult
            {
                d->m_currentJob = KIO::file_delete( op.m_dst );
                d->m_undoJob->emitDeleting( op.m_dst );
                d->m_undoState = MOVINGFILES;
            }
        }
        else if ( d->m_current.isMoveCommand()
                  || d->m_current.m_type == KonqUndoManager::TRASH )
        {
            kDebug(1203) << "KonqUndoManager::stepMovingFiles file_move " << op.m_dst << " " << op.m_src << endl;
            d->m_currentJob = KIO::file_move( op.m_dst, op.m_src, -1, true );
            d->m_undoJob->emitMoving( op.m_dst, op.m_src );
        }

        d->m_current.m_opStack.pop();
        // The above KIO jobs are lowlevel, they don't trigger KDirNotify notification
        // So we need to do it ourselves (but schedule it to the end of the undo, to compress them)
        KUrl url( op.m_dst );
        url.setPath( url.directory() );
        addDirToUpdate( url );

        url = op.m_src;
        url.setPath( url.directory() );
        addDirToUpdate( url );
    }
    else
        d->m_undoState = REMOVINGLINKS;
}

void KonqUndoManager::stepRemovingLinks()
{
    kDebug(1203) << "KonqUndoManager::stepRemovingLinks REMOVINGLINKS" << endl;
    if ( !d->m_linkCleanupStack.isEmpty() )
    {
      KUrl file = d->m_linkCleanupStack.pop();
      kDebug(1203) << "KonqUndoManager::stepRemovingLinks file_delete " << file << endl;
      d->m_currentJob = KIO::file_delete( file );
      d->m_undoJob->emitDeleting( file );

      KUrl url( file );
      url.setPath( url.directory() );
      addDirToUpdate( url );
    }
    else
    {
      d->m_undoState = REMOVINGDIRS;

      if ( d->m_dirCleanupStack.isEmpty() && d->m_current.m_type == KonqUndoManager::MKDIR )
        d->m_dirCleanupStack << d->m_current.m_dst;
    }
}

void KonqUndoManager::stepRemovingDirectories()
{
    if ( !d->m_dirCleanupStack.isEmpty() )
    {
        KUrl dir = d->m_dirCleanupStack.pop();
        kDebug(1203) << "KonqUndoManager::stepRemovingDirectories rmdir " << dir << endl;
        d->m_currentJob = KIO::rmdir( dir );
        d->m_undoJob->emitDeleting( dir );
        addDirToUpdate( dir );
    }
    else
    {
        d->m_current.m_valid = false;
        d->m_currentJob = 0;
        if ( d->m_undoJob )
        {
            kDebug(1203) << "KonqUndoManager::stepRemovingDirectories deleting undojob" << endl;
            d->m_undoJob->emitResult();
            d->m_undoJob = 0;
        }
        QList<KUrl>::ConstIterator it = d->m_dirsToUpdate.begin();
        for( ; it != d->m_dirsToUpdate.end(); ++it ) {
            kDebug() << "Notifying FilesAdded for " << *it << endl;
            org::kde::KDirNotify::emitFilesAdded( (*it).url() );
        }
        emit undoJobFinished();
        broadcastUnlock();
    }
}

void KonqUndoManager::slotPush( QByteArray data )
{
    QDataStream strm( &data, QIODevice::ReadOnly );
    KonqCommand cmd;
    strm >> cmd;
    pushCommand( cmd );
}

void KonqUndoManager::pushCommand( const KonqCommand& cmd )
{
    d->m_commands.push( cmd );
    emit undoAvailable( true );
    emit undoTextChanged( undoText() );
}

void KonqUndoManager::slotPop()
{
    d->m_commands.pop();
    emit undoAvailable( undoAvailable() );
    emit undoTextChanged( undoText() );
}

void KonqUndoManager::slotLock()
{
//  assert( !d->m_lock );
    d->m_lock = true;
    emit undoAvailable( undoAvailable() );
}

void KonqUndoManager::slotUnlock()
{
//  assert( d->m_lock );
    d->m_lock = false;
    emit undoAvailable( undoAvailable() );
}

QByteArray KonqUndoManager::get() const
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << d->m_commands;
    return data;
}

void KonqUndoManager::broadcastPush( const KonqCommand &cmd )
{
    if ( !d->m_syncronized )
    {
        pushCommand( cmd );
        return;
    }

    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    stream << cmd;
    emit push( data ); // DBUS signal
}

void KonqUndoManager::broadcastPop()
{
  if ( !d->m_syncronized )
  {
    slotPop();
    return;
  }

  emit pop(); // DBUS signal
}

void KonqUndoManager::broadcastLock()
{
//  assert( !d->m_lock );

  if ( !d->m_syncronized )
  {
    slotLock();
    return;
  }
  emit lock(); // DBUS signal
}

void KonqUndoManager::broadcastUnlock()
{
//  assert( d->m_lock );

    if ( !d->m_syncronized )
    {
        slotUnlock();
        return;
    }
    emit unlock(); // DBUS signal
}

bool KonqUndoManager::initializeFromKDesky()
{
    // ### workaround for dcop problem and upcoming 2.1 release:
    // in case of huge io operations the amount of data sent over
    // dcop (containing undo information broadcasted for global undo
    // to all konqueror instances) can easily exceed the 64kb limit
    // of dcop. In order not to run into trouble we disable global
    // undo for now! (Simon)
    // ### FIXME: post 2.1
    // TODO KDE4: port to DBUS and test
    return false;
#if 0
    DCOPClient *client = kapp->dcopClient();

    if ( client->appId() == "kdesktop" ) // we are master :)
        return true;

    if ( !client->isApplicationRegistered( "kdesktop" ) )
        return false;

    d->m_commands = DCOPRef( "kdesktop", "KonqUndoManager" ).call( "get" );
    return true;
#endif
}

void KonqUndoManager::setUiInterface( UiInterface* ui )
{
    delete d->m_uiInterface;
    d->m_uiInterface = ui;
}

KonqUndoManager::UiInterface::UiInterface( QWidget* w )
    : m_parentWidget( w )
{
}

void KonqUndoManager::UiInterface::jobError( KIO::Job* job )
{
    job->ui()->showErrorMessage();
}

bool KonqUndoManager::UiInterface::copiedFileWasModified( const KUrl& src, const KUrl& dest, time_t srcTime, time_t destTime ) {
    Q_UNUSED( srcTime ); // not sure it should appear in the msgbox
    //const QDateTime srcDt = QDateTime::fromTime_t( srcTime );
    const QDateTime destDt = QDateTime::fromTime_t( destTime );
    // Possible improvement: only show the time if date is today
    const QString timeStr = KGlobal::locale()->formatDateTime( destDt, KLocale::ShortDate );
    return KMessageBox::warningContinueCancel(
        m_parentWidget,
        i18n( "The file %1 was copied from %2, but since then it has apparently been modified at %3.\n"
              "Undoing the copy will delete the file, and all modifications will be lost.\n"
              "Are you sure you want to delete %4?", dest.pathOrUrl(), src.pathOrUrl(), timeStr, dest.pathOrUrl() ),
        i18n( "Undo File Copy Confirmation" ),
        KStandardGuiItem::cont(),
        KStandardGuiItem::cancel(),
        QString(),
        KMessageBox::Notify | KMessageBox::Dangerous ) == KMessageBox::Continue;
}

bool KonqUndoManager::UiInterface::confirmDeletion( const KUrl::List& files )
{
    // Because undo can happen with an accidental Ctrl-Z, we want to always confirm.
    return KonqOperations::askDeleteConfirmation( files, KonqOperations::DEL,
                                                  KonqOperations::FORCE_CONFIRMATION,
                                                  m_parentWidget );
}

#include "konq_undo.moc"
#include "konq_undo_p.moc"
