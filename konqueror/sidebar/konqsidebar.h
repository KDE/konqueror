/***************************************************************************
                               konqsidebar.h
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

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <QtCore/QPointer>
#include "sidebar_widget.h"

class QWidget;
class KUrl;


class KonqSidebar;
class KonqSidebarFactory;

class KonqSidebarBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    KonqSidebarBrowserExtension(KonqSidebar *part_, Sidebar_Widget *widget_);
    ~KonqSidebarBrowserExtension(){}

protected:
    QPointer<Sidebar_Widget> widget;

    // The following slots are needed for konqueror's standard actions
    // ### Not really, since the sidebar never gets focus, currently.
protected Q_SLOTS:
    void copy() { if (widget) widget->stdAction("copy()"); }
    void cut() { if (widget) widget->stdAction("cut()"); }
    void paste() { if (widget) widget->stdAction("paste()"); }
};

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Joseph WENNINGER <jowenn@bigfoot.com>
 * @version 0.1
 */
class KonqSidebar : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
    KonqSidebar(QWidget *parentWidget, QObject *parent, const QVariantList&);

    /**
     * Destructor
     */
    virtual ~KonqSidebar();

    virtual bool openUrl(const KUrl &url);

protected:
    /**
     * This must be implemented by each part
     */
    KonqSidebarBrowserExtension * m_extension;
    virtual bool openFile();

    virtual void customEvent(QEvent* ev);

private:
     class Sidebar_Widget *m_widget;
};

#endif // KONQSIDEBARPART_H
