/*******************************************************************
* kfind.h
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

#ifndef KFIND_H
#define KFIND_H

#include <QtGui/QWidget>

class KPushButton;

class KQuery;
class KUrl;
class KfindTabWidget;
class KDirLister;

class Kfind: public QWidget
{
    Q_OBJECT

public:
    Kfind(QWidget * parent = 0);
    ~Kfind();

    void setURL( const KUrl &url );

    void setQuery(KQuery * q) { query = q; }
    void searchFinished();

    void saveState( QDataStream *stream );
    void restoreState( QDataStream *stream );

public Q_SLOTS:
    void startSearch();
    void stopSearch();
    void saveResults();

Q_SIGNALS:
    void haveResults(bool);
    void resultSelected(bool);

    void started();
    void destroyMe();

private:
    void setFocus();
    KfindTabWidget *tabWidget;
    KPushButton *mSearch;
    KPushButton *mStop;
    KPushButton *mSave;
    KQuery *query;

public:
    KDirLister *dirlister;
};

#endif
