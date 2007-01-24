/*  This file is part of the KDE project
    Copyright (C) 2000  David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "konq_operations.h"
#include "konq_undo.h"
#include "konq_defaults.h"
#include "konqmimedata.h"

#include <ktoolinvocation.h>
#include <kautomount.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <krun.h>
#include <kshell.h>
#include <kshortcut.h>
#include <kprotocolmanager.h>
#include <kprocess.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/jobclasses.h>
#include <kio/paste.h>
#include <kio/renamedialog.h>
#include <kdirnotify.h>

// For doDrop
#include <kauthorized.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kdesktopfile.h>
#include <kglobalsettings.h>
#include <kimageio.h>

#include <konq_iconviewwidget.h>
#include <QtDBus/QtDBus>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QDropEvent>
#include <QList>
#include <QDir>

#include <assert.h>
#include <unistd.h>

KonqOperations::KonqOperations( QWidget *parent )
    : QObject( parent ),
      m_method( UNKNOWN ), m_info(0), m_pasteInfo(0)
{
    setObjectName( "KonqOperations" );
}

KonqOperations::~KonqOperations()
{
    delete m_info;
    delete m_pasteInfo;
}

void KonqOperations::editMimeType( const QString & mimeType )
{
    QString keditfiletype = QLatin1String("keditfiletype");
    KRun::runCommand( keditfiletype + " " + KProcess::quote(mimeType),
                      keditfiletype, keditfiletype /*unused*/);
}

void KonqOperations::del( QWidget * parent, Operation method, const KUrl::List & selectedUrls )
{
    kDebug(1203) << "KonqOperations::del " << parent->metaObject()->className() << endl;
    if ( selectedUrls.isEmpty() )
    {
        kWarning(1203) << "Empty URL list !" << endl;
        return;
    }

    KonqOperations * op = new KonqOperations( parent );
    int confirmation = DEFAULT_CONFIRMATION;
    op->_del( method, selectedUrls, confirmation );
}

void KonqOperations::emptyTrash( QWidget* parent )
{
    KonqOperations *op = new KonqOperations( parent );
    op->_del( EMPTYTRASH, KUrl("trash:/"), SKIP_CONFIRMATION );
}

void KonqOperations::restoreTrashedItems( const KUrl::List& urls, QWidget* parent )
{
    KonqOperations *op = new KonqOperations( parent );
    op->_restoreTrashedItems( urls );
}

void KonqOperations::mkdir( QWidget *parent, const KUrl & url )
{
    KIO::Job * job = KIO::mkdir( url );
    KonqOperations * op = new KonqOperations( parent );
    op->setOperation( job, MKDIR, url );
    KonqUndoManager::self()->recordJob( KonqUndoManager::MKDIR, KUrl(), url, job );
}

void KonqOperations::doPaste( QWidget * parent, const KUrl & destUrl, const QPoint &pos )
{
    // move or not move ?
    bool move = false;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if ( data->hasFormat( "application/x-kde-cutselection" ) ) {
      move = KonqMimeData::decodeIsCutSelection( data );
      kDebug(1203) << "move (from clipboard data) = " << move << endl;
    }

    KIO::Job *job = KIO::pasteClipboard( destUrl, parent, move );
    if ( job )
    {
        KonqOperations * op = new KonqOperations( parent );
        KIO::CopyJob * copyJob = static_cast<KIO::CopyJob *>(job);
        KIOPasteInfo * pi = new KIOPasteInfo;
        pi->mousePos = pos;
        op->setPasteInfo( pi );
        op->setOperation( job, move ? MOVE : COPY, copyJob->destUrl() );
        KonqUndoManager::self()->recordJob( move ? KonqUndoManager::MOVE : KonqUndoManager::COPY, KUrl::List(), destUrl, job );
    }
}

void KonqOperations::copy( QWidget * parent, Operation method, const KUrl::List & selectedUrls, const KUrl& destUrl )
{
    kDebug(1203) << "KonqOperations::copy() " << parent->metaObject()->className() << endl;
    if ((method!=COPY) && (method!=MOVE) && (method!=LINK))
    {
        kWarning(1203) << "Illegal copy method !" << endl;
        return;
    }
    if ( selectedUrls.isEmpty() )
    {
        kWarning(1203) << "Empty URL list !" << endl;
        return;
    }

    KonqOperations * op = new KonqOperations( parent );
    KIO::Job* job(0);
    if (method == LINK)
        job = KIO::link( selectedUrls, destUrl );
    else if (method == MOVE)
        job = KIO::move( selectedUrls, destUrl );
    else
        job = KIO::copy( selectedUrls, destUrl );

    op->setOperation( job, method, destUrl );

    if (method == COPY)
        KonqUndoManager::self()->recordJob( KonqUndoManager::COPY, selectedUrls, destUrl, job );
    else
        KonqUndoManager::self()->recordJob( method==MOVE?KonqUndoManager::MOVE:KonqUndoManager::LINK, selectedUrls, destUrl, job );
}

