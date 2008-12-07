/*  This file is part of the KDE project
    Copyright (C) 2000 - 2008 David Faure <faure@kde.org>
    Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

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
#include "filetypesview.h"
#include "mimetypewriter.h"

// Qt
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtCore/QTimer>
#include <QtGui/QBoxLayout>
#include <QFile>
#include <qdbusconnection.h>
#include <qdbusmessage.h>

// KDE
#include <kapplication.h>
#include <kbuildsycocaprogressdialog.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <klineedit.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kservicetypeprofile.h>
#include <ksycoca.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

// Local
#include "newtypedlg.h"
#include "filetypedetails.h"
#include "filegroupdetails.h"


K_PLUGIN_FACTORY(FileTypesViewFactory, registerPlugin<FileTypesView>();)
K_EXPORT_PLUGIN(FileTypesViewFactory("filetypes"))


FileTypesView::FileTypesView(QWidget *parent, const QVariantList &)
  : KCModule(FileTypesViewFactory::componentData(), parent)
{
  m_fileTypesConfig = KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals);

  setQuickHelp( i18n("<p><h1>File Associations</h1>"
    " This module allows you to choose which applications are associated"
    " with a given type of file. File types are also referred to as MIME types"
    " (MIME is an acronym which stands for \"Multipurpose Internet Mail"
    " Extensions\").</p><p> A file association consists of the following:"
    " <ul><li>Rules for determining the MIME-type of a file, for example"
    " the filename pattern *.png, which means 'all files with names that end"
    " in .png', is associated with the MIME type \"image/png\";</li>"
    " <li>A short description of the MIME-type, for example the description"
    " of the MIME type \"image/png\" is simply 'PNG image';</li>"
    " <li>An icon to be used for displaying files of the given MIME-type,"
    " so that you can easily identify the type of file in a file"
    " manager or file-selection dialog (at least for the types you use often);</li>"
    " <li>A list of the applications which can be used to open files of the"
    " given MIME-type -- if more than one application can be used then the"
    " list is ordered by priority.</li></ul>"
    " You may be surprised to find that some MIME types have no associated"
    " filename patterns; in these cases, KDE is able to determine the"
    " MIME-type by directly examining the contents of the file.</p>"));

  KServiceTypeProfile::setConfigurationMode();
  setButtons(Help | Apply);
  QString wtstr;

  QHBoxLayout* l = new QHBoxLayout(this);
  QVBoxLayout* leftLayout = new QVBoxLayout();
  leftLayout->setSpacing(KDialog::spacingHint());
  l->addLayout( leftLayout );

  patternFilterLE = new KLineEdit(this);
  patternFilterLE->setClearButtonShown(true);
  patternFilterLE->setClickMessage( i18n("F&ind file type or filename pattern:").remove(':').remove('&')/*TODO remove colon*/ );
  leftLayout->addWidget(patternFilterLE);

  connect(patternFilterLE, SIGNAL(textChanged(const QString &)),
          this, SLOT(slotFilter(const QString &)));

  wtstr = i18n("Enter a part of a filename pattern, and only file types with a "
               "matching file pattern will appear in the list. Alternatively, enter "
               "a part of a file type name as it appears in the list.");

  patternFilterLE->setWhatsThis( wtstr );

  typesLV = new TypesListTreeWidget(this);

  typesLV->setHeaderLabel(i18n("Known Types"));
  leftLayout->addWidget(typesLV);
  connect(typesLV, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
          this, SLOT(updateDisplay(QTreeWidgetItem *)));
  connect(typesLV, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
          this, SLOT(slotDoubleClicked(QTreeWidgetItem *)));

  typesLV->setWhatsThis( i18n("Here you can see a hierarchical list of"
    " the file types which are known on your system. Click on the '+' sign"
    " to expand a category, or the '-' sign to collapse it. Select a file type"
    " (e.g. text/html for HTML files) to view/edit the information for that"
    " file type using the controls on the right.") );

  QHBoxLayout* btnsLay = new QHBoxLayout();
  leftLayout->addLayout(btnsLay);
  btnsLay->addStretch(1);
  KPushButton *addTypeB = new KPushButton(i18n("Add..."), this);
  addTypeB->setIcon(KIcon("list-add"));
  connect(addTypeB, SIGNAL(clicked()), SLOT(addType()));
  btnsLay->addWidget(addTypeB);

  addTypeB->setWhatsThis( i18n("Click here to add a new file type.") );

  m_removeTypeB = new KPushButton(i18n("&Remove"), this);
  m_removeTypeB->setIcon(KIcon("list-remove"));
  connect(m_removeTypeB, SIGNAL(clicked()), SLOT(removeType()));
  btnsLay->addWidget(m_removeTypeB);
  m_removeTypeB->setEnabled(false);

  m_removeTypeB->setWhatsThis( i18n("Click here to remove the selected file type.") );

  // For the right panel, prepare a widget stack
  m_widgetStack = new QStackedWidget(this);

  l->addWidget( m_widgetStack );

  // File Type Details
  m_details = new FileTypeDetails( m_widgetStack );
  connect( m_details, SIGNAL( changed(bool) ),
           this, SLOT( setDirty(bool) ) );
  connect( m_details, SIGNAL( embedMajor(const QString &, bool &) ),
           this, SLOT( slotEmbedMajor(const QString &, bool &)));
  m_widgetStack->insertWidget( 1, m_details /*id*/ );

  // File Group Details
  m_groupDetails = new FileGroupDetails( m_widgetStack );
  connect( m_groupDetails, SIGNAL( changed(bool) ),
           this, SLOT( setDirty(bool) ) );
  m_widgetStack->insertWidget( 2,m_groupDetails /*id*/ );

  // Widget shown on startup
  m_emptyWidget = new QLabel( i18n("Select a file type by name or by extension"), m_widgetStack);
  m_emptyWidget->setAlignment( Qt::AlignCenter );
  m_widgetStack->insertWidget( 3,m_emptyWidget );

  m_widgetStack->setCurrentWidget( m_emptyWidget );

  QTimer::singleShot( 0, this, SLOT( init() ) ); // this takes some time

  connect( KSycoca::self(), SIGNAL( databaseChanged() ), SLOT( slotDatabaseChanged() ) );
}

