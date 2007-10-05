//  -*- c-basic-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 1999 Kurt Granroth <granroth@kde.org>
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include "konqbookmarkbar.h"

#include <qregexp.h>
#include <qfile.h>
#include <qevent.h>
#include <qapplication.h>
#include <qdebug.h>

#include <ktoolbar.h>
#include <kactionmenu.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kmenu.h>
#include <kdebug.h>
#include <kconfiggroup.h>

#include "konqbookmarkmenu.h"
#include "kbookmarkimporter.h"
#include "kbookmarkdombuilder.h"



class KBookmarkBarPrivate
{
public:
    QList<KAction *> m_actions;
    int m_sepIndex;
    QList<int> widgetPositions; //right edge, bottom edge
    QString tempLabel;
    bool m_filteredToolbar;
    bool m_contextMenu;

    KBookmarkBarPrivate() :
        m_sepIndex( -1 )
    {
        // see KBookmarkSettings::readSettings in kio
        KConfig config("kbookmarkrc", KConfig::NoGlobals);
        KConfigGroup cg(&config, "Bookmarks");
        m_filteredToolbar = cg.readEntry( "FilteredToolbar", false );
        m_contextMenu = cg.readEntry( "ContextMenuActions", true );
    }
};


KBookmarkBar::KBookmarkBar( KBookmarkManager* mgr,
                            KonqBookmarkOwner *_owner, KToolBar *_toolBar,
                            QObject *parent )
    : QObject( parent ), m_pOwner(_owner), m_toolBar(_toolBar),
      m_pManager( mgr ), d( new KBookmarkBarPrivate )
{
    m_toolBar->setAcceptDrops( true );
    m_toolBar->installEventFilter( this ); // for drops

    if (d->m_contextMenu)
    {
        m_toolBar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_toolBar, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(contextMenu(const QPoint &)));
    }

    connect( mgr, SIGNAL( changed(const QString &, const QString &) ),
             SLOT( slotBookmarksChanged(const QString &) ) );
    connect( mgr, SIGNAL( configChanged() ),
             SLOT( slotConfigChanged() ) );

    KBookmarkGroup toolbar = getToolbar();
    fillBookmarkBar( toolbar );
    m_toolBarSeparator = new QAction(this);
}

QString KBookmarkBar::parentAddress()
{
    if(d->m_filteredToolbar)
        return "";
    else
        return m_pManager->toolbar().address();
}


KBookmarkGroup KBookmarkBar::getToolbar()
{
    if(d->m_filteredToolbar)
        return m_pManager->root();
    else
        return m_pManager->toolbar();
}

KBookmarkBar::~KBookmarkBar()
{
    //clear();
    qDeleteAll( d->m_actions );
    qDeleteAll( m_lstSubMenus );
    delete d;
}

void KBookmarkBar::clear()
{
    if (m_toolBar)
        m_toolBar->clear();
    qDeleteAll(d->m_actions);
    d->m_actions.clear();
    qDeleteAll( m_lstSubMenus );
    m_lstSubMenus.clear();
}

void KBookmarkBar::slotBookmarksChanged( const QString & group )
{
    KBookmarkGroup tb = getToolbar(); // heavy for non cached toolbar version
    kDebug(7043) << "KBookmarkBar::slotBookmarksChanged( " << group << " )";

    if ( tb.isNull() )
        return;
    
    if( d->m_filteredToolbar )
    {
        clear();
        fillBookmarkBar( tb );
    }
    else if ( KBookmark::commonParent(group, tb.address()) == group)  // Is group a parent of tb.address?
    {
        clear();
        fillBookmarkBar( tb );
    }
    else
    {
        // Iterate recursively into child menus
        for ( QList<KBookmarkMenu *>::ConstIterator smit = m_lstSubMenus.begin(), smend = m_lstSubMenus.end();
              smit != smend; ++smit )
        {
            (*smit)->slotBookmarksChanged( group );
        }
    }
}

