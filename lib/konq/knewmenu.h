/* This file is part of the KDE project
   Copyright (C) 1998-2006 David Faure <faure@kde.org>
                 2003      Sven Leiber <s.leiber@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KNEWMENU_H
#define KNEWMENU_H

#include <kactionmenu.h>
#include <kurl.h>
#include <libkonq_export.h>

class KJob;
namespace KIO { class Job; }

class KDirWatch;

/**
 * The 'New' submenu, both for the File menu and the RMB popup menu.
 * (The same instance can be used by both).
 * It fills the menu with 'Folder' and one item per installed template.
 *
 * To use this class, you need to connect aboutToShow() of the File menu
 * with slotCheckUpToDate() and to call slotCheckUpToDate() before showing
 * the RMB popupmenu.
 *
 * KNewMenu automatically updates the list of templates shown if installed templates
 * are added/updated/deleted.
 *
 * @author David Faure <faure@kde.org>
 * Ideas and code for the new template handling mechanism ('link' desktop files)
 * from Christoph Pickart <pickart@iam.uni-bonn.de>
 */
class LIBKONQ_EXPORT KNewMenu : public KActionMenu
{
  Q_OBJECT
public:

    /**
     * Constructor
     */
    KNewMenu( KActionCollection * parent, QWidget* parentWidget, const QString& name );
    virtual ~KNewMenu();

    /**
     * Set the files the popup is shown for
     * Call this before showing up the menu
     */
    void setPopupFiles(const KUrl::List & _files);
    void setPopupFiles(const KUrl & _file);

public Q_SLOTS:
    /**
     * Checks if updating the list is necessary
     * IMPORTANT : Call this in the slot for aboutToShow.
     */
    void slotCheckUpToDate();

protected Q_SLOTS:
    /**
     * Called when New->Directory... is clicked
     */
    void slotNewDir();

    /**
     * Called when New->* is clicked
     */
    void slotNewFile();

    /**
     * Fills the templates list.
     */
    void slotFillTemplates();

    void slotResult( KJob * );
    // Special case (filename conflict when creating a link=url file)
    void slotRenamed( KIO::Job *, const KUrl&, const KUrl& );

private:

    /**
     * Fills the menu from the templates list.
     */
    void fillMenu();

    /**
     * Opens the desktop files and completes the Entry list
     * Input: the entry list. Output: the entry list ;-)
     */
    void parseFiles();

    /**
     * Make the main menus on the startup.
     */
    void makeMenus();

    /**
     * For entryType
     * LINKTOTEMPLATE: a desktop file that points to a file or dir to copy
     * TEMPLATE: a real file to copy as is (the KDE-1.x solution)
     * SEPARATOR: to put a separator in the menu
     * 0 means: not parsed, i.e. we don't know
     */
    enum { LINKTOTEMPLATE = 1, TEMPLATE, SEPARATOR };

    class KNewMenuPrivate;
    KNewMenuPrivate* d;
};

#endif