void KonqOperations::_del( Operation method, const KUrl::List & _selectedUrls, int confirmation )
{
    KUrl::List selectedUrls;
    for (KUrl::List::ConstIterator it = _selectedUrls.begin(); it != _selectedUrls.end(); ++it)
        if (KProtocolManager::supportsDeleting(*it))
            selectedUrls.append(*it);
    if (selectedUrls.isEmpty()) {
        delete this;
        return;
    }

    m_method = method;
    if ( confirmation == SKIP_CONFIRMATION || askDeleteConfirmation( selectedUrls, confirmation ) )
    {
        //m_srcUrls = selectedUrls;
        KIO::Job *job;
        switch( method )
        {
        case TRASH:
        {
            job = KIO::trash( selectedUrls );
            KonqUndoManager::self()->recordJob( KonqUndoManager::TRASH, selectedUrls, KUrl("trash:/"), job );
            break;
        }
        case EMPTYTRASH:
        {
            // Same as in ktrash --empty
            QByteArray packedArgs;
            QDataStream stream( &packedArgs, QIODevice::WriteOnly );
            stream << (int)1;
            job = KIO::special( KUrl("trash:/"), packedArgs );
            KNotification::event("Trash: emptied", QString() , QPixmap() , 0l /*, QStringList() , KNotification::ContextList() */, KNotification::DefaultEvent );
            break;
        }
        case DEL:
            job = KIO::del( selectedUrls );
            break;
        default:
            kWarning() << "Unknown operation: " << method << endl;
            delete this;
            return;
        }
        job->ui()->setWindow(parentWidget());
        connect( job, SIGNAL( result( KJob * ) ),
                 SLOT( slotResult( KJob * ) ) );
    } else
        delete this;
}

void KonqOperations::_restoreTrashedItems( const KUrl::List& urls )
{
    m_method = RESTORE;
    KonqMultiRestoreJob* job = new KonqMultiRestoreJob( urls, true );
    job->ui()->setWindow(parentWidget());
    connect( job, SIGNAL( result( KJob * ) ),
             SLOT( slotResult( KJob * ) ) );
}

bool KonqOperations::askDeleteConfirmation( const KUrl::List & selectedUrls, int confirmation )
{
    QString keyName;
    bool ask = ( confirmation == FORCE_CONFIRMATION );
    if ( !ask )
    {
        KConfig config("konquerorrc", true, false);
        config.setGroup( "Trash" );
        keyName = ( m_method == DEL ? "ConfirmDelete" : "ConfirmTrash" );
        bool defaultValue = ( m_method == DEL ? DEFAULT_CONFIRMDELETE : DEFAULT_CONFIRMTRASH );
        ask = config.readEntry( keyName, QVariant(defaultValue )).toBool();
    }
    if ( ask )
    {
        KUrl::List::ConstIterator it = selectedUrls.begin();
        QStringList prettyList;
        for ( ; it != selectedUrls.end(); ++it ) {
            if ( (*it).protocol() == "trash" ) {
                QString path = (*it).path();
                // HACK (#98983): remove "0-foo". Note that it works better than
                // displaying KFileItem::name(), for files under a subdir.
                prettyList.append( path.remove(QRegExp("^/[0-9]*-")) );
            } else
                prettyList.append( (*it).pathOrUrl() );
        }

        int result;
        switch(m_method)
        {
        case DEL:
            result = KMessageBox::warningContinueCancelList(
                parentWidget(),
             	i18np( "Do you really want to delete this item?", "Do you really want to delete these %n items?", prettyList.count()),
             	prettyList,
		i18n( "Delete Files" ),
		KStandardGuiItem::del(),
		keyName, KMessageBox::Notify | KMessageBox::Dangerous);
            break;

        case MOVE:
        default:
            result = KMessageBox::warningContinueCancelList(
                parentWidget(),
                i18np( "Do you really want to move this item to the trash?", "Do you really want to move these %n items to the trash?", prettyList.count()),
                prettyList,
		i18n( "Move to Trash" ),
		KGuiItem( i18nc( "Verb", "&Trash" ), "edittrash"),
		keyName, KMessageBox::Notify | KMessageBox::Dangerous);
        }
        if (!keyName.isEmpty())
        {
            // Check kmessagebox setting... erase & copy to konquerorrc.
            KConfig *config = KGlobal::config();
            KConfigGroup saver(config, "Notification Messages");
            if (!saver.readEntry(keyName, QVariant(true)).toBool())
            {
                saver.writeEntry(keyName, true);
                saver.sync();
                KConfig konq_config("konquerorrc", false);
                konq_config.setGroup( "Trash" );
                konq_config.writeEntry( keyName, false );
            }
        }
        return (result == KMessageBox::Continue);
    }
    return true;
}

