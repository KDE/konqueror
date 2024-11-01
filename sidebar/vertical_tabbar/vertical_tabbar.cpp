/*
    SPDX-FileCopyrightText: 2019 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/


#include "vertical_tabbar.h"
#include "verticaltabbarmodel.h"
#include "interfaces/window.h"
#include "interfaces/browser.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QListView>
#include <QApplication>

KonqVerticalTabBar::KonqVerticalTabBar(QWidget *parent,
                            const KConfigGroup &configGroup) : KonqSidebarModule(parent, configGroup),
    m_view(new QListView(parent)),
    m_model(new VerticalTabBarModel(this))
{
    m_window = KonqInterfaces::Browser::browser(qApp)->window(parent);
    m_contextMenu = m_window->tabBarContextMenu(m_view);
    m_model->setWindow(m_window);
    m_view->setModel(m_model);
    m_view->setDragEnabled(true);
    m_view->setAcceptDrops(true);
    m_view->setDragDropMode(QAbstractItemView::InternalMove);
    m_view->setDropIndicatorShown(true);
    connect(m_view, &QAbstractItemView::activated, this, &KonqVerticalTabBar::activateItem);
    connect(m_window, &KonqInterfaces::Window::currentTabChanged, this, &KonqVerticalTabBar::selectTab);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_view, &QWidget::customContextMenuRequested, this, &KonqVerticalTabBar::displayContextMenu);

    selectTab(m_window->activeTab());
}

KonqVerticalTabBar::~KonqVerticalTabBar()
{
}

QWidget* KonqVerticalTabBar::getWidget()
{
    return m_view;
}

void KonqVerticalTabBar::activateItem(const QModelIndex& idx)
{
    m_window->activateTab(m_model->tabForIndex(idx));
}

void KonqVerticalTabBar::selectTab(int tabIdx)
{
    m_view->setCurrentIndex(m_model->index(tabIdx, 0));
}

void KonqVerticalTabBar::displayContextMenu(const QPoint& pt)
{
    QModelIndex idx = m_view->indexAt(pt);
    int tab = m_model->tabForIndex(idx);
    m_contextMenu->execWithWorkingTab(m_view->mapToGlobal(pt), tab);
}

class KonqVerticalTabBarPlugin : public KonqSidebarPlugin
{
public:
    KonqVerticalTabBarPlugin(QObject *parent, const QVariantList &args)
        : KonqSidebarPlugin(parent, args) {}
    ~KonqVerticalTabBarPlugin() override {}

    KonqSidebarModule *createModule(QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused) override
    {
        Q_UNUSED(desktopname);
        Q_UNUSED(unused);

        return new KonqVerticalTabBar(parent, configGroup);
    }

    QList<QAction *> addNewActions(QObject *parent,
                                           const QList<KConfigGroup> &existingModules,
                                           const QVariant &unused) override
    {
        Q_UNUSED(existingModules);
        Q_UNUSED(unused);
        QAction *action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Vertical Tab Bar"));
        action->setIcon(QIcon::fromTheme("document-multiple"));
        return {action};
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
        configGroup.writeEntry("Icon", "document-multiple");
        configGroup.writeEntry("Name", i18nc("@title:tab", "Tabs"));
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_verticaltabbar");
        return true;
    }
};

K_PLUGIN_FACTORY_WITH_JSON(KonqVerticalTabBarPluginFactory, "konqsidebar_verticaltabbar.json", registerPlugin<KonqVerticalTabBarPlugin>();)

#include "vertical_tabbar.moc"

