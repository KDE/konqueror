//  -*- c-basic-offset:4; indent-tabs-mode:nil -*-
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

#include <QObject>
#include <QPointer>
#include <QList>
#include <kbookmark.h>
#include <kactioncollection.h>

class KToolBar;
class KBookmarkMenu;
class KBookmarkOwner;
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
     * @param owner implementation of the KBookmarkOwner interface (callbacks)
     * @param toolBar toolbar to fill
     *
     * The KActionCollection pointer argument is now obsolete.
     *
     * @param parent the parent widget for the bookmark toolbar
     */
    KBookmarkBar(KBookmarkManager *manager,
                 KBookmarkOwner *owner, KToolBar *toolBar,
                 QObject *parent = nullptr);

    ~KBookmarkBar() override;

    QString parentAddress();

public Q_SLOTS:
    void clear();
    void contextMenu(const QPoint &);

    void slotBookmarksChanged(const QString &);
    void slotConfigChanged();

protected:
    void fillBookmarkBar(const KBookmarkGroup &parent);
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    KBookmarkGroup getToolbar();
    void removeTempSep();
    bool handleToolbarDragMoveEvent(const QPoint &pos, const QList<QAction *> &actions, const QString &text);

    KBookmarkOwner *m_pOwner;
    QPointer<KToolBar> m_toolBar;
    KActionCollection *m_actionCollection;
    KBookmarkManager *m_pManager;
    QList<KBookmarkMenu *> m_lstSubMenus;
    QAction *m_toolBarSeparator;

    KBookmarkBarPrivate *const d;
};

#endif // KONQBOOKMARKBAR_H
