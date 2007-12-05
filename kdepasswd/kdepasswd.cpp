/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
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
            "jansen@kde.org", "http://www.stack.nl/~geertj/");
 
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
    QApplication::setWindowIcon( KIcon( "preferences-desktop-user-password" ) );

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

