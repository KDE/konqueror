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
 * The widget containing the tree view showing the history,
 * and the search lineedit on top of it.
 *
 * This widget is shared between the history dialog and the
 * history sidebar module.
 */
class KONQUERORPRIVATE_EXPORT KonqHistoryView : public QWidget
{
    Q_OBJECT

public:
    explicit KonqHistoryView(QWidget *parent);

    KActionCollection *actionCollection()
    {
        return m_collection;
    }
    QTreeView *treeView() const;
    QLineEdit *lineEdit() const;
    QUrl urlForIndex(const QModelIndex &index) const;

Q_SIGNALS:
    void openUrlInNewWindow(const QUrl &url);
    void openUrlInNewTab(const QUrl &url);
    void openUrlInCurrentTab(const QUrl &url);

private Q_SLOTS:
    void slotContextMenu(const QPoint &pos);
    void slotRemoveEntry();
    void slotClearHistory();
    void slotPreferences();
    void slotSortChange(QAction *action);
    void slotFilterTextChanged(const QString &text);
    void slotTimerTimeout();
    void slotNewWindow();
    void slotNewTab();
    void slotCurrentTab();
    void slotCopyLinkLocation();

private:
    QTreeView *m_treeView;
    KActionCollection *m_collection;
    KonqHistoryModel *m_historyModel;
    KonqHistoryProxyModel *m_historyProxyModel;
    QLineEdit *m_searchLineEdit;
    QTimer *m_searchTimer;
};

#endif /* KONQHISTORYVIEW_H */

