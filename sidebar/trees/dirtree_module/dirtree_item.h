/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef DIRTREE_ITEM_H
#define DIRTREE_ITEM_H

#include "konq_sidebartreeitem.h"

#include <kfileitem.h>
#include <QUrl>
#include <konq_operations.h>

#include <QStringList>

class QDropEvent;

class KonqSidebarDirTreeItem : public KonqSidebarTreeItem
{
public:
    KonqSidebarDirTreeItem(KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem, const KFileItem &fileItem);
    KonqSidebarDirTreeItem(KonqSidebarTree *parent, KonqSidebarTreeTopLevelItem *topLevelItem, const KFileItem &fileItem);
    ~KonqSidebarDirTreeItem();

    KFileItem fileItem() const
    {
        return m_fileItem;
    }

    virtual void setOpen(bool open);

    virtual void paintCell(QPainter *_painter, const QColorGroup &_cg, int _column, int _width, int _alignment);

    virtual bool acceptsDrops(const Q3StrList &formats);
    virtual void drop(QDropEvent *ev);
    virtual bool populateMimeData(QMimeData *mimeData, bool move);

    virtual void middleButtonClicked();
    virtual void rightButtonPressed();

    virtual void paste();
    virtual void trash();
    virtual void del();
    virtual void rename(); // start a rename operation
    void rename(const QString &name);    // do the actual renaming

    // The URL to open when this link is clicked
    virtual QUrl externalURL() const;
    virtual QString externalMimeType() const;
    virtual QString toolTipText() const;

    virtual void itemSelected();

    void reset();

    bool hasStandardIcon();

    QString id;

private:
    void delOperation(KonqOperations::Operation method);
    KFileItem m_fileItem;
};

#endif
