//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002 David Faure <faure@kde.org>
//  Copyright (C) 2005 Brad Hards <bradh@frogmouth.net>
//  Copyright (C) 2008 by Dmitry Suzdalev <dimsuz@gmail.com>
//  Copyright (C) 2011 Kai Uwe Broulik <kde@privat.broulik.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <QDate>
#include <kdebug.h>
#include "widgets.h"

#include <kmessagebox.h>
#include <kapplication.h>
#include <kpassivepopup.h>
#include <krecentdocument.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kicondialog.h>
#include <kdirselectdialog.h>
#include <kcolordialog.h>
#include <kwindowsystem.h>
#include <kiconloader.h>
#include <klocale.h>

#include <QtCore/QTimer>
#include <QDesktopWidget>

#include <iostream>

#if defined Q_WS_X11 && ! defined K_WS_QTONLY
#include <netwm.h>
#endif

#include "../config-apps.h"
#ifdef QT_QTDBUS_FOUND
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>
#endif

#ifdef Q_WS_WIN
#include <QFileDialog>
#endif
#include <unistd.h>

using namespace std;

// this class hooks into the eventloop and outputs the id
// of shown dialogs or makes the dialog transient for other winids.
// Will destroy itself on app exit.
class WinIdEmbedder: public QObject
{
public:
    WinIdEmbedder(bool printID, WId winId):
        QObject(qApp), print(printID), id(winId)
    {
        if (qApp)
            qApp->installEventFilter(this);
    }
protected:
    bool eventFilter(QObject *o, QEvent *e);
private:
    bool print;
    WId id;
};

bool WinIdEmbedder::eventFilter(QObject *o, QEvent *e)
{
    if (e->type() == QEvent::Show && o->isWidgetType()
        && o->inherits("KDialog"))
    {
        QWidget *w = static_cast<QWidget*>(o);
        if (print)
            cout << "winId: " << w->winId() << endl;
        if (id)
            KWindowSystem::setMainWindow(w, id);
        deleteLater(); // WinIdEmbedder is not needed anymore after the first dialog was shown
        return false;
    }
    return QObject::eventFilter(o, e);
}

/**
 * Display a passive notification popup using the D-Bus interface, if possible.
 * @return true if the notification was successfully sent, false otherwise.
 */
bool sendVisualNotification(const QString &text, const QString &title, const QString &icon, int timeout)
{
#ifdef QT_QTDBUS_FOUND
  const QString dbusServiceName = "org.freedesktop.Notifications";
  const QString dbusInterfaceName = "org.freedesktop.Notifications";
  const QString dbusPath = "/org/freedesktop/Notifications";

  // check if service already exists on plugin instantiation
  QDBusConnectionInterface* interface = QDBusConnection::sessionBus().interface();

  if (!interface || !interface->isServiceRegistered(dbusServiceName)) {
    //kDebug() << dbusServiceName << "D-Bus service not registered";
    return false;
  }

  if (timeout == 0)
    timeout = 10 * 1000;

  QDBusMessage m = QDBusMessage::createMethodCall(dbusServiceName, dbusPath, dbusInterfaceName, "Notify");
  QList<QVariant> args;

  args.append("kdialog"); // app_name
  args.append(0U); // replaces_id
  args.append(icon); // app_icon
  args.append(title); // summary
  args.append(text); // body
  args.append(QStringList()); // actions - unused for plain passive popups
  args.append(QVariantMap()); // hints - unused atm
  args.append(timeout); // expire timout

  m.setArguments(args);

  QDBusMessage replyMsg = QDBusConnection::sessionBus().call(m);
  if(replyMsg.type() == QDBusMessage::ReplyMessage) {
    if (!replyMsg.arguments().isEmpty()) {
      return true;
    }
    // Not displaying any error messages as this is optional for kdialog
    // and KPassivePopup is a perfectly valid fallback.
    //else {
    //  kDebug() << "Error: received reply with no arguments.";
    //}
  } else if (replyMsg.type() == QDBusMessage::ErrorMessage) {
    //kDebug() << "Error: failed to send D-Bus message";
    //kDebug() << replyMsg;
  } else {
    //kDebug() << "Unexpected reply type";
  }
#endif
  return false;
}