void KonqOperations::doDrop( const KFileItem * destItem, const KUrl & dest, QDropEvent * ev, QWidget * parent )
{
    kDebug(1203) << "doDrop: dest : " << dest.url() << endl;
    QMap<QString, QString> metaData;
    const KUrl::List lst = KUrl::List::fromMimeData( ev->mimeData(), &metaData );
    if ( !lst.isEmpty() ) // Are they urls ?
    {
        kDebug(1203) << "KonqOperations::doDrop metaData: " << metaData.count() << " entries." << endl;
        QMap<QString,QString>::ConstIterator mit;
        for( mit = metaData.begin(); mit != metaData.end(); ++mit )
        {
            kDebug(1203) << "metaData: key=" << mit.key() << " value=" << mit.value() << endl;
        }
        // Check if we dropped something on itself
        KUrl::List::ConstIterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
        {
            kDebug(1203) << "URL : " << (*it).url() << endl;
            if ( dest.equals( *it, KUrl::CompareWithoutTrailingSlash ) )
            {
                // The event source may be the view or an item (icon)
                // Note: ev->source() can be 0L! (in case of kdesktop) (Simon)
                if ( !ev->source() || ev->source() != parent && ev->source()->parent() != parent )
                    KMessageBox::sorry( parent, i18n("You cannot drop a folder on to itself") );
                kDebug(1203) << "Dropped on itself" << endl;
                ev->setAccepted( false );
                return; // do nothing instead of displaying kfm's annoying error box
            }
        }

        // Check the state of the modifiers key at the time of the drop
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

        Qt::DropAction action = ev->dropAction();
        // Check for the drop of a bookmark -> we want a Link action
        if ( ev->provides("application/x-xbel") )
        {
            modifiers |= Qt::ControlModifier | Qt::ShiftModifier;
            action = Qt::LinkAction;
            kDebug(1203) << "KonqOperations::doDrop Bookmark -> emulating Link" << endl;
        }

        KonqOperations * op = new KonqOperations(parent);
        op->setDropInfo( new DropInfo( modifiers, lst, metaData, ev->pos(), action ) );

        // Ok, now we need destItem.
        if ( destItem )
        {
            op->asyncDrop( destItem ); // we have it already
        }
        else
        {
            // we need to stat to get it.
            op->_statUrl( dest, op, SLOT( asyncDrop( const KFileItem * ) ) );
        }
        // In both cases asyncDrop will delete op when done

        ev->acceptProposedAction();
    }
    else
    {
        //kDebug(1203) << "Pasting to " << dest.url() << endl;
        KonqOperations * op = new KonqOperations(parent);
        KIO::CopyJob* job = KIO::pasteMimeSource( ev->mimeData(), dest,
                                                  i18n( "File name for dropped contents:" ),
                                                  parent );
        if ( job ) // 0 if canceled by user
        {
            op->setOperation( job, COPY, job->destUrl() );
            KonqUndoManager::self()->recordJob( KonqUndoManager::COPY, KUrl::List(), dest, job );
        }
        ev->acceptProposedAction();
    }
}

