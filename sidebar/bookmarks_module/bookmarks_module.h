/*
    Copyright (C) 2019 Raphael Rosch <kde-dev@insaner.com>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef BOOKMARKS_MODULE_H
#define BOOKMARKS_MODULE_H

#include <konqsidebarplugin.h>
class QTreeView;
class QStandardItemModel;
class QItemSelection;

class KonqSideBarBookmarksModule : public KonqSidebarModule
{
    Q_OBJECT

public:
    KonqSideBarBookmarksModule(QWidget *parent,
                            const KConfigGroup &configGroup);
    ~KonqSideBarBookmarksModule() override;

    QWidget *getWidget() override;
    void handleURL(const QUrl &hand_url) override;
	
private slots:
    void slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void customEvent(QEvent *ev) override;

private:
    QTreeView *treeView;
    QStandardItemModel *model;
    QUrl m_lastURL;
    QUrl m_initURL;
};

#endif
