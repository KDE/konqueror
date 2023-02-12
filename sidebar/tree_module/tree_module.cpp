/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

/*

TODO:
-sidepanel not triggering changes in session properly
-"Configure sidebar" > "Add new" has no option to actually add anything there
-places panel does not respond to view location changes 
-detect icon size for places panel
-doubleclick on image (to open kuickview) causes sidebar to deselect

-"View mode" to "sidebar" causes crash and ruins session -- cannot undo


BUGS:
-(konq bug) sftp cannot save file being edited, because: "A file named sftp://hostname/path/to/file already exists."
	maybe: https://phabricator.kde.org/D23384 , or https://phabricator.kde.org/D15318
-(konq bug) loading session from cmdline causes crash, but not when konq is loaded fresh

*/



#include "tree_module.h"
#include <konq_events.h>

#include <KLocalizedString>
#include <kpluginfactory.h>

#include <QAction>
#include <QKeyEvent>

#include <QHeaderView>


KonqSideBarTreeModule::KonqSideBarTreeModule(QWidget *parent,
        const KConfigGroup &configGroup)
    : KonqSidebarModule(parent, configGroup)
{
    m_initURL = cleanupURL(QUrl(configGroup.readPathEntry("URL", QString()))); // because the .desktop file url might be "~"
    treeView = new QTreeView(parent);
    treeView->setHeaderHidden(true);
    treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    treeView->setTextElideMode(Qt::ElideMiddle);
    treeView->setDragEnabled(true);

    model = new KDirModel(this);
    sorted_model = new KDirSortFilterProxyModel(this);
    sorted_model->setSortFoldersFirst(true);
    sorted_model->setSourceModel(model); 
    model->dirLister()->setDirOnlyMode(true);
    model->dirLister()->setShowHiddenFiles(configGroup.readEntry("ShowHiddenFolders", false));

    model->openUrl(m_initURL, KDirModel::ShowRoot);

    treeView->setModel(sorted_model);

    for (int i = 1; i <= 6; i++) {
        treeView->setColumnHidden(i, true);
    }

    connect(treeView, &QTreeView::expanded,
            this, &KonqSideBarTreeModule::slotUpdateColWidth);
    connect(treeView, &QTreeView::collapsed,
            this, &KonqSideBarTreeModule::slotUpdateColWidth);

    model->expandToUrl(m_initURL); // KDirModel is async, we'll just have to wait for slotKDirCompleted()
    connect(model, &KDirModel::expand,
            this, &KonqSideBarTreeModule::slotKDirExpand_setRootIndex);
        
    QItemSelectionModel *selectionModel = treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::selectionChanged,
            this, &KonqSideBarTreeModule::slotSelectionChanged);
}

void KonqSideBarTreeModule::customEvent(QEvent *ev) // active view has changed
{
    if (KParts::PartActivateEvent::test(ev)) {
        KParts::ReadOnlyPart* rpart = static_cast<KParts::ReadOnlyPart *>( static_cast<KParts::PartActivateEvent *>(ev)->part() );
        if (!rpart->url().isEmpty()) {
            setSelection(rpart->url());
        }
    }
}

QUrl KonqSideBarTreeModule::cleanupURL(const QUrl &dirtyURL)
{
    if (!dirtyURL.isValid()) {
        return dirtyURL;
    }
    QUrl url = dirtyURL;
    if (url.isRelative()) {
        url.setScheme("file");
        if (url.path() == "~") {
            const QString homePath = QDir::homePath();
            if (!homePath.endsWith("/")) {
                url.setPath(homePath + "/");
            } else {
                url.setPath(homePath);
            }
        }
    }
    return url;
}

KonqSideBarTreeModule::~KonqSideBarTreeModule()
{
}

QWidget *KonqSideBarTreeModule::getWidget()
{
    return treeView;
}

void KonqSideBarTreeModule::handleURL(const QUrl &url)
{
    QUrl handleURL = url;
    
    if (handleURL.scheme().isNull()) {
        setSelectionIndex(QModelIndex());
        m_lastURL = QUrl();
        return;
    }

    m_lastURL = handleURL;
    setSelection(handleURL);
}

