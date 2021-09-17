/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
