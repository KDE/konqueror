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

#include <qdir.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kurl.h>
#include <kauthorized.h>

#include "scan.h"
#include "inode.h"


// ScanManager

ScanManager::ScanManager()
{
  _topDir = 0;
  _listener = 0;
}

ScanManager::ScanManager(const QString& path)
{
  _topDir = 0;
  _listener = 0;
  setTop(path);
}

ScanManager::~ScanManager()
{
  stopScan();
  delete _topDir;
}

void ScanManager::setListener(ScanListener* l)
{
  _listener = l;
}

ScanDir* ScanManager::setTop(const QString& path, int data)
{
  stopScan();
  if (_topDir) { 
    delete _topDir;
    _topDir = 0;
  }
  if (!path.isEmpty()) {
    _topDir = new ScanDir(path, this, 0, data);
  }
  return _topDir;
}

bool ScanManager::scanRunning()
{
  if (!_topDir) return false;

  return _topDir->scanRunning();
}

void ScanManager::startScan(ScanDir* from)
{
  if (!_topDir) return;
  if (!from) from = _topDir;

  if (scanRunning()) stopScan();

  from->clear();
  if (from->parent())
    from->parent()->setupChildRescan();

  _list.append(new ScanItem(from->path(),from));
}

void ScanManager::stopScan()
{
  if (!_topDir) return;

  if (0) kDebug(90100) << "ScanManager::stopScan, scanLength "
		   << _list.count() << endl;

  while( !_list.isEmpty() ) {
    ScanItem* si = _list.takeFirst();
    si->dir->finish();
    delete si;
  }
}

int ScanManager::scan(int data)
{
  if (_list.isEmpty()) return false;
  ScanItem* si = _list.takeFirst();

  int newCount = si->dir->scan(si, _list, data);
  delete si;

  return newCount;
}


// ScanFile

ScanFile::ScanFile()
{
  _size = 0;
  _listener = 0;
}

ScanFile::ScanFile(const QString& n, KIO::fileoffset_t s)
{
  _name = n;
  _size = s;
  _listener = 0;
}

ScanFile::~ScanFile()
{
  if (_listener) _listener->destroyed(this);
}

// ScanDir

ScanDir::ScanDir()
{
  _dirty = true;
  _dirsFinished = -1; /* scan not started */

  _parent = 0;
  _manager = 0;
  _listener = 0;
  _data = 0;  
}

ScanDir::ScanDir(const QString& n, ScanManager* m,
		 ScanDir* p, int data)
  : _name(n)
{
  _dirty = true;
  _dirsFinished = -1; /* scan not started */

  _parent = p;
  _manager = m;
  _listener = 0;
  _data = data;
}

ScanDir::~ScanDir()
{
  if (_listener) _listener->destroyed(this);
}

void ScanDir::setListener(ScanListener* l)
{
  _listener = l;
}

QString ScanDir::path()
{
  if (_parent) {
    QString p = _parent->path();
    if (!p.endsWith(QLatin1Char('/'))) p += QLatin1Char('/');
    return p + _name;
  }

  return _name;
}

void ScanDir::clear()
{
  _dirty = true;
  _dirsFinished = -1; /* scan not started */

  _files.clear();
  _dirs.clear();
}

void ScanDir::update()
{
  if (!_dirty) return;
  _dirty = false;

  _fileCount = 0;
  _dirCount = 0;
  _size = 0;

  if (_dirsFinished == -1) return;

  if (_files.count()>0) {
    _fileCount += _files.count();
    _size = _fileSize;
  }
  if (_dirs.count()>0) {
    _dirCount += _dirs.count();
    ScanDirVector::iterator it;
    for( it = _dirs.begin(); it != _dirs.end(); ++it ) {
      (*it).update();
      _fileCount += (*it)._fileCount;
      _dirCount  += (*it)._dirCount;
      _size      += (*it)._size;
    }
  }
}

