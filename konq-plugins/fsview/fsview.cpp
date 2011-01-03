/* This file is part of FSView.
   Copyright (C) 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

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
 * FSView specialisaton of TreeMap classes.
 */


#include <qdir.h>
#include <q3popupmenu.h>
#include <qtimer.h>
//Added by qt3to4:

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kurl.h>

#include <kio/global.h>
#include <kauthorized.h>
#include <kconfiggroup.h>

#include "fsview.h"


// FSView

QMap<QString, MetricEntry> FSView::_dirMetric;

FSView::FSView(Inode* base, QWidget* parent)
  : TreeMapWidget(base, parent)
{
  setFieldType(0, i18n("Name"));
  setFieldType(1, i18n("Size"));
  setFieldType(2, i18n("File Count"));
  setFieldType(3, i18n("Directory Count"));
  setFieldType(4, i18n("Last Modified"));
  setFieldType(5, i18n("Owner"));
  setFieldType(6, i18n("Group"));
  setFieldType(7, i18n("Mime Type"));

  // defaults
  setVisibleWidth(4, true);
  setSplitMode(TreeMapItem::Rows);
  setFieldForced(0, true); // show directory names
  setFieldForced(1, true); // show directory sizes
  setSelectionMode(TreeMapWidget::Extended);

  _colorMode = Depth;
  _pathDepth = 0;
  _allowRefresh = true;

  _progressPhase = 0;
  _chunkData1 = 0;
  _chunkData2 = 0;
  _chunkData3 = 0;
  _chunkSize1 = 0;
  _chunkSize2 = 0;
  _chunkSize3 = 0;
  _progressSize = 0;
  _progress = 0;
  _dirsFinished = 0;
  _lastDir = 0;

  _config = new KConfig("fsviewrc");

  // restore TreeMap visualization options of last execution
  KConfigGroup tmconfig(_config, "TreeMap");
  restoreOptions(&tmconfig);
  QString str = tmconfig.readEntry("ColorMode");
  if (!str.isEmpty()) setColorMode(str);

  if (_dirMetric.count() == 0) {
    // restore metric cache
    KConfigGroup cconfig(_config, "MetricCache");
    int ccount = cconfig.readEntry("Count", 0);
    int i, f, d;
    double s;
    QString str;
    for (i=1;i<=ccount;i++) {
      str = QString("Dir%1").arg(i);
      if (!cconfig.hasKey(str)) continue;
      str = cconfig.readPathEntry(str, QString());
      s = cconfig.readEntry(QString("Size%1").arg(i), 0.0);
      f = cconfig.readEntry(QString("Files%1").arg(i), 0);
      d = cconfig.readEntry(QString("Dirs%1").arg(i), 0);
      if (s==0.0 || f==0 || d==0) continue;
      setDirMetric(str, s, f, d);
    }
  }

  _sm.setListener(this);
}

FSView::~FSView()
{
  delete _config;
}

void FSView::stop()
{
  _sm.stopScan();
}

void FSView::setPath(const QString &p)
{
  Inode* b = (Inode*)base();
  if (!b) return;

  //kDebug(90100) << "FSView::setPath " << p;

  // stop any previous updating
  stop();

  QFileInfo fi(p);
  _path = fi.absoluteFilePath();
  if (!fi.isDir()) {
    _path = fi.dirPath(true);
  }
  _pathDepth = _path.count('/');

  KUrl u;
  u.setPath(_path);
  if (!KAuthorized::authorizeUrlAction("list", KUrl(), u))
  {
     QString msg = KIO::buildErrorString(KIO::ERR_ACCESS_DENIED, u.prettyUrl());
     KMessageBox::queuedMessageBox(this, KMessageBox::Sorry, msg);
  }

  ScanDir* d = _sm.setTop(_path);

  b->setPeer(d);

  setCaption(QString("%1 - FSView").arg(_path));
  requestUpdate(b);
}

KUrl::List FSView::selectedUrls()
{
  KUrl::List urls;

  foreach(TreeMapItem* i, selection()) {
    KUrl u;
    u.setPath( ((Inode*)i)->path() );
    urls.append(u);
  }
  return urls;
}

