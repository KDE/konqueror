/*
    Copyright (C) 2010 Pino Toscano <pino@kde.org>

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

#include "places_module.h"

#include <kfileplacesmodel.h>
#include <kicon.h>
#include <klocale.h>
#include <kpluginfactory.h>

#include <QAction>

KonqPlacesCustomPlacesView::KonqPlacesCustomPlacesView(QWidget *parent)
    : KFilePlacesView(parent)
    , m_mouseButtons(Qt::NoButton)
    , m_keyModifiers(Qt::NoModifier)
{
    connect(this, SIGNAL(urlChanged(KUrl)),
            this, SLOT(emitUrlChanged(KUrl)));
}

KonqPlacesCustomPlacesView::~KonqPlacesCustomPlacesView()
{
}

void KonqPlacesCustomPlacesView::keyPressEvent(QKeyEvent *event)
{
    m_keyModifiers = event->modifiers();
    KFilePlacesView::keyPressEvent(event);
}

void KonqPlacesCustomPlacesView::mousePressEvent(QMouseEvent *event)
{
    m_mouseButtons = event->buttons();
    KFilePlacesView::mousePressEvent(event);
}

void KonqPlacesCustomPlacesView::emitUrlChanged(const KUrl &url)
{
    emit urlChanged(url, m_mouseButtons, m_keyModifiers);
}


KonqSideBarPlacesModule::KonqSideBarPlacesModule(const KComponentData &componentData,
                                                 QWidget *parent,
                                                 const KConfigGroup &configGroup)
    : KonqSidebarModule(componentData, parent, configGroup)
{
    m_placesView = new KonqPlacesCustomPlacesView(parent);
    m_placesView->setModel(new KFilePlacesModel(m_placesView));
    m_placesView->setShowAll(true);
    m_placesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_placesView, SIGNAL(urlChanged(KUrl, Qt::MouseButtons, Qt::KeyboardModifiers)),
            this, SLOT(slotPlaceUrlChanged(KUrl, Qt::MouseButtons, Qt::KeyboardModifiers)));
}

KonqSideBarPlacesModule::~KonqSideBarPlacesModule()
{
}

QWidget *KonqSideBarPlacesModule::getWidget()
{
    return m_placesView;
}

void KonqSideBarPlacesModule::slotPlaceUrlChanged(const KUrl &url, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
{
    if ((buttons & Qt::MidButton) != 0 || (modifiers & Qt::ControlModifier) != 0) {
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
    virtual ~KonqSidebarPlacesPlugin() {}

    virtual KonqSidebarModule* createModule(const KComponentData &componentData,
                                            QWidget *parent,
                                            const KConfigGroup &configGroup,
                                            const QString &desktopname,
                                            const QVariant &unused)
    {
        Q_UNUSED(desktopname);
        Q_UNUSED(unused);
        return new KonqSideBarPlacesModule(componentData, parent, configGroup);
    }

    virtual QList<QAction*> addNewActions(QObject *parent,
                                          const QList<KConfigGroup> &existingModules,
                                          const QVariant& unused)
    {
        Q_UNUSED(existingModules);
        Q_UNUSED(unused);
        QAction* action = new QAction(parent);
        action->setText(i18nc("@action:inmenu Add", "Places Sidebar Module"));
        action->setIcon(KIcon("folder-favorites"));
        return QList<QAction *>() << action;
    }

    virtual QString templateNameForNewModule(const QVariant &actionData,
                                             const QVariant &unused) const
    {
        Q_UNUSED(actionData);
        Q_UNUSED(unused);
        return QString::fromLatin1("placessidebarplugin%1.desktop");
    }

    virtual bool createNewModule(const QVariant &actionData,
                                 KConfigGroup &configGroup,
                                 QWidget *parentWidget,
                                 const QVariant &unused)
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

K_PLUGIN_FACTORY(KonqSidebarPlacesPluginFactory, registerPlugin<KonqSidebarPlacesPlugin>(); )
K_EXPORT_PLUGIN(KonqSidebarPlacesPluginFactory())

#include "places_module.moc"
