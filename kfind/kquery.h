/*******************************************************************
* kquery.h
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

#ifndef KQUERY_H
#define KQUERY_H

#include <time.h>

#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include <QtCore/QQueue>
#include <QtCore/QList>
#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtCore/QStringList>

#include <kio/job.h>
#include <kurl.h>
#include <kprocess.h>

class KFileItem;

class KQuery : public QObject
{
  Q_OBJECT

 public:
  KQuery(QObject *parent = 0);
  ~KQuery();

    /* Functions to set Query requirements */
  void setSizeRange( int mode, KIO::filesize_t value1, KIO::filesize_t value2);
  void setTimeRange( time_t from, time_t to );
  void setRegExp( const QString &regexp, bool caseSensitive );
  void setRecursive( bool recursive );
  void setPath(const KUrl & url );
  void setFileType( int filetype );
  void setMimeType( const QStringList & mimetype );
  void setContext( const QString & context, bool casesensitive,
  bool search_binary, bool useRegexp );
  void setUsername( const QString &username );
  void setGroupname( const QString &groupname );
  void setMetaInfo(const QString &metainfo, const QString &metainfokey);
  void setUseFileIndex(bool);
  void setShowHiddenFiles(bool);

  void start();
  void kill();
  const KUrl& url()              {return m_url;}

 private:
  /* Check if file meets the find's requirements*/
  inline void processQuery(const KFileItem &);

 public Q_SLOTS:
  /* List of files found using slocate */
  void slotListEntries(QStringList);
  
 protected Q_SLOTS:
  /* List of files found using KIO */
  void slotListEntries(KIO::Job *, const KIO::UDSEntryList &);
  void slotResult(KJob *);
  void slotCanceled(KJob *);
  
  void slotreadyReadStandardOutput();
  void slotreadyReadStandardError();
  void slotendProcessLocate(int, QProcess::ExitStatus);

 Q_SIGNALS:
    void foundFileList( QList< QPair<KFileItem,QString> >);
    void result(int);

 private:
  void checkEntries();

  int m_filetype;
  int m_sizemode;
  KIO::filesize_t m_sizeboundary1;
  KIO::filesize_t m_sizeboundary2;
  KUrl m_url;
  time_t m_timeFrom;
  time_t m_timeTo;
  QRegExp m_regexp;// regexp for file content
  bool m_recursive;
  QStringList m_mimetype;
  QString m_context;
  QString m_username;
  QString m_groupname;
  QString m_metainfo;
  QString m_metainfokey;
  bool m_casesensitive;
  bool m_search_binary;
  bool m_regexpForContent;
  bool m_useLocate;
  bool m_showHiddenFiles;
  QByteArray bufferLocate;
  QStringList locateList;
  KProcess *processLocate;
  QList<QRegExp*> m_regexps;// regexps for file name
//  QValueList<bool> m_regexpsContainsGlobs;  // what should this be good for ? Alex
  KIO::ListJob *job;
  bool m_insideCheckEntries;
  QQueue<KFileItem> m_fileItems;
  QRegExp metaKeyRx;
  int m_result;
  QStringList ignore_mimetypes;
  QStringList ooo_mimetypes;     // OpenOffice.org mimetypes
  QStringList koffice_mimetypes;
  
  QList< QPair<KFileItem,QString> > m_foundFilesList;
};

#endif
