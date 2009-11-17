/*
    Copyright (c) 2009 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "history_module.h"
#include <kdebug.h>
#include <konqhistoryview.h>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QTreeView>

#include <kicon.h>
#include <klocale.h>
#include <kpluginfactory.h>

KonqSidebarHistoryModule::KonqSidebarHistoryModule(const KComponentData &componentData, QWidget *parent,
                                                   const KConfigGroup& configGroup)
    : KonqSidebarModule(componentData, parent, configGroup)
{
    m_historyView = new KonqHistoryView(parent);
    connect(m_historyView->treeView(), SIGNAL(activated(QModelIndex)), this, SLOT(slotActivated(QModelIndex)));
    connect(m_historyView->treeView(), SIGNAL(pressed(QModelIndex)), this, SLOT(slotPressed(QModelIndex)));
    connect(m_historyView->treeView(), SIGNAL(clicked(QModelIndex)), this, SLOT(slotClicked(QModelIndex)));
    connect(m_historyView, SIGNAL(openUrlInNewWindow(KUrl)), this, SLOT(slotOpenWindow(KUrl)));
    connect(m_historyView, SIGNAL(openUrlInNewTab(KUrl)), this, SLOT(slotOpenTab(KUrl)));
}

KonqSidebarHistoryModule::~KonqSidebarHistoryModule()
{
}

QWidget * KonqSidebarHistoryModule::getWidget()
{
    return m_historyView;
}

// LMB activation (single or double click) handling
void KonqSidebarHistoryModule::slotActivated(const QModelIndex& index)
{
    if (m_lastPressedButtons == Qt::MidButton) // already handled by slotClicked
        return;
    const KUrl url = m_historyView->urlForIndex(index);
    if (url.isValid()) {
        emit openUrlRequest(url);
    }
}

// Needed for MMB handling; no convenient API in QAbstractItemView
void KonqSidebarHistoryModule::slotPressed(const QModelIndex& index)
{
    Q_UNUSED(index);
    m_lastPressedButtons = qApp->mouseButtons();
}

// MMB handling
void KonqSidebarHistoryModule::slotClicked(const QModelIndex& index)
{
    if (m_lastPressedButtons & Qt::MidButton) {
        const KUrl url = m_historyView->urlForIndex(index);
        if (url.isValid()) {
            createNewWindow(url);
        }
    }
}

void KonqSidebarHistoryModule::slotOpenWindow(const KUrl& url)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    KParts::BrowserArguments browserArgs;
    browserArgs.setForcesNewWindow(true);
    createNewWindow(url, args, browserArgs);
}

void KonqSidebarHistoryModule::slotOpenTab(const KUrl& url)
{
    KParts::OpenUrlArguments args;
    args.setActionRequestedByUser(true);
    KParts::BrowserArguments browserArgs;
    browserArgs.setNewTab(true);
    createNewWindow(url, args, browserArgs);
}

class KonqSidebarHistoryPlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarHistoryPlugin(QObject* parent, const QVariantList& args)
        : KonqSidebarPlugin(parent, args) {}
    virtual ~KonqSidebarHistoryPlugin() {}

    virtual KonqSidebarModule* createModule(const KComponentData &componentData, QWidget *parent,
                                            const KConfigGroup& configGroup,
                                            const QString &desktopname,
                                            const QVariant& unused)
    {
        Q_UNUSED(unused);
        Q_UNUSED(desktopname);
        return new KonqSidebarHistoryModule(componentData, parent, configGroup);
    }

    virtual QList<QAction*> addNewActions(QObject* parent,
                                          const QList<KConfigGroup>& existingModules,
                                          const QVariant& unused)
    {
        Q_UNUSED(unused);
        Q_UNUSED(existingModules);
        QAction* action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "History Sidebar Module"));
        action->setIcon(KIcon("view-history"));
        return QList<QAction *>() << action;
    }

    virtual QString templateNameForNewModule(const QVariant& actionData,
                                             const QVariant& unused) const
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);
        return QString::fromLatin1("historyplugin%1.desktop");
    }

    virtual bool createNewModule(const QVariant& actionData, KConfigGroup& configGroup,
                                 QWidget* parentWidget,
                                 const QVariant& unused)
    {
        Q_UNUSED(parentWidget);
        Q_UNUSED(actionData);
        Q_UNUSED(unused);

        configGroup.writeEntry("Type", "Link");
        configGroup.writeEntry("Icon", "view-history");
        configGroup.writeEntry("Name", i18nc("@title:tab", "History"));
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_history");
        return true;
    }
};

K_PLUGIN_FACTORY(KonqSidebarHistoryPluginFactory, registerPlugin<KonqSidebarHistoryPlugin>(); )
K_EXPORT_PLUGIN(KonqSidebarHistoryPluginFactory())

#include "history_module.moc"
