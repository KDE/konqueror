/* This file is part of the KDE project
   Copyright (C) 2000, 2007 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 or at your option version 3 as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "keditfiletype.h"
#include "mimetypewriter.h"

// Qt
#include <QtCore/QFile>

// KDE
#include <kapplication.h>
#include <kaboutdata.h>
#include <kbuildsycocaprogressdialog.h>
#include <kdebug.h>
#include <kcmdlineargs.h>
#include <ksycoca.h>
#include <kservicetypeprofile.h>
#include <kstandarddirs.h>
#include <klocale.h>

// Local
#include "filetypedetails.h"
#include "typeslistitem.h"


#if defined Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

FileTypeDialog::FileTypeDialog( MimeTypeData* mime )
  : KDialog( 0 ),
    m_mimeTypeData(mime)
{
  setButtons( Cancel | Apply | Ok );
  showButtonSeparator( false );

  init();
}

FileTypeDialog::~FileTypeDialog()
{
    delete m_details;
}

void FileTypeDialog::init()
{
    m_details = new FileTypeDetails( this );
    m_details->setMimeTypeData( m_mimeTypeData );

    // This code is very similar to kcdialog.cpp
    setMainWidget( m_details );
    connect(m_details, SIGNAL(changed(bool)), this, SLOT(clientChanged(bool)));
    // TODO setHelp()
    enableButton(Apply, false);

    connect( KSycoca::self(), SIGNAL( databaseChanged() ), SLOT( slotDatabaseChanged() ) );

    connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
    connect( this, SIGNAL( applyClicked() ), SLOT( slotApply() ) );
}

void FileTypeDialog::save()
{
  if (m_mimeTypeData->isDirty()) {
    if (m_mimeTypeData->sync())
        MimeTypeWriter::runUpdateMimeDatabase();
    KBuildSycocaProgressDialog::rebuildKSycoca(this);
  }
}

void FileTypeDialog::slotOk()
{
  save();
  accept();
}

void FileTypeDialog::slotApply()
{
  save();
}

void FileTypeDialog::clientChanged(bool state)
{
  // enable/disable buttons
  enableButton(User1, state);
  enableButton(Apply, state);
}

void FileTypeDialog::slotDatabaseChanged()
{
    if ( KSycoca::self()->isChanged("xdgdata-mime") // changes in mimetype definitions
         || KSycoca::self()->isChanged("services") ) { // changes in .desktop files
        m_details->refresh();
    }
}

#include "keditfiletype.moc"

int main(int argc, char ** argv)
{
  KServiceTypeProfile::setConfigurationMode();
  KAboutData aboutData( "keditfiletype", "filetypes", ki18n("KEditFileType"), "1.0",
                        ki18n("KDE file type editor - simplified version for editing a single file type"),
                        KAboutData::License_GPL,
                        ki18n("(c) 2000, KDE developers") );
  aboutData.addAuthor(ki18n("Preston Brown"),KLocalizedString(), "pbrown@kde.org");
  aboutData.addAuthor(ki18n("David Faure"),KLocalizedString(), "faure@kde.org");

  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineOptions options;
  options.add("parent <winid>", ki18n("Makes the dialog transient for the window specified by winid"));
  options.add("+mimetype", ki18n("File type to edit (e.g. text/html)"));
  KCmdLineArgs::addCmdLineOptions( options ); // Add our own options.
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (args->count() == 0)
    KCmdLineArgs::usage();

  QString arg = args->arg(0);
  MimeTypeData* mimeTypeData = 0;
  const bool createType = arg.startsWith('*');
  if ( createType ) {
      QString mimeString = "application/x-kdeuser%1";
      QString mimeTypeName;
      int inc = 0;
      bool ok = false;
      do {
          ++inc;
          mimeTypeName = mimeString.arg(inc);
          ok = !KMimeType::mimeType(mimeTypeName);
      } while (!ok);

      QStringList patterns;
      if ( arg.length() > 2 )
          patterns << arg.toLower() << arg.toUpper();
      QString comment;
      if ( arg.startsWith( "*." ) && arg.length() >= 3 ) {
          const QString type = arg.mid( 3 ).prepend( arg[2].toUpper() );
          comment = i18n( "%1 File", type );
      }

      mimeTypeData = new MimeTypeData(mimeTypeName, true); // new mimetype
      mimeTypeData->setComment(comment);
      mimeTypeData->setPatterns(patterns);
  }
  else {
      const QString mimeTypeName = arg;
      KMimeType::Ptr mime = KMimeType::mimeType(mimeTypeName);
      if (!mime)
          kFatal() << "Mimetype" << mimeTypeName << "not found" ;

      mimeTypeData = new MimeTypeData(mime);
  }

  FileTypeDialog dlg( mimeTypeData );
#if defined Q_WS_X11
  if( args->isSet( "parent" )) {
    bool ok;
    long id = QString(args->getOption("parent")).toLong(&ok);
    if (ok)
      XSetTransientForHint( QX11Info::display(), dlg.winId(), id );
  }
#endif
  args->clear();
  if ( !createType )
    dlg.setCaption( i18n("Edit File Type %1", mimeTypeData->name()) );
  else {
    dlg.setCaption( i18n("Create New File Type %1", mimeTypeData->name()) );
    dlg.enableButton( KDialog::Apply, true );
  }

  dlg.show(); // non-modal

  return app.exec();
}