void KonqOperations::asyncDrop( const KFileItem * destItem )
{
    assert(m_info); // setDropInfo should have been called before asyncDrop
    m_destUrl = destItem->url();

    //kDebug(1203) << "KonqOperations::asyncDrop destItem->mode=" << destItem->mode() << " url=" << m_destUrl << endl;
    // Check what the destination is
    if ( destItem->isDir() )
    {
        doDropFileCopy();
        return;
    }
    if ( !m_destUrl.isLocalFile() )
    {
        // We dropped onto a remote URL that is not a directory!
        // (e.g. an HTTP link in the sidebar).
        // Can't do that, but we can't prevent it before stating the dest....
        kWarning(1203) << "Cannot drop onto " << m_destUrl << endl;
        delete this;
        return;
    }
    if ( destItem->mimetype() == "application/x-desktop")
    {
        // Local .desktop file. What type ?
        KDesktopFile desktopFile( m_destUrl.path() );
        if ( desktopFile.hasApplicationType() )
        {
            QString error;
            const QStringList urlStrList = m_info->urls.toStringList();
            if ( KToolInvocation::startServiceByDesktopPath( m_destUrl.path(), urlStrList, &error ) > 0 )
                KMessageBox::error( parentWidget(), error );
        }
        else
        {
            // Device or Link -> adjust dest
            if ( desktopFile.hasDeviceType() && desktopFile.hasKey("MountPoint") ) {
                QString point = desktopFile.readEntry( "MountPoint" );
                m_destUrl.setPath( point );
                QString dev = desktopFile.readDevice();
                QString mp = KIO::findDeviceMountPoint( dev );
                // Is the device already mounted ?
                if ( !mp.isNull() ) {
                    doDropFileCopy();
                }
#ifndef Q_WS_WIN
                else
                {
                    const bool ro = desktopFile.readEntry( "ReadOnly", false );
                    const QByteArray fstype = desktopFile.readEntry( "FSType" ).toLatin1();
                    KAutoMount* am = new KAutoMount( ro, fstype, dev, point, m_destUrl.path(), false );
                    connect( am, SIGNAL( finished() ), this, SLOT( doDropFileCopy() ) );
                }
#endif
                return;
            }
            else if ( desktopFile.hasLinkType() && desktopFile.hasKey("URL") ) {
                m_destUrl = desktopFile.readPathEntry("URL");
                doDropFileCopy();
                return;
            }
            // else, well: mimetype, service, servicetype or .directory. Can't really drop anything on those.
        }
    }
    else
    {
        // Should be a local executable
        // (If this fails, there is a bug in KFileItem::acceptsDrops / KDirModel::flags)
        kDebug(1203) << "KonqOperations::doDrop " << m_destUrl.path() << "should be an executable" << endl;
        Q_ASSERT ( access( QFile::encodeName(m_destUrl.path()), X_OK ) == 0 );
        KProcess proc;
        proc << m_destUrl.path() ;
        // Launch executable for each of the files
        KUrl::List lst = m_info->urls;
        KUrl::List::Iterator it = lst.begin();
        for ( ; it != lst.end() ; it++ )
            proc << (*it).path(); // assume local files
        kDebug(1203) << "starting " << m_destUrl.path() << " with " << lst.count() << " arguments" << endl;
        proc.start( KProcess::DontCare );
    }
    delete this;
}

