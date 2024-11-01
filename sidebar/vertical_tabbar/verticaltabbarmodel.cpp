//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "verticaltabbarmodel.h"
#include "interfaces/window.h"

#include <QUrl>
#include <QMimeData>
#include <QDataStream>
#include <QApplication>

#include <QDebug>

QString VerticalTabBarModel::dragAndDropMimetype()
{
   static QString s_dragAndDropMimetype = QStringLiteral("application/x-konquerorverticaltabbar");
   return s_dragAndDropMimetype;
}

VerticalTabBarModel::VerticalTabBarModel(QObject* parent) : QStandardItemModel(parent)
{
}

void VerticalTabBarModel::setWindow(KonqInterfaces::Window* window)
{
    m_window = window;
    connect(m_window, &KonqInterfaces::Window::tabAdded, this, &VerticalTabBarModel::addTab);
    connect(m_window, &KonqInterfaces::Window::tabTitleChanged, this, &VerticalTabBarModel::updateTabTitle);
    connect(m_window, &KonqInterfaces::Window::tabMoved, this, &VerticalTabBarModel::moveTab);
    connect(m_window, &KonqInterfaces::Window::tabRemoved, this, &VerticalTabBarModel::removeTab);
    connect(m_window, &KonqInterfaces::Window::tabUrlChanged, this, &VerticalTabBarModel::updateTabToolTip);
    fill();
}

QStandardItem * VerticalTabBarModel::createItemForTab(int tabIdx) const
{
    QStandardItem *it = new QStandardItem(m_window->tabFavicon(tabIdx), m_window->tabTitle(tabIdx));
    it->setToolTip(m_window->tabUrl(tabIdx).toDisplayString());
    it->setEditable(false);
    it->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled|Qt::ItemIsEnabled|Qt::ItemIsDropEnabled);
    return it;
}

QStandardItem* VerticalTabBarModel::itemForTab(int tabIdx) const
{
    return item(tabIdx);
}

int VerticalTabBarModel::tabForIndex(const QModelIndex& idx)
{
    return idx.row();
}

int VerticalTabBarModel::tabForItem(QStandardItem* item) const
{
    return item->row();
}

void VerticalTabBarModel::fill()
{
    clear();
    for (int i = 0; i < m_window->tabsCount(); ++i) {
        QStandardItem *it = createItemForTab(i);
        appendRow(it);
    }
}

void VerticalTabBarModel::addTab(int tabIdx)
{
    QStandardItem *it = createItemForTab(tabIdx);
    insertRow(tabIdx, it);
}

void VerticalTabBarModel::updateTabTitle(int tabIdx, const QString &title)
{
    QStandardItem *it = itemForTab(tabIdx);
    if (it) {
        it->setText(title);
        it->setIcon(m_window->tabFavicon(tabIdx));
    }
}

void VerticalTabBarModel::updateTabToolTip(int tabIdx)
{
    QStandardItem *it = itemForTab(tabIdx);
    if (it) {
        it->setToolTip(m_window->tabUrl(tabIdx).toDisplayString());
    }
}

void VerticalTabBarModel::moveTab(int fromTabIdx, int toTabIdx)
{
    QStandardItem *it = itemForTab(fromTabIdx);
    takeRow(it->row());
    insertRow(toTabIdx, it);
}

void VerticalTabBarModel::removeTab(int tabIdx)
{
    QStandardItem *it = itemForTab(tabIdx);
    takeRow(it->row());
    delete it;
}

QStringList VerticalTabBarModel::mimeTypes() const
{
    return {dragAndDropMimetype()};
}

QMimeData* VerticalTabBarModel::mimeData(const QModelIndexList& indexes) const
{
    if (indexes.isEmpty()) {
        return nullptr;
    }
    QByteArray data;
    QDataStream ds(&data, QIODeviceBase::WriteOnly);
    for (const QModelIndex idx : indexes) {
        QStandardItem *it = itemFromIndex(idx);
        if (!it) {
            continue;
        }
        ds << tabForItem(it);
    }
    QMimeData *mime = new QMimeData;
    mime->setData(dragAndDropMimetype(), data);
    return mime;
}

bool VerticalTabBarModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    if (!canDropMimeData(data, action, row, column, parent)) {
        return false;
    }
    QDataStream ds(data->data(dragAndDropMimetype()));
    int fromIdx = -1;
    ds >> fromIdx;
    if (fromIdx < 0) {
        return false;
    }
    if (row > fromIdx) {
        --row; //If the destination row is after the origin row, it must be decreased by 1
    }
    m_window->moveTab(fromIdx, row);
    return false;
}
