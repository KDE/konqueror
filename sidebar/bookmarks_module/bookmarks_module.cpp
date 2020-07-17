/*
    Copyright (C) 2019 Raphael Rosch <kde-dev@insaner.com>

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


#include "bookmarks_module.h"
#include <konq_events.h>

#include <KParts/PartActivateEvent>
#include <KLocalizedString>
#include <kpluginfactory.h>
#include <QAction>
#include <QIcon>
#include <QStandardItemModel>
#include <QTreeView>


KonqSideBarBookmarksModule::KonqSideBarBookmarksModule(QWidget *parent,
                            const KConfigGroup &configGroup)
    : KonqSidebarModule(parent, configGroup)
{
    treeView = new QTreeView(parent);
    treeView->setHeaderHidden(true);
    model = new QStandardItemModel(this);

    QStandardItem* item = new QStandardItem(QIcon::fromTheme(configGroup.readEntry("Icon", QString())), configGroup.readEntry("Name", QString()));
    m_initURL = QUrl(configGroup.readPathEntry("URL", QString()));
    item->setData(m_initURL);
    item->setEditable(false);

    model->appendRow(item);
    treeView->setModel(model);

    QItemSelectionModel *selectionModel = treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &KonqSideBarBookmarksModule::slotSelectionChanged);
}

KonqSideBarBookmarksModule::~KonqSideBarBookmarksModule()
{
}

QWidget *KonqSideBarBookmarksModule::getWidget()
{
    return treeView;
}

void KonqSideBarBookmarksModule::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) 
{
    QModelIndex index = treeView->selectionModel()->currentIndex();
    QUrl urlFromIndex = model->itemFromIndex(index)->data().toUrl();

    if (urlFromIndex != m_lastURL && urlFromIndex == m_initURL) {
        emit openUrlRequest(m_initURL);
    }
    m_lastURL = urlFromIndex;
}

void KonqSideBarBookmarksModule::customEvent(QEvent *ev) // active view has changed
{
    if (KParts::PartActivateEvent::test(ev)) {
        KParts::ReadOnlyPart* rpart = static_cast<KParts::ReadOnlyPart *>( static_cast<KParts::PartActivateEvent *>(ev)->part() ); 
        if (!rpart->url().isEmpty()) {
            handleURL(rpart->url());
        }
    }
}

void KonqSideBarBookmarksModule::handleURL(const QUrl &thisURL)
{
    if (thisURL != m_lastURL) {
        if (thisURL == m_initURL) {
            treeView->setCurrentIndex(model->index(0, 0));
        } else {
            treeView->selectionModel()->clearSelection();
        }
        m_lastURL = thisURL;
    }
}



class KonqSidebarBookmarksPlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarBookmarksPlugin(QObject *parent, const QVariantList &args)
        : KonqSidebarPlugin(parent, args) {}
    virtual ~KonqSidebarBookmarksPlugin() {}

    KonqSidebarModule *createModule(QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused) override
    {
        Q_UNUSED(desktopname);
        Q_UNUSED(unused);

        return new KonqSideBarBookmarksModule(parent, configGroup);
    }

    QList<QAction *> addNewActions(QObject *parent,
                                           const QList<KConfigGroup> &existingModules,
                                           const QVariant &unused) override
    {
        Q_UNUSED(existingModules);
        Q_UNUSED(unused);
        QAction *action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Bookmarks Sidebar Module"));
        action->setIcon(QIcon::fromTheme("bookmarks"));
        return QList<QAction *>() << action;
    }

    QString templateNameForNewModule(const QVariant &actionData,
            const QVariant &unused) const override
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);
        return QString::fromLatin1("treesidebarplugin%1.desktop");
    }

    bool createNewModule(const QVariant &actionData,
                                 KConfigGroup &configGroup,
                                 QWidget *parentWidget,
                                 const QVariant &unused) override
    {
        Q_UNUSED(actionData);
        Q_UNUSED(parentWidget);
        Q_UNUSED(unused);
        configGroup.writeEntry("Type", "Link");
        configGroup.writeEntry("Icon", "bookmarks");
        configGroup.writeEntry("Name", i18nc("@title:tab", "Bookmarks"));
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_bookmarks");
        return true;
    }
};

K_PLUGIN_FACTORY(KonqSidebarBookmarksPluginFactory, registerPlugin<KonqSidebarBookmarksPlugin>();)
// K_EXPORT_PLUGIN(KonqSidebarBookmarksPluginFactory())

#include "bookmarks_module.moc"

