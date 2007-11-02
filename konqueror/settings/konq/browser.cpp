/* This file is part of the KDE project
   Copyright (C) 2002 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Own
#include "browser.h"

// Qt
#include <QtGui/QLayout>
#include <QtGui/QTabWidget>
#include <QtCore/QFile>
#include <QtGui/QBoxLayout>

// KDE
#include <klocale.h>
#include <kdialog.h>
#include <fixx11h.h>
#include <kcmoduleloader.h>

// Local
#include "behaviour.h"
#include "previews.h"
#include "konqkcmfactory.h"

/// ### Why is this called browser? It contains filemanagement options

KBrowserOptions::KBrowserOptions(QWidget *parent, const QVariantList &)
    : KCModule( KonqKcmFactory::componentData(), parent )
{
  KSharedConfig::Ptr config = KSharedConfig::openConfig("konquerorrc");
  QString group = "FMSettings";

  QVBoxLayout *layout = new QVBoxLayout(this);
  QTabWidget *tab = new QTabWidget(this);
  layout->addWidget(tab);

  // appearance = ...

  behavior = new KBehaviourOptions(tab);
  behavior->layout()->setMargin( KDialog::marginHint() );

  previews = new KPreviewOptions(tab);
  previews->layout()->setMargin( KDialog::marginHint() );


  //tab->addTab(appearance, i18n("&Appearance"));
  tab->addTab(behavior, i18n("&Behavior"));
  tab->addTab(previews, i18n("&Previews && Meta-Data"));

  //connect(appearance, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  connect(behavior, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  connect(previews, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));

  connect(tab, SIGNAL(currentChanged(QWidget *)),
          this, SIGNAL(quickHelpChanged()));
  m_tab = tab;
}

void KBrowserOptions::load()
{
  //appearance->load();
  behavior->load();
  previews->load();
}

void KBrowserOptions::defaults()
{
  //appearance->defaults();
  behavior->defaults();
  previews->defaults();
}

void KBrowserOptions::save()
{
  //appearance->save();
  behavior->save();
  previews->save();
}

QString KBrowserOptions::quickHelp() const
{
  QWidget *w = m_tab->currentWidget();
  KCModule *m = qobject_cast<KCModule *>(w);
  if (m) {
     return m->quickHelp();
  }
  return QString();
}

#include "browser.moc"