static void outputStringList(const QStringList &list, bool separateOutput)
{
    if ( separateOutput) {
	for ( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
	    cout << (*it).toLocal8Bit().data() << endl;
	}
    } else {
	for ( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
	    cout << (*it).toLocal8Bit().data() << " ";
	}
	cout << endl;
    }
}


KGuiItem configuredYes(const QString &text)
{
  return KGuiItem( text, "dialog-ok" );
}

KGuiItem configuredNo(const QString &text)
{
  return KGuiItem( text, "process-stop" );
}

KGuiItem configuredCancel(const QString &text)
{
  return KGuiItem( text, "dialog-cancel" );
}

KGuiItem configuredContinue(const QString &text)
{
  return KGuiItem( text, "arrow-right" );
}

static int directCommand(KCmdLineArgs *args)
{
    QString title;
    bool separateOutput = false;
    bool printWId = args->isSet("print-winid");
    QString defaultEntry;

    // --title text
    KCmdLineArgs *qtargs = KCmdLineArgs::parsedArgs("qt"); // --title is a qt option
    if(qtargs->isSet("title")) {
      title = qtargs->getOption("title");
    }

    // --separate-output
    if (args->isSet("separate-output"))
    {
      separateOutput = true;
    }

    WId winid = 0;
    bool attach = args->isSet("attach");
    if(attach) {
#ifdef Q_WS_WIN
        winid = reinterpret_cast<WId>(args->getOption("attach").toLong(&attach, 0));  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#else
        winid = args->getOption("attach").toLong(&attach, 0);  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#endif
    } else if(args->isSet("embed")) {
        /* KDialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
         * For consistancy, we silently map --embed to --attach */
        attach = true;
#ifdef Q_WS_WIN
        winid = reinterpret_cast<WId>(args->getOption("embed").toLong(&attach, 0));  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#else
        winid = args->getOption("embed").toLong(&attach, 0);  //C style parsing.  If the string begins with "0x", base 16 is used; if the string begins with "0", base 8 is used; otherwise, base 10 is used.
#endif
    }

    if (printWId || attach)
    {
      (void)new WinIdEmbedder(printWId, winid);
    }

    // button labels
    // Initialize with default labels
    KGuiItem yesButton = KStandardGuiItem::yes();
    KGuiItem noButton = KStandardGuiItem::no();
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    KGuiItem continueButton = KStandardGuiItem::cont();

    // Customize the asked labels
    if (args->isSet("yes-label")) {
          yesButton = configuredYes( args->getOption("yes-label") );
    }
    if (args->isSet("no-label")) {
        noButton = configuredNo( args->getOption("no-label") );
    }
    if (args->isSet("cancel-label")) {
        cancelButton = configuredCancel( args->getOption("cancel-label") );
    }
    if (args->isSet("continue-label")) {
        continueButton = configuredContinue( args->getOption("continue-label") );
    }

    // --yesno and other message boxes
    KMessageBox::DialogType type = (KMessageBox::DialogType) 0;
    QByteArray option;
    if (args->isSet("yesno")) {
        option = "yesno";
        type = KMessageBox::QuestionYesNo;
    }
    else if (args->isSet("yesnocancel")) {
        option = "yesnocancel";
        type = KMessageBox::QuestionYesNoCancel;
    }
    else if (args->isSet("warningyesno")) {
        option = "warningyesno";
        type = KMessageBox::WarningYesNo;
    }
    else if (args->isSet("warningcontinuecancel")) {
        option = "warningcontinuecancel";
        type = KMessageBox::WarningContinueCancel;
    }
    else if (args->isSet("warningyesnocancel")) {
        option = "warningyesnocancel";
        type = KMessageBox::WarningYesNoCancel;
    }
    else if (args->isSet("sorry")) {
        option = "sorry";
        type = KMessageBox::Sorry;
    }
    else if (args->isSet("error")) {
        option = "error";
        type = KMessageBox::Error;
    }
    else if (args->isSet("msgbox")) {
        option = "msgbox";
        type = KMessageBox::Information;
    }

    if ( !option.isEmpty() )
    {
        KConfig* dontagaincfg = NULL;
        // --dontagain
        QString dontagain; // QString()
        if (args->isSet("dontagain"))
        {
          QString value = args->getOption("dontagain");
          QStringList values = value.split( ':', QString::SkipEmptyParts );
          if( values.count() == 2 )
          {
            dontagaincfg = new KConfig( values[ 0 ] );
            KMessageBox::setDontShowAskAgainConfig( dontagaincfg );
            dontagain = values[ 1 ];
          }
          else
            qDebug( "Incorrect --dontagain!" );
        }
        int ret;

        QString text = Widgets::parseString(args->getOption(option));

        if ( type == KMessageBox::WarningContinueCancel ) {
            ret = KMessageBox::messageBox( 0, type, text, title, continueButton,
                noButton, cancelButton, dontagain );
        } else {
            ret = KMessageBox::messageBox( 0, type, text, title,
                yesButton, noButton, cancelButton, dontagain );
        }
        delete dontagaincfg;
        // ret is 1 for Ok, 2 for Cancel, 3 for Yes, 4 for No and 5 for Continue.
        // We want to return 0 for ok, yes and continue, 1 for no and 2 for cancel
        return (ret == KMessageBox::Ok || ret == KMessageBox::Yes || ret == KMessageBox::Continue) ? 0
                     : ( ret == KMessageBox::No ? 1 : 2 );
    }

    // --inputbox text [init]
    if (args->isSet("inputbox"))
    {
      QString result;
      QString init;

      if (args->count() > 0)
          init = args->arg(0);

      const bool retcode = Widgets::inputBox(0, title, args->getOption("inputbox"), init, result);
      cout << result.toLocal8Bit().data() << endl;
      return retcode ? 0 : 1;
    }


    // --password text
    if (args->isSet("password"))
    {
      QString result;
      const bool retcode = Widgets::passwordBox(0, title, args->getOption("password"), result);
      cout << qPrintable(result) << endl;
      return retcode ? 0 : 1;
    }

    // --passivepopup
    if (args->isSet("passivepopup"))
    {
        int timeout = 0;
        if (args->count() > 0) {
            timeout = 1000 * args->arg(0).toInt();
        }

        if (timeout < 0) {
            timeout = -1;
        }

	// since --icon is a kde option, we need to parse the kde options here as well
	KCmdLineArgs *kdeargs = KCmdLineArgs::parsedArgs("kde");

	// Use --icon parameter for passivepopup as well
	QString icon;
	if (kdeargs->isSet("icon")) {
	  icon = kdeargs->getOption("icon");
	} else {
	  icon = "dialog-information";	// Use generic (i)-icon if none specified
	}

        // try to use more stylish notifications
        if (sendVisualNotification(args->getOption("passivepopup"), title, icon, timeout))
          return 0;

        // ...did not work, use KPassivePopup as fallback

        // parse timeout time again, so it does not auto-close the fallback (timer cannot handle -1 time)
        if (args->count() > 0) {
            timeout = 1000 * args->arg(0).toInt();
        }
        if (timeout <= 0) {
            timeout = 10*1000;    // 10 seconds should be a decent time for auto-closing (you can override this using a parameter)
        }

        QPixmap passiveicon;
        if (kdeargs->isSet("icon")) {  // Only show icon if explicitly requested
            passiveicon = KIconLoader::global()->loadIcon(icon, KIconLoader::Dialog);
        }
        KPassivePopup *popup = KPassivePopup::message( KPassivePopup::Boxed, // style
                                                       title,
                                                       args->getOption("passivepopup"),
                                                       passiveicon,
                                                       (QWidget*)0UL, // parent
                                                       timeout );
        KDialog::centerOnScreen( popup );
        QTimer *timer = new QTimer();
        QObject::connect( timer, SIGNAL(timeout()), qApp, SLOT(quit()) );
        QObject::connect( popup, SIGNAL(clicked()), qApp, SLOT(quit()) );
        timer->setSingleShot( true );
        timer->start( timeout );

#ifdef Q_WS_X11
	QString geometry;
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs("kde");
	if (args && args->isSet("geometry"))
		geometry = args->getOption("geometry");
	if ( !geometry.isEmpty()) {
	    int x, y;
	    int w, h;
	    int m = XParseGeometry( geometry.toLatin1(), &x, &y, (unsigned int*)&w, (unsigned int*)&h);
	    if ( (m & XNegative) )
		x = KApplication::desktop()->width()  + x - w;
	    if ( (m & YNegative) )
		y = KApplication::desktop()->height() + y - h;
	    popup->setAnchor( QPoint(x, y) );
	}
#endif
	qApp->exec();
	return 0;
      }

    // --textbox file [width] [height]
    if (args->isSet("textbox"))
    {
        int w = 0;
        int h = 0;

        if (args->count() == 2) {
            w = args->arg(0).toInt();
            h = args->arg(1).toInt();
        }

        return Widgets::textBox(0, w, h, title, args->getOption("textbox"));
    }

    // --textinputbox file [width] [height]
    if (args->isSet("textinputbox"))
    {
      int w = 400;
      int h = 200;

      if (args->count() >= 3) {
	w = args->arg(1).toInt();
	h = args->arg(2).toInt();
      }

      QString init;
      if (args->count() >= 1) {
	  init = args->arg(0);
      }

      QString result;
      int ret = Widgets::textInputBox(0, w, h, title, args->getOption("textinputbox"), init, result);
      cout << qPrintable(result) << endl;
      return ret;
    }

    // --combobox <text> item [item] ..."
    if (args->isSet("combobox")) {
        QStringList list;
        if (args->count() >= 1) {
            for (int i = 0; i < args->count(); i++) {
                list.append(args->arg(i));
            }
            const QString text = args->getOption("combobox");
	    if (args->isSet("default")) {
	        defaultEntry = args->getOption("default");
	    }
            QString result;
	    const bool retcode = Widgets::comboBox(0, title, text, list, defaultEntry, result);
            cout << result.toLocal8Bit().data() << endl;
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --menu text [tag item] [tag item] ...
    if (args->isSet("menu")) {
        QStringList list;
        if (args->count() >= 2) {
            for (int i = 0; i < args->count(); i++) {
                list.append(args->arg(i));
            }
            const QString text = args->getOption("menu");
	    if (args->isSet("default")) {
	        defaultEntry = args->getOption("default");
	    }
            QString result;
            const bool retcode = Widgets::listBox(0, title, text, list, defaultEntry, result);
            if (1 == retcode) { // OK was selected
	        cout << result.toLocal8Bit().data() << endl;
	    }
            return retcode ? 0 : 1;
        }
        return -1;
    }

    // --checklist text [tag item status] [tag item status] ...
    if (args->isSet("checklist")) {
        QStringList list;
        if (args->count() >= 3) {
            for (int i = 0; i < args->count(); i++) {
                list.append(args->arg(i));
            }

            const QString text = args->getOption("checklist");
            QStringList result;

            const bool retcode = Widgets::checkList(0, title, text, list, separateOutput, result);

            for (int i=0; i<result.count(); i++)
                if (!result.at(i).toLocal8Bit().isEmpty()) {
		    cout << result.at(i).toLocal8Bit().data() << endl;
		}
            exit( retcode ? 0 : 1 );
        }
        return -1;
    }

    // --radiolist text width height menuheight [tag item status]
    if (args->isSet("radiolist")) {
        QStringList list;
        if (args->count() >= 3) {
            for (int i = 0; i < args->count(); i++) {
                list.append(args->arg(i));
            }

            const QString text = args->getOption("radiolist");
            QString result;
            const bool retcode = Widgets::radioBox(0, title, text, list, result);
            cout << result.toLocal8Bit().data() << endl;
            exit( retcode ? 0 : 1 );
        }
        return -1;
    }

    // getopenfilename [startDir] [filter]
    if (args->isSet("getopenfilename")) {
        QString startDir;
        QString filter;
        startDir = args->getOption("getopenfilename");
        if (args->count() >= 1)  {
            filter = args->arg(0);
        }
	KFileDialog dlg( startDir, filter, 0 );
	dlg.setOperationMode( KFileDialog::Opening );

	if (args->isSet("multiple")) {
	    dlg.setMode(KFile::Files | KFile::LocalOnly);
	} else {
	    dlg.setMode(KFile::File | KFile::LocalOnly);
	}
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isEmpty() ? i18nc("@title:window", "Open") : title);
	dlg.exec();

        if (args->isSet("multiple")) {
	    QStringList result = dlg.selectedFiles();
	    if ( !result.isEmpty() ) {
		outputStringList( result, separateOutput );
		return 0;
	    }
	} else {
	    QString result = dlg.selectedFile();
	    if (!result.isEmpty())  {
		cout << result.toLocal8Bit().data() << endl;
		return 0;
	    }
	}
        return 1; // canceled
    }


    // getsaveurl [startDir] [filter]
    // getsavefilename [startDir] [filter]
    if ( (args->isSet("getsavefilename") ) || (args->isSet("getsaveurl") ) ) {
        QString startDir;
        QString filter;
	if ( args->isSet("getsavefilename") ) {
	    startDir = args->getOption("getsavefilename");
	} else {
	    startDir = args->getOption("getsaveurl");
	}
        if (args->count() >= 1)  {
            filter = args->arg(0);
        }
	// copied from KFileDialog::getSaveFileName(), so we can add geometry
	bool specialDir = ( startDir.at(0) == ':' );
	if ( !specialDir ) {
	    KFileItem kfi(KFileItem::Unknown, KFileItem::Unknown, KUrl(startDir));
	    specialDir = kfi.isDir();
	}
	KFileDialog dlg( specialDir ? startDir : QString(), filter, 0 );
	if ( !specialDir )
	    dlg.setSelection( startDir );
	dlg.setOperationMode( KFileDialog::Saving );
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isEmpty() ? i18nc("@title:window", "Save As") : title);
	dlg.exec();

	if ( args->isSet("getsaveurl") ) {
	    KUrl result = dlg.selectedUrl();
	    if ( result.isValid())  {

		cout << result.url().toLocal8Bit().data() << endl;
		return 0;
	    }
	} else { // getsavefilename
	    QString result = dlg.selectedFile();
	    if (!result.isEmpty())  {
		KRecentDocument::add(result);
		cout << result.toLocal8Bit().data() << endl;
		return 0;
	    }
	}
        return 1; // canceled
    }

    // getexistingdirectory [startDir]
    if (args->isSet("getexistingdirectory")) {
        QString startDir;
        startDir = args->getOption("getexistingdirectory");
	QString result;
#ifdef Q_WS_WIN
	result = QFileDialog::getExistingDirectory( 0, title, startDir,
	                                            QFileDialog::DontResolveSymlinks |
	                                            QFileDialog::ShowDirsOnly);
#else
	KUrl url;
	KDirSelectDialog myDialog( startDir, true, 0 );

	kapp->setTopWidget( &myDialog );

	Widgets::handleXGeometry(&myDialog);
	if ( !title.isEmpty() )
	    myDialog.setCaption( title );

	if ( myDialog.exec() == QDialog::Accepted )
	    url =  myDialog.url();

	if ( url.isValid() )
	    result = url.path();
#endif
        if (!result.isEmpty())  {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // getopenurl [startDir] [filter]
    if (args->isSet("getopenurl")) {
        QString startDir;
        QString filter;
        startDir = args->getOption("getopenurl");
        if (args->count() >= 1)  {
            filter = args->arg(0);
        }
	KFileDialog dlg( startDir, filter, 0 );
	dlg.setOperationMode( KFileDialog::Opening );

	if (args->isSet("multiple")) {
	    dlg.setMode(KFile::Files);
	} else {
	    dlg.setMode(KFile::File);
	}
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isEmpty() ? i18nc("@title:window", "Open") : title);
	dlg.exec();

        if (args->isSet("multiple")) {
	    KUrl::List result = dlg.selectedUrls();
	    if ( !result.isEmpty() ) {
		outputStringList( result.toStringList(), separateOutput );
		return 0;
	    }
	} else {
	    KUrl result = dlg.selectedUrl();
	    if (!result.isEmpty())  {
		cout << result.url().toLocal8Bit().data() << endl;
		return 0;
	    }
	}
        return 1; // canceled
    }

    // geticon [group] [context]
    if (args->isSet("geticon")) {
        QString groupStr, contextStr;
        groupStr = args->getOption("geticon");
        if (args->count() >= 1)  {
            contextStr = args->arg(0);
        }
        const KIconLoader::Group group =
            ( groupStr == QLatin1String( "Desktop" ) ) ?     KIconLoader::Desktop :
            ( groupStr == QLatin1String( "Toolbar" ) ) ?     KIconLoader::Toolbar :
            ( groupStr == QLatin1String( "MainToolbar" ) ) ? KIconLoader::MainToolbar :
            ( groupStr == QLatin1String( "Small" ) ) ?       KIconLoader::Small :
            ( groupStr == QLatin1String( "Panel" ) ) ?       KIconLoader::Panel :
            ( groupStr == QLatin1String( "Dialog" ) ) ?      KIconLoader::Dialog :
            ( groupStr == QLatin1String( "User" ) ) ?        KIconLoader::User :
            /* else */                                       KIconLoader::NoGroup;

        const KIconLoader::Context context =
            ( contextStr == QLatin1String( "Action" ) ) ?        KIconLoader::Action :
            ( contextStr == QLatin1String( "Application" ) ) ?   KIconLoader::Application :
            ( contextStr == QLatin1String( "Device" ) ) ?        KIconLoader::Device :
            ( contextStr == QLatin1String( "FileSystem" ) ) ?    KIconLoader::FileSystem :
            ( contextStr == QLatin1String( "MimeType" ) ) ?      KIconLoader::MimeType :
            ( contextStr == QLatin1String( "Animation" ) ) ?     KIconLoader::Animation :
            ( contextStr == QLatin1String( "Category" ) ) ?      KIconLoader::Category :
            ( contextStr == QLatin1String( "Emblem" ) ) ?        KIconLoader::Emblem :
            ( contextStr == QLatin1String( "Emote" ) ) ?         KIconLoader::Emote :
            ( contextStr == QLatin1String( "International" ) ) ? KIconLoader::International :
            ( contextStr == QLatin1String( "Place" ) ) ?         KIconLoader::Place :
            ( contextStr == QLatin1String( "StatusIcon" ) ) ?    KIconLoader::StatusIcon :
            // begin: KDE3 compatibility (useful?)
            ( contextStr == QLatin1String( "Devices" ) ) ?       KIconLoader::Device :
            ( contextStr == QLatin1String( "MimeTypes" ) ) ?     KIconLoader::MimeType :
            ( contextStr == QLatin1String( "FileSystems" ) ) ?   KIconLoader::FileSystem :
            ( contextStr == QLatin1String( "Applications" ) ) ?  KIconLoader::Application :
            ( contextStr == QLatin1String( "Actions" ) ) ?       KIconLoader::Action :
            // end: KDE3 compatibility
            /* else */                                           KIconLoader::Any;

	KIconDialog dlg((QWidget*)0L);
	kapp->setTopWidget( &dlg );
	dlg.setup( group, context);
        dlg.setIconSize(KIconLoader::SizeHuge);

	if (!title.isEmpty())
	    dlg.setCaption(title);

	Widgets::handleXGeometry(&dlg);

	QString result = dlg.openDialog();

        if (!result.isEmpty())  {
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // canceled
    }

    // --progressbar text totalsteps
    if (args->isSet("progressbar"))
    {
       cout << "org.kde.kdialog-" << getpid() << " /ProgressDialog" << endl;
       if (fork())
           _exit(0);
       close(1);

       int totalsteps = 100;
       const QString text = args->getOption("progressbar");

       if (args->count() == 1)
           totalsteps = args->arg(0).toInt();

       return Widgets::progressBar(0, title, text, totalsteps) ? 1 : 0;
    }

    // --getcolor
    if (args->isSet("getcolor")) {
        KColorDialog dlg((QWidget*)0L, true);

        if (args->isSet("default")) {
            defaultEntry = args->getOption("default");
            dlg.setColor(defaultEntry);
        }
        Widgets::handleXGeometry(&dlg);
        kapp->setTopWidget(&dlg);
        dlg.setCaption(title.isEmpty() ? i18nc("@title:window", "Choose Color") : title);

        if (dlg.exec() == KColorDialog::Accepted) {
            QString result;
            if (dlg.color().isValid()) {
                result = dlg.color().name();
            } else {
                result = dlg.defaultColor().name();
            }
            cout << result.toLocal8Bit().data() << endl;
            return 0;
        }
        return 1; // cancelled
    }
    if (args->isSet("slider"))
    {
       int miniValue = 0;
       int maxValue = 0;
       int step = 0;
       const QString text = args->getOption( "slider" );
       if ( args->count() == 3 )
       {
           miniValue = args->arg(0).toInt();
           maxValue = args->arg( 1 ).toInt();
           step = args->arg( 2 ).toInt();
       }
       int result = 0;

       const bool returnCode = Widgets::slider(0, title, text, miniValue, maxValue, step, result);
       if ( returnCode )
           cout << result << endl;
       return returnCode;
    }
    if (args->isSet("calendar"))
    {
       const QString text = args->getOption( "calendar" );
       QDate result;

       const bool returnCode = Widgets::calendar(0, title, text, result);
       if ( returnCode )
           cout << result.toString().toLocal8Bit().data() << endl;
       return returnCode;
    }

    KCmdLineArgs::usage();
    return -2; // NOTREACHED
}


int main(int argc, char *argv[])
{
  KAboutData aboutData( "kdialog", 0, ki18n("KDialog"),
                        "1.0", ki18n( "KDialog can be used to show nice dialog boxes from shell scripts" ),
			KAboutData::License_GPL,
                        ki18n("(C) 2000, Nick Thompson"));
  aboutData.addAuthor(ki18n("David Faure"), ki18n("Current maintainer"),"faure@kde.org");
  aboutData.addAuthor(ki18n("Brad Hards"), KLocalizedString(), "bradh@frogmouth.net");
  aboutData.addAuthor(ki18n("Nick Thompson"),KLocalizedString(), 0/*"nickthompson@lucent.com" bounces*/);
  aboutData.addAuthor(ki18n("Matthias Hölzer"),KLocalizedString(),"hoelzer@kde.org");
  aboutData.addAuthor(ki18n("David Gümbel"),KLocalizedString(),"david.guembel@gmx.net");
  aboutData.addAuthor(ki18n("Richard Moore"),KLocalizedString(),"rich@kde.org");
  aboutData.addAuthor(ki18n("Dawit Alemayehu"),KLocalizedString(),"adawit@kde.org");
  aboutData.addAuthor(ki18n("Kai Uwe Broulik"),KLocalizedString(),"kde@privat.broulik.de");
  aboutData.setProgramIconName("system-run");

  KCmdLineArgs::init(argc, argv, &aboutData);

  KCmdLineOptions options;
  options.add("yesno <text>", ki18n("Question message box with yes/no buttons"));
  options.add("yesnocancel <text>", ki18n("Question message box with yes/no/cancel buttons"));
  options.add("warningyesno <text>", ki18n("Warning message box with yes/no buttons"));
  options.add("warningcontinuecancel <text>", ki18n("Warning message box with continue/cancel buttons"));
  options.add("warningyesnocancel <text>", ki18n("Warning message box with yes/no/cancel buttons"));
  options.add("yes-label <text>", ki18n("Use text as Yes button label"));
  options.add("no-label <text>", ki18n("Use text as No button label"));
  options.add("cancel-label <text>", ki18n("Use text as Cancel button label"));
  options.add("continue-label <text>", ki18n("Use text as Continue button label"));
  options.add("sorry <text>", ki18n("'Sorry' message box"));
  options.add("error <text>", ki18n("'Error' message box"));
  options.add("msgbox <text>", ki18n("Message Box dialog"));
  options.add("inputbox <text> <init>", ki18n("Input Box dialog"));
  options.add("password <text>", ki18n("Password dialog"));
  options.add("textbox <file> [width] [height]", ki18n("Text Box dialog"));
  options.add("textinputbox <text> <init> [width] [height]", ki18n("Text Input Box dialog"));
  options.add("combobox <text> item [item] [item] ...", ki18n("ComboBox dialog"));
  options.add("menu <text> [tag item] [tag item] ...", ki18n("Menu dialog"));
  options.add("checklist <text> [tag item status] ...", ki18n("Check List dialog"));
  options.add("radiolist <text> [tag item status] ...", ki18n("Radio List dialog"));
  options.add("passivepopup <text> <timeout>", ki18n("Passive Popup"));
  options.add("getopenfilename [startDir] [filter]", ki18n("File dialog to open an existing file"));
  options.add("getsavefilename [startDir] [filter]", ki18n("File dialog to save a file"));
  options.add("getexistingdirectory [startDir]", ki18n("File dialog to select an existing directory"));
  options.add("getopenurl [startDir] [filter]", ki18n("File dialog to open an existing URL"));
  options.add("getsaveurl [startDir] [filter]", ki18n("File dialog to save a URL"));
  options.add("geticon [group] [context]", ki18n("Icon chooser dialog"));
  options.add("progressbar <text> [totalsteps]", ki18n("Progress bar dialog, returns a D-Bus reference for communication"));
  options.add("getcolor", ki18n("Color dialog to select a color"));
  // TODO gauge stuff, reading values from stdin
  options.add("title <text>", ki18n("Dialog title"));
  options.add("default <text>", ki18n("Default entry to use for combobox, menu and color"));
  options.add("multiple", ki18n("Allows the --getopenurl and --getopenfilename options to return multiple files"));
  options.add("separate-output", ki18n("Return list items on separate lines (for checklist option and file open with --multiple)"));
  options.add("print-winid", ki18n("Outputs the winId of each dialog"));
  options.add("dontagain <file:entry>", ki18n("Config file and option name for saving the \"do-not-show/ask-again\" state"));
  options.add( "slider <text> [minvalue] [maxvalue] [step]", ki18n( "Slider dialog box, returns selected value" ) );
  options.add( "calendar <text>", ki18n( "Calendar dialog box, returns selected date" ) );
  /* kdialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
     * For backwards compatibility, we silently map --embed to --attach */
  options.add("attach <winid>", ki18n("Makes the dialog transient for an X app specified by winid"));
  options.add("embed <winid>");

  options.add("+[arg]", ki18n("Arguments - depending on main option"));

  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  // execute direct kdialog command
  return directCommand(args);
}

