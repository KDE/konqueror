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

/* Test Directory Scanning. Usually not build. */

#include <stdio.h>
#include <unistd.h>

#include "scan.h"

class MyListener: public ScanListener
{
public:
  void scanStarted(ScanDir* d)
  { 
    printf("Started Scan on %s\n", qPrintable(d->name()));
  };

  void sizeChanged(ScanDir* d)
  {
    printf("Change in %s: Dirs %d, Files %d",
	   qPrintable(d->name()),
	   d->dirCount(), d->fileCount());
    printf("Size %llu\n", (unsigned long long int)d->size());
  }

  void scanFinished(ScanDir* d)
  {
    printf("Finished Scan on %s\n", qPrintable(d->name()));
  }
};

int main(int argc, char* argv[])
{
  ScanManager m("/opt");
  if (argc>1) m.setTop(argv[1]);

  m.setListener(new MyListener());
  m.startScan();
  while(m.scan(1));
}