bool FSView::getDirMetric(const QString& k,
			  double& s, unsigned int& f, unsigned int& d)
{
  QMap<QString, MetricEntry>::iterator it;

  it = _dirMetric.find(k);
  if (it == _dirMetric.end()) return false;

  s = (*it).size;
  f = (*it).fileCount;
  d = (*it).dirCount;

  if (0) kDebug(90100) << "getDirMetric " << k;
  if (0) kDebug(90100) << " - got size " << s << ", files " << f;

  return true;
}

void FSView::setDirMetric(const QString& k,
			  double s, unsigned int f, unsigned int d)
{
  if (0) kDebug(90100) << "setDirMetric '" << k << "': size "
		   << s << ", files " << f << ", dirs " << d << endl;
  _dirMetric.insert(k, MetricEntry(s, f, d));
}

void FSView::requestUpdate(Inode* i)
{
  if (0) kDebug(90100) << "FSView::requestUpdate(" << i->path()
		   << ")" << endl;

  ScanDir* peer = i->dirPeer();
  if (!peer) return;

  peer->clear();
  i->clear();

  if (!_sm.scanRunning()) {
    QTimer::singleShot(0, this, SLOT(doUpdate()));
    QTimer::singleShot(100, this, SLOT(doRedraw()));

    /* start new progress chunk */
    _progressPhase = 1;
    _chunkData1 += 3;
    _chunkData2 = _chunkData1 + 1;
    _chunkData3 = _chunkData1 + 2;
    _chunkSize1 = 0;
    _chunkSize2 = 0;
    _chunkSize3 = 0;
    peer->setData(_chunkData1);

    _progressSize = 0;
    _progress = 0;
    _dirsFinished = 0;
    _lastDir = 0;
    emit started();
  }

  _sm.startScan(peer);
}

void FSView::scanFinished(ScanDir* d)
{
  /* if finished directory was from last progress chunk, increment */
  int data = d->data();
  switch(_progressPhase) {
  case 1:
    if (data == _chunkData1) _chunkSize1--;
    break;
  case 2:
    if (data == _chunkData1) _progress++;
    if (data == _chunkData2) _chunkSize2--;
    break;
  case 3:
    if ((data == _chunkData1) ||
	(data == _chunkData2)) _progress++;
    if (data == _chunkData3) _chunkSize3--;
    break;
  case 4:
    if ((data == _chunkData1) ||
	(data == _chunkData2) ||
	(data == _chunkData3)) _progress++;
    break;
  default:
    break;
  }

  _lastDir = d;
  _dirsFinished++;

  if (0) kDebug(90100) << "FSFiew::scanFinished: " << d->path()
		   << ", Data " << data
		   << ", Progress " << _progress << "/"
		   << _progressSize << endl;
}

void FSView::selected(TreeMapItem* i)
{
  setPath(((Inode*)i)->path());
}

void FSView::contextMenu(TreeMapItem* i, const QPoint& p)
{
  KMenu popup;

  KMenu* spopup = new KMenu();
  KMenu* dpopup = new KMenu();
  KMenu* apopup = new KMenu();
  KMenu* fpopup = new KMenu();

  // choosing from the selection menu will give a selectionChanged() signal
  addSelectionItems(spopup, 901, i);
  popup.insertItem(i18n("Go To"), spopup, 900);

  popup.insertItem(i18n("Go Up"), 2);
  popup.insertSeparator();
  popup.insertItem(i18n("Stop Refresh"), 3);
  popup.setItemEnabled(3, _sm.scanRunning());
  popup.insertItem(i18n("Refresh"), 5);
  popup.setItemEnabled(5, !_sm.scanRunning());

  if (i) popup.insertItem(i18n("Refresh '%1'", i->text(0)), 4);
  popup.insertSeparator();
  addDepthStopItems(dpopup, 1001, i);
  popup.insertItem(i18n("Stop at Depth"), dpopup, 1000);
  addAreaStopItems(apopup, 1101, i);
  popup.insertItem(i18n("Stop at Area"), apopup, 1100);
  addFieldStopItems(fpopup, 1201, i);
  popup.insertItem(i18n("Stop at Name"), fpopup, 1200);

  popup.insertSeparator();

  KMenu* cpopup = new KMenu();
  addColorItems(cpopup, 1401);
  popup.insertItem(i18n("Color Mode"), cpopup, 1400);
  KMenu* vpopup = new KMenu();
  addVisualizationItems(vpopup, 1301);
  popup.insertItem(i18n("Visualization"), vpopup, 1300);

  _allowRefresh = false;
  int r = popup.actions().indexOf(popup.exec(mapToGlobal(p)));
  _allowRefresh = true;

  if (r==1)
    selected(i);
  else if (r==2) {
    Inode* i = (Inode*) base();
    if (i) setPath(i->path()+"/..");
  }
  else if (r==3)
    stop();
  else if (r==4) {
    //((Inode*)i)->refresh();
    requestUpdate( (Inode*)i );
  }
  else if (r==5) {
    Inode* i = (Inode*) base();
    if (i) requestUpdate(i);
  }
}

