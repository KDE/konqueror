/* This file is part of the KDE project
   Copyright 2009 Pino Toscano <pino@kde.org>
   Copyright 2009 Daivd Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQHISTORYVIEW_H
#define KONQHISTORYVIEW_H

#include <konqprivate_export.h>
#include <QWidget>
class KUrl;
class QModelIndex;
class QTreeView;
class QTimer;
class KLineEdit;
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
    explicit KonqHistoryView(QWidget* parent);

    KActionCollection *actionCollection() { return m_collection; }
    QTreeView* treeView() const;
    KLineEdit* lineEdit() const;
    KUrl urlForIndex(const QModelIndex& index) const;

Q_SIGNALS:
    void openUrlInNewWindow(const KUrl& url);
    void openUrlInNewTab(const KUrl& url);

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
    void slotCopyLinkLocation();

private:
    QTreeView* m_treeView;
    KActionCollection *m_collection;
    KonqHistoryModel *m_historyModel;
    KonqHistoryProxyModel *m_historyProxyModel;
    KLineEdit *m_searchLineEdit;
    QTimer *m_searchTimer;
};


#endif /* KONQHISTORYVIEW_H */

