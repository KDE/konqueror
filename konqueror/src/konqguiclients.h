/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_guiclients_h__
#define __konq_guiclients_h__

#include <kactioncollection.h>
#include <konq_popupmenu.h>
#include <QtCore/QObject>
#include <QtCore/QHash>

#include <kxmlguiclient.h>
#include <kservice.h>

class QAction;
class KonqMainWindow;
class KonqView;

/**
 * PopupMenuGUIClient has most of the konqueror logic for KonqPopupMenu.
 * It holds an actionCollection and takes care of the preview and tabhandling groups for KonqPopupMenu.
 */
class PopupMenuGUIClient : public QObject
{
    Q_OBJECT
public:
    // The action groups are inserted into @p actionGroups
    PopupMenuGUIClient( const KService::List &embeddingServices,
                        KParts::BrowserExtension::ActionGroupMap& actionGroups,
                        QAction* showMenuBar, QAction* stopFullScreen );
    virtual ~PopupMenuGUIClient();

    KActionCollection* actionCollection() { return &m_actionCollection; }

signals:
    void openEmbedded(KService::Ptr service);

private slots:
    void slotOpenEmbedded();

private:
    QAction* addEmbeddingService( int idx, const QString &name, const KService::Ptr &service );

    KActionCollection m_actionCollection;
    KService::List m_embeddingServices;
};

class ToggleViewGUIClient : public QObject
{
  Q_OBJECT
public:
  explicit ToggleViewGUIClient( KonqMainWindow *mainWindow );
  virtual ~ToggleViewGUIClient();

  bool empty() const { return m_empty; }

  QList<QAction*> actions() const;
  QAction *action( const QString &name ) { return m_actions[ name ]; }

  void saveConfig( bool add, const QString &serviceName );

private Q_SLOTS:
  void slotToggleView( bool toggle );
  void slotViewAdded( KonqView *view );
  void slotViewRemoved( KonqView *view );
private:
  KonqMainWindow *m_mainWindow;
  QHash<QString,QAction*> m_actions;
  bool m_empty;
  QMap<QString,bool> m_mapOrientation;
};

#endif
