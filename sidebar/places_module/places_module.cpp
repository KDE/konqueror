/*
    SPDX-FileCopyrightText: 2010 Pino Toscano <pino@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "places_module.h"

#include <QIcon>
#include <QAction>
#include <QGuiApplication>

#include <kfileplacesmodel.h>
#include <KLocalizedString>
#include <kpluginfactory.h>


KonqSideBarPlacesModule::KonqSideBarPlacesModule(QWidget *parent,
        const KConfigGroup &configGroup)
    : KonqSidebarModule(parent, configGroup)
{
    m_placesView = new KFilePlacesView(parent);
    m_placesView->setModel(new KFilePlacesModel(m_placesView));
    m_placesView->setShowAll(true);
    m_placesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_placesView->setAutoResizeItemsEnabled(false);
    // m_placesView->setResizeMode(QListView::Fixed);
    int iconSize = m_placesView->style()->pixelMetric(QStyle::PM_SmallIconSize); // this would best be done by detecting the size of icons for other widgets
    m_placesView->setIconSize(QSize(iconSize, iconSize));
        
    connect(m_placesView, &KFilePlacesView::urlChanged, this, &KonqSideBarPlacesModule::slotPlaceUrlChanged);
}

QWidget *KonqSideBarPlacesModule::getWidget()
{
    return m_placesView;
}

void KonqSideBarPlacesModule::slotPlaceUrlChanged(const QUrl &url)
{
    const Qt::MouseButtons buttons = QGuiApplication::mouseButtons();
    const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();

    if ((buttons & Qt::MiddleButton) != 0 || (modifiers & Qt::ControlModifier) != 0) {
        emit createNewWindow(url);
    } else {
        emit openUrlRequest(url);
    }
}

class KonqSidebarPlacesPlugin : public KonqSidebarPlugin
{
public:
    KonqSidebarPlacesPlugin(QObject *parent, const QVariantList &args)
        : KonqSidebarPlugin(parent, args) {}
    ~KonqSidebarPlacesPlugin() override {}

    KonqSidebarModule *createModule(QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused) override
    {
        Q_UNUSED(desktopname);
        Q_UNUSED(unused);
        return new KonqSideBarPlacesModule(parent, configGroup);
    }

    QList<QAction *> addNewActions(QObject *parent,
                                           const QList<KConfigGroup> &existingModules,
                                           const QVariant &unused) override
    {
        Q_UNUSED(existingModules);
        Q_UNUSED(unused);
        QAction *action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Places Sidebar Module"));
        action->setIcon(QIcon::fromTheme("folder-favorites"));
        return QList<QAction *>() << action;
    }

    QString templateNameForNewModule(const QVariant &actionData,
            const QVariant &unused) const override
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);
        return QString::fromLatin1("placessidebarplugin%1.desktop");
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
        configGroup.writeEntry("Name", i18nc("@title:tab", "Places"));
        configGroup.writeEntry("X-KDE-KonqSidebarModule", "konqsidebar_places");
        return true;
    }
};

K_PLUGIN_FACTORY_WITH_JSON(KonqSidebarPlacesPluginFactory, "konqsidebar_places.json", registerPlugin<KonqSidebarPlacesPlugin>();)

#include "places_module.moc"
