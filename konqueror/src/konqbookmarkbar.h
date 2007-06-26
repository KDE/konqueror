//  -*- c-basic-offset:4; indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 1999 Kurt Granroth <granroth@kde.org>

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
#ifndef KONQBOOKMARKBAR_H
#define KONQBOOKMARKBAR_H

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QList>
#include <kbookmark.h>
#include <kaction.h>

class KToolBar;
class KBookmarkMenu;
class KonqBookmarkOwner;
class KActionCollection;
class KAction;
class KBookmarkBarPrivate;

/**
 * This class provides a bookmark toolbar.  Using this class is nearly
 * identical to using KBookmarkMenu so follow the directions
 * there.
 */
//FIXME rename KonqBookmarkBar
class KBookmarkBar : public QObject
{
    Q_OBJECT
public:
    /**
     * Fills a bookmark toolbar
     *
     * @param manager the bookmark manager
     * @param owner implementation of the KonqBookmarkOwner interface (callbacks)
     * @param toolBar toolbar to fill
     *
     * The KActionCollection pointer argument is now obsolete.
     *
     * @param parent the parent widget for the bookmark toolbar
     */
    KBookmarkBar( KBookmarkManager* manager,
                  KonqBookmarkOwner *owner, KToolBar *toolBar,
                  QObject *parent = 0);

    virtual ~KBookmarkBar();

    QString parentAddress();

public Q_SLOTS:
    void clear();
    void contextMenu( const QPoint & );

    void slotBookmarksChanged( const QString & );

protected:
    void fillBookmarkBar( KBookmarkGroup & parent );
    virtual bool eventFilter( QObject *o, QEvent *e );

private:
    KBookmarkGroup getToolbar();
    void removeTempSep();
    bool handleToolbarDragMoveEvent(const QPoint& pos, const QList<KAction *>& actions, const QString &text);

    KonqBookmarkOwner *m_pOwner;
    QPointer<KToolBar> m_toolBar;
    KActionCollection *m_actionCollection;
    KBookmarkManager *m_pManager;
    QList<KBookmarkMenu *> m_lstSubMenus;
    QAction* m_toolBarSeparator;

    KBookmarkBarPrivate * const d;
};

#endif // KONQBOOKMARKBAR_H