void KonqOperations::doDropFileCopy()
{
    assert(m_info); // setDropInfo - and asyncDrop - should have been called before asyncDrop
    const KUrl::List lst = m_info->urls;
    Qt::DropAction action = m_info->action;
    bool isDesktopFile = false;
    bool itemIsOnDesktop = false;
    bool allItemsAreFromTrash = true;
    KUrl::List mlst; // list of items that can be moved
    for (KUrl::List::ConstIterator it = lst.begin(); it != lst.end(); ++it)
    {
        bool local = (*it).isLocalFile();
        if ( KProtocolManager::supportsDeleting( *it ) && (!local || QFileInfo((*it).directory()).isWritable() ))
            mlst.append(*it);
        if ( local && KDesktopFile::isDesktopFile((*it).path()))
            isDesktopFile = true;
        if ( local && (*it).path().startsWith(KGlobalSettings::desktopPath()))
            itemIsOnDesktop = true;
        if ( local || (*it).protocol() != "trash" )
            allItemsAreFromTrash = false;
    }

    bool linkOnly = false;
    if (isDesktopFile && !KAuthorized::authorizeKAction("run_desktop_files") &&
        (m_destUrl.path( KUrl::AddTrailingSlash ) == KGlobalSettings::desktopPath()) )
    {
       linkOnly = true;
    }

    if ( !mlst.isEmpty() && m_destUrl.protocol() == "trash" )
    {
        if ( itemIsOnDesktop && !KAuthorized::authorizeKAction("editable_desktop_icons") )
        {
            delete this;
            return;
        }

        m_method = TRASH;
        if ( askDeleteConfirmation( mlst, DEFAULT_CONFIRMATION ) )
            action = Qt::MoveAction;
        else
        {
            delete this;
            return;
        }
    }
    else if ( allItemsAreFromTrash || m_destUrl.protocol() == "trash" ) {
        // No point in asking copy/move/link when using dnd from or to the trash.
        action = Qt::MoveAction;
    }
    else if ( (
        ((m_info->keyboardModifiers & Qt::ControlModifier) == 0) &&
        ((m_info->keyboardModifiers & Qt::ShiftModifier) == 0) ) || linkOnly )
    {
        // Neither control nor shift are pressed => show popup menu

        // TODO move this code out somehow. Allow user of KonqOperations to add his own actions...
        KonqIconViewWidget *iconView = dynamic_cast<KonqIconViewWidget*>(parent());
        bool bSetWallpaper = false;
        if ( iconView && iconView->maySetWallpaper() && lst.count() == 1 )
	{
            KUrl url = lst.first();
            KMimeType::Ptr mime = KMimeType::findByUrl( url );
            if ( mime && ( ( KImageIO::isSupported(mime->name(), KImageIO::Reading) ) ||
                 mime->is( "image/svg+xml" ) ) )
            {
                bSetWallpaper = true;
            }
        }

        // Check what the source can do
        KUrl url = lst.first(); // we'll assume it's the same for all URLs (hack)
        bool sReading = KProtocolManager::supportsReading( url );
        bool sDeleting = KProtocolManager::supportsDeleting( url );
        bool sMoving = KProtocolManager::supportsMoving( url );
        // Check what the destination can do
        bool dWriting = KProtocolManager::supportsWriting( m_destUrl );
        if ( !dWriting )
        {
            delete this;
            return;
        }

        QMenu popup;
        QString seq = QKeySequence( Qt::ShiftModifier ).toString();
        seq.chop(1); // chop superfluous '+'
        QAction* popupMoveAction = new QAction(i18n( "&Move Here" ) + "\t" + seq, this);
        popupMoveAction->setIcon(KIcon("goto"));
        seq = QKeySequence( Qt::ControlModifier ).toString();
        seq.chop(1);
        QAction* popupCopyAction = new QAction(i18n( "&Copy Here" ) + "\t" + seq, this);
        popupCopyAction->setIcon(KIcon("editcopy"));
        seq = QKeySequence( Qt::ControlModifier + Qt::ShiftModifier ).toString();
        seq.chop(1);
        QAction* popupLinkAction = new QAction(i18n( "&Link Here" ) + "\t" + seq, this);
        popupLinkAction->setIcon(KIcon("www"));
        QAction* popupWallAction = new QAction( i18n( "Set as &Wallpaper" ), this );
        popupWallAction->setIcon(KIcon("background"));
        QAction* popupCancelAction = new QAction(i18n( "C&ancel" ) + "\t" + QKeySequence( Qt::Key_Escape ).toString(), this);
        popupCancelAction->setIcon(KIcon("cancel"));

        if ( sReading && !linkOnly)
            popup.addAction(popupCopyAction);

        if (!mlst.isEmpty() && (sMoving || (sReading && sDeleting)) && !linkOnly )
            popup.addAction(popupMoveAction);

        popup.addAction(popupLinkAction);

        if (bSetWallpaper)
            popup.addAction(popupWallAction);

        popup.addSeparator();
        popup.addAction(popupCancelAction);

        QAction* result = popup.exec( m_info->mousePos );

        if(result == popupCopyAction)
            action = Qt::CopyAction;
        else if(result == popupMoveAction)
            action = Qt::MoveAction;
        else if(result == popupLinkAction)
            action = Qt::LinkAction;
        else if(result == popupWallAction)
        {
            kDebug(1203) << "setWallpaper iconView=" << iconView << " url=" << lst.first().url() << endl;
            if (iconView && iconView->isDesktop() ) iconView->setWallpaper(lst.first());
            delete this;
            return;
        }
        else if(result == popupCancelAction || !result)
        {
            delete this;
            return;
        }
    }

    KIO::Job * job = 0;
    switch ( action ) {
    case Qt::MoveAction :
        job = KIO::move( lst, m_destUrl );
        job->setMetaData( m_info->metaData );
        setOperation( job, m_method == TRASH ? TRASH : MOVE, m_destUrl );
        KonqUndoManager::self()->recordJob(
            m_method == TRASH ? KonqUndoManager::TRASH : KonqUndoManager::MOVE,
            lst, m_destUrl, job );
        return; // we still have stuff to do -> don't delete ourselves
    case Qt::CopyAction :
        job = KIO::copy( lst, m_destUrl );
        job->setMetaData( m_info->metaData );
        setOperation( job, COPY, m_destUrl );
        KonqUndoManager::self()->recordJob( KonqUndoManager::COPY, lst, m_destUrl, job );
        return;
    case Qt::LinkAction :
        kDebug(1203) << "KonqOperations::asyncDrop lst.count=" << lst.count() << endl;
        job = KIO::link( lst, m_destUrl );
        job->setMetaData( m_info->metaData );
        setOperation( job, LINK, m_destUrl );
        KonqUndoManager::self()->recordJob( KonqUndoManager::LINK, lst, m_destUrl, job );
        return;
    default : kError(1203) << "Unknown action " << (int)action << endl;
    }
    delete this;
}

