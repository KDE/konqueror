/***************************************************************************
                               sidebar_part.h
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
#ifndef KONQSIDEBARPART_H
#define KONQSIDEBARPART_H

#include <kparts_version.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <QPointer>
#include "sidebar_widget.h"

class KonqSidebarPart;

class KonqSidebarBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    KonqSidebarBrowserExtension(KonqSidebarPart *part, Sidebar_Widget *widget);
    ~KonqSidebarBrowserExtension() override {}

protected:
    QPointer<Sidebar_Widget> widget;

    // The following slots are needed for konqueror's standard actions
    // They are called from the RMB popup menu
protected Q_SLOTS:
    void copy()
    {
        if (widget) {
            widget->stdAction("copy");
        }
    }
    void cut()
    {
        if (widget) {
            widget->stdAction("cut");
        }
    }
    void paste()
    {
        if (widget) {
            widget->stdAction("paste");
        }
    }
    void pasteTo(const QUrl &)
    {
        if (widget) {
            widget->stdAction("pasteToSelection");
        }
    }
};

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Joseph WENNINGER <jowenn@kde.org>
 * @version 0.1
 */
class KonqSidebarPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
#if KPARTS_VERSION >= QT_VERSION_CHECK(5, 77, 0)
    KonqSidebarPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData& metaData, const QVariantList &);
#else
    KonqSidebarPart(QWidget *parentWidget, QObject *parent, const QVariantList &);
#endif

    /**
     * Destructor
     */
    ~KonqSidebarPart() override;

    bool openUrl(const QUrl &url) override;

protected:
    /**
     * This must be implemented by each part
     */
    KonqSidebarBrowserExtension *m_extension;
    bool openFile() override;

    void customEvent(QEvent *ev) override;

private:
    Sidebar_Widget *m_widget;
};

#endif // KONQSIDEBARPART_H