void FSView::saveMetric(KConfigGroup* g)
{
  QMap<QString, MetricEntry>::iterator it;
  int c = 1;
  for (it=_dirMetric.begin();it!=_dirMetric.end();++it) {
    g->writePathEntry(QString("Dir%1").arg(c), it.key());
    g->writeEntry(QString("Size%1").arg(c), (*it).size);
    g->writeEntry(QString("Files%1").arg(c), (*it).fileCount);
    g->writeEntry(QString("Dirs%1").arg(c), (*it).dirCount);
    c++;
  }
  g->writeEntry("Count", c-1);
}

void FSView::setColorMode(FSView::ColorMode cm)
{
  if (_colorMode == cm) return;

  _colorMode = cm;
  redraw();
}

bool FSView::setColorMode(const QString &mode)
{
  if (mode == "None")       setColorMode(None);
  else if (mode == "Depth") setColorMode(Depth);
  else if (mode == "Name")  setColorMode(Name);
  else if (mode == "Owner") setColorMode(Owner);
  else if (mode == "Group") setColorMode(Group);
  else if (mode == "Mime")  setColorMode(Mime);
  else return false;

  return true;
}

QString FSView::colorModeString() const
{
  QString mode;
  switch(_colorMode) {
  case None:  mode = "None"; break;
  case Depth: mode = "Depth"; break;
  case Name:  mode = "Name"; break;
  case Owner: mode = "Owner"; break;
  case Group: mode = "Group"; break;
  case Mime:  mode = "Mime"; break;
  default:    mode = "Unknown"; break;
  }
  return mode;
}

void FSView::addColorItems(KMenu* popup, int id)
{
  _colorID = id;
  popup->setCheckable(true);

  connect(popup, SIGNAL(activated(int)),
          this, SLOT(colorActivated(int)));

  popup->insertItem(i18n("None"),      id);
  popup->insertItem(i18n("Depth"),     id+1);
  popup->insertItem(i18n("Name"),      id+2);
  popup->insertItem(i18n("Owner"),     id+3);
  popup->insertItem(i18n("Group"),     id+4);
  popup->insertItem(i18n("Mime Type"), id+5);

  switch(colorMode()) {
    case None:  popup->setItemChecked(id,true); break;
    case Depth: popup->setItemChecked(id+1,true); break;
    case Name:  popup->setItemChecked(id+2,true); break;
    case Owner: popup->setItemChecked(id+3,true); break;
    case Group: popup->setItemChecked(id+4,true); break;
    case Mime:  popup->setItemChecked(id+5,true); break;
    default: break;
  }
}

void FSView::colorActivated(int id)
{
  if (id == _colorID)        setColorMode(None);
  else if (id == _colorID+1) setColorMode(Depth);
  else if (id == _colorID+2) setColorMode(Name);
  else if (id == _colorID+3) setColorMode(Owner);
  else if (id == _colorID+4) setColorMode(Group);
  else if (id == _colorID+5) setColorMode(Mime);
}

void FSView::keyPressEvent( QKeyEvent* e)
{
  if (e->key() == Qt::Key_Escape && !_pressed && (selection().size() > 0)) {
    // For consistency with Dolphin, deselect all on Escape if we're not dragging.
    TreeMapItem* changed = selection().commonParent();
    if (changed) {
      clearSelection(changed);
    }
  }
  else {
    TreeMapWidget::keyPressEvent(e);
  }
}

