/* vi: ts=8 sts=4 sw=4
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>

 Permission to use, copy, modify, and distribute this software
 and its documentation for any purpose and without fee is hereby
 granted, provided that the above copyright notice appear in all
 copies and that both that the copyright notice and this
 permission notice and warranty disclaimer appear in supporting
 documentation, and that the name of the author not be used in
 advertising or publicity pertaining to distribution of the
 software without specific, written prior permission.

 The author disclaim all warranties with regard to this
 software, including all implied warranties of merchantability
 and fitness.  In no event shall the author be liable for any
 special, indirect or consequential damages or any damages
 whatsoever resulting from loss of use, data or profits, whether
 in an action of contract, negligence or other tortious action,
 arising out of or in connection with the use or performance of
 this software.
 */

#include "passwddlg.h"
#include "passwd.h"

#include <klocale.h>
#include <kmessagebox.h>

KDEpasswd1Dialog::KDEpasswd1Dialog()
    : KPasswordDialog()
{
    setCaption(i18n("Change Password"));
    setPrompt(i18n("Please enter your current password:"));
}


KDEpasswd1Dialog::~KDEpasswd1Dialog()
{
}

void KDEpasswd1Dialog::accept()
{
    PasswdProcess proc(0);

    int ret = proc.checkCurrent(password().toLocal8Bit());
    switch (ret)
    {
    case -1:
    {
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Conversation with 'passwd' failed.") + msg;
	KMessageBox::error(this, msg);
	done(Rejected);
	return;
    }

    case 0:
        return KPasswordDialog::accept();

    case PasswdProcess::PasswdNotFound:
	KMessageBox::error(this, i18n("Could not find the program 'passwd'."));
	done(Rejected);
	return;

    case PasswdProcess::PasswordIncorrect:
        KMessageBox::sorry(this, i18n("Incorrect password. Please try again."));
	return;

    default:
	KMessageBox::error(this, i18n("Internal error: illegal return value "
		"from PasswdProcess::checkCurrent."));
	done(Rejected);
	return;
    }
}


// static
int KDEpasswd1Dialog::getPassword(QByteArray &password)
{
    KDEpasswd1Dialog *dlg = new KDEpasswd1Dialog();
    int res = dlg->exec();
    if (res == Accepted)
	password = dlg->password().toLocal8Bit();
    delete dlg;
    return res;
}



KDEpasswd2Dialog::KDEpasswd2Dialog(const char *oldpass, const QByteArray &user)
    : KNewPasswordDialog()
{
    m_Pass = oldpass;
    m_User = user;

    setCaption(i18n("Change Password"));
    if (m_User.isEmpty())
        setPrompt(i18n("Please enter your new password:"));
    else
        setPrompt(i18n("Please enter the new password for user <b>%1</b>:", QString::fromLocal8Bit(m_User)));
}


KDEpasswd2Dialog::~KDEpasswd2Dialog()
{
}


void  KDEpasswd2Dialog::accept()
{
    PasswdProcess proc(m_User);
    
    QString p=password();

    if (p.length() > 8)
    {
	switch(KMessageBox::warningYesNoCancel(this,
		m_User.isEmpty() ?
		i18n("Your password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is.") :
		i18n("The password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is.")
			,
		i18n("Password Too Long"),
		KGuiItem(i18n("Truncate")),
		KGuiItem(i18n("Use as Is")),
		KStandardGuiItem::cancel(),
		"truncatePassword"))
	{
	case KMessageBox::Yes :
		p=p.left(8);
		break;
	case KMessageBox::No :
		break;
	default : return;
	}
    }

    int ret = proc.exec(m_Pass, p.toLocal8Bit());
    switch (ret)
    {
    case 0:
    {
        hide();
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Your password has been changed.") + msg;
        KMessageBox::information(0L, msg);
        return KNewPasswordDialog::accept();
    }

    case PasswdProcess::PasswordNotGood:
    {
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Your password has not been changed.") + msg;

        // The pw change did not succeed. Print the error.
        KMessageBox::sorry(this, msg);
        return;
    }

    default:
        QString msg = QString::fromLocal8Bit(proc.error());
        if (!msg.isEmpty())
            msg = "<p>\"<i>" + msg + "</i>\"";
        msg = "<qt>" + i18n("Conversation with 'passwd' failed.") + msg;
	KMessageBox::sorry(this, msg);
	done(Rejected);
	return;
    }

    return KNewPasswordDialog::accept();

}


#include "passwddlg.moc"
