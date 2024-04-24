//  -*- c-basic-offset:4; indent-tabs-mode:nil -*-
/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1999 Kurt Granroth <granroth@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
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
class KBookmarkManager;

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
    KBookmarkManager *m_pManager;
    QList<KBookmarkMenu *> m_lstSubMenus;
    QAction *m_toolBarSeparator;

    KBookmarkBarPrivate *const d;
};

#endif // KONQBOOKMARKBAR_H
