/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef dirtree_module_h
#define dirtree_module_h

#include <konq_sidebartreemodule.h>
#include <kfileitem.h>
#include <Qt3Support/Q3Dict>
#include <Qt3Support/Q3PtrDict>

class KDirLister;
class KonqSidebarTree;
class KonqSidebarTreeItem;
class KonqSidebarDirTreeItem;

class KonqSidebarDirTreeModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT
public:
    KonqSidebarDirTreeModule(KonqSidebarTree *parentTree, bool);
    virtual ~KonqSidebarDirTreeModule();

    virtual void addTopLevelItem(KonqSidebarTreeTopLevelItem *item);

    virtual void openTopLevelItem(KonqSidebarTreeTopLevelItem *item);

    virtual void followURL(const QUrl &url);

    // Called by KonqSidebarDirTreeItem
    void openSubFolder(KonqSidebarTreeItem *item);
    void addSubDir(KonqSidebarTreeItem *item);
    void removeSubDir(KonqSidebarTreeItem *item, bool childrenonly = false);

private Q_SLOTS:
    void slotNewItems(const KFileItemList &);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &);
    void slotDeleteItem(const KFileItem &item);
    void slotRedirection(const QUrl &oldUrl, const QUrl &newUrl);
    void slotListingStopped(const QUrl &url);

private:
    //KonqSidebarTreeItem * findDir( const QUrl &_url );
    void listDirectory(KonqSidebarTreeItem *item);
    QList<QUrl> selectedUrls();

    // URL -> item
    // Each KonqSidebarDirTreeItem is indexed on item->id() and
    // all item->alias'es
    Q3Dict<KonqSidebarTreeItem> m_dictSubDirs;

    // KFileItem -> item
    QHash<KFileItem, KonqSidebarTreeItem *> m_ptrdictSubDirs;

    KDirLister *m_dirLister;

    QUrl m_selectAfterOpening;

    KonqSidebarTreeTopLevelItem *m_topLevelItem;

    bool m_showArchivesAsFolders;
};

#endif
