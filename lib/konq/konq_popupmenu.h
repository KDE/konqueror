/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konqpopupmenu_h
#define __konqpopupmenu_h

#include <sys/types.h>

#include <QtCore/QMap>
#include <QtGui/QMenu>

#include <kaction.h>
#include <kactioncollection.h>
#include <kfileitem.h>
#include <kparts/browserextension.h>
#include <kservice.h>

#include <libkonq_export.h>

class KNewMenu;
//class KonqPopupMenuPlugin;
class KBookmarkManager;
class KonqPopupMenuPrivate;

/**
 * This class implements the popup menu for URLs in konqueror and kdesktop
 * It's usage is very simple : on right click, create the KonqPopupMenu instance
 * with the correct arguments, then exec() to make it appear, then destroy it.
 *
 * Users of KonqPopupMenu include: konqueror, the media applet, the trash applet
 * (and the desktop icons, in kde3)
 */
class LIBKONQ_EXPORT KonqPopupMenu : public QMenu
{
  Q_OBJECT
public:

  /**
   * Flags set by the calling application (konqueror/kdesktop), unlike
   * KParts::BrowserExtension::PopupFlags, which are set by the calling part
   */
  typedef uint Flags;
  enum { NoFlags = 0,
         ShowNewWindow = 1 };
         // WARNING: bitfield. Next item is 2

  /**
   * Constructor
   * @param manager the bookmark manager for the "add to bookmark" action
   * Only used if KParts::BrowserExtension::ShowBookmark is set
   * @param items the list of file items the popupmenu should be shown for
   * @param viewURL the URL shown in the view, to test for RMB click on view background
   * @param actions list of actions the caller wants to see in the menu
   * @param newMenu "New" menu, shared with the File menu, in konqueror
   * @param parentWidget the widget we're showing this popup for. Helps destroying
   * the popup if the widget is destroyed before the popup.
   * @param appFlags flags from the KonqPopupMenu::Flags enum, set by the calling application
   * @param partFlags flags from the BrowserExtension enum, set by the calling part
   *
   * The actions to pass in include :
   * showmenubar, go_back, go_forward, go_up, cut, copy, paste, pasteto
   * The others items are automatically inserted.
   *
   * @todo that list is probably not be up-to-date
   */
  KonqPopupMenu( const KFileItemList &items,
                 const KUrl& viewURL,
                 KActionCollection & actions,
                 KNewMenu * newMenu,
                 Flags appFlags,
                 KParts::BrowserExtension::PopupFlags partFlags /*= KParts::BrowserExtension::DefaultPopupItems*/,
                 QWidget * parentWidget,
                 KBookmarkManager *manager = 0,
                 const KParts::BrowserExtension::ActionGroupMap& actionGroups = KParts::BrowserExtension::ActionGroupMap()
      );

  /**
   * Don't forget to destroy the object
   */
  ~KonqPopupMenu();

  /**
   * Set the title of the URL, when the popupmenu is opened for a single URL.
   * This is used if the user chooses to add a bookmark for this URL.
   */
  void setURLTitle( const QString& urlTitle );

#if 0 // was only used by plugins
  class LIBKONQ_EXPORT ProtocolInfo {
   public:
    ProtocolInfo();
    bool supportsReading()  const;
    bool supportsWriting()  const;
    bool supportsDeleting() const;
    bool supportsMoving()   const;
    bool trashIncluded()    const;
   private:
    friend class KonqPopupMenuPrivate;
    bool m_Reading:1;
    bool m_Writing:1;
    bool m_Deleting:1;
    bool m_Moving:1;
    bool m_TrashIncluded:1;
  };
#endif

  KActionCollection *actionCollection() const;
  QString mimeType( ) const;
  KUrl url( ) const;
  KFileItemList fileItemList() const;
  KUrl::List popupURLList( ) const;
    //ProtocolInfo protocolInfo() const;

private:
  Q_PRIVATE_SLOT(d, void slotPopupNewDir())
  Q_PRIVATE_SLOT(d, void slotPopupNewView())
  Q_PRIVATE_SLOT(d, void slotPopupEmptyTrashBin())
  Q_PRIVATE_SLOT(d, void slotPopupRestoreTrashedItems())
  Q_PRIVATE_SLOT(d, void slotPopupAddToBookmark())
  Q_PRIVATE_SLOT(d, void slotPopupMimeType())
  Q_PRIVATE_SLOT(d, void slotPopupProperties())
  Q_PRIVATE_SLOT(d, void slotOpenShareFileDialog())

private:
  void init (QWidget * parentWidget, Flags kpf, KParts::BrowserExtension::PopupFlags itemFlags);

  KonqPopupMenuPrivate *d;
};

#endif
