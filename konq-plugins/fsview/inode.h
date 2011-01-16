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

#ifndef INODE_H
#define INODE_H

#include <qmap.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <QPixmap>

#include <kmimetype.h>

#include "treemap.h"
#include "scan.h"


/**
 * A specialized version of a TreeMapItem
 * for representation of an Directory or File.
 *
 * These are dynamically created on drawing.
 * The real breadth-first scanning of the filesystem
 * uses ScanDir:scan.
 */
class Inode: public TreeMapItem, public ScanListener
{
public:
  Inode();
  Inode(ScanDir*, Inode*);
  Inode(ScanFile*, Inode*);
  ~Inode();
  void init(const QString&);

  void setPeer(ScanDir*);

  TreeMapItemList* children();

  double value() const;
  double size() const;
  unsigned int fileCount() const;
  unsigned int dirCount() const;
  QString path() const;
  QString text(int i) const;
  QPixmap pixmap(int i) const;
  QColor backColor() const;
  KMimeType::Ptr mimeType() const;

  const QFileInfo& fileInfo() const { return _info; }
  ScanDir* dirPeer() { return _dirPeer; }
  ScanFile* filePeer() { return _filePeer; }
  bool isDir() { return (_dirPeer != 0); }

  void sizeChanged(ScanDir*);
  void scanFinished(ScanDir*);
  void destroyed(ScanDir*);
  void destroyed(ScanFile*);

private:
  void setMetrics(double, unsigned int);

  QFileInfo _info;
  ScanDir* _dirPeer;
  ScanFile* _filePeer;

  double _sizeEstimation;
  unsigned int _fileCountEstimation, _dirCountEstimation;

  bool _resortNeeded;

  // Cached values, calculated lazy.
  // This means a change even in const methods, thus has to be "mutable"
  mutable bool _mimeSet, _mimePixmapSet;
  mutable KMimeType::Ptr _mimeType;
  mutable QPixmap _mimePixmap;
};

#endif
