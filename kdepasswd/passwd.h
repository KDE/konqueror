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

#ifndef PASSWD_H
#define PASSWD_H

#include <QtCore/QByteRef>
#include <kdesu/process.h>

/**
 * A C++ API to passwd.
 */

class PasswdProcess
    : public KDESu::PtyProcess
{
public:
    PasswdProcess(const QByteArray &user = QByteArray());
    ~PasswdProcess();

    enum Errors { PasswdNotFound=1, PasswordIncorrect, PasswordNotGood };

    int checkCurrent(const char *oldpass);
    int exec(const char *oldpass, const char *newpass, int check=0);

    QByteArray error() { return m_Error; }

private:
    bool isPrompt(const QByteArray &line, const char *word=0L);
    int ConversePasswd(const char *oldpass, const char *newpass,
            int check);

    QByteArray m_User, m_Error;
    bool bOtherUser;
};


#endif // PASSWD_H
