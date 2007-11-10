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
#include "useragentconfigdlg.h"
#include "ui_useragentconfigdlg.h"

// Local
#include "useragentinfo.h"

// Qt
#include <QtGui/QBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QValidator>
#include <QtGui/QWidget>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurllabel.h>


class UserAgentConfigWidget : public QWidget, public Ui::UserAgentConfigDlgUI
{
public:
  UserAgentConfigWidget(QWidget * parent = 0, Qt::WindowFlags f = 0)
  :QWidget(parent, f)
  {
    setupUi(this);
  }
};

class UserAgentSiteNameValidator : public QValidator
{
public:
  UserAgentSiteNameValidator(QObject *parent)
  :QValidator(parent)
  {
    setObjectName("UserAgentSiteNameValidator");
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
                   :KDialog(parent, f),
                    m_userAgentInfo(info)
{
  setModal( true );
  setWindowTitle ( caption );

  m_widget = new UserAgentConfigWidget(this);
  setMainWidget(m_widget);

  if (!m_userAgentInfo)
  {
    setEnabled( false );
    return;
  }

  m_widget->aliasComboBox->clear();
  m_widget->aliasComboBox->addItems( m_userAgentInfo->userAgentAliasList() );
  m_widget->aliasComboBox->insertItem(0, QString());
  m_widget->aliasComboBox->model()->sort(0);
  m_widget->aliasComboBox->setCurrentIndex(0);
  UserAgentSiteNameValidator* validator = new UserAgentSiteNameValidator(this);
  m_widget->siteLineEdit->setValidator(validator);
  m_widget->siteLineEdit->setFocus();

  connect(m_widget->siteLineEdit, SIGNAL(textChanged(const QString&)),
          SLOT(onHostNameChanged(const QString&)));

  connect(m_widget->aliasComboBox, SIGNAL(activated(const QString&)),
          SLOT(onAliasChanged(const QString&)));

  enableButtonOk(false);
}

UserAgentConfigDlg::~UserAgentConfigDlg()
{
}

void UserAgentConfigDlg::onAliasChanged( const QString& text )
{
  if ( text.isEmpty() )
    m_widget->identityLineEdit->setText( QString() );
  else
    m_widget->identityLineEdit->setText( m_userAgentInfo->agentStr(text) );

  const bool enable =  (!m_widget->siteLineEdit->text().isEmpty() && !text.isEmpty());
  enableButtonOk(enable);
}

void UserAgentConfigDlg::onHostNameChanged( const QString& text )
{
  const bool enable = (!text.isEmpty() && !m_widget->aliasComboBox->currentText().isEmpty());
  enableButtonOk(enable);
}

void UserAgentConfigDlg::setSiteName( const QString& text )
{
  m_widget->siteLineEdit->setText( text );
}

void UserAgentConfigDlg::setIdentity( const QString& text )
{
  int id = m_widget->aliasComboBox->findText( text );
  if ( id != -1 )
     m_widget->aliasComboBox->setCurrentIndex( id );
   m_widget->identityLineEdit->setText(m_widget->aliasComboBox->currentText());
  if ( !m_widget->siteLineEdit->isEnabled() )
    m_widget->aliasComboBox->setFocus();
}

QString UserAgentConfigDlg::siteName()
{
  QString site_name = m_widget->siteLineEdit->text().toLower();
  site_name = site_name.remove( "https://" );
  site_name = site_name.remove( "http://" );
  return site_name;
}

QString UserAgentConfigDlg::identity()
{
  return m_widget->aliasComboBox->currentText();
}

QString UserAgentConfigDlg::alias()
{
  return m_widget->identityLineEdit->text();
}

#include "useragentconfigdlg.moc"
