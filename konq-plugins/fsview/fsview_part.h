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
 * The KPart embedding the FSView widget
 */

#ifndef FSVIEW_PART_H
#define FSVIEW_PART_H

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kio/jobclasses.h>

#include "fsview.h"

class KAboutData;
class KActionMenu;

class FSViewPart;

class FSViewBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

public:
  explicit FSViewBrowserExtension(FSViewPart *viewPart);
  ~FSViewBrowserExtension();

protected slots:
  void selected(TreeMapItem*);
  void refresh();

  void copy() { copySelection( false ); }
  void cut() { copySelection( true ); }
  void trash(Qt::MouseButtons, Qt::KeyboardModifiers modifiers);
  void del();
  void editMimeType();

private:
  void copySelection( bool move );

  FSView* _view;
};

class FSJob: public KIO::Job
{
  Q_OBJECT

public:
  explicit FSJob(FSView*);

  virtual void kill( bool quietly = true );

public slots:
  void progressSlot(int percent, int dirs, const QString& lastDir);

private:
  FSView* _view;
};


class FSViewPart : public KParts::ReadOnlyPart
{
  Q_OBJECT
  Q_PROPERTY( bool supportsUndo READ supportsUndo )
public:
  FSViewPart(QWidget *parentWidget,
             QObject *parent, const QList<QVariant> &args);

  virtual ~FSViewPart();

  bool supportsUndo() const { return false; }

  FSView* view() const { return _view; }

public slots:
  void updateActions();
  void contextMenu(TreeMapItem*,const QPoint&);
  void showInfo();
  void showHelp();
  void startedSlot();
  void completedSlot(int dirs);
  void slotShowVisMenu();
  void slotShowAreaMenu();
  void slotShowDepthMenu();
  void slotShowColorMenu();
  void slotSettingsChanged(int);
  void slotProperties();

protected:
  /**
   * This must be implemented by each part
   */
  virtual bool openFile();
  virtual bool openUrl(const KUrl &url);
  virtual bool closeUrl();

private:
  FSView* _view;
  FSJob* _job;
  FSViewBrowserExtension* _ext;
  KActionMenu *_visMenu, *_areaMenu, *_depthMenu, *_colorMenu;
  void setNonStandardActionEnabled(const char* actionName, bool enabled);
};

#endif // FSVIEW_PART_H
