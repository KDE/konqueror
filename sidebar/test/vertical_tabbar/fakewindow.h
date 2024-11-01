//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef FAKEWINDOW_H
#define FAKEWINDOW_H

#include "interfaces/window.h"

#include <QUrl>
#include <QIcon>
#include <QString>
#include <QMenu>

class QWidget;

/**
 * @todo write docs
 */
class FakeWindow : public KonqInterfaces::Window
{
    Q_OBJECT

public:

    FakeWindow(QObject *parent);
    ~FakeWindow() override = default;

    QWidget *widget() override;
    int tabsCount() const override;
    QIcon tabFavicon(int idx) override;
    QString tabTitle(int idx) const override;
    QUrl tabUrl(int idx) const override;
    int activeTab() const override;

    KonqInterfaces::TabBarContextMenu* tabBarContextMenu(QWidget*) const override {return nullptr;};

    struct Tab {
        QString title;
        QUrl url;
        QIcon favicon;
    };

    void addTab(int idx, const Tab &newTab);
    void removeTab(int idx);
    void changeTitle(int idx, const QString &newTitle);
    void changeUrl(int idx, const QUrl &newUrl);


    QWidget* m_widget;
    int m_activeTab = 0;
    QList<Tab> m_tabs;

public Q_SLOTS:
    virtual void activateTab(int idx) override;
    virtual void moveTab(int fromIdx, int toIdx) override;

};

#endif // FAKEWINDOW_H
