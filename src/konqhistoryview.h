/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2009 Pino Toscano <pino@kde.org>
    SPDX-FileCopyrightText: 2009 Daivd Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQHISTORYVIEW_H
#define KONQHISTORYVIEW_H

#include <konqprivate_export.h>
#include <QWidget>
class QUrl;
class QModelIndex;
class QTreeView;
class QTimer;
class QLineEdit;
class KonqHistoryProxyModel;
class KonqHistoryModel;
class KActionCollection;

/**
 * @brief Widget to display history
 *
 * It contains a `QTreeView` to display history and a lineedit above it to search
 * history.
 *
 * This widget is shared between the history dialog and the
 * history sidebar module.
 */
class KONQUERORPRIVATE_EXPORT KonqHistoryView : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent the parent widget
     */
    explicit KonqHistoryView(QWidget *parent);

    /**
     * @brief The action collection used by the widget
     * @return the action collection used by the widget
     */
    KActionCollection *actionCollection()
    {
        return m_collection;
    }

    /**
     * @brief The tree view where the history is shown
     * @return the tree view where the history is shown
     */
    QTreeView *treeView() const;

    /**
     * @brief The line edit used for searching history
     * @return the line edit used for searching history
     */
    QLineEdit *lineEdit() const;

    /**
     * @brief The URL of the item corresponding to the given index
     *
     * @param index the index
     * @return the URL of the history item corresponding to @p index or an invalid
     * URL if the index doesn't correspond to an history item (for example, because
     * it's a group)
     */
    QUrl urlForIndex(const QModelIndex &index) const;

Q_SIGNALS:
    /**
     * @brief Signal emitted when the user chooses to open an URL in a new window
     *
     * @param url the URL to open
     */
    void openUrlInNewWindow(const QUrl &url);

    /**
     * @brief Signal emitted when the user chooses to open an URL in a new tab
     *
     * @param url the URL to open
     */
    void openUrlInNewTab(const QUrl &url);

    /**
     * @brief Signal emitted when the user chooses to open an URL in the current tab
     *
     * @param url the URL to open
     */
    void openUrlInCurrentTab(const QUrl &url);

private Q_SLOTS:

    /**
     * @brief Slot called when the context menu should be shown
     *
     * It creates and shows the context menu.
     */
    void slotContextMenu(const QPoint &pos);

    /**
     * @brief Slot called when the user chooses to remove the current entry from history
     *
     * It removes the current entry from the history
     */
    void slotRemoveEntry();

    /**
     * @brief Slot called when the user chooses to clear the history
     *
     * It displays a confirmation dialog then asks the KonqHistoryProvider to clear
     * the history
     */
    void slotClearHistory();

    /**
     * @brief Slot called when the user asks to show the history settings dialog
     *
     * It shows a dialog with the history kcm
     */
    void slotPreferences();

    /**
     * @brief Slot called when the user chooses one of the actions which change
     * history sorting
     *
     * @param action the chosen action
     *
     * It changes the sorting mode in the configuration object.
     * @note Even if this may not be obvious from the UI, this change is permanent
     * and will overwrite the settings made with the configuration dialog.
     */
    void slotSortChange(QAction *action);

    /**
     * @brief Slot called when the text in the search line edit changes
     *
     * It starts a timer which, when timeouts updates the filter in the model
     * @param text the new search text (unused)
     */
    void slotFilterTextChanged(const QString &text);

    /**
     * @brief Slot called when the timer created by slotFilterTextChanged() times out
     *
     * It updates the model filter using the text in the line edit
     */
    void slotTimerTimeout();

    /**
     * @brief Slot called when the user chooses the action to show the current item in a new window
     *
     * It emits the openUrlInNewWindow() for the current item in the tree view.
     */
    void slotNewWindow();
    /**
     * @brief Slot called when the user chooses the action to show the current item in a new tab
     *
     * It emits the openUrlInNewTab() for the current item in the tree view.
     */
    void slotNewTab();
    /**
     * @brief Slot called when the user chooses the action to show the current item in the current tab
     *
     * It emits the openUrlInCurrentTab() for the current item in the tree view.
     */
    void slotCurrentTab();

    /**
     * @brief Slot called when the user chooses to copy the URL of the current item
     *
     * It copies the URL of the current item to the clipboard and the selection.
     */
    void slotCopyLinkLocation();

private:
    QTreeView *m_treeView; //!< The view which shows history
    KActionCollection *m_collection; //!< The action collection
    KonqHistoryModel *m_historyModel; //!< The model containing history
    KonqHistoryProxyModel *m_historyProxyModel; //!< The model to filter history
    QLineEdit *m_searchLineEdit; //!< The line edit which the user can use to filter history
    QTimer *m_searchTimer; //!< The timer used to update the model's filter
};

#endif /* KONQHISTORYVIEW_H */

