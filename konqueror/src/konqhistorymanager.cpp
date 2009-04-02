/* This file is part of the KDE project
   Copyright (C) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright     2007 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqhistorymanager.h"
#include "konqhistorymanageradaptor.h"
#include <konq_historyloader.h>
#include <kbookmarkmanager.h>

#include <QtDBus/QtDBus>
#include <kapplication.h>
#include <kdebug.h>
#include <ksavefile.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kcompletion.h>

#include <zlib.h> // for crc32
#include <kconfiggroup.h>

KonqHistoryManager::KonqHistoryManager( KBookmarkManager* bookmarkManager, QObject *parent )
    : KParts::HistoryProvider( parent ),
      m_bookmarkManager(bookmarkManager)
{
    m_updateTimer = new QTimer( this );

    // defaults
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs( config, "HistorySettings" );
    m_maxCount = cs.readEntry( "Maximum of History entries", 500 );
    m_maxCount = qMax( 1, m_maxCount );
    m_maxAgeDays = cs.readEntry( "Maximum age of History entries", 90);

    // take care of the completion object
    m_pCompletion = new KCompletion;
    m_pCompletion->setOrder( KCompletion::Weighted );

    // and load the history
    loadHistory();

    connect( m_updateTimer, SIGNAL( timeout() ), SLOT( slotEmitUpdated() ));

    (void) new KonqHistoryManagerAdaptor( this );
    const QString dbusPath = "/KonqHistoryManager";
    const QString dbusInterface = "org.kde.Konqueror.HistoryManager";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    // We add a QDBusMessage argument to find the sender of the message,
    // but a simpler solution is to just inherit from QDBusContext.
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyClear", this, SLOT(slotNotifyClear(QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyHistoryEntry", this, SLOT(slotNotifyHistoryEntry(QByteArray,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyMaxAge", this, SLOT(slotNotifyMaxAge(int,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyMaxCount", this, SLOT(slotNotifyMaxCount(int,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyRemove", this, SLOT(slotNotifyRemove(QString,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyRemoveList", this, SLOT(slotNotifyRemoveList(QStringList,QDBusMessage)));
}


KonqHistoryManager::~KonqHistoryManager()
{
    delete m_pCompletion;
    clearPending();
}

static QString dbusService()
{
    return QDBusConnection::sessionBus().baseService();
}

bool KonqHistoryManager::isSenderOfSignal( const QDBusMessage& msg )
{
    return dbusService() == msg.service();
}

bool KonqHistoryManager::loadHistory()
{
    clearPending();
    m_pCompletion->clear();

    KonqHistoryLoader loader;
    if (!loader.loadHistory()) {
        return false;
    }

    m_history = loader.entries();
    adjustSize();

    QListIterator<KonqHistoryEntry> it(m_history);
    while (it.hasNext()) {
        const KonqHistoryEntry& entry = it.next();
        const QString prettyUrlString = entry.url.prettyUrl();
        addToCompletion(prettyUrlString, entry.typedUrl, entry.numberOfTimesVisited);

        // and fill our baseclass.
        const QString urlString = entry.url.url();
        KParts::HistoryProvider::insert(urlString);
        // DF: also insert the "pretty" version if different
        // This helps getting 'visited' links on websites which don't use fully-escaped urls.
        if (urlString != prettyUrlString)
            KParts::HistoryProvider::insert(prettyUrlString);
    }

    return true;
}

bool KonqHistoryManager::saveHistory()
{
    const QString filename = KStandardDirs::locateLocal("data", QLatin1String("konqueror/konq_history"));
    KSaveFile file(filename);
    if ( !file.open() ) {
        kWarning() << "Can't open " << file.fileName() ;
        return false;
    }

    QDataStream fileStream ( &file );
    fileStream << KonqHistoryLoader::historyVersion();

    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );

    QListIterator<KonqHistoryEntry> it( m_history );
    while ( it.hasNext() ) {
        //We use QUrl for marshalling URLs in entries in the V4
        //file format
        it.next().save(stream, KonqHistoryEntry::NoFlags);
    }

    quint32 crc = crc32( 0, reinterpret_cast<unsigned char *>( data.data() ), data.size() );
    fileStream << crc << data;

    file.finalize(); //check for error here?

    return true;
}


void KonqHistoryManager::adjustSize()
{
    if (m_history.isEmpty())
        return;

    KonqHistoryEntry entry = m_history.first();
    const QDateTime expirationDate( QDate::currentDate().addDays( -m_maxAgeDays ) );

    while ( m_history.count() > (qint32)m_maxCount ||
            (m_maxAgeDays > 0 && entry.lastVisited.isValid() && entry.lastVisited < expirationDate) ) // i.e. entry is expired
    {
	removeFromCompletion( entry.url.prettyUrl(), entry.typedUrl );

        QString urlString = entry.url.url();
	KParts::HistoryProvider::remove( urlString );

        addToUpdateList( urlString );

	emit entryRemoved( m_history.first() );
	m_history.removeFirst();

        if ( m_history.isEmpty() )
            break;
	entry = m_history.first();
    }
}


void KonqHistoryManager::addPending( const KUrl& url, const QString& typedUrl,
				     const QString& title )
{
    addToHistory( true, url, typedUrl, title );
}

void KonqHistoryManager::confirmPending( const KUrl& url,
					 const QString& typedUrl,
					 const QString& title )
{
    addToHistory( false, url, typedUrl, title );
}


void KonqHistoryManager::addToHistory( bool pending, const KUrl& _url,
				       const QString& typedUrl,
				       const QString& title )
{
    //kDebug(1202) << _url << "Typed URL:" << typedUrl << ", Title:" << title;

    if ( filterOut( _url ) ) // we only want remote URLs
	return;

    // http URLs without a path will get redirected immediately to url + '/'
    if ( _url.path().isEmpty() && _url.protocol().startsWith("http") )
	return;

    KUrl url( _url );
    bool hasPass = url.hasPass();
    url.setPass( QString() ); // No password in the history, especially not in the completion!
    url.setHost( url.host().toLower() ); // All host parts lower case
    KonqHistoryEntry entry;
    QString u = url.prettyUrl();
    entry.url = url;
    if ( (u != typedUrl) && !hasPass )
	entry.typedUrl = typedUrl;

    // we only keep the title if we are confirming an entry. Otherwise,
    // we might get bogus titles from the previous url (actually it's just
    // konqueror's window caption).
    if ( !pending && u != title )
	entry.title = title;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;

    // always remove from pending if available, otherwise the else branch leaks
    // if the map already contains an entry for this key.
    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.find( u );
    if ( it != m_pending.end() ) {
        delete it.value();
        m_pending.erase( it );
    }

    if ( !pending ) {
	if ( it != m_pending.end() ) {
	    // we make a pending entry official, so we just have to update
	    // and not increment the counter. No need to care about
	    // firstVisited, as this is not taken into account on update.
	    entry.numberOfTimesVisited = 0;
	}
    }

    else {
	// We add a copy of the current history entry of the url to the
	// pending list, so that we can restore it if the user canceled.
	// If there is no entry for the url yet, we just store the url.
        KonqHistoryList::iterator oldEntry = findEntry( url );
	m_pending.insert( u, oldEntry != m_history.end() ?
                          new KonqHistoryEntry( *oldEntry ) : 0 );
    }

    // notify all konqueror instances about the entry
    emitAddToHistory( entry );
}

// interface of KParts::HistoryManager
// Usually, we only record the history for non-local URLs (i.e. filterOut()
// returns false). But when using the HistoryProvider interface, we record
// exactly those filtered-out urls.
// Moreover, we  don't get any pending/confirming entries, just one insert()
void KonqHistoryManager::insert( const QString& url )
{
    KUrl u ( url );
    if ( !filterOut( u ) || u.protocol() == "about" ) { // remote URL
	return;
    }
    // Local URL -> add to history
    KonqHistoryEntry entry;
    entry.url = u;
    entry.firstVisited = QDateTime::currentDateTime();
    entry.lastVisited = entry.firstVisited;
    emitAddToHistory( entry );
}

void KonqHistoryManager::emitAddToHistory( const KonqHistoryEntry& entry )
{
    QByteArray data;
    QDataStream stream( &data, QIODevice::WriteOnly );
    entry.save(stream, KonqHistoryEntry::MarshalUrlAsStrings);
    stream << dbusService();
    // Protection against very long urls (like data:)
    if ( data.size() > 4096 )
        return;
    emit notifyHistoryEntry( data );
}


void KonqHistoryManager::removePending( const KUrl& url )
{
    // kDebug(1202) << "Removing pending..." << url;

    if ( url.isLocalFile() )
	return;

    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.find( url.prettyUrl() );
    if ( it != m_pending.end() ) {
	KonqHistoryEntry *oldEntry = it.value(); // the old entry, may be 0
	emitRemoveFromHistory( url ); // remove the current pending entry

	if ( oldEntry ) // we had an entry before, now use that instead
	    emitAddToHistory( *oldEntry );

	delete oldEntry;
	m_pending.erase( it );
    }
}

// clears the pending list and makes sure the entries get deleted.
void KonqHistoryManager::clearPending()
{
    QMap<QString,KonqHistoryEntry*>::iterator it = m_pending.begin();
    while ( it != m_pending.end() ) {
	delete it.value();
	++it;
    }
    m_pending.clear();
}

void KonqHistoryManager::emitRemoveFromHistory( const KUrl& url )
{
    emit notifyRemove( url.url() );
}

void KonqHistoryManager::emitRemoveListFromHistory( const KUrl::List& urls )
{
    emit notifyRemoveList( urls.toStringList() );
}

void KonqHistoryManager::emitClear()
{
    emit notifyClear();
}

void KonqHistoryManager::emitSetMaxCount( int count )
{
    emit notifyMaxCount( count );
}

void KonqHistoryManager::emitSetMaxAge( int days )
{
    emit notifyMaxAge( days );
}

///////////////////////////////////////////////////////////////////
// DBUS called methods

void KonqHistoryManager::slotNotifyHistoryEntry( const QByteArray & data,
                                                 const QDBusMessage& msg )
{
    KonqHistoryEntry e;
    QDataStream stream( const_cast<QByteArray *>( &data ), QIODevice::ReadOnly );

    //This is important - we need to switch to a consistent marshalling format for
    //communicating between different konqueror instances. Since during an upgrade
    //some "old" copies may still running, we use the old format for the DBUS transfers.
    //This doesn't make that much difference performance-wise for single entries anyway.
    e.load(stream, KonqHistoryEntry::MarshalUrlAsStrings);
    //kDebug(1202) << "Got new entry from Broadcast:" << e.url;

    KonqHistoryList::iterator existingEntry = findEntry( e.url );
    QString urlString = e.url.url();
    const bool newEntry = existingEntry == m_history.end();

    KonqHistoryEntry entry;

    if ( !newEntry ) {
        entry = *existingEntry;
    } else { // create a new history entry
	entry.url = e.url;
	entry.firstVisited = e.firstVisited;
	entry.numberOfTimesVisited = 0; // will get set to 1 below
	KParts::HistoryProvider::insert( urlString );
    }

    if ( !e.typedUrl.isEmpty() )
	entry.typedUrl = e.typedUrl;
    if ( !e.title.isEmpty() )
	entry.title = e.title;
    entry.numberOfTimesVisited += e.numberOfTimesVisited;
    entry.lastVisited = e.lastVisited;

    if ( newEntry )
        m_history.append( entry );
    else {
        *existingEntry = entry;
    }

    addToCompletion( entry.url.prettyUrl(), entry.typedUrl );

    // bool pending = (e.numberOfTimesVisited != 0);

    adjustSize();

    // note, no need to do the updateBookmarkMetadata for every
    // history object, only need to for the broadcast sender as
    // the history object itself keeps the data consistant.
    bool updated = m_bookmarkManager ? m_bookmarkManager->updateAccessMetadata( urlString ) : false;

    if ( isSenderOfSignal( msg ) ) {
	// we are the sender of the broadcast, so we save
	saveHistory();
	// note, bk save does not notify, and we don't want to!
	if (updated)
	    m_bookmarkManager->save();
    }

    addToUpdateList( urlString );
    emit entryAdded( entry );
}

void KonqHistoryManager::slotNotifyMaxCount( int count, const QDBusMessage& msg )
{
    m_maxCount = count;
    clearPending();
    adjustSize();

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs( config, "HistorySettings" );
    cs.writeEntry( "Maximum of History entries", m_maxCount );

    if ( isSenderOfSignal( msg ) ) {
	saveHistory();
	cs.sync();
    }
}

void KonqHistoryManager::slotNotifyMaxAge( int days, const QDBusMessage& msg  )
{
    m_maxAgeDays = days;
    clearPending();
    adjustSize();

    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cs( config, "HistorySettings" );
    cs.writeEntry( "Maximum age of History entries", m_maxAgeDays );

    if ( isSenderOfSignal( msg ) ) {
	saveHistory();
	cs.sync();
    }
}

void KonqHistoryManager::slotNotifyClear( const QDBusMessage& msg )
{
    clearPending();
    m_history.clear();
    m_pCompletion->clear();

    if ( isSenderOfSignal( msg ) )
	saveHistory();

    KParts::HistoryProvider::clear(); // also emits the cleared() signal
}

void KonqHistoryManager::slotNotifyRemove( const QString& urlStr, const QDBusMessage& msg )
{
    KUrl url( urlStr );
    kDebug(1202) << "Broadcast: remove entry:" << url;

    KonqHistoryList::iterator existingEntry = findEntry( url );

    if ( existingEntry != m_history.end() ) {
        const KonqHistoryEntry entry = *existingEntry; // make copy, due to erase call below
	removeFromCompletion( entry.url.prettyUrl(), entry.typedUrl );

        const QString urlString = entry.url.url();
	KParts::HistoryProvider::remove( urlString );

        addToUpdateList( urlString );

	m_history.erase( existingEntry );
	emit entryRemoved( entry );

	if ( isSenderOfSignal( msg ) )
	    saveHistory();
    }
}

void KonqHistoryManager::slotNotifyRemoveList( const QStringList& urls, const QDBusMessage& msg )
{
    kDebug(1202) << "Broadcast: removing list!";

    bool doSave = false;
    QStringList::const_iterator it = urls.begin();
    for ( ; it != urls.end(); ++it ) {
        KUrl url = *it;
        KonqHistoryList::iterator existingEntry = m_history.findEntry( url );
        if ( existingEntry != m_history.end() ) {
            const KonqHistoryEntry entry = *existingEntry; // make copy, due to erase call below
	    removeFromCompletion( entry.url.prettyUrl(), entry.typedUrl );

            const QString urlString = entry.url.url();
	    KParts::HistoryProvider::remove( urlString );

            addToUpdateList( urlString );

            m_history.erase( existingEntry );
            emit entryRemoved( entry );

            doSave = true;
	}
    }

    if ( doSave && isSenderOfSignal( msg ) )
        saveHistory();
}

KonqHistoryList::iterator KonqHistoryManager::findEntry( const KUrl& url )
{
    // small optimization (dict lookup) for items _not_ in our history
    if ( !KParts::HistoryProvider::contains( url.url() ) )
        return m_history.end();

    return m_history.findEntry( url );
}

bool KonqHistoryManager::filterOut( const KUrl& url )
{
    return ( url.isLocalFile() || url.host().isEmpty() );
}

void KonqHistoryManager::slotEmitUpdated()
{
    emit KParts::HistoryProvider::updated( m_updateURLs );
    m_updateURLs.clear();
}

#if 0 // unused
QStringList KonqHistoryManager::allURLs() const
{
    QStringList list;
    QListIterator<KonqHistoryEntry> it( m_history );
    while ( it.hasNext() ) {
        list.append( it.next().url.url() );
    }
    return list;
}
#endif

void KonqHistoryManager::addToCompletion( const QString& url, const QString& typedUrl,
                                          int numberOfTimesVisited )
{
    m_pCompletion->addItem( url, numberOfTimesVisited );
    // typed urls have a higher priority
    m_pCompletion->addItem( typedUrl, numberOfTimesVisited +10 );
}

void KonqHistoryManager::removeFromCompletion( const QString& url, const QString& typedUrl )
{
    m_pCompletion->removeItem( url );
    m_pCompletion->removeItem( typedUrl );
}

//////////////////////////////////////////////////////////////////


KonqHistoryList::iterator KonqHistoryList::findEntry( const KUrl& url )
{
    // we search backwards, probably faster to find an entry
    KonqHistoryList::iterator it = end();
    while ( it != begin() ) {
        --it;
	if ( (*it).url == url )
	    return it;
    }
    return end();
}

void KonqHistoryList::removeEntry( const KUrl& url )
{
    iterator it = findEntry( url );
    if ( it != end() )
        erase( it );
}

#include "konqhistorymanager.moc"
