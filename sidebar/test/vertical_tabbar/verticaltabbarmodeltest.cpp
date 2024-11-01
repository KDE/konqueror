//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "verticaltabbarmodeltest.h"
#include "fakewindow.h"
#include "vertical_tabbar/verticaltabbarmodel.h"

#include <QTest>
#include <QIcon>
#include <QTreeView>
#include <QTimer>
#include <QSignalSpy>
#include <QMimeData>

QTEST_MAIN(VerticalTabbarModelTest);

FakeWindow::Tab VerticalTabbarModelTest::createTab(int i) const
{
    QPixmap pix(16, 16);
    m_lastUsedColorComponents.red += 5;
    pix.fill({m_lastUsedColorComponents.red, 0, 0});
    return FakeWindow::Tab{QString("t%1").arg(i), QUrl(QString("http://%1").arg(i)), pix};
}

FakeWindow::Tab VerticalTabbarModelTest::createTab(const QString& title) const
{
    QPixmap pix(16, 16);
    m_lastUsedColorComponents.green += 5;
    pix.fill({0, m_lastUsedColorComponents.green, 0});
    return FakeWindow::Tab{title, QUrl(QString("http://%1").arg(title)), pix};
}

QVariant VerticalTabbarModelTest::data(int row, int role) const
{
    return m_model->data(m_model->index(row, 0), role);
}

void VerticalTabbarModelTest::init()
{
    m_window = new FakeWindow(this);
    for (int i = 0; i < s_initialTabsCount; ++i) {
        m_window->m_tabs.append(createTab(i));
    }
    m_model = new VerticalTabBarModel(this);
    m_model->setWindow(m_window);
}

void VerticalTabbarModelTest::testReadTabsAtCreation()
{
    QCOMPARE(m_model->rowCount(), m_window->m_tabs.count());
}

void VerticalTabbarModelTest::testTabbAdded_data()
{
    QTest::addColumn<int>("newIdx");

    QTest::addRow("at beginning") << 0;
    QTest::addRow("at end") << s_initialTabsCount;
    QTest::addRow("in middle") << 5;
}

void VerticalTabbarModelTest::testTabbAdded()
{
    QFETCH(int, newIdx);

    int oldCount = m_window->m_tabs.count();
    QCOMPARE(m_model->rowCount(), oldCount);
    FakeWindow::Tab newTab = createTab("newtab");
    m_window->addTab(newIdx, newTab);
    QCOMPARE(m_model->rowCount(), oldCount+1);
    QCOMPARE(data(newIdx).toString(), newTab.title);
    QCOMPARE(data(newIdx, Qt::DecorationRole).value<QIcon>(), newTab.favicon);
    QVERIFY(!m_model->index(newIdx, 0).parent().isValid());
}

QStringList VerticalTabbarModelTest::titlesFromIndexes(const IntList indexes) const
{
    QStringList titles;
    std::transform(indexes.constBegin(), indexes.constEnd(), std::back_inserter(titles), [this](int i){return data(i).toString();});
    return titles;
}

QStringList VerticalTabbarModelTest::titles(int count) const
{
    if (count < 0) {
        count = m_model->rowCount();
    }
    QStringList titles;
    for (int i = 0; i < count; ++i) {
        titles.append(data(i).toString());
    }
    return titles;
}

VerticalTabbarModelTest::IntList VerticalTabbarModelTest::completeTabList(const IntList &list, std::optional<int> maybeCount, std::optional<int> maybeFirst)
{
    int count = maybeCount ? *maybeCount : s_initialTabsCount;
    int value = maybeFirst ? *maybeFirst : list.count();
    IntList completed(list);
    for (int i = list.count(); i < count; ++i) {
        completed << value;
        value++;
    }
    return completed;
}

void VerticalTabbarModelTest::testTabMoved_data()
{
    QTest::addColumn<int>("fromIdx");
    QTest::addColumn<int>("toIdx");
    QTest::addColumn<IntList>("newSequence");

    QTest::addRow("from middle to left one step") << 2 << 1<< completeTabList({0, 2, 1, 3, 4});
    QTest::addRow("from middle to left multiple steps") << 3 << 1 << completeTabList({0, 3, 1, 2, 4});
    QTest::addRow("from middle to right one step") << 2 << 3 << completeTabList({0, 1, 3, 2, 4});
    QTest::addRow("from middle to right multiple steps") << 1 << 3 << completeTabList({0, 2, 3, 1, 4});
    QTest::addRow("from start to right one step") << 0 << 1 << completeTabList({1, 0, 2, 3, 4});
    QTest::addRow("from start to right multiple steps") << 0 << 2 << completeTabList({1, 2, 0, 3, 4});
    QTest::addRow("from end to left one step") << 4 << 3 << completeTabList({0, 1, 2, 4, 3});
    QTest::addRow("from end to left multiple steps") << 4 << 2 << completeTabList({0, 1, 4, 2, 3});
}

void VerticalTabbarModelTest::testTabMoved()
{
    QFETCH(int, fromIdx);
    QFETCH(int, toIdx);
    QFETCH(IntList, newSequence);

    QStringList expectedTitles = titlesFromIndexes(newSequence);
    m_window->moveTab(fromIdx, toIdx);
    QCOMPARE(m_model->rowCount(), s_initialTabsCount);
    QStringList movedTitles = titles();
    QCOMPARE(movedTitles, expectedTitles);
}

void VerticalTabbarModelTest::testTabRemoved_data()
{
    QTest::addColumn<int>("idx");
    QTest::addColumn<IntList>("newSequence");
    QTest::addRow("from middle") << 2 << completeTabList({0, 1, 3, 4}, s_initialTabsCount-1, 5);
    QTest::addRow("from start") << 0 << completeTabList({1, 2, 3, 4}, s_initialTabsCount-1, 5);
    QTest::addRow("from end") << 4 << completeTabList({0, 1, 2, 3}, s_initialTabsCount-1, 5);
}

void VerticalTabbarModelTest::testTabRemoved()
{
    QFETCH(int, idx);
    QFETCH(IntList, newSequence);
    QStringList expTitles = titlesFromIndexes(newSequence);
    int oldCount = m_window->m_tabs.count();
    m_window->removeTab(idx);
    QCOMPARE(m_model->rowCount(), oldCount-1);
    QCOMPARE(titles(), expTitles);
}

void VerticalTabbarModelTest::testDropMimeData_data()
{
    QTest::addColumn<int>("draggedTab");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("expectedRow");

    QTest::addRow("moved from begin to end") << 0 << s_initialTabsCount << s_initialTabsCount-1;
    QTest::addRow("moved from middle to end") << 5 << s_initialTabsCount << s_initialTabsCount-1;
    QTest::addRow("moved from middle to begin") << 5 << 0 << 0;
    QTest::addRow("moved from middle to middle forwards") << 5 << 10 << 9;
    QTest::addRow("moved from middle to middle backwards") << 5 << 2 << 2;
}

void VerticalTabbarModelTest::testDropMimeData()
{
    QFETCH(int, draggedTab);
    QFETCH(int, row);
    QFETCH(int, expectedRow);

    QStandardItem *clickOnIt = m_model->itemForTab(draggedTab);
    QString expectedTxt = clickOnIt->text();
    QModelIndex releasedOnIdx = m_model->index(row, 0);
    QMimeData *data = m_model->mimeData({clickOnIt->index()});
    QModelIndex  releaseOnParent = releasedOnIdx.parent();
    m_model->dropMimeData(data, Qt::DropAction::MoveAction, row, 0, releaseOnParent);
    QCOMPARE(m_model->item(expectedRow)->text(), expectedTxt);

}
