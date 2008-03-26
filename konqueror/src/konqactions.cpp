/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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

#include "konqactions.h"

#include <assert.h>

#include <QToolButton>

#include <ktoolbar.h>
#include <kdebug.h>

#include <konqpixmapprovider.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kapplication.h>

#include "konqview.h"
#include "konqsettingsxt.h"
#include <kauthorized.h>

#include <algorithm>
#include <QtGui/QBoxLayout>

template class QList<KonqHistoryEntry*>;

/////////////////

//static - used by back/forward popups in KonqMainWindow
void KonqActions::fillHistoryPopup(const QList<HistoryEntry*> &history, int historyIndex,
                                   QMenu * popup,
                                   bool onlyBack,
                                   bool onlyForward)
{
  assert ( popup ); // kill me if this 0... :/

  //kDebug(1202) << "fillHistoryPopup position: " << history.at();
  HistoryEntry * current = history[ historyIndex ];
  int index = 0;
  if (onlyBack || onlyForward)
  {
      index += historyIndex; // Jump to current item
      if ( !onlyForward ) --index; else ++index; // And move off it
  }

  QFontMetrics fm = popup->fontMetrics();
  uint i = 0;
  while ( index < history.count() && index >= 0 )
  {
        QString text = history[ index ]->title;
        text = fm.elidedText(text, Qt::ElideMiddle, fm.maxWidth() * 30);
        text.replace( "&", "&&" );
        const QString iconName = KonqPixmapProvider::self()->iconNameFor(history[index]->url);
        QAction* action = new QAction(KIcon(iconName), text, popup);
        action->setData(i);
        popup->addAction(action);
        if (++i > 10)
            break;
        if (!onlyForward) --index; else ++index;
  }
  //kDebug(1202) << "After fillHistoryPopup position: " << history.at();
}

///////////////////////////////

static int s_maxEntries = 0;

KonqMostOftenURLSAction::KonqMostOftenURLSAction( const QString& text,
						  QObject* parent )
    : KActionMenu( KIcon("goto-page"), text, parent ),
      m_parsingDone(false)
{
    setDelayed( false );

    connect(menu(), SIGNAL(aboutToShow()), SLOT(slotFillMenu()));
    connect(menu(), SIGNAL(triggered(QAction*)), SLOT(slotActivated(QAction*)));
    // Need to do all this upfront for a correct initial state
    init();
}

KonqMostOftenURLSAction::~KonqMostOftenURLSAction()
{
}

void KonqMostOftenURLSAction::init()
{
    s_maxEntries = KonqSettings::numberofmostvisitedURLs();

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    setEnabled( !mgr->entries().isEmpty() && s_maxEntries > 0 );
}

K_GLOBAL_STATIC( KonqHistoryList, s_mostEntries )

void KonqMostOftenURLSAction::inSort( const KonqHistoryEntry& entry ) {
    KonqHistoryList::iterator it = std::lower_bound( s_mostEntries->begin(),
                                                     s_mostEntries->end(),
                                                     entry,
                                                     numberOfVisitOrder );
    s_mostEntries->insert( it, entry );
}

void KonqMostOftenURLSAction::parseHistory() // only ever called once
{
    KonqHistoryManager *mgr = KonqHistoryManager::kself();

    connect( mgr, SIGNAL( entryAdded( const KonqHistoryEntry& )),
             SLOT( slotEntryAdded( const KonqHistoryEntry& )));
    connect( mgr, SIGNAL( entryRemoved( const KonqHistoryEntry& )),
             SLOT( slotEntryRemoved( const KonqHistoryEntry& )));
    connect( mgr, SIGNAL( cleared() ), SLOT( slotHistoryCleared() ));

    const KonqHistoryList mgrEntries = mgr->entries();
    KonqHistoryList::const_iterator it = mgrEntries.begin();
    const KonqHistoryList::const_iterator end = mgrEntries.end();
    for ( int i = 0; it != end && i < s_maxEntries; ++i, ++it ) {
	s_mostEntries->append( *it );
    }
    qSort( s_mostEntries->begin(), s_mostEntries->end(), numberOfVisitOrder );

    while ( it != end ) {
	const KonqHistoryEntry& leastOften = s_mostEntries->first();
	const KonqHistoryEntry& entry = *it;
	if ( leastOften.numberOfTimesVisited < entry.numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    inSort( entry );
	}

	++it;
    }
}

void KonqMostOftenURLSAction::slotEntryAdded( const KonqHistoryEntry& entry )
{
    // if it's already present, remove it, and inSort it
    s_mostEntries->removeEntry( entry.url );

    if ( s_mostEntries->count() >= s_maxEntries ) {
	const KonqHistoryEntry& leastOften = s_mostEntries->first();
	if ( leastOften.numberOfTimesVisited < entry.numberOfTimesVisited ) {
	    s_mostEntries->removeFirst();
	    inSort( entry );
	}
    }

    else
	inSort( entry );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotEntryRemoved( const KonqHistoryEntry& entry )
{
    s_mostEntries->removeEntry( entry.url );
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotHistoryCleared()
{
    s_mostEntries->clear();
    setEnabled( false );
}

static void createHistoryAction(const KonqHistoryEntry& entry, QMenu* menu)
{
    // we take either title, typedUrl or URL (in this order)
    const QString text = entry.title.isEmpty() ? (entry.typedUrl.isEmpty() ?
                                                  entry.url.prettyUrl() :
                                                  entry.typedUrl) :
                         entry.title;
    QAction* action = new QAction(
        KIcon(KonqPixmapProvider::self()->iconNameFor(entry.url)),
        text, menu);
    action->setData(entry.url);
    menu->addAction(action);
}

void KonqMostOftenURLSAction::slotFillMenu()
{
    if (!m_parsingDone) { // first time
	parseHistory();
        m_parsingDone = true;
    }

    menu()->clear();

    for (int id = s_mostEntries->count() - 1; id >= 0; --id) {
        createHistoryAction(s_mostEntries->at(id), menu());
    }
    setEnabled( !s_mostEntries->isEmpty() );
}

void KonqMostOftenURLSAction::slotActivated(QAction* action)
{
    emit activated(action->data().value<KUrl>());
}

///////////////////////////////

KonqHistoryAction::KonqHistoryAction(const QString& text, QObject* parent)
    : KActionMenu(KIcon("goto-page"), text, parent)
{
    setDelayed(false);
    connect(menu(), SIGNAL(aboutToShow()), SLOT(slotFillMenu()));
    connect(menu(), SIGNAL(triggered(QAction*)), SLOT(slotActivated(QAction*)));
    setEnabled(!KonqHistoryManager::kself()->entries().isEmpty());
}

KonqHistoryAction::~KonqHistoryAction()
{
}

void KonqHistoryAction::slotFillMenu()
{
    menu()->clear();

    // Use the same configuration as the "most visited urls" action
    s_maxEntries = KonqSettings::numberofmostvisitedURLs();

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    const KonqHistoryList mgrEntries = mgr->entries();
    int idx = mgrEntries.count() - 1;
    // mgrEntries is "oldest first", so take the last s_maxEntries entries.
    for (int n = 0; idx >= 0 && n < s_maxEntries; --idx, ++n) {
        createHistoryAction(mgrEntries.at(idx), menu());
    }
}

void KonqHistoryAction::slotActivated(QAction* action)
{
    emit activated(action->data().value<KUrl>());
}

#include "konqactions.moc"
