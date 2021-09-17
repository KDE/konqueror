/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
