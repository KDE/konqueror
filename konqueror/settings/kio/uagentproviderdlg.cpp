/**
 * Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Own
#include "uagentproviderdlg.h"

// Local
#include "useragentinfo.h"

// Qt
#include <QtGui/QBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QValidator>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurllabel.h>

class UASiteNameValidator : public QValidator
{
public:
  UASiteNameValidator(QObject *parent)
  :QValidator(parent)
  {
    setObjectName("UASiteNameValidator");
  }

  State validate(QString &input, int &) const
  {
    if (input.isEmpty())
      return Intermediate;

    if (input.startsWith(QChar('.')))
      return Invalid;

    const int length = input.length();

    for(int i = 0 ; i < length; i++)
    {
      if (!input[i].isLetterOrNumber() && input[i] != '.' && input[i] != '-')
        return Invalid;
    }

    return Acceptable;
  }
};


UserAgentConfigDlg::UserAgentConfigDlg( const QString& caption, UserAgentInfo* info,
                              QWidget *parent, Qt::WindowFlags f )
              :QDialog(parent, f),
               m_userAgentInfo(info)
{
  ui.setupUi(this);

  setModal( true );
  setWindowTitle ( caption );

  if (!m_userAgentInfo)
  {
    setEnabled( false );
    return;
  }

  init();
}

UserAgentConfigDlg::~UserAgentConfigDlg()
{
}

void UserAgentConfigDlg::init()
{
  ui.aliasComboBox->clear();
  ui.aliasComboBox->addItems( m_userAgentInfo->userAgentAliasList() );
  ui.aliasComboBox->insertItem( 0, "" );
  ui.aliasComboBox->model()->sort( 0 );
  ui.aliasComboBox->setCurrentIndex( 0 );
  ui.siteLineEdit->setFocus();
}

void UserAgentConfigDlg::on_aliasComboBox_activated( const QString& text )
{
  if ( text.isEmpty() )
    ui.identityLineEdit->setText( QString() );
  else
    ui.identityLineEdit->setText( m_userAgentInfo->agentStr(text) );

  const bool enable =  (!ui.siteLineEdit->text().isEmpty() && !text.isEmpty());
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void UserAgentConfigDlg::on_siteLineEdit_textChanged( const QString& text )
{
  const bool enable = (!text.isEmpty() && !ui.aliasComboBox->currentText().isEmpty());
  ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void UserAgentConfigDlg::setSiteName( const QString& text )
{
  ui.siteLineEdit->setText( text );
}

void UserAgentConfigDlg::setIdentity( const QString& text )
{
  int id = ui.aliasComboBox->findText( text );
  if ( id != -1 )
     ui.aliasComboBox->setCurrentIndex( id );
  on_aliasComboBox_activated( ui.aliasComboBox->currentText() );
  if ( !ui.siteLineEdit->isEnabled() )
    ui.aliasComboBox->setFocus();
}

QString UserAgentConfigDlg::siteName()
{
  QString site_name=ui.siteLineEdit->text().toLower();
  site_name = site_name.remove( "https://" );
  site_name = site_name.remove( "http://" );
  return site_name;
}

QString UserAgentConfigDlg::identity()
{
  return ui.aliasComboBox->currentText();
}

QString UserAgentConfigDlg::alias()
{
  return ui.identityLineEdit->text();
}

#include "uagentproviderdlg.moc"
