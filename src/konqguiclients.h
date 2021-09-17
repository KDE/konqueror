/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_guiclients_h__
#define __konq_guiclients_h__

#include <kactioncollection.h>
#include <konq_popupmenu.h>
#include <QObject>
#include <QHash>

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
    PopupMenuGUIClient(const KService::List &embeddingServices,
                       KonqPopupMenu::ActionGroupMap &actionGroups,
                       QAction *showMenuBar, QAction *stopFullScreen);
    ~PopupMenuGUIClient() override;

    KActionCollection *actionCollection()
    {
        return &m_actionCollection;
    }

signals:
    void openEmbedded(KService::Ptr service);

private slots:
    void slotOpenEmbedded();

private:
    QAction *addEmbeddingService(int idx, const QString &name, const KService::Ptr &service);

    KActionCollection m_actionCollection;
    KService::List m_embeddingServices;
};

class ToggleViewGUIClient : public QObject
{
    Q_OBJECT
public:
    explicit ToggleViewGUIClient(KonqMainWindow *mainWindow);
    ~ToggleViewGUIClient() override;

    bool empty() const
    {
        return m_empty;
    }

    QList<QAction *> actions() const;
    QAction *action(const QString &name)
    {
        return m_actions[ name ];
    }

    void saveConfig(bool add, const QString &serviceName);

private Q_SLOTS:
    void slotToggleView(bool toggle);
    void slotViewAdded(KonqView *view);
    void slotViewRemoved(KonqView *view);
private:
    KonqMainWindow *m_mainWindow;
    QHash<QString, QAction *> m_actions;
    bool m_empty;
    QMap<QString, bool> m_mapOrientation;
};

#endif
