/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/


#include "bookmarks_module.h"
#include <konq_events.h>
#include "libkonq_utils.h"

#include <KParts/PartActivateEvent>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KBookmark>
#include <KBookmarkManager>

#include <QAction>
#include <QIcon>
#include <QStandardItemModel>
#include <QTreeView>
#include <QMouseEvent>

KonqSidebarBookmarksModuleView::KonqSidebarBookmarksModuleView(QWidget* parent) : QTreeView(parent)
{
}

KonqSidebarBookmarksModuleView::~KonqSidebarBookmarksModuleView()
{
}

void KonqSidebarBookmarksModuleView::mousePressEvent(QMouseEvent* event)
{
    QTreeView::mousePressEvent(event);
    if (event->button() != Qt::MiddleButton) {
        m_middleMouseBtnPressed = false;
        return;
    }
    m_middleMouseBtnPressed = true;
}

void KonqSidebarBookmarksModuleView::mouseReleaseEvent(QMouseEvent* event)
{
    QTreeView::mouseReleaseEvent(event);
    if (!m_middleMouseBtnPressed) {
        return;
    }
    m_middleMouseBtnPressed = false;
    if (event->button() != Qt::MiddleButton || !rect().contains(event->pos())) {
        return;
    }
    QModelIndex idx = indexAt(event->position().toPoint());
    if (idx.isValid()) {
        emit middleClick(idx);
    }
}

KonqSideBarBookmarksModule::BookmarkGroupTraverser::BookmarkGroupTraverser(KonqSideBarBookmarksModule* mod) :
    m_module(mod)
{
}

KonqSideBarBookmarksModule::BookmarkGroupTraverser::~BookmarkGroupTraverser() noexcept
{
}

QList<QStandardItem*> KonqSideBarBookmarksModule::BookmarkGroupTraverser::createSubtree(const KBookmarkGroup &grp)
{
    traverse(grp);
    return m_items;
}

void KonqSideBarBookmarksModule::BookmarkGroupTraverser::visit(const KBookmark& bm)
{
    QStandardItem *it = m_module->itemFromBookmark(bm);
    if (m_currentGroup) {
        m_currentGroup->appendRow(it);
    } else {
        m_items << it;
    }
}

void KonqSideBarBookmarksModule::BookmarkGroupTraverser::visitEnter(const KBookmarkGroup& grp)
{
    QStandardItem *it = m_module->itemFromBookmark(grp);
    if (m_currentGroup) {
        m_currentGroup->appendRow(it);
    } else {
        m_items << it;
    }
    m_currentGroup = it;
}

void KonqSideBarBookmarksModule::BookmarkGroupTraverser::visitLeave(const KBookmarkGroup&)
{
    m_currentGroup = m_currentGroup->parent();
}

KonqSideBarBookmarksModule::KonqSideBarBookmarksModule(QWidget *parent,
                            const KConfigGroup &configGroup)
    : KonqSidebarModule(parent, configGroup)
{
    m_initURL = QUrl(configGroup.readPathEntry("URL", QString("")));
    if (m_initURL == QUrl(QStringLiteral("bookmarks:"))) {
        m_bmManager = Konq::userBookmarksManager();
    } else {
        m_bmManager = new KBookmarkManager(m_initURL.path(), this);
        m_initURL.setScheme(QStringLiteral("bookmarks"));
        m_initURL.setFragment(m_initURL.path());
        m_initURL.setPath("bookmarksfile");
    }
    treeView = new KonqSidebarBookmarksModuleView(parent);
    treeView->setHeaderHidden(true);
    model = new QStandardItemModel(this);
    fill();
    treeView->setModel(model);

    connect(treeView, &QTreeView::activated, this, [this](const QModelIndex &idx){activateItem(idx, false);});
    connect(treeView, &KonqSidebarBookmarksModuleView::middleClick, this, [this](const QModelIndex &idx){activateItem(idx, true);});
    connect(m_bmManager, &KBookmarkManager::changed, this, &KonqSideBarBookmarksModule::updateBookmarkGroup);
    treeView->expand(model->item(0)->index());
}

