//
//  Copyright (C) 1998 Matthias Hoelzer <hoelzer@kde.org>
//  Copyright (C) 2002 David Faure <faure@kde.org>
//  Copyright (C) 2005 Brad Hards <bradh@frogmouth.net>
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


#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include <QtCore/QFile>
#include <QtCore/QDataStream>
#include <QtCore/QRegExp>
#include <QtCore/QTimer>
#include <QtGui/QFileDialog>
#include <QtGui/QDesktopWidget>
#include <kmessagebox.h>
#include <kapplication.h>
#include <kdebug.h>
//#include <ktopwidget.h>
#include <kxmlguiwindow.h>
#include <kpassivepopup.h>
#include <krecentdocument.h>

#include "widgets.h"

#include <klocale.h>
#include <QtGui/QDialog>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kfiledialog.h>
#include <kicondialog.h>
#include <kdirselectdialog.h>

#if defined Q_WS_X11 && ! defined K_WS_QTONLY
#include <netwm.h>
#endif

using namespace std;

#if defined(Q_WS_X11)
extern "C" { int XSetTransientForHint( Display *, unsigned long, unsigned long ); }
#endif // Q_WS_X11

// this class hooks into the eventloop and outputs the id
// of shown dialogs or makes the dialog transient for other winids.
// Will destroy itself on app exit.
class WinIdEmbedder: public QObject
{
public:
    WinIdEmbedder(bool printID, WId winId):
        QObject(kapp), print(printID), id(winId)
    {
        if (kapp)
            kapp->installEventFilter(this);
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
#ifdef Q_WS_X11
        if (id)
            XSetTransientForHint(w->x11Info().display(), w->winId(), id);
#endif
        deleteLater(); // WinIdEmbedder is not needed anymore after the first dialog was shown
        return false;
    }
    return QObject::eventFilter(o, e);
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

static int directCommand(KCmdLineArgs *args)
{
    QString title;
    bool separateOutput = false;
    bool printWId = args->isSet("print-winid");
    bool embed = args->isSet("embed");
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
    if (printWId || embed)
    {
      WId id = 0;
      if (embed) {
          bool ok;
          long l = QString(args->getOption("embed")).toLong(&ok);
          if (ok)
              id = (WId)l;
      }
      (void)new WinIdEmbedder(printWId, id);
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

        QString text = args->getOption( option );
        int pos;
        while ((pos = text.indexOf( QLatin1String("\\n") )) >= 0)
        {
            text.replace(pos, 2, QLatin1String("\n"));
        }

        if ( type == KMessageBox::WarningContinueCancel ) {
            /* TODO configurable button texts*/
            ret = KMessageBox::messageBox( 0, type, text, title, KStandardGuiItem::cont(),
                KStandardGuiItem::no(), KStandardGuiItem::cancel(), dontagain );
        } else {
            ret = KMessageBox::messageBox( 0, type, text, title /*, TODO configurable button texts*/,
                KStandardGuiItem::yes(), KStandardGuiItem::no(), KStandardGuiItem::cancel(), dontagain );
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

      bool retcode = Widgets::inputBox(0, title, args->getOption("inputbox"), init, result);
      cout << result.toLocal8Bit().data() << endl;
      return retcode ? 0 : 1;
    }


    // --password text
    if (args->isSet("password"))
    {
      QString result;
      bool retcode = Widgets::passwordBox(0, title, args->getOption("password"), result);
      cout << result.data() << endl;
      return retcode ? 0 : 1;
    }

    // --passivepopup
    if (args->isSet("passivepopup"))
      {
        int duration = 0;
        if (args->count() > 0)
            duration = 1000 * args->arg(0).toInt();
        if (duration == 0)
            duration = 10000;
	KPassivePopup *popup = KPassivePopup::message( KPassivePopup::Balloon, // style
						       title,
						       args->getOption("passivepopup"),
						       QPixmap() /* don't crash 0*/, // icon
						       (QWidget*)0UL, // parent
						       duration );
	QTimer *timer = new QTimer();
	QObject::connect( timer, SIGNAL( timeout() ), kapp, SLOT( quit() ) );
	QObject::connect( popup, SIGNAL( clicked() ), kapp, SLOT( quit() ) );
        timer->setSingleShot( true );
	timer->start( duration );

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
	kapp->exec();
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

      if (args->count() == 4) {
	w = args->arg(2).toInt();
	h = args->arg(3).toInt();
      }

      QStringList list;
      list.append(args->getOption("textinputbox"));

      if (args->count() >= 1) {
	for (int i = 0; i < args->count(); i++)
	  list.append(args->arg(i));
      }

      QString result;
      int ret = Widgets::textInputBox(0, w, h, title, list, result);
      cout << result.data() << endl;
      return ret;
    }

    // --combobox <text> [tag item] [tag item] ..."
    if (args->isSet("combobox")) {
        QStringList list;
        if (args->count() >= 2) {
            for (int i = 0; i < args->count(); i++) {
                list.append(args->arg(i));
            }
            QString text = args->getOption("combobox");
	    if (args->isSet("default")) {
	        defaultEntry = args->getOption("default");
	    }
            QString result;
	    bool retcode = Widgets::comboBox(0, title, text, list, defaultEntry, result);
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
            QString text = args->getOption("menu");
	    if (args->isSet("default")) {
	        defaultEntry = args->getOption("default");
	    }
            QString result;
            bool retcode = Widgets::listBox(0, title, text, list, defaultEntry, result);
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

            QString text = args->getOption("checklist");
            QStringList result;

            bool retcode = Widgets::checkList(0, title, text, list, separateOutput, result);

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

            QString text = args->getOption("radiolist");
            QString result;
            bool retcode = Widgets::radioBox(0, title, text, list, result);
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
	dlg.setCaption(title.isNull() ? i18n("Open") : title);
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
	KFileDialog dlg( specialDir ? startDir : QString(), filter, 0 );
	if ( !specialDir )
	    dlg.setSelection( startDir );
	dlg.setOperationMode( KFileDialog::Saving );
	Widgets::handleXGeometry(&dlg);
	kapp->setTopWidget( &dlg );
	dlg.setCaption(title.isNull() ? i18n("Save As") : title);
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
	if ( !title.isNull() )
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
	dlg.setCaption(title.isNull() ? i18n("Open") : title);
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
        K3Icon::Group group = K3Icon::NoGroup;
        if ( groupStr == QLatin1String( "Desktop" ) )
            group = K3Icon::Desktop;
        else if ( groupStr == QLatin1String( "Toolbar" ) )
            group = K3Icon::Toolbar;
        else if ( groupStr == QLatin1String( "MainToolbar" ) )
            group = K3Icon::MainToolbar;
        else if ( groupStr == QLatin1String( "Small" ) )
            group = K3Icon::Small;
        else if ( groupStr == QLatin1String( "Panel" ) )
            group = K3Icon::Panel;
        else if ( groupStr == QLatin1String( "User" ) )
            group = K3Icon::User;
        K3Icon::Context context = K3Icon::Any;
        // From kicontheme.cpp
        if ( contextStr == QLatin1String( "Devices" ) )
            context = K3Icon::Device;
        else if ( contextStr == QLatin1String( "MimeTypes" ) )
            context = K3Icon::MimeType;
        else if ( contextStr == QLatin1String( "FileSystems" ) )
            context = K3Icon::FileSystem;
        else if ( contextStr == QLatin1String( "Applications" ) )
            context = K3Icon::Application;
        else if ( contextStr == QLatin1String( "Actions" ) )
            context = K3Icon::Action;

	KIconDialog dlg((QWidget*)0L);
	kapp->setTopWidget( &dlg );
	dlg.setup( group, context);
	if (!title.isNull())
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
       QString text = args->getOption("progressbar");

       if (args->count() == 1)
           totalsteps = args->arg(0).toInt();

       return Widgets::progressBar(0, title, text, totalsteps) ? 1 : 0;
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

  KCmdLineArgs::init(argc, argv, &aboutData);

  KCmdLineOptions options;
  options.add("yesno <text>", ki18n("Question message box with yes/no buttons"));
  options.add("yesnocancel <text>", ki18n("Question message box with yes/no/cancel buttons"));
  options.add("warningyesno <text>", ki18n("Warning message box with yes/no buttons"));
  options.add("warningcontinuecancel <text>", ki18n("Warning message box with continue/cancel buttons"));
  options.add("warningyesnocancel <text>", ki18n("Warning message box with yes/no/cancel buttons"));
  options.add("sorry <text>", ki18n("'Sorry' message box"));
  options.add("error <text>", ki18n("'Error' message box"));
  options.add("msgbox <text>", ki18n("Message Box dialog"));
  options.add("inputbox <text> <init>", ki18n("Input Box dialog"));
  options.add("password <text>", ki18n("Password dialog"));
  options.add("textbox <file> [width] [height]", ki18n("Text Box dialog"));
  options.add("textinputbox <text> <init> [width] [height]", ki18n("Text Input Box dialog"));
  options.add("combobox <text> [tag item] [tag item] ...", ki18n("ComboBox dialog"));
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
  // TODO gauge stuff, reading values from stdin
  options.add("title <text>", ki18n("Dialog title"));
  options.add("default <text>", ki18n("Default entry to use for combobox and menu"));
  options.add("multiple", ki18n("Allows the --getopenurl and --getopenfilename options to return multiple files"));
  options.add("separate-output", ki18n("Return list items on separate lines (for checklist option and file open with --multiple)"));
  options.add("print-winid", ki18n("Outputs the winId of each dialog"));
  options.add("embed <winid>", ki18n("Makes the dialog transient for an X app specified by winid"));
  options.add("dontagain <file:entry>", ki18n("Config file and option name for saving the \"don't-show/ask-again\" state"));
  options.add("+[arg]", ki18n("Arguments - depending on main option"));
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  // execute direct kdialog command
  return directCommand(args);
}