void FSView::saveFSOptions()
{
  KConfigGroup tmconfig(_config, "TreeMap");
  saveOptions(&tmconfig);
  tmconfig.writeEntry("ColorMode", colorModeString());

  KConfigGroup gconfig(_config, "General");
  gconfig.writeEntry("Path", _path);

  KConfigGroup cconfig(_config, "MetricCache");
  saveMetric(&cconfig);
}

void FSView::quit()
{
  saveFSOptions();
  KApplication::kApplication()->quit();
}

void FSView::doRedraw()
{
  // we update progress every 1/4 second, and redraw every second
  static int redrawCounter = 0;

  bool redo = _sm.scanRunning();
  if (!redo) redrawCounter = 0;

  if ((_progress>0) && (_progressSize>0) && _lastDir) {
    int percent = _progress * 100 / _progressSize;
    if (0) kDebug(90100) << "FSView::progress "
		     << _progress << "/" << _progressSize
		     << "= " << percent << "%, "
		     << _dirsFinished << " dirs read, in "
		     << _lastDir->path() << endl;
    emit progress(percent, _dirsFinished, _lastDir->path());
  }


  if (_allowRefresh && ((redrawCounter%4)==0)) {
    if (0) kDebug(90100) << "doRedraw " << _sm.scanLength();
    redraw();
  }
  else
    redo = true;

  if (redo) {
    QTimer::singleShot(500, this, SLOT(doRedraw()));
    redrawCounter++;
  }
}


void FSView::doUpdate()
{
  for(int i=0;i<5;i++) {
    switch(_progressPhase) {
    case 1:
      _chunkSize1 += _sm.scan(_chunkData1);
      if (_chunkSize1 > 100) {
	_progressPhase = 2;

	/* Go to maximally 33% by scaling with 3 */
	_progressSize = 3 * _chunkSize1;

	if (1) kDebug(90100) << "Phase 2: CSize " << _chunkSize1;
      }
      break;

    case 2:
      /* progress phase 2 */
      _chunkSize2 += _sm.scan(_chunkData2);
      /* switch to Phase 3 if we reach 80 % of Phase 2 */
      if (_progress * 3 > _progressSize * 8/10) {
	_progressPhase = 3;

	/* Goal: Keep percentage equal from phase 2 to 3 */
	double percent = (double)_progress / _progressSize;
	/* We scale by factor 2/3 aferwards */
	percent = percent * 3/2;

	int todo = _chunkSize2 + (_progressSize/3 - _progress);
	_progressSize = (int) ((double)todo / (1.0 - percent));
	_progress = _progressSize - todo;

	/* Go to maximally 66% by scaling with 1.5 */
	_progressSize = _progressSize *3/2;

	if (1) kDebug(90100) << "Phase 3: CSize " << _chunkSize2
			 << ", Todo " << todo
			 << ", Progress " << _progress
			 << "/" << _progressSize << endl;
      }
      break;

    case 3:
      /* progress phase 3 */
      _chunkSize3 += _sm.scan(_chunkData3);
      /* switch to Phase 4 if we reach 80 % of Phase 3 */
      if (_progress * 3/2 > _progressSize * 8/10) {
	_progressPhase = 4;

	/* Goal: Keep percentage equal from phase 2 to 3 */
	double percent = (double)_progress / _progressSize;
	int todo = _chunkSize3 + (_progressSize*2/3 - _progress);
	_progressSize = (int)((double)todo / (1.0 - percent) + .5);
	_progress = _progressSize - todo;

	if (1) kDebug(90100) << "Phase 4: CSize " << _chunkSize3
			 << ", Todo " << todo
			 << ", Progress " << _progress
			 << "/" << _progressSize << endl;
      }

    default:
      _sm.scan(-1);
      break;
    }
  }

  if (_sm.scanRunning())
    QTimer::singleShot(0, this, SLOT(doUpdate()));
  else
    emit completed(_dirsFinished);
}

#include "fsview.moc"