KonqSideBarBookmarksModule::~KonqSideBarBookmarksModule()
{
}

KonqSidebarModule::UrlType KonqSideBarBookmarksModule::urlType() const
{
    return KonqSidebarModule::UrlType::File;
}

QStandardItem * KonqSideBarBookmarksModule::itemFromBookmark(const KBookmark& bm)
{
    if (bm.isNull()) {
        return nullptr;
    }
    QStandardItem *it = new QStandardItem();
    it->setIcon(QIcon::fromTheme(bm.icon()));
    it->setText(bm.text());
    it->setToolTip(bm.description());
    it->setData(bm.address(), AddressRole);
    if (!bm.isGroup()) {
        it->setData(bm.url(), UrlRole);
    }
    it->setEditable(false);
    return it;
}

QString KonqSideBarBookmarksModule::address(QStandardItem* it) const
{
    return it ? it->data(AddressRole).toString() : QString();
}

QUrl KonqSideBarBookmarksModule::url(QStandardItem* it) const
{
    return it ? it->data(UrlRole).toUrl() : QUrl();
}

KBookmark KonqSideBarBookmarksModule::bookmark(QStandardItem* it) const
{
    return m_bmManager->findByAddress(address(it));
}

void KonqSideBarBookmarksModule::fill()
{
    model->clear();
    QStandardItem* item = new QStandardItem(QIcon::fromTheme(configGroup().readEntry("Icon", QString())), configGroup().readEntry("Name", QString()));
    item->setData(m_initURL, UrlRole);
    item->setEditable(false);
    model->appendRow(item);
    QList<QStandardItem*> rows = BookmarkGroupTraverser(this).createSubtree(m_bmManager->findByAddress(QStringLiteral("/")).toGroup());
    for (auto r : rows) {
        model->appendRow(r);
    }
}

QStandardItem * KonqSideBarBookmarksModule::itemAtAddress(const QString& address)
{
    QStringList path = address.split('/', Qt::SkipEmptyParts);
    QStandardItem *it = model->item(0);
    for (const QString &s : path) {
        int n = s.toInt();
        it = it->child(n);
        if (!it) {
            return nullptr;
        }
    }
    return it;
}

void KonqSideBarBookmarksModule::updateBookmarkGroup(const QString& address)
{
    QStandardItem *it = address.isEmpty() ? nullptr : itemAtAddress(address);
    if (!it) {
        fill();
        return;
    }
    QStandardItem *parent = it->parent();
    int pos = it->row();
    bool expanded = treeView->isExpanded(it->index());
    parent->removeRow(pos);
    KBookmarkGroup grp = m_bmManager->findByAddress(address).toGroup();
    it = itemFromBookmark(grp);
    if (!it) {
        return;
    }
    parent->insertRow(pos, it);
    QList<QStandardItem*> rows = BookmarkGroupTraverser(this).createSubtree(grp);

    if (rows.isEmpty()) {
        return;
    }
    it->appendRows(rows);
    treeView->setExpanded(it->index(), expanded);
}

QWidget *KonqSideBarBookmarksModule::getWidget()
{
    return treeView;
}

void KonqSideBarBookmarksModule::activateItem(const QModelIndex& idx, bool newTab)
{
    QUrl itemUrl = url(model->itemFromIndex(idx));
    if (itemUrl == m_lastURL) {
        return;
    }
    BrowserArguments bargs;
    bargs.setNewTab(newTab);
    bargs.setForcesNewWindow(newTab);
    //TODO currently, bargs are ignored because this class doesn't have a BrowserExtension
    // (see KonqView::connectPart in konqview.cpp)/. See how to work around this problem
    emit openUrlRequest(itemUrl, {}, bargs);
    m_lastURL = itemUrl;
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