void KBookmarkBar::slotConfigChanged()
{
    KConfig config("kbookmarkrc", KConfig::NoGlobals);
    KConfigGroup cg(&config, "Bookmarks");
    d->m_filteredToolbar = cg.readEntry( "FilteredToolbar", false );
    d->m_contextMenu = cg.readEntry( "ContextMenuActions", true );
    clear();
    fillBookmarkBar(getToolbar());
}

void KBookmarkBar::fillBookmarkBar(const KBookmarkGroup & parent)
{
    if (parent.isNull())
        return;

    qDebug()<<"fillBookmarkBar"<<parent.text();

    for (KBookmark bm = parent.first(); !bm.isNull(); bm = parent.next(bm))
    {
        qDebug()<<"bm is "<<bm.text();
        // Filtered special cases
        if(d->m_filteredToolbar)
        {
            if(bm.isGroup() && !bm.showInToolbar() )
		fillBookmarkBar(bm.toGroup());	       

	    if(!bm.showInToolbar())
		continue;
        }


        if (!bm.isGroup())
        {
	    if ( bm.isSeparator() )
                m_toolBar->addSeparator();
            else
            {
                KAction *action = new KBookmarkAction( bm, m_pOwner, 0 );
                m_toolBar->addAction(action);
                d->m_actions.append( action );
            }
        }
        else
        {
            KBookmarkActionMenu *action = new KBookmarkActionMenu(bm, 0);
            action->setDelayed( false );
            m_toolBar->addAction(action);
            d->m_actions.append( action );
            KBookmarkMenu *menu = new KonqBookmarkMenu(m_pManager, m_pOwner, action, bm.address());
            m_lstSubMenus.append( menu );
        }
    }
}

void KBookmarkBar::removeTempSep()
{
    if (m_toolBarSeparator)
        m_toolBar->removeAction(m_toolBarSeparator);
	    
}

/**
 * Handle a QDragMoveEvent event on a toolbar drop
 * @return true if the event should be accepted, false if the event should be ignored
 * @param pos the current QDragMoveEvent position
 * @param the toolbar
 * @param actions the list of actions plugged into the bar
 *        returned action was dropped on
 */
bool KBookmarkBar::handleToolbarDragMoveEvent(const QPoint& p, const QList<KAction *>& actions, const QString &text)
{
    if(d->m_filteredToolbar)
        return false;
    int pos = m_toolBar->orientation() == Qt::Horizontal ? p.x() : p.y();
    Q_ASSERT( actions.isEmpty() || (m_toolBar == qobject_cast<KToolBar*>(actions.first()->associatedWidgets().value(0))) );
    m_toolBar->setUpdatesEnabled(false);
    removeTempSep();

    bool foundWidget = false;
    // Right To Left
    // only relevant if the toolbar is Horizontal!
    bool rtl = QApplication::isRightToLeft() && m_toolBar->orientation() == Qt::Horizontal;
    m_toolBarSeparator->setText(text);

    // Empty toolbar
    if(actions.isEmpty())
    {
        d->m_sepIndex = 0;
        m_toolBar->addAction(m_toolBarSeparator);
        m_toolBar->setUpdatesEnabled(true);
        return true;
    }

    // else find the toolbar button
    for(int i = 0; i < d->widgetPositions.count(); ++i)
    {
        if( rtl ^ (pos <= d->widgetPositions[i]) )
        {
            foundWidget = true;
            d->m_sepIndex = i;
            break;
        }
    }

    QString address;

    if (foundWidget) // found the containing button
    {
        int leftOrTop = d->m_sepIndex == 0 ? 0 : d->widgetPositions[d->m_sepIndex-1];
        int rightOrBottom = d->widgetPositions[d->m_sepIndex];
        if ( rtl ^ (pos >= (leftOrTop + rightOrBottom)/2) )
        {
            // if in second half of button then
            // we jump to next index
            d->m_sepIndex++;
        }
        if(d->m_sepIndex != actions.count())
        {
            QAction *before = m_toolBar->actions()[d->m_sepIndex];
            m_toolBar->insertAction(before, m_toolBarSeparator);
        }
        else
        {
            m_toolBar->addAction(m_toolBarSeparator);
        }
        m_toolBar->setUpdatesEnabled(true);
        return true;
    }
    else // (!foundWidget)
    {
        // if !b and not past last button, we didn't find button
        if (rtl ^ (pos <= d->widgetPositions[d->widgetPositions.count()-1]) )
        {
            m_toolBar->setUpdatesEnabled(true);
            return false;
        }
        else // location is beyond last action, assuming we want to add in the end
        {
            d->m_sepIndex = actions.count();
            m_toolBar->addAction(m_toolBarSeparator);
            m_toolBar->setUpdatesEnabled(true);
            return true;
        }
    }
}

