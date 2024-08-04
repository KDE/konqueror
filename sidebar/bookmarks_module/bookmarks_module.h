/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BOOKMARKS_MODULE_H
#define BOOKMARKS_MODULE_H

#include <konqsidebarplugin.h>

#include <KBookmark>

#include <QTreeView>

class QStandardItemModel;
class QStandardItem;
class QItemSelection;
class KBookmarkManager;

class KonqSidebarBookmarksModuleView : public QTreeView
{
    Q_OBJECT

public:
    KonqSidebarBookmarksModuleView(QWidget *parent = nullptr);
    ~KonqSidebarBookmarksModuleView();

Q_SIGNALS:
    void middleClick(const QModelIndex &idx);

protected:
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;

private:
    bool m_middleMouseBtnPressed = false;
};

class KonqSideBarBookmarksModule : public KonqSidebarModule
{
    Q_OBJECT

public:
    KonqSideBarBookmarksModule(QWidget *parent,
                            const KConfigGroup &configGroup);
    ~KonqSideBarBookmarksModule() override;

    QWidget *getWidget() override;

public:
    QStandardItem *itemFromBookmark(const KBookmark &bm);
    UrlType urlType() const override;

private slots:
    void customEvent(QEvent *ev) override;
    void updateBookmarkGroup(const QString &address);
    void activateItem(const QModelIndex &idx, bool newTab);

private:
    void fill();
    QStandardItem *itemAtAddress(const QString &address);
    QUrl url(QStandardItem *it) const;
    QString address(QStandardItem *it) const;
    KBookmark bookmark(QStandardItem *it) const;
    enum Roles {UrlRole = Qt::UserRole, AddressRole};

private:
    KBookmarkManager *m_bmManager;
    KonqSidebarBookmarksModuleView *treeView;
    QStandardItemModel *model;
    QUrl m_lastURL;
    QUrl m_initURL;

    class BookmarkGroupTraverser : public KBookmarkGroupTraverser
    {
    public:
        BookmarkGroupTraverser(KonqSideBarBookmarksModule *mod);
        ~BookmarkGroupTraverser() override;

        QList<QStandardItem*> createSubtree(const KBookmarkGroup &grp);
    protected:
        void visit(const KBookmark & ) override;
        void visitEnter(const KBookmarkGroup & ) override;
        void visitLeave(const KBookmarkGroup & ) override;

    private:
        KonqSideBarBookmarksModule *m_module;
        QStandardItem *m_currentGroup = nullptr;
        QList<QStandardItem*> m_items;
    };
};

#endif
