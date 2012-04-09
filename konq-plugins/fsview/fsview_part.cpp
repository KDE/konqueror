/* This file is part of FSView.
   Copyright (C) 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>
   Some file management code taken from the Dolphin file manager (C) 2006-2009,
   by Peter Penz <peter.penz19@mail.com>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

/*
 * The KPart embedding the FSView widget
 */

#include <qclipboard.h>
#include <qtimer.h>

#include <kcomponentdata.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kpluginfactory.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kaction.h>
#include <kmenu.h>
#include <kglobalsettings.h>
#include <kprotocolmanager.h>
#include <kio/job.h>
#include <kio/deletejob.h>
#include <kmessagebox.h>
#include <kactionmenu.h>
#include <kactioncollection.h>
#include <kio/jobuidelegate.h>
// from kdebase/libkonq...
#include <konq_operations.h>
#include <konqmimedata.h>
#include <ktoolinvocation.h>
#include <kconfiggroup.h>
#include <KDebug>
#include <KIcon>

#include <QApplication>

#include "fsview_part.h"


K_PLUGIN_FACTORY( FSViewPartFactory, registerPlugin<FSViewPart>(); )
K_EXPORT_PLUGIN( FSViewPartFactory(KAboutData(
               "fsview",
               0,
               ki18n("FSView"),
               "0.1.1",
               ki18n("Filesystem Utilization Viewer"),
               KAboutData::License_GPL,
               ki18n("(c) 2003-2005, Josef Weidendorfer"))) )


// FSJob, for progress

FSJob::FSJob(FSView* v)
  : KIO::Job()
{
  _view = v;
  QObject::connect(v, SIGNAL(progress(int,int,QString)),
                   this, SLOT(progressSlot(int,int,QString)));
}

void FSJob::kill(bool /*quietly*/)
{
  _view->stop();

  Job::kill();
}

void FSJob::progressSlot(int percent, int dirs, const QString& cDir)
{
  if (percent<100) {
    emitPercent(percent, 100);
    slotInfoMessage(this, i18np("Read 1 folder, in %2",
                               "Read %1 folders, in %2",
                               dirs , cDir),QString());
  }
  else
    slotInfoMessage(this, i18np("1 folder", "%1 folders", dirs),QString());
}


// FSViewPart

