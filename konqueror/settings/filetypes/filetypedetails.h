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

#ifndef _FILETYPEDETAILS_H
#define _FILETYPEDETAILS_H

#include <QtGui/QTabWidget>
class MimeTypeData;
class TypesListItem;
class QLabel;
class QLineEdit;
class QListWidget;
class QGroupBox;
class QButtonGroup;
class QCheckBox;
class QRadioButton;
class QPushButton;
class KServiceListWidget;

/**
 * This widget contains the right part of the file type configuration
 * dialog, that shows the details for a file type.
 * It is implemented as a separate class so that it can be used by
 * the keditfiletype program to show the details of a single mimetype.
 */
class FileTypeDetails : public QTabWidget
{
  Q_OBJECT
public:
  FileTypeDetails(QWidget *parent = 0);

    /**
     * Set a non-gui "mimetype data" to work on,
     * and optionally a gui "treeview item", to update its icon if set.
     */
    void setMimeTypeData( MimeTypeData * mimeTypeData, TypesListItem* item = 0 );

protected:
  void updateRemoveButton();
  void updateAskSave();

Q_SIGNALS:
  void embedMajor(const QString &major, bool &embed); // To adjust whether major type is being embedded
  void changed(bool);

protected Q_SLOTS:
    //void updateIcon(const QString &icon);
  void updateDescription(const QString &desc);
  void addExtension();
  void removeExtension();
  void enableExtButtons();
  void slotAutoEmbedClicked(int button);
  void slotAskSaveToggled(bool);

private:
    MimeTypeData* m_mimeTypeData;
    TypesListItem* m_item; // can be 0, in keditfiletype!

  // First tab - General
  QLabel *iconButton;
  QListWidget *extensionLB;
  QPushButton *addExtButton, *removeExtButton;
  QLineEdit *description;
  KServiceListWidget *serviceListWidget;

  // Second tab - Embedding
  QGroupBox *m_autoEmbedBox;
  QButtonGroup *m_autoEmbedGroup;
  KServiceListWidget *embedServiceListWidget;
  QRadioButton *m_rbOpenSeparate;
  QCheckBox *m_chkAskSave;
  QRadioButton *m_rbGroupSettings;
};

#endif
