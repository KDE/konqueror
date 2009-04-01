/*******************************************************************
* kfinddlg.h
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#ifndef KFINDDLG_H
#define KFINDDLG_H

#include <kdialog.h>
#include <kdirlister.h>
#include <kdirwatch.h>

class QString;
class QDir;

class KQuery;
class KUrl;
class KFileItem;
class KfindTabWidget;
class KFindTreeView;
class KStatusBar;

class KfindDlg: public KDialog
{
Q_OBJECT

public:
  explicit KfindDlg(const KUrl & url, QWidget * parent = 0);
  ~KfindDlg();
  void copySelection();

  void setStatusMsg(const QString &);
  void setProgressMsg(const QString &);

private:
  /*Return a QStringList of all subdirs of d*/
  QStringList getAllSubdirs(QDir d);

public Q_SLOTS:
  void startSearch();
  void stopSearch();
  void newSearch();
  void addFiles( const QList< QPair<KFileItem,QString> > & );
  void setFocus();
  void slotResult(int);
//  void slotSearchDone();
  void  about ();
  void slotDeleteItem(const QString&);
  void slotNewItems( const QString&  );
  
  void finishAndClose();

Q_SIGNALS:
  void haveResults(bool);
  void resultSelected(bool);

private:
  KfindTabWidget *tabWidget;
  KFindTreeView * win;

  bool isResultReported;
  KQuery *query;
  KStatusBar *mStatusBar;
  KDirLister *dirlister;
  KDirWatch *dirwatch;
};

#endif
