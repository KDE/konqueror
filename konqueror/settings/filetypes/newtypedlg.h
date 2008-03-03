/* This file is part of the KDE project
   Copyright 2000 Kurt Granroth <granroth@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _NEWTYPEDLG_H
#define _NEWTYPEDLG_H

#include <kdialog.h>

class QStringList;
class KLineEdit;
class QComboBox;

/**
 * A dialog for creating a new file type, with
 * - a combobox for choosing the group
 * - a line-edit for entering the name of the file type
 * The rest (description, patterns, icon, apps) can be set later in the filetypesview anyway.
 */
class NewTypeDialog : public KDialog
{
public:
    explicit NewTypeDialog(const QStringList &groups, QWidget *parent);
    QString group() const;
    QString text() const;
private:
    KLineEdit *m_typeEd;
    QComboBox *m_groupCombo;
};

#endif