FileTypesView::~FileTypesView()
{
}

void FileTypesView::setDirty(bool state)
{
  emit changed(state);
  m_dirty = state;
}

void FileTypesView::init()
{
  show();
  setEnabled( false );

  setCursor( Qt::WaitCursor );
  readFileTypes();
  unsetCursor();

  setDirty(false);
  setEnabled( true );
}

// only call this method once on startup, then never again! Otherwise, newly
// added Filetypes will be lost.
void FileTypesView::readFileTypes()
{
    typesLV->clear();
    m_majorMap.clear();
    m_itemList.clear();

    const KMimeType::List mimetypes = KMimeType::allMimeTypes();
    KMimeType::List::const_iterator it2(mimetypes.constBegin());
    for (; it2 != mimetypes.constEnd(); ++it2) {
	QString mimetype = (*it2)->name();
	int index = mimetype.indexOf('/');
	QString maj = mimetype.left(index);
	QString min = mimetype.right(mimetype.length() - index+1);

        TypesListItem* groupItem = m_majorMap.value(maj);
	if ( !groupItem ) {
	    groupItem = new TypesListItem( typesLV, maj );
	    m_majorMap.insert( maj, groupItem );
	}

	TypesListItem *item = new TypesListItem(groupItem, (*it2));
	m_itemList.append( item );
    }
    updateDisplay(0L);

}

void FileTypesView::slotEmbedMajor(const QString &major, bool &embed)
{
    TypesListItem *groupItem = m_majorMap.value(major);
    if (!groupItem)
        return;

    embed = (groupItem->mimeTypeData().autoEmbed() == MimeTypeData::Yes);
}

void FileTypesView::slotFilter(const QString & patternFilter)
{
    for (int i = 0; i < typesLV->topLevelItemCount(); ++i) {
        typesLV->topLevelItem(i)->setHidden(true);
    }

    // insert all items and their group that match the filter
    Q_FOREACH(TypesListItem* it, m_itemList) {
        const MimeTypeData& mimeTypeData = it->mimeTypeData();
        if ( patternFilter.isEmpty() || mimeTypeData.matchesFilter(patternFilter) ) {
            TypesListItem *group = m_majorMap.value( mimeTypeData.majorType() );
            Q_ASSERT(group);
            if (group) {
                group->setHidden(false);
                it->setHidden(false);
            }
        } else {
            it->setHidden(true);
        }
    }
}

void FileTypesView::addType()
{
    const QStringList allGroups = m_majorMap.keys();

    NewTypeDialog dialog(allGroups, this);

    if (dialog.exec()) {
        QString newMimeType = dialog.group() + '/' + dialog.text();

        QTreeWidgetItemIterator it(typesLV);

        TypesListItem *group = m_majorMap.value(dialog.group());
        if ( !group ) {
            group = new TypesListItem(typesLV, dialog.group());
            m_majorMap.insert(dialog.group(), group);
        }

        // find out if our group has been filtered out -> insert if necessary
        QTreeWidgetItem *item = typesLV->topLevelItem(0);
        bool insert = true;
        while ( item ) {
            if ( item == group ) {
                insert = false;
                break;
            }
            item = typesLV->itemBelow(item);
        }
        if ( insert )
            typesLV->addTopLevelItem( group );

        TypesListItem *tli = new TypesListItem(group, newMimeType);
        m_itemList.append( tli );

        group->setExpanded(true);
        tli->setSelected(true);

        setDirty(true);
    }
}

