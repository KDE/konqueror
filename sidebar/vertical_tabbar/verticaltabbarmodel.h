//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef VERTICALTABBARMODEL_H
#define VERTICALTABBARMODEL_H

#include "vertical_tabbar_export.h"

#include <QStandardItemModel>
#include <QPointer>

namespace KonqInterfaces {
    class Window;
}

/**
 * @brief Model used to represent tabs in the vertical tabbar sidebar module
 */
class VERTICAL_TABBAR_EXPORT VerticalTabBarModel : public QStandardItemModel
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    VerticalTabBarModel(QObject* parent);

    ~VerticalTabBarModel() = default; //!< Destructor

    /**
     * @brief The window where the sidebar module is
     * @param the object representing the window
     */
    void setWindow(KonqInterfaces::Window *window);

    /**
     * @brief Override of `QStandardItemModel::mimeTypes()`
     *
     * @return a list containing only the value returned by dragAndDropMimetype()
     */
    QStringList mimeTypes() const override;

    /**
     * @brief Override of `QStandardItemModel::mimeData()`
     *
     * @return an object containing the number of the tab associated with the index
     */
    QMimeData* mimeData(const QModelIndexList & indexes) const override;

    /**
     * @brief Override of `QStandardItemModel::dropMimeData()`
     *
     * It moves the dragged tab at the dropped position, allowing to rearrange tabs by drag and drop.
     *
     * @note This doesn't just change the order of items in the model, but rearranges tabs in the main window
     *
     * @param data as in `QAbstractItemModel::dropMimeData()`. The data must be encoded in the mimetype returned by dragAndDropMimetype()
     * @param action as in `QAbstractItemModel::dropMimeData()`
     * @param row as in `QAbstractItemModel::dropMimeData()`
     * @param column as in `QAbstractItemModel::dropMimeData()`
     * @param parent as in `QAbstractItemModel::dropMimeData()`
     * @return `false`
     */
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    /**
     * @brief The number of the tab corresponding to the given model index
     *
     * @note Currently, the number of the tab is the same as the row of the index. This won't be true anymore when
     * tab grouping will be implemented
     * @param idx the model index
     * @return the number of the tab corresponding to @p idx
     */
    int tabForIndex(const QModelIndex &idx);

public Q_SLOTS:
    /**
     * @brief Adds an item corresponding to the given tab
     * @param tabIdx the number of the tab to add
     */
    void addTab(int tabIdx);
    /**
     * @brief Changes the title of the item corresponding to the given tab
     * @param tabIdx the number of the tab
     * @param title the new title of the tab
     */
    void updateTabTitle(int tabIdx, const QString &title);
    /**
     * @brief Changes the tool tip of the item corresponding to the given tab
     * @param tabIdx the number of the tab
     */
    void updateTabToolTip(int tabIdx);
    /**
     * @brief Moves the item corresponding to a given tab
     * @param fromTabIdx the original number of the tab
     * @param toTabIdx the new number of the tab
     */
    void moveTab(int fromTabIdx, int toTabIdx);
    /**
     * @brief Removes the item corresponding to the tab with the given index
     * @param tabIdx the number of the tab to remove
     */
    void removeTab(int tabIdx);

private Q_SLOTS:
    /**
     * @brief Fills the model with items corresponding to the tabs in the window
     *
     * Any existing item will be removed
     */
    void fill();

private:

    /**
     * @brif Creates an item representing the given tab
     * @param tabIdx the number of the tab
     * @return an item corresponding to the tab with number @p tabIdx
     */
    QStandardItem* createItemForTab(int tabIdx) const;

    /**
     * @brief The item corresponding to the given tab number
     * @return the item corresponding to the given tab number
     * @note Currently, the number of the tab is the same as the row of the item. This won't be true anymore when
     * tab grouping will be implemented
     */
    QStandardItem* itemForTab(int tabIdx) const;
    /**
     * @brief The number of the tab corresponding to the given item
     * @param item the item
     * @return the number of the tab corresponding to @p item
     * @note Currently, the number of the tab is the same as the row of the item. This won't be true anymore when
     * tab grouping will be implemented
     */
    int tabForItem(QStandardItem *item) const;

    /**
     * @brief The name of the mimetype format using for drag and drop
     * @return the string `application/x-konquerorverticaltabbar`
     */
    static QString dragAndDropMimetype();

private:
    QPointer<KonqInterfaces::Window> m_window; //!< The main window

#ifdef BUILD_TABMODELTEST
    friend class VerticalTabbarModelTest;
#endif

};

#endif // VERTICALTABBARMODEL_H
