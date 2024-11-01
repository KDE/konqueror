//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef VERTICALTABBARMODELTEST_H
#define VERTICALTABBARMODELTEST_H

#include "fakewindow.h"

#include <QObject>

class VerticalTabBarModel;
class QStandardItem;

class VerticalTabbarModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();

    void testReadTabsAtCreation();

    void testTabbAdded_data();
    void testTabbAdded();

    void testTabMoved_data();
    void testTabMoved();

    void testTabRemoved_data();
    void testTabRemoved();

    void testDropMimeData_data();
    void testDropMimeData();

private:

    using IntList=QList<int>;

    static IntList completeTabList(const IntList &list, std::optional<int> maybeCount = std::nullopt, std::optional<int> maybeFirst = std::nullopt);

    QVariant data(int row, int role = Qt::DisplayRole) const;
    QStringList titlesFromIndexes(const IntList indexes) const;
    QStringList titles(int count = -1) const;

    FakeWindow::Tab createTab(int i) const;
    FakeWindow::Tab createTab(const QString &title) const;

    static constexpr int s_initialTabsCount = 20;
    FakeWindow *m_window;
    VerticalTabBarModel *m_model;

    struct ColorComponents {
        int red = 0;
        int green = 0;
        int blue = 0;
    };
    //Tabs created automatically use red favicons; tabs created with a specific title
    //use green favicons. This member contains the last-used value for each component:
    //Using a greater value than this ensures that a unique color is always used.
    //Don't forget to update the last used color
    mutable ColorComponents m_lastUsedColorComponents;
};

#endif // VERTICALTABBARMODELTEST_H