void KonqSideBarTreeModule::setSelection(const QUrl &target_url, bool do_openURLreq) // do_openURLreq=true)
{
    QModelIndex index = sorted_model->mapFromSource(model->indexForUrl(target_url));

    m_lastURL = target_url;

    if (!index.isValid() && target_url.scheme() == m_initURL.scheme()) {
        if (do_openURLreq) {
            connect(model, &KDirModel::expand,
                this,  &KonqSideBarTreeModule::slotKDirExpand_setSelection   );
            model->expandToUrl(target_url); // KDirModel is async, we'll just have to wait for slotKDirExpand_setSelection()          
        }
    }

    setSelectionIndex(index);
}

void KonqSideBarTreeModule::setSelectionIndex(const QModelIndex &index)
{
    if (index == treeView->selectionModel()->currentIndex()) {
        return;
    }
    treeView->expand(index);
    treeView->scrollTo(index);
    treeView->setCurrentIndex(index);
}

void KonqSideBarTreeModule::slotSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) 
{
    QModelIndex index = treeView->selectionModel()->currentIndex();

    QUrl urlFromIndex = getUrlFromIndex(index);
    if (index.isValid() && m_lastURL != urlFromIndex) {
        emit openUrlRequest(urlFromIndex);
    }
    slotUpdateColWidth();
}

// needed because when there is only one column, QTreeView does not trigger resize
void KonqSideBarTreeModule::slotUpdateColWidth()
{
    treeView->resizeColumnToContents(0);
}

// needed because KDirModel is async
void KonqSideBarTreeModule::slotKDirExpand_setRootIndex()
{
    QModelIndex index = getIndexFromUrl(m_initURL);
    if (index.isValid()) {
        disconnect(model, &KDirModel::expand,
            this, &KonqSideBarTreeModule::slotKDirExpand_setRootIndex);
        treeView->setRootIndex(index.parent());
        treeView->expand(index);
    }
}

void KonqSideBarTreeModule::slotKDirExpand_setSelection(const QModelIndex &index)
{
    QUrl urlFromIndex = getUrlFromIndex(index); // these are only going to be valid
    if (urlFromIndex == m_lastURL) {
        disconnect(model, &KDirModel::expand,
            this, &KonqSideBarTreeModule::slotKDirExpand_setSelection);
        setSelection(m_lastURL, false);
    }
    slotUpdateColWidth();
}


// resolves index to the correct model (due to use of KDirSortFilterProxyModel)
QModelIndex KonqSideBarTreeModule::resolveIndex(const QModelIndex &index)
{
    if (index.isValid() && index.model() != model && model != nullptr) {
        return static_cast<const KDirSortFilterProxyModel*>(index.model())->mapToSource(index);
    } else {
        return index;
    }
}

QUrl KonqSideBarTreeModule::getUrlFromIndex(const QModelIndex &index)
{
    QUrl resolvedUrl;

    if (index.isValid()) {
        KFileItem itemForIndex = model->itemForIndex(resolveIndex(index));
        if (!itemForIndex.isNull()) {
            resolvedUrl = itemForIndex.url();
        }
    }

    return resolvedUrl;
}

QModelIndex KonqSideBarTreeModule::getIndexFromUrl(const QUrl &url) const
{
    return sorted_model->mapFromSource(model->indexForUrl(url));
}


class KonqSidebarTreePlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarTreePlugin(QObject *parent, const QVariantList &args)
        : KonqSidebarPlugin(parent, args) {}
    ~KonqSidebarTreePlugin() override {}

    KonqSidebarModule *createModule(QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused) override
    {
        Q_UNUSED(desktopname);
        Q_UNUSED(unused);

        return new KonqSideBarTreeModule(parent, configGroup);
    }

    QList<QAction *> addNewActions(QObject *parent,
                                           const QList<KConfigGroup> &existingModules,
                                           const QVariant &unused) override
    {
        Q_UNUSED(existingModules);
        Q_UNUSED(unused);
        QAction *action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Tree Sidebar Module"));
        action->setIcon(QIcon::fromTheme("folder-favorites"));
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
        configGroup.writeEntry("Icon", "folder-favorites");
        configGroup.writeEntry("Name", i18nc("@title:tab", "Tree"));
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_tree");
        return true;
    }
};

K_PLUGIN_FACTORY(KonqSidebarTreePluginFactory, registerPlugin<KonqSidebarTreePlugin>();)
// K_EXPORT_PLUGIN(KonqSidebarTreePluginFactory())

#include "tree_module.moc"
