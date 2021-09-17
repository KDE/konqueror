/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KONQ_SIDEBARTREEMODULE_H
#define KONQ_SIDEBARTREEMODULE_H

#include <QObject>
#include "konq_sidebartree.h"

class KonqSidebarTreeTopLevelItem;
class KonqSidebarTree;

/**
 * The base class for KonqSidebarTree Modules. It defines the interface
 * between the generic KonqSidebarTree and the particular modules
 * (directory tree, history, bookmarks, ...)
 */
class KonqSidebarTreeModule
{
public:
    explicit KonqSidebarTreeModule(KonqSidebarTree *parentTree, bool showHidden = false)
        : m_pTree(parentTree), m_showHidden(showHidden) {}
    virtual ~KonqSidebarTreeModule() {}

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem(KonqSidebarTreeTopLevelItem *item) = 0;

    // Open this toplevel item - you don't need to reimplement if
    // you create the item's children right away
    virtual void openTopLevelItem(KonqSidebarTreeTopLevelItem *) {}

    // Follow a URL opened in another view - only implement if the module
    // has anything to do with URLs
    virtual void followURL(const QUrl &) {}

    KonqSidebarTree *tree() const
    {
        return m_pTree;
    }
    bool showHidden()
    {
        return m_showHidden;
    }
    virtual void setShowHidden(bool showhidden)
    {
        m_showHidden = showhidden;
    }

    virtual bool handleTopLevelContextMenu(KonqSidebarTreeTopLevelItem *, const QPoint &)
    {
        return false;
    }

protected:
    KonqSidebarTree *m_pTree;
    bool m_showHidden;
};

#endif // KONQ_SIDEBARTREEMODULE_H
