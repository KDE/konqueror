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
#include <Qt3Support/Q3ButtonGroup>
#include <QtGui/QWhatsThis>
#include <kdialog.h>
#include <klocale.h>

FileGroupDetails::FileGroupDetails(QWidget *parent)
    : QWidget( parent )
{
  QVBoxLayout *secondLayout = new QVBoxLayout;
  secondLayout->setMargin(0);
  secondLayout->setSpacing(KDialog::spacingHint());

  m_autoEmbed = new Q3ButtonGroup( i18n("Left Click Action"));
  m_autoEmbed->setOrientation( Qt::Vertical );
  m_autoEmbed->layout()->setSpacing( KDialog::spacingHint() );
  secondLayout->addWidget( m_autoEmbed );
  // The order of those two items is very important. If you change it, fix typeslistitem.cpp !
  QRadioButton *r1 = new QRadioButton( i18n("Show file in embedded viewer"));
  QRadioButton *r2 = new QRadioButton( i18n("Show file in separate viewer"));
  m_autoEmbed->layout()->addWidget(r1);
  m_autoEmbed->layout()->addWidget(r2);
  connect(m_autoEmbed, SIGNAL( clicked( int ) ), SLOT( slotAutoEmbedClicked( int ) ));

  m_autoEmbed->setWhatsThis( i18n("Here you can configure what the Konqueror file manager"
    " will do when you click on a file belonging to this group. Konqueror can display the file in"
    " an embedded viewer or start up a separate application. You can change this setting for a"
    " specific file type in the 'Embedding' tab of the file type configuration.") );

  secondLayout->addStretch();
  secondLayout->addWidget(m_autoEmbed);
  setLayout(secondLayout);
}

void FileGroupDetails::setMimeTypeData( MimeTypeData * mimeTypeData )
{
    Q_ASSERT( mimeTypeData->isMeta() );
    m_mimeTypeData = mimeTypeData;
    m_autoEmbed->setButton( m_mimeTypeData->autoEmbed() );
}

void FileGroupDetails::slotAutoEmbedClicked(int button)
{
  if ( !m_mimeTypeData )
    return;
  m_mimeTypeData->setAutoEmbed( (MimeTypeData::AutoEmbed)button );
  emit changed(true);
}

#include "filegroupdetails.moc"
