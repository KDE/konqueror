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
#include <kurl.h>

class KonqMainWindow;
class KonqHistoryView;
class QModelIndex;

class KonqHistoryDialog : public KDialog
{
    Q_OBJECT

public:
    KonqHistoryDialog(KonqMainWindow *parent = 0);
    ~KonqHistoryDialog();

    QSize sizeHint() const;

private Q_SLOTS:
    void slotOpenWindow(const KUrl& url);
    void slotOpenTab(const KUrl& url);
    void slotOpenWindowForIndex(const QModelIndex& index);

private:
    KonqHistoryView* m_historyView;
    KonqMainWindow* m_mainWindow;
};

#endif // KONQ_HISTORYDIALOG_H
