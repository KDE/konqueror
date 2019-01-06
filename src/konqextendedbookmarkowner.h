/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#ifndef KONQEXTENDEDBOOKMARKOWNER_H
#define KONQEXTENDEDBOOKMARKOWNER_H

#include <konqbookmarkmenu.h>

class KonqExtendedBookmarkOwner : public KBookmarkOwner
{
public:
    KonqExtendedBookmarkOwner(KonqMainWindow *);
    QString currentTitle() const override;
    QUrl currentUrl() const override;
    bool supportsTabs() const override;
    QList<FutureBookmark> currentBookmarkList() const override;
    void openBookmark(const KBookmark &bm, Qt::MouseButtons mb, Qt::KeyboardModifiers km) override;
    void openInNewTab(const KBookmark &bm) override;
    void openInNewWindow(const KBookmark &bm) override;
    void openFolderinTabs(const KBookmarkGroup &grp) override;

private:
    KonqMainWindow *m_pKonqMainWindow;
};

#endif