void KonqOperations::rename( QWidget * parent, const KUrl & oldurl, const KUrl& newurl )
{
    kDebug(1203) << "KonqOperations::rename oldurl=" << oldurl << " newurl=" << newurl << endl;
    if ( oldurl == newurl )
        return;

    KUrl::List lst;
    lst.append(oldurl);
    KIO::Job * job = KIO::moveAs( oldurl, newurl, !oldurl.isLocalFile() );
    KonqOperations * op = new KonqOperations( parent );
    op->setOperation( job, MOVE, newurl );
    KonqUndoManager::self()->recordJob( KonqUndoManager::RENAME, lst, newurl, job );
    // if moving the desktop then update config file and emit
    if ( oldurl.isLocalFile() && oldurl.path( KUrl::AddTrailingSlash ) == KGlobalSettings::desktopPath() )
    {
        kDebug(1203) << "That rename was the Desktop path, updating config files" << endl;
        KConfig *globalConfig = KGlobal::config();
        KConfigGroup cgs( globalConfig, "Paths" );
        cgs.writePathEntry("Desktop" , newurl.path(), KConfigBase::Persistent|KConfigBase::Global );
        cgs.sync();
        KGlobalSettings::self()->emitChange(KGlobalSettings::SettingsChanged, KGlobalSettings::SETTINGS_PATHS);
    }
}

void KonqOperations::setOperation( KIO::Job * job, Operation method, const KUrl & dest )
{
    m_method = method;
    m_destUrl = dest;
    if ( job )
    {
        job->ui()->setWindow(parentWidget());
        connect( job, SIGNAL( result( KJob * ) ),
                 SLOT( slotResult( KJob * ) ) );
        KIO::CopyJob *copyJob = dynamic_cast<KIO::CopyJob*>(job);
        KonqIconViewWidget *iconView = dynamic_cast<KonqIconViewWidget*>(parent());
        if (copyJob && iconView)
        {
            connect(copyJob, SIGNAL(aboutToCreate(KIO::Job *,const QList<KIO::CopyInfo> &)),
                 this, SLOT(slotAboutToCreate(KIO::Job *,const QList<KIO::CopyInfo> &)));
            // TODO move this connect into the iconview!
            connect(this, SIGNAL(aboutToCreate(const QPoint &, const QList<KIO::CopyInfo> &)),
                 iconView, SLOT(slotAboutToCreate(const QPoint &, const QList<KIO::CopyInfo> &)));
        }
    }
    else // for link
        slotResult( 0L );
}

void KonqOperations::slotAboutToCreate(KIO::Job *, const QList<KIO::CopyInfo> &files)
{
    emit aboutToCreate( m_info ? m_info->mousePos : m_pasteInfo ? m_pasteInfo->mousePos : QPoint(), files);
}

void KonqOperations::statUrl( const KUrl & url, const QObject *receiver, const char *member, QWidget* parent )
{
    KonqOperations * op = new KonqOperations( parent );
    op->m_method = STAT;
    op->_statUrl( url, receiver, member );
}

