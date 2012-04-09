/***************************************************************************
                               konqsidebartest.h
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
#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <QLabel>
#include <QLayout>

class SidebarTest : public KonqSidebarModule
{
    Q_OBJECT
public:
    SidebarTest(const KComponentData &componentData, QWidget *parent, const QString &desktopName, const KConfigGroup& configGroup)
        : KonqSidebarModule(componentData, parent, configGroup)
    {
        Q_UNUSED(desktopName);
        widget = new QLabel("Init Value", parent);
    }
    ~SidebarTest(){}
    virtual QWidget *getWidget(){return widget;}
protected:
    QLabel *widget;
    virtual void handleURL(const KUrl &url)
    {
        widget->setText(url.url());
    }
};

#endif
