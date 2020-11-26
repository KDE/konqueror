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

#ifndef TREE_MODULE_H
#define TREE_MODULE_H

#include <konqsidebarplugin.h>

#include <QTreeView>
#include <QDebug>

#include <QUrl>
#include <QDir>
#include <KParts/PartActivateEvent>

#include <KDirModel>
#include <KDirLister>
#include <KDirSortFilterProxyModel>
#include <QDirIterator>

#include <kio_version.h>


class KonqSideBarTreeModule : public KonqSidebarModule
{
    Q_OBJECT

public:
    KonqSideBarTreeModule(QWidget *parent,
                            const KConfigGroup &configGroup);
    ~KonqSideBarTreeModule() override;

    QWidget *getWidget() override;
    void handleURL(const QUrl &hand_url) override;

private slots:
    void slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void slotUpdateColWidth();
    void slotKDirExpand_setRootIndex();
    void slotKDirExpand_setSelection(const QModelIndex &index);
    void customEvent(QEvent *ev) override;

private:
    void setSelection(const QUrl &target_url, bool do_openURLreq=true);
    void setSelectionIndex(const QModelIndex &index);
    QUrl getUrlFromIndex(const QModelIndex &index);
    QModelIndex resolveIndex(const QModelIndex &index);
    QModelIndex getIndexFromUrl(const QUrl &url) const;
    QUrl cleanupURL(const QUrl &url);

    QTreeView *treeView;
    QUrl m_lastURL;
    QUrl m_initURL;
    bool m_ignoreHandle = false;

    KDirModel *model;
    KDirSortFilterProxyModel *sorted_model;
};

#endif