int ScanDir::scan(ScanItem* si, ScanItemList& list, int data)
{
  clear();
  _dirsFinished = 0;
  _fileSize = 0;
  _dirty = true;

  KUrl u;
  u.setPath(si->absPath);
  if (!KAuthorized::authorizeUrlAction("list", KUrl(), u)) {
    if (_parent)
      _parent->subScanFinished();

    return 0;
  }

  QDir d(si->absPath);
  const QStringList fileList = d.entryList( QDir::Files |
				      QDir::Hidden | QDir::NoSymLinks );

  if (fileList.count()>0) {
    KDE_struct_stat buff;

    _files.reserve(fileList.count());

    QStringList::ConstIterator it;
    for (it = fileList.constBegin(); it != fileList.constEnd(); ++it ) {
      KDE::lstat( si->absPath + QLatin1Char('/') + (*it), &buff );
      _files.append( ScanFile(*it, buff.st_size) );
      _fileSize += buff.st_size;
    }
  }

  const QStringList dirList = d.entryList( QDir::Dirs | 
				     QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot );

  if (dirList.count()>0) {
    _dirs.reserve(dirList.count());

    QStringList::ConstIterator it;
    for (it = dirList.constBegin(); it != dirList.constEnd(); ++it ) {      
      _dirs.append( ScanDir(*it, _manager, this, data) );
      list.append( new ScanItem( si->absPath + '/' + (*it), 
				 &(_dirs.last()) ));
    }
    _dirCount += _dirs.count();
  }

  callScanStarted();
  callSizeChanged();

  if (_dirs.count() == 0) {
    callScanFinished();

    if (_parent)
      _parent->subScanFinished();
  }

  return _dirs.count();
}

void ScanDir::subScanFinished()
{
  _dirsFinished++;
  callSizeChanged();

  if (0) kDebug(90100) << "ScanDir::subScanFinished [" << path()
			<< "]: " << _dirsFinished << "/" << _dirs.count() << endl;



  if (_dirsFinished < _dirs.count()) return;
  
  /* all subdirs read */
  callScanFinished();

  if (_parent)
    _parent->subScanFinished();
}

void ScanDir::finish()
{
  if (scanRunning()) {
    _dirsFinished = _dirs.count();
    callScanFinished();
  }

  if (_parent)
    _parent->finish();
}

void ScanDir::setupChildRescan()
{
  if (_dirs.count() == 0) return;

  _dirsFinished = 0;
  ScanDirVector::iterator it;
  for( it = _dirs.begin(); it != _dirs.end(); ++it )
    if ((*it).scanFinished()) _dirsFinished++;

  if (_parent && 
      (_dirsFinished < _dirs.count()) )
    _parent->setupChildRescan();

  callScanStarted();
}

void ScanDir::callScanStarted()
{
  if (0) kDebug(90100) << "ScanDir:Started [" << path()
			<< "]: size " << size() << ", files " << fileCount() << endl;

  ScanListener* mListener = _manager ? _manager->listener() : 0;

  if (_listener) _listener->scanStarted(this);
  if (mListener) mListener->scanStarted(this);
}
  
void ScanDir::callSizeChanged()
{
  if (0) kDebug(90100) << ". [" << path()
			<< "]: size " << size() << ", files " << fileCount() << endl;

  _dirty = true;

  if (_parent) _parent->callSizeChanged();

  ScanListener* mListener = _manager ? _manager->listener() : 0;

  if (_listener) _listener->sizeChanged(this);
  if (mListener) mListener->sizeChanged(this);
}

void ScanDir::callScanFinished()
{
  if (0) kDebug(90100) << "ScanDir:Finished [" << path()
			<< "]: size " << size() << ", files " << fileCount() << endl;

  ScanListener* mListener = _manager ? _manager->listener() : 0;

  if (_listener) _listener->scanFinished(this);
  if (mListener) mListener->scanFinished(this);
}

