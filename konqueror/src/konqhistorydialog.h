/* This file is part of the KDE project
   Copyright 2009 Pino Toscano <pino@kde.org>

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

#ifndef KONQ_HISTORYDIALOG_H
#define KONQ_HISTORYDIALOG_H

#include <kdialog.h>

class QTimer;
class QTreeView;
class KActionCollection;
class KLineEdit;
class KonqHistoryModel;
class KonqHistoryProxyModel;

class KonqHistoryDialog : public KDialog
{
    Q_OBJECT

public:
    KonqHistoryDialog(QWidget *parent = 0);
    ~KonqHistoryDialog();

    QSize sizeHint() const;

private slots:
    void slotContextMenu(const QPoint &pos);
    void slotNewWindow();
    void slotRemoveEntry();
    void slotClearHistory();
    void slotPreferences();
    void slotSortChange(QAction *action);
    void slotFilterTextChanged(const QString &text);
    void slotTimerTimeout();

private:
    QTreeView *m_treeView;
    KActionCollection *m_collection;
    KonqHistoryModel *m_historyModel;
    KonqHistoryProxyModel *m_historyProxyModel;
    KLineEdit *m_searchLineEdit;
    QTimer *m_searchTimer;
};

#endif // KONQ_HISTORYDIALOG_H