void KonqOperations::_statUrl( const KUrl & url, const QObject *receiver, const char *member )
{
    connect( this, SIGNAL( statFinished( const KFileItem * ) ), receiver, member );
    KIO::StatJob * job = KIO::stat( url /*, false?*/ );
    job->ui()->setWindow(parentWidget());
    connect( job, SIGNAL( result( KJob * ) ),
             SLOT( slotStatResult( KJob * ) ) );
}

void KonqOperations::slotStatResult( KJob * job )
{
    if ( job->error())
    {
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();
    }
    else
    {
        KIO::StatJob * statJob = static_cast<KIO::StatJob*>(job);
        KFileItem * item = new KFileItem( statJob->statResult(), statJob->url() );
        emit statFinished( item );
        delete item;
    }
    // If we're only here for a stat, we're done. But not if we used _statUrl internally
    if ( m_method == STAT )
        delete this;
}

void KonqOperations::slotResult( KJob * job )
{
    if (job && job->error())
    {
        static_cast<KIO::Job*>( job )->ui()->showErrorMessage();
    }
    if ( m_method == EMPTYTRASH ) {
        // Update konq windows opened on trash:/
        org::kde::KDirNotify::emitFilesAdded( "trash:/" ); // yeah, files were removed, but we don't know which ones...
    }
    delete this;
}

void KonqOperations::rename( QWidget * parent, const KUrl & oldurl, const QString & name )
{
    KUrl newurl( oldurl );
    newurl.setPath( oldurl.directory( KUrl::AppendTrailingSlash ) + name );
    kDebug(1203) << "KonqOperations::rename("<<name<<") called. newurl=" << newurl << endl;
    rename( parent, oldurl, newurl );
}

void KonqOperations::newDir( QWidget * parent, const KUrl & baseUrl )
{
    bool ok;
    QString name = i18n( "New Folder" );
    if ( baseUrl.isLocalFile() && QFileInfo( baseUrl.path( KUrl::AddTrailingSlash ) + name ).exists() )
        name = KIO::RenameDialog::suggestName( baseUrl, i18n( "New Folder" ) );

    name = KInputDialog::getText ( i18n( "New Folder" ),
        i18n( "Enter folder name:" ), name, &ok, parent );
    if ( ok && !name.isEmpty() )
    {
        KUrl url;
        if ((name[0] == '/') || (name[0] == '~'))
        {
           url.setPath(KShell::tildeExpand(name));
        }
        else
        {
           name = KIO::encodeFileName( name );
           url = baseUrl;
           url.addPath( name );
        }
        KonqOperations::mkdir( parent, url );
    }
}

////

KonqMultiRestoreJob::KonqMultiRestoreJob( const KUrl::List& urls, bool showProgressInfo )
    : KIO::Job( showProgressInfo ),
      m_urls( urls ), m_urlsIterator( m_urls.begin() ),
      m_progress( 0 )
{
    QTimer::singleShot(0, this, SLOT(slotStart()));
}

void KonqMultiRestoreJob::slotStart()
{
    // Well, it's not a total in bytes, so this would look weird
    //if ( m_urlsIterator == m_urls.begin() ) // first time: emit total
    //    emit totalSize( m_urls.count() );

    if ( m_urlsIterator != m_urls.end() )
    {
        const KUrl& url = *m_urlsIterator;

        KUrl new_url = url;
        if ( new_url.protocol()=="system"
          && new_url.path().startsWith("/trash") )
        {
            QString path = new_url.path();
	    path.remove(0, 6);
	    new_url.setProtocol("trash");
	    new_url.setPath(path);
        }

        Q_ASSERT( new_url.protocol() == "trash" );
        QByteArray packedArgs;
        QDataStream stream( &packedArgs, QIODevice::WriteOnly );
        stream << (int)3 << new_url;
        KIO::Job* job = KIO::special( new_url, packedArgs );
        addSubjob( job );
    }
    else // done!
    {
        org::kde::KDirNotify::emitFilesRemoved(m_urls.toStringList() );
        emitResult();
    }
}

void KonqMultiRestoreJob::slotResult( KJob *job )
{
    if ( job->error() )
    {
        KIO::Job::slotResult( job ); // will set the error and emit result(this)
        return;
    }
    removeSubjob(job);
    // Move on to next one
    ++m_urlsIterator;
    ++m_progress;
    //emit processedSize( this, m_progress );
    emitPercent( m_progress, m_urls.count() );
    slotStart();
}

QWidget* KonqOperations::parentWidget() const
{
    return static_cast<QWidget *>( parent() );
}

#include "konq_operations.moc"
