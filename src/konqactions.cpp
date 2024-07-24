/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konqactions.h"

#include "konqview.h"
#include "konqsettings.h"
#include "konqpixmapprovider.h"

#include "konqdebug.h"

#include <kwidgetsaddons_version.h>

#include <QIcon>
#include <QMenu>
#include <QAction>

#include <algorithm>

template class QList<KonqHistoryEntry *>;

/////////////////

//static - used by back/forward popups in KonqMainWindow
void KonqActions::fillHistoryPopup(const QList<HistoryEntry *> &history, int historyIndex,
                                   QMenu *popup,
                                   bool onlyBack,
                                   bool onlyForward)
{
    Q_ASSERT(popup);   // kill me if this 0... :/

    //qCDebug(KONQUEROR_LOG) << "fillHistoryPopup position: " << history.at();
    int index = 0;
    if (onlyBack || onlyForward) { // this if() is always true nowadays.
        index += historyIndex; // Jump to current item
        if (!onlyForward) {
            --index;
        } else {
            ++index;    // And move off it
        }
    }

    QFontMetrics fm = popup->fontMetrics();
    int i = 0;
    while (index < history.count() && index >= 0) {
        QString text = history[ index ]->title;
        text = fm.elidedText(text, Qt::ElideMiddle, fm.maxWidth() * 30);
        text.replace('&', QLatin1String("&&"));
        const QString iconName = KonqPixmapProvider::self()->iconNameFor(history[index]->url);
        QAction *action = new QAction(QIcon::fromTheme(iconName), text, popup);
        action->setData(index - historyIndex);
        //qCDebug(KONQUEROR_LOG) << text << index - historyIndex;
        popup->addAction(action);
        if (++i > 10) {
            break;
        }
        if (!onlyForward) {
            --index;
        } else {
            ++index;
        }
    }
    //qCDebug(KONQUEROR_LOG) << "After fillHistoryPopup position: " << history.at();
}

///////////////////////////////

static int s_maxEntries = 0;

KonqMostOftenURLSAction::KonqMostOftenURLSAction(const QString &text,
        QObject *parent)
    : KActionMenu(QIcon::fromTheme(QStringLiteral("go-jump")), text, parent),
      m_parsingDone(false)
{
    setPopupMode(QToolButton::InstantPopup);
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
    s_maxEntries = Konq::Settings::numberofmostvisitedURLs();

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    setEnabled(!mgr->entries().isEmpty() && s_maxEntries > 0);
}

Q_GLOBAL_STATIC(KonqHistoryList, s_mostEntries)

void KonqMostOftenURLSAction::inSort(const KonqHistoryEntry &entry)
{
    KonqHistoryList::iterator it = std::lower_bound(s_mostEntries->begin(),
                                   s_mostEntries->end(),
                                   entry,
                                   numberOfVisitOrder);
    s_mostEntries->insert(it, entry);
}

void KonqMostOftenURLSAction::parseHistory() // only ever called once
{
    KonqHistoryManager *mgr = KonqHistoryManager::kself();

    connect(mgr, SIGNAL(entryAdded(KonqHistoryEntry)),
            SLOT(slotEntryAdded(KonqHistoryEntry)));
    connect(mgr, SIGNAL(entryRemoved(KonqHistoryEntry)),
            SLOT(slotEntryRemoved(KonqHistoryEntry)));
    connect(mgr, SIGNAL(cleared()), SLOT(slotHistoryCleared()));

    const KonqHistoryList mgrEntries = mgr->entries();
    KonqHistoryList::const_iterator it = mgrEntries.begin();
    const KonqHistoryList::const_iterator end = mgrEntries.end();
    for (int i = 0; it != end && i < s_maxEntries; ++i, ++it) {
        s_mostEntries->append(*it);
    }
    std::sort(s_mostEntries->begin(), s_mostEntries->end(), numberOfVisitOrder);

    while (it != end) {
        const KonqHistoryEntry &leastOften = s_mostEntries->first();
        const KonqHistoryEntry &entry = *it;
        if (leastOften.numberOfTimesVisited < entry.numberOfTimesVisited) {
            s_mostEntries->removeFirst();
            inSort(entry);
        }

        ++it;
    }
}

void KonqMostOftenURLSAction::slotEntryAdded(const KonqHistoryEntry &entry)
{
    // if it's already present, remove it, and inSort it
    s_mostEntries->removeEntry(entry.url);

    if (s_mostEntries->count() >= s_maxEntries) {
        const KonqHistoryEntry &leastOften = s_mostEntries->first();
        if (leastOften.numberOfTimesVisited < entry.numberOfTimesVisited) {
            s_mostEntries->removeFirst();
            inSort(entry);
        }
    }

    else {
        inSort(entry);
    }
    setEnabled(!s_mostEntries->isEmpty());
}

void KonqMostOftenURLSAction::slotEntryRemoved(const KonqHistoryEntry &entry)
{
    s_mostEntries->removeEntry(entry.url);
    setEnabled(!s_mostEntries->isEmpty());
}

void KonqMostOftenURLSAction::slotHistoryCleared()
{
    s_mostEntries->clear();
    setEnabled(false);
}

static void createHistoryAction(const KonqHistoryEntry &entry, QMenu *menu)
{
    // we take either title, typedUrl or URL (in this order)
    const QString text = entry.title.isEmpty() ? (entry.typedUrl.isEmpty() ?
                         entry.url.toDisplayString() :
                         entry.typedUrl) :
                         entry.title;
    QAction *action = new QAction(
        QIcon::fromTheme(KonqPixmapProvider::self()->iconNameFor(entry.url)),
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
    setEnabled(!s_mostEntries->isEmpty());
}

void KonqMostOftenURLSAction::slotActivated(QAction *action)
{
    emit activated(action->data().value<QUrl>());
}

///////////////////////////////

KonqHistoryAction::KonqHistoryAction(const QString &text, QObject *parent)
    : KActionMenu(QIcon::fromTheme(QStringLiteral("go-jump")), text, parent)
{
    setPopupMode(QToolButton::InstantPopup);
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
    s_maxEntries = Konq::Settings::numberofmostvisitedURLs();

    KonqHistoryManager *mgr = KonqHistoryManager::kself();
    const KonqHistoryList mgrEntries = mgr->entries();
    int idx = mgrEntries.count() - 1;
    // mgrEntries is "oldest first", so take the last s_maxEntries entries.
    for (int n = 0; idx >= 0 && n < s_maxEntries; --idx, ++n) {
        createHistoryAction(mgrEntries.at(idx), menu());
    }
}

void KonqHistoryAction::slotActivated(QAction *action)
{
    emit activated(action->data().value<QUrl>());
}
