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

#ifndef PASSWDDLG_H
#define PASSWDDLG_H

#include <kpassworddialog.h>
#include <knewpassworddialog.h>
#include <QtCore/QByteRef>

class KDEpasswd1Dialog
    : public KPasswordDialog
{
    Q_OBJECT

public:
    KDEpasswd1Dialog();
    ~KDEpasswd1Dialog();

    static int getPassword(QByteArray &password);

    void accept();
};


class KDEpasswd2Dialog
    : public KNewPasswordDialog
{
    Q_OBJECT

public:
    KDEpasswd2Dialog(const char *oldpass, const QByteArray &user);
    ~KDEpasswd2Dialog();

    void accept();


protected:
    bool checkPassword(const char *password);

private:
    const char *m_Pass;
    QByteArray m_User;
};



#endif // PASSWDDLG_H
