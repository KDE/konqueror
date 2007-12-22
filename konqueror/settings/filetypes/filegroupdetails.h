/* This file is part of the KDE project
   Copyright (C) 2000, 2007 David Faure <faure@kde.org>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>

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
#ifndef _FILEGROUPDETAILS_H
#define _FILEGROUPDETAILS_H

#include <QtGui/QWidget>
class MimeTypeData;
class Q3ButtonGroup;

/**
 * This widget contains the details for a filetype group.
 * Currently this only involves the embedding configuration.
 */
class FileGroupDetails : public QWidget
{
  Q_OBJECT
public:
  FileGroupDetails(QWidget *parent = 0);

    void setMimeTypeData( MimeTypeData * mimeTypeData );

Q_SIGNALS:
  void changed(bool);

protected Q_SLOTS:
  void slotAutoEmbedClicked(int button);

private:
    MimeTypeData * m_mimeTypeData;

  // Embedding config
  Q3ButtonGroup *m_autoEmbed;
};

#endif
