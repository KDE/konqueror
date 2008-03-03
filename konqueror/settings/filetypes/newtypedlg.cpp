/* This file is part of the KDE project
   Copyright 2000 Kurt Granroth <granroth@kde.org>
             2008 David Faure <faure@kde.org>

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

// Own
#include "newtypedlg.h"

// Qt
#include <QtGui/QBoxLayout>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

// KDE
#include <klineedit.h>
#include <klocale.h>


NewTypeDialog::NewTypeDialog(const QStringList &groups, QWidget *parent)
  : KDialog( parent )
{
  setModal( true );
  setCaption( i18n( "Create New File Type" ) );
  setButtons( Ok | Cancel );
  showButtonSeparator( true );

  QWidget* main = mainWidget();

  QGridLayout *grid = new QGridLayout(main);
  grid->setColumnStretch(1, 1);
  grid->setSpacing(spacingHint());

  // Line 0: group selection

  QLabel *l = new QLabel(i18n("Group:"), main);
  grid->addWidget(l, 0, 0);

  m_groupCombo = new QComboBox(main);
  //m_groupCombo->setEditable( true ); M.O.: Currently, the code in filetypesview isn't capable of handling
  //new top level types; so better not let them be added than crash.
  m_groupCombo->addItems(groups);
  grid->addWidget(m_groupCombo, 0, 1);
  l->setBuddy(m_groupCombo);

  m_groupCombo->setWhatsThis( i18n("Select the category under which"
    " the new file type should be added.") );

  // Line 1: mimetype name

  l = new QLabel(i18n("Type name:"), main);
  grid->addWidget(l, 1, 0);

  m_typeEd = new KLineEdit(main);
  grid->addWidget(m_typeEd, 1, 1);
  l->setBuddy(m_typeEd);

  m_typeEd->setWhatsThis(i18n("Type the name of the file type. For instance, if you selected 'image' as category and you type 'custom' here, the file type 'image/custom' will be created."));

  m_typeEd->setFocus();

  // Set a minimum size so that caption is not half-hidden
  setMinimumSize( 300, 50 );
}

QString NewTypeDialog::group() const
{
    return m_groupCombo->currentText();
}

QString NewTypeDialog::text() const
{
    return m_typeEd->text();
}
