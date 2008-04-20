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

#include <kdeversion.h>
#include <kuniqueapplication.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kuser.h>
#include <kdebug.h>

#include "passwd.h"
#include "passwddlg.h"


int main(int argc, char **argv)
{
    KAboutData aboutData("kdepasswd", 0, ki18n("KDE passwd"),
            KDE_VERSION_STRING, ki18n("Changes a UNIX password."),
            KAboutData::License_Artistic, ki18n("Copyright (c) 2000 Geert Jansen"));
    aboutData.addAuthor(ki18n("Geert Jansen"), ki18n("Maintainer"),
            "jansen@kde.org");
    aboutData.setProgramIconName( "preferences-desktop-user-password" );

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("+[user]", ki18n("Change password of this user"));
    KCmdLineArgs::addCmdLineOptions(options);
    KUniqueApplication::addCmdLineOptions();


    if (!KUniqueApplication::start()) {
        kDebug() << "kdepasswd is already running";
        return 0;
    }

    KUniqueApplication app;

    KUser ku;
    QString user;
    bool bRoot = ku.isSuperUser();
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (args->count())
        user = args->arg(0);

    /* You must be able to run "kdepasswd loginName" */
    if ( !user.isEmpty() && user!=KUser().loginName() && !bRoot)
    {
        KMessageBox::sorry(0, i18n("You need to be root to change the password of other users."));
        return 0;
    }

    QByteArray oldpass;
    if (!bRoot)
    {
        int result = KDEpasswd1Dialog::getPassword(oldpass);
        if (result != KDEpasswd1Dialog::Accepted)
            return 0;
    }

    KDEpasswd2Dialog *dlg = new KDEpasswd2Dialog(oldpass, user.toLocal8Bit());


    dlg->exec();

    return 0;
}

