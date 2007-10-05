/*  This file is part of the KDE libraries
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; version 2
    of the License.

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
#include "kcustommenueditor.h"

// Qt
#include <QtCore/QDir>
#include <QtCore/QRegExp>
#include <QtGui/QImage>
#include <QtGui/QPushButton>

// KDE
#include <k3listview.h>
#include <kconfigbase.h>
#include <kconfiggroup.h>
#include <kdialogbuttonbox.h>
#include <kglobal.h>
#include <khbox.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kopenwithdialog.h>
#include <kservice.h>
#include <kstandarddirs.h>


class KCustomMenuEditor::Item : public Q3ListViewItem
{
public:
   Item(Q3ListView *parent, KService::Ptr service)
     : Q3ListViewItem(parent),
       s(service)
   {
      init();
   }

   Item(Q3ListViewItem *parent, KService::Ptr service)
     : Q3ListViewItem(parent),
       s(service)
   {
      init();
   }

   void init()
   {
      QString serviceName = s->name();

      // item names may contain ampersands. To avoid them being converted
      // to accelators, replace them with two ampersands.
      serviceName.replace("&", "&&");

      QPixmap normal = KIconLoader::global()->loadIcon(s->icon(), KIconLoader::Small,
                              0, KIconLoader::DefaultState, QStringList(), 0L, true);

      // make sure they are not larger than 16x16
      if (normal.width() > 16 || normal.height() > 16) {
          QImage tmp = normal.toImage();
          tmp = tmp.scaled(16, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
          normal = QPixmap::fromImage(tmp);
      }
      setText(0, serviceName);
      setPixmap(0, normal);
   }

   KService::Ptr s;
};

class KCustomMenuEditor::KCustomMenuEditorPrivate
{
public:
    QPushButton * pbRemove;
    QPushButton * pbMoveUp;
    QPushButton * pbMoveDown;
};

KCustomMenuEditor::KCustomMenuEditor(QWidget *parent)
  : KDialog(parent),
    m_listView(0),d(new KCustomMenuEditorPrivate)
{
   setCaption( i18n("Menu Editor") );
   setButtons( Ok | Cancel );
   setDefaultButton(Ok);
   showButtonSeparator(true);
   KHBox *page = new KHBox(this);
   setMainWidget(page);
   m_listView = new K3ListView(page);
   m_listView->addColumn(i18n("Menu"));
   m_listView->setFullWidth(true);
   m_listView->setSorting(-1);
   KDialogButtonBox *buttonBox = new KDialogButtonBox(page, Qt::Vertical);
   buttonBox->addButton(i18n("New..."),QDialogButtonBox::ActionRole, this, SLOT(slotNewItem()));
   d->pbRemove=buttonBox->addButton(i18n("Remove"),QDialogButtonBox::DestructiveRole, this, SLOT(slotRemoveItem()));
   d->pbMoveUp=buttonBox->addButton(i18n("Move Up"),QDialogButtonBox::ActionRole, this, SLOT(slotMoveUp()));
   d->pbMoveDown=buttonBox->addButton(i18n("Move Down"),QDialogButtonBox::ActionRole, this, SLOT(slotMoveDown()));
   buttonBox->layout();
   connect( m_listView, SIGNAL( selectionChanged () ), this, SLOT( refreshButton() ) );
   refreshButton();
}

KCustomMenuEditor::~KCustomMenuEditor()
{
    delete d;
}

void KCustomMenuEditor::refreshButton()
{
    Q3ListViewItem *item = m_listView->currentItem();
    d->pbRemove->setEnabled( item );
    d->pbMoveUp->setEnabled( item && item->itemAbove() );
    d->pbMoveDown->setEnabled( item && item->itemBelow() );
}

void
KCustomMenuEditor::load(KConfig *cfg)
{
   KConfigGroup cg(cfg, QString());
   int count = cg.readEntry("NrOfItems", 0);
   Q3ListViewItem *last = 0;
   for(int i = 0; i < count; i++)
   {
      QString entry = cg.readPathEntry(QString("Item%1").arg(i+1));
      if (entry.isEmpty())
         continue;

      // Try KSycoca first.
      KService::Ptr menuItem = KService::serviceByDesktopPath( entry );
      if (!menuItem)
         menuItem = KService::serviceByDesktopName( entry );
      if (!menuItem)
         menuItem = new KService( entry );

      if (!menuItem->isValid())
         continue;

      Q3ListViewItem *item = new Item(m_listView, menuItem);
      item->moveItem(last);
      last = item;
   }
}

void
KCustomMenuEditor::save(KConfig *cfg)
{
   // First clear the whole config file.
   QStringList groups = cfg->groupList();
   for(QStringList::ConstIterator it = groups.begin();
      it != groups.end(); ++it)
   {
      cfg->deleteGroup(*it);
   }

   KConfigGroup cg(cfg, QString());
   Item * item = (Item *) m_listView->firstChild();
   int i = 0;
   while(item)
   {
      i++;
      QString path = item->s->entryPath();
      if (QDir::isRelativePath(path) || QDir::isRelativePath(KGlobal::dirs()->relativeLocation("xdgdata-apps", path)))
         path = item->s->desktopEntryName();
      cg.writePathEntry(QString("Item%1").arg(i), path);
      item = (Item *) item->nextSibling();
   }
   cg.writeEntry("NrOfItems", i);
}

void
KCustomMenuEditor::slotNewItem()
{
   Q3ListViewItem *item = m_listView->currentItem();

   KOpenWithDialog dlg(this);
   dlg.setSaveNewApplications(true);

   if (dlg.exec())
   {
      KService::Ptr s = dlg.service();
      if (s && s->isValid())
      {
         Item *newItem = new Item(m_listView, s);
         newItem->moveItem(item);
      }
      refreshButton();
   }
}

void
KCustomMenuEditor::slotRemoveItem()
{
   Q3ListViewItem *item = m_listView->currentItem();
   if (!item)
      return;

   delete item;
   refreshButton();
}

void
KCustomMenuEditor::slotMoveUp()
{
   Q3ListViewItem *item = m_listView->currentItem();
   if (!item)
      return;

   Q3ListViewItem *searchItem = m_listView->firstChild();
   while(searchItem)
   {
      Q3ListViewItem *next = searchItem->nextSibling();
      if (next == item)
      {
         searchItem->moveItem(item);
         break;
      }
      searchItem = next;
   }
   refreshButton();
}

void
KCustomMenuEditor::slotMoveDown()
{
   Q3ListViewItem *item = m_listView->currentItem();
   if (!item)
      return;

   Q3ListViewItem *after = item->nextSibling();
   if (!after)
      return;

   item->moveItem( after );
   refreshButton();
}

#include "kcustommenueditor.moc"