void FileTypesView::removeType()
{
  TypesListItem *current = (TypesListItem *) typesLV->currentItem();

  if ( !current )
      return;

  const MimeTypeData& mimeTypeData = current->mimeTypeData();

  // Can't delete groups
  if ( mimeTypeData.isMeta() )
      return;
  // nor essential mimetypes
  if ( mimeTypeData.isEssential() )
      return;
      
  QTreeWidgetItem *li = typesLV->itemAbove(current);
  if (!li)
      li = typesLV->itemBelow(current);
  if (!li)
      li = current->parent();

  if (!mimeTypeData.isNew())
      removedList.append(mimeTypeData.name());
  current->parent()->takeChild(current->parent()->indexOfChild(current));
  m_itemList.removeAll(current);
  setDirty(true);

  if ( li )
      li->setSelected(true);
}

void FileTypesView::slotDoubleClicked(QTreeWidgetItem *item)
{
  if ( !item ) return;
  item->setExpanded( !item->isExpanded() );
}

void FileTypesView::updateDisplay(QTreeWidgetItem *item)
{
  if (!item)
  {
    m_widgetStack->setCurrentWidget( m_emptyWidget );
    m_removeTypeB->setEnabled(false);
    return;
  }

  bool wasDirty = m_dirty;

  TypesListItem *tlitem = (TypesListItem *) item;
  MimeTypeData& mimeTypeData = tlitem->mimeTypeData();

  if (mimeTypeData.isMeta()) // is a group
  {
    m_widgetStack->setCurrentWidget( m_groupDetails );
    m_groupDetails->setMimeTypeData( &mimeTypeData );
    m_removeTypeB->setEnabled(false);
  }
  else
  {
    m_widgetStack->setCurrentWidget( m_details );
    m_details->setMimeTypeData( &mimeTypeData );
    bool canRemove = !mimeTypeData.isEssential();
    if (canRemove && !mimeTypeData.isNew()) {
        // We can only remove mimetypes that we defined ourselves, not those from freedesktop.org
        canRemove = MimeTypeWriter::hasDefinitionFile(mimeTypeData.name());
    }
    m_removeTypeB->setEnabled(canRemove);
  }

  // Updating the display indirectly called change(true)
  if ( !wasDirty )
    setDirty(false);
}

void FileTypesView::save()
{
    bool needUpdateMimeDb = false;
    bool didIt = false;
    // first, remove those items which we are asked to remove.
    Q_FOREACH(const QString& mime, removedList) {
        MimeTypeWriter::removeOwnMimeType(mime);
        didIt = true;
        needUpdateMimeDb = true;
    }

  // now go through all entries and sync those which are dirty.
  // don't use typesLV, it may be filtered
  QMap<QString,TypesListItem*>::iterator it1 = m_majorMap.begin();
  while ( it1 != m_majorMap.end() ) {
    TypesListItem *tli = *it1;
    if (tli->mimeTypeData().isDirty()) {
      kDebug() << "Entry " << tli->name() << " is dirty. Saving.";
      if (tli->mimeTypeData().sync())
          needUpdateMimeDb = true;
      didIt = true;
    }
    ++it1;
  }
    Q_FOREACH(TypesListItem* tli, m_itemList) {
        if (tli->mimeTypeData().isDirty()) {
            kDebug() << "Entry " << tli->name() << " is dirty. Saving.";
            if (tli->mimeTypeData().sync())
                needUpdateMimeDb = true;
            didIt = true;
        }
    }

  m_fileTypesConfig->sync();

  setDirty(false);

  if (needUpdateMimeDb) {
      MimeTypeWriter::runUpdateMimeDatabase();
  }
  if (didIt) {
      KBuildSycocaProgressDialog::rebuildKSycoca(this);

      // Trigger reparseConfiguration of filetypesrc in konqueror
      QDBusMessage message =
          QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
      QDBusConnection::sessionBus().send(message);
  }
}

void FileTypesView::load()
{
    readFileTypes();
}

void FileTypesView::slotDatabaseChanged()
{
    if ( KSycoca::self()->isChanged("xdgdata-mime") // changes in mimetype definitions
         || KSycoca::self()->isChanged("services") ) { // changes in .desktop files

        m_details->refresh();

        // ksycoca has new KMimeTypes objects for us, make sure to update
        // our 'copies' to be in sync with it. Not important for OK, but
        // important for Apply (how to differentiate those 2?).
        // See BR 35071.

        Q_FOREACH(TypesListItem* tli, m_itemList) {
            tli->mimeTypeData().refresh();
        }
    }
}

void FileTypesView::defaults()
{
}

#include "filetypesview.moc"


