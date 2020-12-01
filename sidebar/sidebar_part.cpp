/***************************************************************************
                               konqsidebar.cpp
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "sidebar_part.h"

#if KPARTS_VERSION >= QT_VERSION_CHECK(5, 77, 0)
#include <KPluginMetaData>
#else
#include <kaboutdata.h>
#endif

#include <QApplication>

#include <konq_events.h>
#include <kacceleratormanager.h>
#include <KLocalizedString>

K_PLUGIN_CLASS_WITH_JSON(KonqSidebarPart, "konq_sidebartng.json")

#if KPARTS_VERSION >= QT_VERSION_CHECK(5, 77, 0)
KonqSidebarPart::KonqSidebarPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData& metaData, const QVariantList &)
#else
KonqSidebarPart::KonqSidebarPart(QWidget *parentWidget, QObject *parent, const QVariantList &)
#endif
    : KParts::ReadOnlyPart(parent)
{
#if KPARTS_VERSION >= QT_VERSION_CHECK(5, 77, 0)
    setMetaData(metaData);
#else
    KAboutData aboutData("konqsidebartng", i18n("Extended Sidebar"), "0.2");
    aboutData.addAuthor(i18n("Joseph Wenninger"), "", "jowenn@kde.org");
    aboutData.addAuthor(i18n("David Faure"), "", "faure@kde.org");
    aboutData.addAuthor(i18n("Raphael Rosch"), "", "kde-dev@insaner.com");
    setComponentData(aboutData);
#endif

    QString currentProfile = parentWidget->window()->property("currentProfile").toString();
    if (currentProfile.isEmpty()) {
        currentProfile = "default";
    }
    m_widget = new Sidebar_Widget(parentWidget, this, currentProfile);
    m_extension = new KonqSidebarBrowserExtension(this, m_widget);
    connect(m_widget, &Sidebar_Widget::started, this, &KParts::ReadOnlyPart::started);
    connect(m_widget, &Sidebar_Widget::completed, this, QOverload<>::of(&KParts::ReadOnlyPart::completed));
    connect(m_extension, &KonqSidebarBrowserExtension::addWebSideBar, m_widget, &Sidebar_Widget::addWebSideBar);
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

KonqSidebarBrowserExtension::KonqSidebarBrowserExtension(KonqSidebarPart *part, Sidebar_Widget *widget_)
    : KParts::BrowserExtension(part), widget(widget_)
{
}

#include "sidebar_part.moc"
