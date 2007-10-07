/* This file is part of the KDE project
   Copyright (C) 1998-2007 David Faure <faure@kde.org>

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

#ifndef KONQ_MENUACTIONS_P_H
#define KONQ_MENUACTIONS_P_H

#include <kfileitem.h>
#include <kactioncollection.h>
#include <QActionGroup>
#include <QObject>
#include <kdesktopfileactions.h>

typedef QList<KDesktopFileActions::Service> ServiceList;

class KonqMenuActionsPrivate : public QObject
{
    Q_OBJECT
public:
    KonqMenuActionsPrivate();

    int insertServicesSubmenus(const QMap<QString, ServiceList>& list, QMenu* menu, bool isBuiltin);
    int insertServices(const ServiceList& list, QMenu* menu, bool isBuiltin);

private Q_SLOTS:
    void slotExecuteService(QAction* act);

public:
    KFileItemList m_items;
    KUrl m_url;
    KUrl::List m_urlList;
    QString m_mimeType;
    QString m_mimeGroup;
    bool m_isDirectory;
    bool m_readOnly;

    // TODO try action->setData(QVariant::fromValue(service))
    QMap<QAction *, KDesktopFileActions::Service> m_mapPopupServices;
    QActionGroup m_executeServiceActionGroup;
    KActionCollection m_ownActions; // TODO connect to statusbar for help on actions
};

#endif /* KONQ_MENUACTIONS_P_H */