void KBookmarkBar::contextMenu(const QPoint & pos)
{
    KBookmarkActionInterface * action = dynamic_cast<KBookmarkActionInterface *>( m_toolBar->actionAt(pos) );
    if(!action)
        return;
    KMenu * menu = new KonqBookmarkContextMenu(action->bookmark(), m_pManager, m_pOwner);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(m_toolBar->mapToGlobal(pos));
}

// TODO    *** drop improvements ***
// open submenus on drop interactions
bool KBookmarkBar::eventFilter( QObject *, QEvent *e )
{
    if (d->m_filteredToolbar) 
        return false; // todo: make this limit the actions

    if ( e->type() == QEvent::DragLeave )
    {
        removeTempSep();
    }
    else if ( e->type() == QEvent::Drop )
    {
        removeTempSep();

        QDropEvent *dev = static_cast<QDropEvent*>( e );
        QList<KBookmark> list = KBookmark::List::fromMimeData( dev->mimeData() );
        if ( list.isEmpty() )
            return false;
        if (list.count() > 1)
            kWarning(7043) << "Sorry, currently you can only drop one address "
                "onto the bookmark bar!" << endl;
        KBookmark toInsert = list.first();

        KBookmarkGroup parentBookmark = getToolbar();

        if(d->m_sepIndex == 0)
        {
            KBookmark newBookmark = parentBookmark.addBookmark(toInsert.fullText(), toInsert.url() );

            parentBookmark.moveBookmark( newBookmark, KBookmark() );
            m_pManager->emitChanged( parentBookmark );
            return true;
        }
        else
        {
            KBookmark after = parentBookmark.first();

            for(int i=0; i < d->m_sepIndex - 1 ; ++i)
                after = parentBookmark.next(after);
            KBookmark newBookmark = parentBookmark.addBookmark(toInsert.fullText(), toInsert.url() );

            parentBookmark.moveBookmark( newBookmark, after );
            m_pManager->emitChanged( parentBookmark );
            return true;
        }
    }
    else if ( e->type() == QEvent::DragMove || e->type() == QEvent::DragEnter )
    {
        QDragMoveEvent *dme = static_cast<QDragMoveEvent*>( e );
        if (!KBookmark::List::canDecode( dme->mimeData() ))
            return false;

        //cache text, save positions (inserting the temporary widget changes the positions)
        if(e->type() == QEvent::DragEnter)
        {
            QList<KBookmark> list = KBookmark::List::fromMimeData( dme->mimeData() );
            if ( list.isEmpty() )
                return false;
            d->tempLabel  = list.first().url().pathOrUrl();

            d->widgetPositions.clear();

            for (int i = 0; i < m_toolBar->actions().count(); ++i)
                if (QWidget* button = m_toolBar->widgetForAction(m_toolBar->actions()[i]))
                    if(m_toolBar->orientation() == Qt::Horizontal)
                        if(QApplication::isLeftToRight())
                            d->widgetPositions.push_back(button->geometry().right());
                        else
                            d->widgetPositions.push_back(button->geometry().left());
                    else
                        d->widgetPositions.push_back(button->geometry().bottom());
        }

        bool accept = handleToolbarDragMoveEvent(dme->pos(), d->m_actions, d->tempLabel);
        if (accept)
        {
            dme->accept();
            return true; //Really?
        }
    }
    return false;
}

#include "konqbookmarkbar.moc"
