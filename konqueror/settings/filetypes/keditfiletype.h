/* This file is part of the KDE project
   Copyright (C) 2000, 2007 David Faure <faure@kde.org>

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

#ifndef __keditfiletype_h
#define __keditfiletype_h

#include <kdialog.h>
#include <kmimetype.h>

class MimeTypeData;
class FileTypeDetails;

// A dialog for ONE file type to be edited.
class FileTypeDialog : public KDialog
{
    Q_OBJECT
public:
    FileTypeDialog(MimeTypeData* mime);
    ~FileTypeDialog();

protected Q_SLOTS:
  virtual void slotOk();
  virtual void slotApply();
  void clientChanged(bool state);
  void slotDatabaseChanged();

protected:
  void save();

private:
  void init();
  FileTypeDetails * m_details;
  MimeTypeData* m_mimeTypeData;
};

#endif

