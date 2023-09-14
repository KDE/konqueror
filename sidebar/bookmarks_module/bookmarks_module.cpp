/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/


#include "bookmarks_module.h"
#include <konq_events.h>

#include <KParts/PartActivateEvent>
#include <KLocalizedString>
#include <KPluginFactory>
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
    ~KonqSidebarBookmarksPlugin() override {}

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

K_PLUGIN_FACTORY_WITH_JSON(KonqSidebarBookmarksPluginFactory, "konqsidebar_bookmarks.json", registerPlugin<KonqSidebarBookmarksPlugin>();)

#include "bookmarks_module.moc"

