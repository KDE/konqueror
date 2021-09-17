/*
    konqsidebartest.h
    -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    SPDX-FileCopyrightText: 2001 Joseph Wenninger
    email                : jowenn@kde.org
*/

/***************************************************************************
 *                                                                         *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *                                                                         *
 ***************************************************************************/
#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <QLabel>
#include <QUrl>

class SidebarTest : public KonqSidebarModule
{
    Q_OBJECT
public:
    SidebarTest(QWidget *parent, const QString &desktopName, const KConfigGroup &configGroup)
        : KonqSidebarModule(parent, configGroup)
    {
        Q_UNUSED(desktopName);
        widget = new QLabel("Init Value", parent);
    }
    ~SidebarTest() override {}
    QWidget *getWidget() override
    {
        return widget;
    }
protected:
    QLabel *widget;
    void handleURL(const QUrl &url) override
    {
        widget->setText(url.url());
    }
};

#endif
