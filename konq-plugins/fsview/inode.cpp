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
 * FSView specialisaton of TreeMapItem class.
 */


#include <kurl.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include "inode.h"
#include "scan.h"
#include "fsview.h"

// Inode

Inode::Inode()
{
  _dirPeer = 0;
  _filePeer = 0;
  init("");
}

Inode::Inode(ScanDir* d, Inode* parent)
  : TreeMapItem(parent)
{
  QString absPath;
  if (parent) {
    absPath = parent->path();
    if (!absPath.endsWith("/")) absPath += '/';
  }
  absPath += d->name();

  _dirPeer = d;
  _filePeer = 0;

  init(absPath);
}

Inode::Inode(ScanFile* f, Inode* parent)
  : TreeMapItem(parent)
{
  QString absPath;
  if (parent)
    absPath = parent->path() + '/';
  absPath += f->name();

  _dirPeer = 0;
  _filePeer = f;

  init(absPath);
}

Inode::~Inode()
{
  if (0) kDebug(90100) << "~Inode [" << path()
			<< "]" << endl;

  /* reset Listener of old Peer */
  if (_dirPeer)
    _dirPeer->setListener(0);
  if (_filePeer)
    _filePeer->setListener(0);
}

void Inode::setPeer(ScanDir* d)
{
  /* reset Listener of old Peer */
  if (_dirPeer)
    _dirPeer->setListener(0);
  if (_filePeer)
    _filePeer->setListener(0);

  _dirPeer = d;
  _filePeer = 0;
  init(d->name());
}

QString Inode::path() const
{
  return _info.absoluteFilePath();
}

void Inode::init(const QString& path)
{
  if (0) kDebug(90100) << "Inode::init [" << path
			<< "]" << endl;

  _info = QFileInfo(path);

  if (!FSView::getDirMetric(path, _sizeEstimation, 
			    _fileCountEstimation, 
			    _dirCountEstimation)) {
    _sizeEstimation = 0.0;
    _fileCountEstimation = 0;
    _dirCountEstimation = 0;
  }

  _mimeSet = false;
  _mimePixmapSet = false;
  _resortNeeded = false;

  clear();

  /* we want to get notifications about dir changes */
  if (_dirPeer)
    _dirPeer->setListener(this);
  if (_filePeer)
    _filePeer->setListener(this);
  
  if (_dirPeer && _dirPeer->scanFinished())
    scanFinished(_dirPeer);
}

/* ScanListener interface */
void Inode::sizeChanged(ScanDir* d)
{
  if (0) kDebug(90100) << "Inode::sizeChanged [" << path() << "] in "
		   << d->name() << ": size " << d->size() << endl;

  _resortNeeded = true;
}

void Inode::scanFinished(ScanDir* d)
{
  if (0) kDebug(90100) << "Inode::scanFinished [" << path() << "] in "
		   << d->name() << ": size " << d->size() << endl;

  _resortNeeded = true;

  /* no estimation any longer */
  _sizeEstimation = 0.0;
  _fileCountEstimation = 0;
  _dirCountEstimation = 0;

  // cache metrics if "important" (for "/usr" is dd==3)
  int dd = ((FSView*)widget())->pathDepth() + depth();
  int files = d->fileCount();
  int dirs = d->dirCount();

  if ((files < 500) && (dirs < 50)) {
    if (dd>4 && (files < 50) && (dirs < 5)) return;
  }

  FSView::setDirMetric(path(), d->size(), files, dirs);
}

void Inode::destroyed(ScanDir* d)
{
  if (_dirPeer == d) _dirPeer = 0;

  // remove children
  clear();
}

void Inode::destroyed(ScanFile* f)
{
  if (_filePeer == f) _filePeer = 0;
}



TreeMapItemList* Inode::children()
{
  if (!_dirPeer) return 0;

  if (!_children) {
    if (!_dirPeer->scanStarted()) return 0;

    _children = new TreeMapItemList;

    setSorting(-1);

    ScanFileVector& files = _dirPeer->files();
    if (files.count()>0) {
      ScanFileVector::iterator it;
      for( it = files.begin(); it != files.end(); ++it )
	new Inode( &(*it), this);
    }

    ScanDirVector& dirs = _dirPeer->dirs();
    if (dirs.count()>0) {
      ScanDirVector::iterator it;
      for( it = dirs.begin(); it != dirs.end(); ++it ) {
	new Inode( &(*it), this);
      }
    }

    setSorting(-2);
    _resortNeeded = false;
  }

  if (_resortNeeded) {    
    resort();
    _resortNeeded = false;
  }

  return _children;
}  



