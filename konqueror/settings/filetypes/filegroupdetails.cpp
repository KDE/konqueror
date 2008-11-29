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
#include "filegroupdetails.h"
#include "mimetypedata.h"

#include <QtGui/QLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>

#include <klocale.h>

FileGroupDetails::FileGroupDetails(QWidget *parent)
    : QWidget( parent )
{
  QVBoxLayout *secondLayout = new QVBoxLayout(this);

  QGroupBox *autoEmbedBox = new QGroupBox( i18n("Left Click Action") );
  m_autoEmbed = new QButtonGroup( autoEmbedBox );
  secondLayout->addWidget( autoEmbedBox );
  // The order of those two items is very important. If you change it, fix typeslistitem.cpp !
  QRadioButton *r1 = new QRadioButton( i18n("Show file in embedded viewer"));
  QRadioButton *r2 = new QRadioButton( i18n("Show file in separate viewer"));
  QVBoxLayout *autoEmbedBoxLayout = new QVBoxLayout(autoEmbedBox);
  autoEmbedBoxLayout->addWidget(r1);
  autoEmbedBoxLayout->addWidget(r2);
  m_autoEmbed->addButton(r1, 0);
  m_autoEmbed->addButton(r2, 1);
  connect(m_autoEmbed, SIGNAL( buttonClicked( int ) ), SLOT( slotAutoEmbedClicked( int ) ));

  autoEmbedBox->setWhatsThis( i18n("Here you can configure what the Konqueror file manager"
    " will do when you click on a file belonging to this group. Konqueror can display the file in"
    " an embedded viewer or start up a separate application. You can change this setting for a"
    " specific file type in the 'Embedding' tab of the file type configuration.") );

  secondLayout->addStretch();
}

void FileGroupDetails::setMimeTypeData( MimeTypeData * mimeTypeData )
{
    Q_ASSERT( mimeTypeData->isMeta() );
    m_mimeTypeData = mimeTypeData;
    m_autoEmbed->button( m_mimeTypeData->autoEmbed() )->setChecked( true );
}

void FileGroupDetails::slotAutoEmbedClicked(int button)
{
  if ( !m_mimeTypeData )
    return;
  m_mimeTypeData->setAutoEmbed( (MimeTypeData::AutoEmbed)button );
  emit changed(true);
}

#include "filegroupdetails.moc"
