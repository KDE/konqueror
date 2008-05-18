/*  This file is part of the KDE project
    Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __konq_sessiondlg_h__
#define __konq_sessiondlg_h__

#include <kdialog.h>

#include <QtCore/QMap>
#include <QString>

class KonqViewManager;
class QListWidgetItem;

/**
 * This is the konqueror sesions administration dialog, which allows the user
 * to add, modify, rename and delete konqueror sessions.
 */
class KonqSessionDlg : public KDialog
{
    Q_OBJECT
public:
    KonqSessionDlg( KonqViewManager *manager, QWidget *parent = 0L );
    ~KonqSessionDlg();

protected Q_SLOTS:
    void slotOpen();
    void slotRename(KUrl dirpathTo = KUrl());
    void slotNew();
    void slotDelete();
    void slotSave();
    void slotSelectionChanged();

private:
    class KonqSessionDlgPrivate;
    KonqSessionDlgPrivate * const d;
    void loadAllSessions(const QString & = QString());
};

class KonqNewSessionDlg : public KDialog
{
    Q_OBJECT
public:
    KonqNewSessionDlg( QWidget *parent = 0L, QString sessionName = QString() );
    ~KonqNewSessionDlg();

protected Q_SLOTS:
    void slotAddSession();
    void slotTextChanged(const QString& text);
private:
    class KonqNewSessionDlgPrivate;
    KonqNewSessionDlgPrivate * const d;
};

#endif
