/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef bookmark_module_h
#define bookmark_module_h

#include <QMap>
#include <QObject>

#include <konq_sidebartreemodule.h>
#include <kdialog.h>
#include <klocale.h>

class KActionCollection;
class KLineEdit;
class KBookmarkGroup;
class KonqSidebarBookmarkItem;

/**
 * This module displays bookmarks in the tree
 */
class KonqSidebarBookmarkModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT
public:
    KonqSidebarBookmarkModule(KonqSidebarTree *parentTree);
    virtual ~KonqSidebarBookmarkModule();

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem(KonqSidebarTreeTopLevelItem *item);
    virtual bool handleTopLevelContextMenu(KonqSidebarTreeTopLevelItem *, const QPoint &);

    void showPopupMenu();

protected Q_SLOTS:
    void slotBookmarksChanged(const QString &);
    void slotMoved(Q3ListViewItem *, Q3ListViewItem *, Q3ListViewItem *);
    void slotDropped(K3ListView *, QDropEvent *, Q3ListViewItem *, Q3ListViewItem *);
    void slotCreateFolder();
    void slotDelete();
    void slotProperties(KonqSidebarBookmarkItem *bi = 0);
    void slotOpenNewWindow();
    void slotOpenTab();
    void slotCopyLocation();

protected:
    void fillListView();
    void fillGroup(KonqSidebarTreeItem *parentItem, const KBookmarkGroup &group);
    KonqSidebarBookmarkItem *findByAddress(const QString &address) const;

private Q_SLOTS:
    void slotOpenChange(Q3ListViewItem *);

private:
    KonqSidebarTreeTopLevelItem *m_topLevelItem;
    KonqSidebarBookmarkItem *m_rootItem;

    KActionCollection *m_collection;

    bool m_ignoreOpenChange;
    QMap<QString, bool> m_folderOpenState;
};

class BookmarkEditDialog : public KDialog
{
    Q_OBJECT

public:
    BookmarkEditDialog(const QString &title, const QString &url,
                       QWidget * = nullptr, const char * = nullptr,
                       const QString &caption = i18nc("@title:window", "Add Bookmark"));

    QString finalUrl() const;
    QString finalTitle() const;

private:
    KLineEdit *m_title, *m_location;
};

#endif