double Inode::size() const
{
  // sizes of files are always correct
  if (_filePeer) return _filePeer->size();
  if (!_dirPeer) return 0;

  double size = _dirPeer->size();  
  return (_sizeEstimation > size) ? _sizeEstimation : size;
}

double Inode::value() const
{
  return size();
}

unsigned int Inode::fileCount() const
{
  unsigned int fileCount = 1;

  if (_dirPeer) fileCount = _dirPeer->fileCount();

  if (_fileCountEstimation > fileCount)
    fileCount = _fileCountEstimation;

  return fileCount;
}

unsigned int Inode::dirCount() const
{
  unsigned int dirCount = 0;

  if (_dirPeer) dirCount = _dirPeer->dirCount();

  if (_dirCountEstimation > dirCount)
    dirCount = _dirCountEstimation;

  return dirCount;
}


QColor Inode::backColor() const
{
  QString n;
  int id = 0;

  switch( ((FSView*)widget())->colorMode() ) {
  case FSView::Depth: 
    {
      int d = ((FSView*)widget())->pathDepth() + depth();
      return QColor::fromHsv((100*d)%360, 192,128);
    }

  case FSView::Name:   n = text(0); break;
  case FSView::Owner:  id = _info.ownerId(); break;
  case FSView::Group:  id = _info.groupId(); break;
  case FSView::Mime:   n = text(7); break;

  default:
    break;
  }

  if (id>0) n = QString::number(id);

  if (n.isEmpty())
    return widget()->colorGroup().button();
  
  const char* str = qPrintable(n);
  int h = 0, s = 100;
  while (*str) {
    h = (h * 37 + s* (unsigned)*str) % 256;
    s = (s * 17 + h* (unsigned)*str) % 192;
    str++;
  }
  return QColor::fromHsv(h, 64+s, 192);
}

KMimeType::Ptr Inode::mimeType() const
{
  if (!_mimeSet) {
    KUrl u;
    u.setPath(path());
    _mimeType = KMimeType::findByUrl( u, 0, true, false );

    _mimeSet = true;
  }
  return _mimeType;
}

QString Inode::text(int i) const
{
  if (i==0) {
    QString name;
    if (_dirPeer) {
      name = _dirPeer->name();
      if (!name.endsWith("/")) name += '/';
    }
    else if (_filePeer) name = _filePeer->name();

    return name;
  }
  if (i==1) {
    QString text;
    double s = size();

    if (s < 1000)
      text = QString("%1 B").arg((int)(s+.5));
    else if (s < 10 * 1024)
      text = QString("%1 kB").arg(KGlobal::locale()->formatNumber(s/1024+.005,2));
    else if (s < 100 * 1024)
      text = QString("%1 kB").arg(KGlobal::locale()->formatNumber(s/1024+.05,1));
    else if (s < 1000 * 1024)
      text = QString("%1 kB").arg((int)(s/1024+.5));
    else if (s < 10 * 1024 * 1024)
      text = QString("%1 MB").arg(KGlobal::locale()->formatNumber(s/1024/1024+.005,2));
    else if (s < 100 * 1024 * 1024)
      text = QString("%1 MB").arg(KGlobal::locale()->formatNumber(s/1024/1024+.05,1));
    else if (s < 1000 * 1024 * 1024)
      text = QString("%1 MB").arg((int)(s/1024/1024+.5));
    else
      text =  QString("%1 GB").arg(KGlobal::locale()->formatNumber(s/1024/1024/1024+.005,2));

    if (_sizeEstimation>0) text += '+';
    return text;
  }

  if ((i==2) || (i==3)) {
    /* file/dir count makes no sense for files */
    if (_filePeer) return QString();

    QString text;
    unsigned int f = (i==2) ? fileCount() : dirCount();

    if (f>0) {
      while (f>1000) {
        text = QString("%1 %2").arg(QString::number(f).right(3)).arg(text);
        f /= 1000;
      }
      text = QString("%1 %2").arg(QString::number(f)).arg(text);
      if (_fileCountEstimation>0) text += '+';
    }
    return text;
  }

  if (i==4) return _info.lastModified().toString();
  if (i==5) return _info.owner();
  if (i==6) return _info.group();
  if (i==7) return mimeType()->comment();
  return QString();
}

QPixmap Inode::pixmap(int i) const
{
  if (i!=0) return QPixmap();

  if (!_mimePixmapSet) {
    KUrl u(path());
    _mimePixmap = KIconLoader::global()->loadMimeTypeIcon(mimeType()->iconName(u), KIconLoader::Small);
    _mimePixmapSet = true;
  }
  return _mimePixmap;
}