FSViewPart::FSViewPart(QWidget *parentWidget,
                       QObject *parent,
                       const QList<QVariant>& /* args */)
    : KParts::ReadOnlyPart(parent)
{
    // we need an instance
    setComponentData( FSViewPartFactory::componentData() );

    _view = new FSView(new Inode(), parentWidget);
    _view->setWhatsThis( i18n("<p>This is the FSView plugin, a graphical "
				"browsing mode showing filesystem utilization "
				"by using a tree map visualization.</p>"
				"<p>Note that in this mode, automatic updating "
				"when filesystem changes are made "
				"is intentionally <b>not</b> done.</p>"
				"<p>For details on usage and options available, "
				"see the online help under "
				"menu 'Help/FSView Manual'.</p>"));

    _view->show();
    setWidget(_view);

    _ext = new FSViewBrowserExtension(this);
    _job = 0;

    _areaMenu = new KActionMenu (i18n("Stop at Area"),
				 actionCollection());
    actionCollection()->addAction( "treemap_areadir", _areaMenu );
    _depthMenu = new KActionMenu (i18n("Stop at Depth"),
				 actionCollection());
    actionCollection()->addAction( "treemap_depthdir", _depthMenu );
    _visMenu = new KActionMenu (i18n("Visualization"),
				 actionCollection());
    actionCollection()->addAction( "treemap_visdir", _visMenu );

    _colorMenu = new KActionMenu (i18n("Color Mode"),
				  actionCollection());
    actionCollection()->addAction( "treemap_colordir", _colorMenu );

    QAction* action;
    action = actionCollection()->addAction(  "help_fsview");
    action->setText(i18n("&FSView Manual"));
    action->setIcon(KIcon("fsview"));
    action->setToolTip(i18n("Show FSView manual"));
    action->setWhatsThis(i18n("Opens the help browser with the "
			      "FSView documentation"));
    connect(action, SIGNAL(triggered()), this, SLOT(showHelp()));

    QObject::connect (_visMenu->menu(), SIGNAL (aboutToShow()),
		      SLOT (slotShowVisMenu()));
    QObject::connect (_areaMenu->menu(), SIGNAL (aboutToShow()),
		      SLOT (slotShowAreaMenu()));
    QObject::connect (_depthMenu->menu(), SIGNAL (aboutToShow()),
		      SLOT (slotShowDepthMenu()));
    QObject::connect (_colorMenu->menu(), SIGNAL (aboutToShow()),
		      SLOT (slotShowColorMenu()));

    slotSettingsChanged(KGlobalSettings::SETTINGS_MOUSE);
    connect( KGlobalSettings::self(), SIGNAL(settingsChanged(int)),
             SLOT(slotSettingsChanged(int)) );

    QObject::connect(_view,SIGNAL(returnPressed(TreeMapItem*)),
                     _ext,SLOT(selected(TreeMapItem*)));
    QObject::connect(_view,SIGNAL(selectionChanged()),
		     this,SLOT(updateActions()));
    QObject::connect(_view,
                     SIGNAL(contextMenuRequested(TreeMapItem*,QPoint)),
		     this,
                     SLOT(contextMenu(TreeMapItem*,QPoint)));

    QObject::connect(_view, SIGNAL(started()), this, SLOT(startedSlot()));
    QObject::connect(_view, SIGNAL(completed(int)),
		     this, SLOT(completedSlot(int)));

    // Create common file management actions - this is necessary in KDE4
    // as these common actions are no longer automatically part of KParts.
    // Much of this is taken from Dolphin.
    // FIXME: Renaming didn't even seem to work in KDE3! Implement (non-inline) renaming
    // functionality.
    //KAction* renameAction = m_actionCollection->addAction("rename");
    //rename->setText(i18nc("@action:inmenu Edit", "Rename..."));
    //rename->setShortcut(Qt::Key_F2);

    KAction* moveToTrashAction = actionCollection()->addAction("move_to_trash");
    moveToTrashAction->setText(i18nc("@action:inmenu File", "Move to Trash"));
    moveToTrashAction->setIcon(KIcon("user-trash"));
    moveToTrashAction->setShortcut(QKeySequence::Delete);
    connect(moveToTrashAction, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)),
            _ext, SLOT(trash(Qt::MouseButtons,Qt::KeyboardModifiers)));

    KAction* deleteAction = actionCollection()->addAction("delete");
    deleteAction->setIcon(KIcon("edit-delete"));
    deleteAction->setText(i18nc("@action:inmenu File", "Delete"));
    deleteAction->setShortcut(Qt::SHIFT | Qt::Key_Delete);
    connect(deleteAction, SIGNAL(triggered()), _ext, SLOT(del()));

    KAction *editMimeTypeAction = actionCollection()->addAction( "editMimeType" );
    editMimeTypeAction->setText( i18nc("@action:inmenu Edit", "&Edit File Type..." ) );
    connect(editMimeTypeAction, SIGNAL(triggered()), SLOT(slotEditMimeType()));

    KAction *propertiesAction = actionCollection()->addAction( "properties" );
    propertiesAction->setText( i18nc("@action:inmenu File", "Properties") );
    propertiesAction->setIcon(KIcon("document-properties"));
    propertiesAction->setShortcut(Qt::ALT | Qt::Key_Return);
    connect(propertiesAction, SIGNAL(triggered()), SLOT(slotProperties()));

    QTimer::singleShot(1, this, SLOT(showInfo()));

    updateActions();

    setXMLFile( "fsview_part.rc" );
}


FSViewPart::~FSViewPart()
{
  kDebug(90100) << "FSViewPart Destructor";

  delete _job;
  _view->saveFSOptions();
}

void FSViewPart::slotSettingsChanged(int category)
{
  if (category != KGlobalSettings::SETTINGS_MOUSE) return;

  QObject::disconnect(_view,SIGNAL(clicked(TreeMapItem*)),
		      _ext,SLOT(selected(TreeMapItem*)));
  QObject::disconnect(_view,SIGNAL(doubleClicked(TreeMapItem*)),
		      _ext,SLOT(selected(TreeMapItem*)));

  if (KGlobalSettings::singleClick())
    QObject::connect(_view,SIGNAL(clicked(TreeMapItem*)),
		     _ext,SLOT(selected(TreeMapItem*)));
  else
    QObject::connect(_view,SIGNAL(doubleClicked(TreeMapItem*)),
		     _ext,SLOT(selected(TreeMapItem*)));
}

