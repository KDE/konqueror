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
#include "useragentselectordlg.h"

// Local
#include "useragentinfo.h"

// Qt
#include <QBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QValidator>
#include <QWidget>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurllabel.h>


class UserAgentSiteNameValidator : public QValidator
{
public:
    UserAgentSiteNameValidator (QObject* parent)
        : QValidator (parent)
    {
        setObjectName (QLatin1String ("UserAgentSiteNameValidator"));
    }

    State validate (QString& input, int&) const
    {
        if (input.isEmpty())
            return Intermediate;

        if (input.startsWith (QChar ('.')))
            return Invalid;

        const int length = input.length();

        for (int i = 0 ; i < length; i++) {
            if (!input[i].isLetterOrNumber() && input[i] != '.' && input[i] != '-')
                return Invalid;
        }

        return Acceptable;
    }
};


UserAgentSelectorDlg::UserAgentSelectorDlg (UserAgentInfo* info, QWidget* parent, Qt::WindowFlags f)
    : KDialog (parent, f),
      mUserAgentInfo (info)
{
    mUi.setupUi (mainWidget());

    if (!mUserAgentInfo) {
        setEnabled (false);
        return;
    }

    mUi.aliasComboBox->clear();
    mUi.aliasComboBox->addItems (mUserAgentInfo->userAgentAliasList());
    mUi.aliasComboBox->insertItem (0, QString());
    mUi.aliasComboBox->model()->sort (0);
    mUi.aliasComboBox->setCurrentIndex (0);

    UserAgentSiteNameValidator* validator = new UserAgentSiteNameValidator (this);
    mUi.siteLineEdit->setValidator (validator);
    mUi.siteLineEdit->setFocus();

    connect (mUi.siteLineEdit, SIGNAL (textEdited (QString)),
             SLOT (onHostNameChanged (QString)));
    connect (mUi.aliasComboBox, SIGNAL (activated (QString)),
             SLOT (onAliasChanged (QString)));

    enableButtonOk (false);
}

UserAgentSelectorDlg::~UserAgentSelectorDlg()
{
}

void UserAgentSelectorDlg::onAliasChanged (const QString& text)
{
    if (text.isEmpty())
        mUi.identityLineEdit->setText (QString());
    else
        mUi.identityLineEdit->setText (mUserAgentInfo->agentStr (text));

    const bool enable = (!mUi.siteLineEdit->text().isEmpty() && !text.isEmpty());
    enableButtonOk (enable);
}

void UserAgentSelectorDlg::onHostNameChanged (const QString& text)
{
    const bool enable = (!text.isEmpty() && !mUi.aliasComboBox->currentText().isEmpty());
    enableButtonOk (enable);
}

void UserAgentSelectorDlg::setSiteName (const QString& text)
{
    mUi.siteLineEdit->setText (text);
}

void UserAgentSelectorDlg::setIdentity (const QString& text)
{
    const int id = mUi.aliasComboBox->findText (text);
    if (id != -1)
        mUi.aliasComboBox->setCurrentIndex (id);

    mUi.identityLineEdit->setText (mUserAgentInfo->agentStr (mUi.aliasComboBox->currentText()));
    if (!mUi.siteLineEdit->isEnabled())
        mUi.aliasComboBox->setFocus();
}

QString UserAgentSelectorDlg::siteName()
{
    return mUi.siteLineEdit->text().toLower();
}

QString UserAgentSelectorDlg::identity()
{
    return mUi.aliasComboBox->currentText();
}

QString UserAgentSelectorDlg::alias()
{
    return mUi.identityLineEdit->text();
}

#include "useragentselectordlg.moc"
