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


#include <ktoolbar.h>
#include <kactionmenu.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kmenu.h>
#include <kdebug.h>

#include "konqbookmarkmenu.h"
#include "kbookmarkimporter.h"
#include "kbookmarkdombuilder.h"



class KBookmarkBarPrivate
{
public:
    QList<KAction *> m_actions;
    KBookmarkManager* m_filteredMgr;
    int m_sepIndex;
    QList<int> widgetPositions; //right edge, bottom edge
    QString tempLabel;
    bool m_filteredToolbar;
    bool m_contextMenu;

    KBookmarkBarPrivate() :
        m_filteredMgr( 0 ),
        m_sepIndex( -1 )
    {
        // see KBookmarkSettings::readSettings in kio
        KConfig config("kbookmarkrc", KConfig::NoGlobals);
        KConfigGroup cg(&config, "Bookmarks");
        m_filteredToolbar = cg.readEntry( "FilteredToolbar", false );
        m_contextMenu = cg.readEntry( "ContextMenuActions", true );
    }
};

// usage of KXBELBookmarkImporterImpl is just plain evil, but it reduces code dup. so...
class ToolbarFilter : public KXBELBookmarkImporterImpl
{
public:
    ToolbarFilter() : m_visible(false) { ; }
    void filter( const KBookmarkGroup &grp ) { traverse(grp); }
private:
    virtual void visit( const KBookmark & );
    virtual void visitEnter( const KBookmarkGroup & );
    virtual void visitLeave( const KBookmarkGroup & );
private:
    bool m_visible;
    KBookmarkGroup m_visibleStart;
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

    KBookmarkGroup toolbar = getToolbar();
    fillBookmarkBar( toolbar );
    m_toolBarSeparator = new QAction(this);
}

QString KBookmarkBar::parentAddress()
{
    return d->m_filteredMgr ? QString() : m_pManager->toolbar().address();
}

#define CURRENT_TOOLBAR() ( \
    d->m_filteredMgr ? d->m_filteredMgr->root()  \
                          : m_pManager->toolbar() )

#define CURRENT_MANAGER() ( \
    d->m_filteredMgr ? d->m_filteredMgr  \
                          : m_pManager )

KBookmarkGroup KBookmarkBar::getToolbar()
{
    if ( d->m_filteredToolbar )
    {
        if ( !d->m_filteredMgr ) {
            d->m_filteredMgr = KBookmarkManager::createTempManager();
        } else {
            KBookmarkGroup bkRoot = d->m_filteredMgr->root();
            QList<KBookmark> bks;
            for (KBookmark bm = bkRoot.first(); !bm.isNull(); bm = bkRoot.next(bm))
                bks.append( bm );
            for ( QList<KBookmark>::const_iterator bkit = bks.begin(), bkend = bks.end() ; bkit != bkend ; ++bkit ) {
                bkRoot.deleteBookmark( (*bkit) );
            }
        }
        ToolbarFilter filter;
        KBookmarkDomBuilder builder( d->m_filteredMgr->root(),
                                     d->m_filteredMgr );
        builder.connectImporter( &filter );
        filter.filter( m_pManager->root() );
    }

    return CURRENT_TOOLBAR();
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
    m_toolBar->clear();
    qDeleteAll(d->m_actions);
    d->m_actions.clear();
    qDeleteAll( m_lstSubMenus );
    m_lstSubMenus.clear();
}

void KBookmarkBar::slotBookmarksChanged( const QString & group )
{
    KBookmarkGroup tb = getToolbar(); // heavy for non cached toolbar version
    kDebug(7043) << "slotBookmarksChanged( " << group << " )" << endl;

    if ( tb.isNull() )
        return;

    if ( KBookmark::commonParent(group, tb.address()) == group  // Is group a parent of tb.address?
         || d->m_filteredToolbar )
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

void KBookmarkBar::fillBookmarkBar(KBookmarkGroup & parent)
{
    if (parent.isNull())
        return;

    for (KBookmark bm = parent.first(); !bm.isNull(); bm = parent.next(bm))
    {
        if (!bm.isGroup())
        {
            if ( bm.isSeparator() )
                m_toolBar->addSeparator();
            else
            {
                KAction *action = new KonqBookmarkAction( bm, m_pOwner, 0 );
                m_toolBar->addAction(action);
                d->m_actions.append( action );
            }
        }
        else
        {
            KonqBookmarkActionMenu *action = new KonqBookmarkActionMenu(bm, 0);
            action->setDelayed( false );
            m_toolBar->addAction(action);
            d->m_actions.append( action );
            KBookmarkMenu *menu = new KonqBookmarkMenu(CURRENT_MANAGER(), m_pOwner, action, bm.address());
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
    action->contextMenu(m_toolBar->mapToGlobal(pos), m_pManager, m_pOwner);
}

// TODO    *** drop improvements ***
// open submenus on drop interactions
bool KBookmarkBar::eventFilter( QObject *, QEvent *e )
{
    if (d->m_filteredMgr) // note, we assume m_pManager in various places,
                          // this shouldn't really be the case
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
            KBookmark newBookmark = parentBookmark.addBookmark(
                m_pManager, toInsert.fullText(),
                toInsert.url() );

            parentBookmark.moveItem( newBookmark, KBookmark() );
            m_pManager->emitChanged( parentBookmark );
            return true;
        }
        else
        {
            KBookmark after = parentBookmark.first();

            for(int i=0; i < d->m_sepIndex - 1 ; ++i)
                after = parentBookmark.next(after);
            KBookmark newBookmark = parentBookmark.addBookmark(
                    m_pManager, toInsert.fullText(),
                    toInsert.url() );

            parentBookmark.moveItem( newBookmark, after );
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

static bool showInToolbar( const KBookmark &bk ) {
    return (bk.internalElement().attributes().namedItem("showintoolbar").toAttr().value() == "yes");
}

void ToolbarFilter::visit( const KBookmark &bk ) {
    //kDebug() << "visit(" << bk.text() << ")" << endl;
    if ( m_visible || showInToolbar(bk) )
        KXBELBookmarkImporterImpl::visit(bk);
}

void ToolbarFilter::visitEnter( const KBookmarkGroup &grp ) {
    //kDebug() << "visitEnter(" << grp.text() << ")" << endl;
    if ( !m_visible && showInToolbar(grp) )
    {
        m_visibleStart = grp;
        m_visible = true;
    }
    if ( m_visible )
        KXBELBookmarkImporterImpl::visitEnter(grp);
}

void ToolbarFilter::visitLeave( const KBookmarkGroup &grp ) {
    //kDebug() << "visitLeave()" << endl;
    if ( m_visible )
        KXBELBookmarkImporterImpl::visitLeave(grp);
    if ( m_visible && grp.address() == m_visibleStart.address() )
        m_visible = false;
}

#include "konqbookmarkbar.moc"
