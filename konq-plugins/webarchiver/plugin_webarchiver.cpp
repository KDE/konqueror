/* This file is part of Webarchiver
 *  Copyright (C) 2001 by Andreas Schlapbach <schlpbch@iam.unibe.ch>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

/* $Id$ */

/*
 * There are two recursions within this code:
 * - Recursively create DOM-Tree for referenced links which get recursively
 *   converted to HTML
 *
 * => This code has the potential to download whole sites to a TarGz-Archive
 */

//#define DEBUG_WAR

#include <qdir.h>
#include <qfile.h>

#include <kaction.h>
#include <kcomponentdata.h>
#include <kglobalsettings.h>

#include <kfiledialog.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <khtmlview.h>
#include <khtml_part.h>
#include <kdebug.h>
#include <kpluginfactory.h>
#include <kactioncollection.h>
#include "plugin_webarchiver.h"
#include "archivedialog.h"

K_PLUGIN_FACTORY(PluginWebArchiverFactory, registerPlugin<PluginWebArchiver>();)
K_EXPORT_PLUGIN( PluginWebArchiverFactory( "webarchiver" ) )

PluginWebArchiver::PluginWebArchiver( QObject* parent,
                                      const QVariantList & )
  : Plugin( parent )
{
  QAction *a = actionCollection()->addAction( "archivepage");
  a->setText(i18n("Archive &Web Page..."));
  a->setIcon(KIcon("webarchiver"));
  connect(a, SIGNAL(triggered()), this, SLOT(slotSaveToArchive()));
}

PluginWebArchiver::~PluginWebArchiver()
{
}

void PluginWebArchiver::slotSaveToArchive()
{
  // ## Unicode ok?
  if( !parent() || !parent()->inherits("KHTMLPart"))
    return;
  KHTMLPart *part = qobject_cast<KHTMLPart *>( parent() );

  QString archiveName = QString::fromUtf8(part->htmlDocument().title().string().toUtf8());

  if (archiveName.isEmpty())
    archiveName = i18n("Untitled");

  KConfig config( "webarchiverrc", KConfig::SimpleConfig );
  KConfigGroup configGroup = config.group( "Recent" );

  archiveName = archiveName.simplified();
  archiveName.replace( "\\s:", " ");		// what is this intended to do?
  archiveName.replace( "?", "");
  archiveName.replace( ":", "");
  archiveName.replace( "/", "");
  // Replace space with underscore, proposed Frank Pieczynski <pieczy@knuut.de>
  archiveName = archiveName.replace( QRegExp("\\s+"), "_");

  QString lastCWD = configGroup.readPathEntry( "savedialogcwd",
                                               KGlobalSettings::documentPath() );
  archiveName = lastCWD + "/" + archiveName + ".war";

  KUrl url = KFileDialog::getSaveUrl(archiveName, i18n("*.war *.tgz|Web Archives"), part->widget(),
					  i18n("Save Page as Web-Archive") );

  if (url.isEmpty()) { return; }

  if (!(url.isValid())) {
    const QString title = i18nc( "@title:window", "Invalid URL" );
    const QString text = i18n( "The URL\n%1\nis not valid." , url.prettyUrl());
    KMessageBox::sorry(part->widget(), text, title );
    return;
  }

  lastCWD = url.directory();
  if (! lastCWD.isNull())
  {
    configGroup.writePathEntry( "savedialogcwd", lastCWD );
    config.sync();
  }

  const QFile file(url.path());
  if (file.exists()) {
    const QString title = i18nc( "@title:window", "File Exists" );
    const QString text = i18n( "Do you really want to overwrite:\n%1?" , url.prettyUrl());
    if (KMessageBox::Continue != KMessageBox::warningContinueCancel( part->widget(), text, title, KGuiItem(i18n("Overwrite")) ) ) {
      return;
    }
  }

  //
  // It is very important to make the archive dialog a child of the KHTMLPart!
  // If not crashes due to dangling refs will happen. For example if Konqueror quits
  // while archiving runs @c part becomes invalid. Furthermore the various @ref QHash<>
  // members of @ref ArchiveDialog contain DOM elements that use ref counting. Upon
  // exit of Konqueror @c part gets destroyed *before* our @ref ArchiveDialog . Since
  // our running ArchiveDialog keeps the DOM ref counts up KHTML triggers an assertion
  // in KHTMLGlobal
  //
  // In contrast if @ref ArchiveDialog is a child of the part view Qt ensures that all
  // child dialogs are destroyed _before_ @c part is destroyed.
  //
  ArchiveDialog *dialog=new ArchiveDialog(part->view(), url.path(), part);
  dialog->show();
  dialog->archive();
}

#include "plugin_webarchiver.moc"
