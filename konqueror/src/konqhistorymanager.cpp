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
#include <konq_historyloader.h>
#include <kbookmarkmanager.h>

#include <QtDBus/QtDBus>
#include <QTimer>
#include <kdebug.h>
#include <kconfig.h>
#include <kcompletion.h>

#include <kconfiggroup.h>

KonqHistoryManager::KonqHistoryManager( KBookmarkManager* bookmarkManager, QObject *parent )
    : KonqHistoryProvider( parent ),
      m_bookmarkManager(bookmarkManager)
{
    m_updateTimer = new QTimer( this );

    // take care of the completion object
    m_pCompletion = new KCompletion;
    m_pCompletion->setOrder( KCompletion::Weighted );

    // and load the history
    loadHistory();

    connect(m_updateTimer, SIGNAL(timeout()), SLOT(slotEmitUpdated()));
    connect(this, SIGNAL(cleared()), SLOT(slotCleared()));
    connect(this, SIGNAL(entryRemoved(KonqHistoryEntry)), SLOT(slotEntryRemoved(KonqHistoryEntry)));
}


KonqHistoryManager::~KonqHistoryManager()
{
    delete m_pCompletion;
    clearPending();
}

bool KonqHistoryManager::loadHistory()
{
    clearPending();
    m_pCompletion->clear();

    if (!KonqHistoryProvider::loadHistory())
        return false;

    QListIterator<KonqHistoryEntry> it(entries());
    while (it.hasNext()) {
        const KonqHistoryEntry& entry = it.next();
        const QString prettyUrlString = entry.url.prettyUrl();
        addToCompletion(prettyUrlString, entry.typedUrl, entry.numberOfTimesVisited);
    }

    return true;
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
        KonqHistoryList::const_iterator oldEntry = constFindEntry( url );
	m_pending.insert( u, (oldEntry != entries().constEnd()) ?
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

///////////////////////////////////////////////////////////////////
// DBUS called methods

bool KonqHistoryManager::filterOut(const KUrl& url)
{
    return (url.isLocalFile() || url.host().isEmpty());
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
    QListIterator<KonqHistoryEntry> it(entries());
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

void KonqHistoryManager::addToUpdateList( const QString& url )
{
    m_updateURLs.append( url );
    m_updateTimer->setSingleShot( true );
    m_updateTimer->start( 500 );
}

// Called by KonqHistoryProviderPrivate::slotNotifyClear()
void KonqHistoryManager::slotCleared()
{
    clearPending();
    m_pCompletion->clear();
}

void KonqHistoryManager::finishAddingEntry(const KonqHistoryEntry& entry, bool isSender)
{
    const QString urlString = entry.url.url();
    addToCompletion(entry.url.prettyUrl(), entry.typedUrl);
    addToUpdateList(urlString);
    KonqHistoryProvider::finishAddingEntry(entry, isSender);

    // note, no need to do the updateBookmarkMetadata for every
    // history object, only need to for the broadcast sender as
    // the history object itself keeps the data consistant.
    // ### why does the comment do exactly the opposite from the code?
    const bool updated = m_bookmarkManager ? m_bookmarkManager->updateAccessMetadata(urlString) : false;

    if (isSender) {
	// note, bk save does not notify, and we don't want to!
	if (updated)
	    m_bookmarkManager->save();
    }
}

void KonqHistoryManager::slotEntryRemoved(const KonqHistoryEntry& entry)
{
    const QString urlString = entry.url.url();
    removeFromCompletion(entry.url.prettyUrl(), entry.typedUrl);
    addToUpdateList(urlString);
}

#include "konqhistorymanager.moc"