void FSViewPart::showInfo()
{
    QString info;
    info = i18n("FSView intentionally does not support automatic updates "
		"when changes are made to files or directories, "
		"currently visible in FSView, from the outside.\n"
		"For details, see the 'Help/FSView Manual'.");

    KMessageBox::information( _view, info, QString(), "ShowFSViewInfo");
}

void FSViewPart::showHelp()
{
    KToolInvocation::startServiceByDesktopName("khelpcenter",
					    QString("help:/konqueror/index.html#fsview"));
}

void FSViewPart::startedSlot()
{
  _job = new FSJob(_view);
  _job->setUiDelegate(new KIO::JobUiDelegate());
  emit started(_job);
}

void FSViewPart::completedSlot(int dirs)
{
  if (_job) {
    _job->progressSlot(100, dirs, QString());
    delete _job;
    _job = 0;
  }

  KConfigGroup cconfig(_view->config(), "MetricCache");
  _view->saveMetric(&cconfig);

  emit completed();
}

void FSViewPart::slotShowVisMenu()
{
  _visMenu->menu()->clear();
  _view->addVisualizationItems(_visMenu->menu(), 1301);
}

void FSViewPart::slotShowAreaMenu()
{
  _areaMenu->menu()->clear();
  _view->addAreaStopItems(_areaMenu->menu(), 1001, 0);
}

void FSViewPart::slotShowDepthMenu()
{
  _depthMenu->menu()->clear();
  _view->addDepthStopItems(_depthMenu->menu(), 1501, 0);
}

void FSViewPart::slotShowColorMenu()
{
  _colorMenu->menu()->clear();
  _view->addColorItems(_colorMenu->menu(), 1401);
}

bool FSViewPart::openFile() // never called since openUrl is reimplemented
{
  kDebug(90100) << "FSViewPart::openFile " << localFilePath();
  _view->setPath(localFilePath());

  return true;
}

bool FSViewPart::openUrl(const KUrl &url)
{
  kDebug(90100) << "FSViewPart::openUrl " << url.path();

  if (!url.isValid()) return false;
  if (!url.isLocalFile()) return false;

  setUrl(url);
  emit setWindowCaption( this->url().prettyUrl() );

  _view->setPath(this->url().path());

  return true;
}

bool FSViewPart::closeUrl()
{
  kDebug(90100) << "FSViewPart::closeUrl ";

  _view->stop();

  return true;
}

void FSViewPart::setNonStandardActionEnabled(const char* actionName, bool enabled)
{
  QAction *action = actionCollection()->action(actionName);
  action->setEnabled(enabled);
}

void FSViewPart::updateActions()
{
  int canDel = 0, canCopy = 0, canMove = 0;
  KUrl::List urls;

  foreach(TreeMapItem* i, _view->selection()) {
    KUrl u;
    u.setPath( ((Inode*)i)->path() );
    urls.append(u);
    canCopy++;
    if ( KProtocolManager::supportsDeleting(  u ) )
	canDel++;
    if ( KProtocolManager::supportsMoving(  u ) )
	canMove++;
  }
  
  // Standard KBrowserExtension actions.
  emit _ext->enableAction("copy", canCopy > 0 );
  emit _ext->enableAction("cut", canMove > 0 );
  // Custom actions.
  //setNonStandardActionEnabled("rename", canMove > 0 ); // FIXME
  setNonStandardActionEnabled("move_to_trash", canDel > 0);
  setNonStandardActionEnabled("delete", canDel > 0);
  setNonStandardActionEnabled("editMimeType", _view->selection().count() == 1);
  setNonStandardActionEnabled("properties", _view->selection().count() == 1);

  emit _ext->selectionInfo(urls);

  if (canCopy>0)
      stateChanged("has_selection");
  else
      stateChanged("has_no_selection");

  kDebug(90100) << "FSViewPart::updateActions, deletable " << canDel;
}

