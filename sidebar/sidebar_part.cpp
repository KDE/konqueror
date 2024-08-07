/*
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "sidebar_part.h"

#include <KPluginMetaData>
#include <KPluginFactory>

#include <QApplication>

#include <konq_events.h>
#include <kacceleratormanager.h>
#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(KonqSidebarPart, "konq_sidebartng.json")

KonqSidebarPart::KonqSidebarPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData& metaData, const QVariantList &)
    : KParts::ReadOnlyPart(parent, metaData)
{
    QString currentProfile = parentWidget->window()->property("currentProfile").toString();
    if (currentProfile.isEmpty()) {
        currentProfile = "default";
    }
    m_widget = new Sidebar_Widget(parentWidget, this, currentProfile);
    m_extension = new KonqSidebarNavigationExtension(this, m_widget);
    connect(m_widget, &Sidebar_Widget::started, this, &KParts::ReadOnlyPart::started);
    connect(m_widget, &Sidebar_Widget::completed, this, QOverload<>::of(&KParts::ReadOnlyPart::completed));
    connect(m_extension, &KonqSidebarNavigationExtension::addWebSideBar, m_widget, &Sidebar_Widget::addWebSideBar);
    KAcceleratorManager::setNoAccel(m_widget);
    setWidget(m_widget);
}

KonqSidebarPart::~KonqSidebarPart()
{
}

bool KonqSidebarPart::openFile()
{
    return true;
}

bool KonqSidebarPart::openUrl(const QUrl &url)
{
    return m_widget->openUrl(url);
}

void KonqSidebarPart::customEvent(QEvent *ev)
{
    if (KonqFileSelectionEvent::test(ev) ||
            KonqFileMouseOverEvent::test(ev) ||
            KParts::PartActivateEvent::test(ev)) {
        // Forward the event to the widget
        QApplication::sendEvent(widget(), ev);
    }
}

////

KonqSidebarNavigationExtension::KonqSidebarNavigationExtension(KonqSidebarPart *part, Sidebar_Widget *widget_)
    : BrowserExtension(part), widget(widget_)
{
}

#include "sidebar_part.moc"
