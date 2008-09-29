/*
Copyright (C) 2008 Xavier Vello <xavier.vello@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Own
#include "bookmarks.h"

// Qt
#include <QtGui/QBoxLayout>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>

// KDE
#include <kconfig.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <knuminput.h>


K_PLUGIN_FACTORY_DECLARATION(KioConfigFactory)

BookmarksConfigModule::BookmarksConfigModule(QWidget *parent, const QVariantList &)
                  :KCModule(KioConfigFactory::componentData(), parent)
{
  ui.setupUi(this);
  load();
}

BookmarksConfigModule::~BookmarksConfigModule()
{
}

void BookmarksConfigModule::load()
{
  KConfig *c = new KConfig("kiobookmarksrc");
  KConfigGroup group = c->group("General");

  ui.sbColumns->setValue(group.readEntry("Columns", 4));
  ui.cbShowBackgrounds->setChecked(group.readEntry("ShowBackgrounds", true));
  ui.cbShowRoot->setChecked(group.readEntry("ShowRoot", true));
  ui.cbFlattenTree->setChecked(group.readEntry("FlattenTree", false));
  ui.cbShowPlaces->setChecked(group.readEntry("ShowPlaces", true));

  // Config changed notifications...
  connect ( ui.sbColumns, SIGNAL(valueChanged(int)), SLOT(configChanged()) );
  connect ( ui.cbShowBackgrounds, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.cbShowRoot, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.cbFlattenTree, SIGNAL(toggled(bool)), SLOT(configChanged()) );
  connect ( ui.cbShowPlaces, SIGNAL(toggled(bool)), SLOT(configChanged()) );

  delete c;
  emit changed( false );
}

void BookmarksConfigModule::save()
{
  KConfig *c = new KConfig("kiobookmarksrc");
  KConfigGroup group = c->group("General");
  group.writeEntry("Columns", ui.sbColumns->value() );
  group.writeEntry("ShowBackgrounds", ui.cbShowBackgrounds->isChecked() );
  group.writeEntry("ShowRoot", ui.cbShowRoot->isChecked() );
  group.writeEntry("FlattenTree", ui.cbFlattenTree->isChecked() );
  group.writeEntry("ShowPlaces", ui.cbShowPlaces->isChecked() );

  c->sync();
  delete c;
  emit changed( false );
}

void BookmarksConfigModule::defaults()
{
  ui.sbColumns->setValue( 4 );
  ui.cbShowBackgrounds->setChecked( true );
  ui.cbShowRoot->setChecked( true );
  ui.cbShowPlaces->setChecked( true );
  ui.cbFlattenTree->setChecked( false );
}

QString BookmarksConfigModule::quickHelp() const
{
  return i18n( "<h1>My Bookmarks</h1><p>This module lets you configure the bookmarks home page.</p>"
               "<p>The bookmarks home page is accessible at <a href=\"bookmarks:/\">bookmarks:/</a>.</p>" );
}

void BookmarksConfigModule::configChanged()
{
  emit changed( true );
}

#include "bookmarks.moc"