void FSViewPart::contextMenu(TreeMapItem* /*item*/,const QPoint& p)
{
    int canDel = 0, canCopy = 0, canMove = 0;
    KFileItemList items;

    foreach(TreeMapItem* i, _view->selection()) {
	KUrl u;
	u.setPath( ((Inode*)i)->path() );
	QString mimetype = ((Inode*)i)->mimeType()->name();
	const QFileInfo& info = ((Inode*)i)->fileInfo();
	mode_t mode =
	    info.isFile() ? S_IFREG :
	    info.isDir() ? S_IFDIR :
	    info.isSymLink() ? S_IFLNK : (mode_t)-1;
	items.append(KFileItem(u, mimetype, mode));

	canCopy++;
	if ( KProtocolManager::supportsDeleting( u ) )
	    canDel++;
	if ( KProtocolManager::supportsMoving( u ) )
	    canMove++;
    }

    KParts::BrowserExtension::ActionGroupMap actionGroups;
    QList<QAction *> editActions;
    if (canDel >0) {
	editActions.append(actionCollection()->action("move_to_trash"));
	editActions.append(actionCollection()->action("delete"));
    }
    if (canMove)
	editActions.append(actionCollection()->action("rename"));
    actionGroups.insert("editactions", editActions);

    if (items.count()>0)
      emit _ext->popupMenu(_view->mapToGlobal(p), items,
		     KParts::OpenUrlArguments(),
		     KParts::BrowserArguments(),
		     KParts::BrowserExtension::ShowUrlOperations |
		     KParts::BrowserExtension::ShowProperties,
		     actionGroups);
}


// FSViewBrowserExtension

FSViewBrowserExtension::FSViewBrowserExtension(FSViewPart* viewPart)
  :KParts::BrowserExtension(viewPart)
{
  _view = viewPart->view();
}

FSViewBrowserExtension::~FSViewBrowserExtension()
{}



void FSViewBrowserExtension::del()
{
  const KUrl::List list = _view->selectedUrls();
  const bool del = KonqOperations::askDeleteConfirmation(list,
      KonqOperations::DEL,
      KonqOperations::DEFAULT_CONFIRMATION,
      _view);

  if (del) {
    KIO::Job* job = KIO::del(list);
    connect(job, SIGNAL(result(KJob*)),
            this, SLOT(refresh()));
  }
}

void FSViewBrowserExtension::trash(Qt::MouseButtons, Qt::KeyboardModifiers modifiers)
{
  bool deleteNotTrash = ((modifiers & Qt::ShiftModifier) != 0);
  if (deleteNotTrash) 
  {
    del();
    return;
  }
  else
  {
    KonqOperations::del(_view, KonqOperations::TRASH, _view->selectedUrls());
    KonqOperations* o = _view->findChild<KonqOperations*>("KonqOperations");
    if (o) connect(o, SIGNAL(destroyed()), SLOT(refresh()));
    return;
  }
}

void FSViewBrowserExtension::copySelection( bool move )
{
  QMimeData* data = new QMimeData;
  KonqMimeData::populateMimeData( data, KUrl::List(), _view->selectedUrls(), move );
  QApplication::clipboard()->setMimeData( data );
}

void FSViewBrowserExtension::editMimeType()
{
  Inode* i = (Inode*) _view->selection().first();
  if (i)
    KonqOperations::editMimeType( i->mimeType()->name(),_view );
}


// refresh treemap at end of KIO jobs
void FSViewBrowserExtension::refresh()
{
  // only need to refresh common ancestor for all selected items
  TreeMapItem* commonParent = _view->selection().commonParent();
  if (!commonParent) return;

  /* if commonParent is a file, update parent directory */
  if ( !((Inode*)commonParent)->isDir() ) {
    commonParent = commonParent->parent();
    if (!commonParent) return;
  }

  kDebug(90100) << "FSViewPart::refreshing "
	    << ((Inode*)commonParent)->path() << endl;

  _view->requestUpdate( (Inode*)commonParent );
}

void FSViewBrowserExtension::selected(TreeMapItem* i)
{
  if (!i) return;

  KUrl url;
  url.setPath( ((Inode*)i)->path() );
  emit openUrlRequest(url);
}



#include "fsview_part.moc"
