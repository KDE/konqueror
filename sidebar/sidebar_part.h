/*
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQSIDEBARPART_H
#define KONQSIDEBARPART_H

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
    KonqSidebarPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData& metaData, const QVariantList &);

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
