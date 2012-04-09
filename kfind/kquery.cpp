/*******************************************************************
* kquery.cpp
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

#include "kquery.h"

#include <stdlib.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtCore/QTextStream>
#include <QtCore/QList>
#include <kdebug.h>
#include <kmimetype.h>
#include <kfileitem.h>
#include <kfilemetainfo.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kzip.h>

KQuery::KQuery(QObject *parent)
  : QObject(parent),
    m_filetype(0), m_sizemode(0), m_sizeboundary1(0),
    m_sizeboundary2(0), m_timeFrom(0), m_timeTo(0),
    m_recursive(false),m_casesensitive(false),
    m_search_binary(false), m_regexpForContent(false),
    m_useLocate(false), m_showHiddenFiles(false),
    job(0), m_insideCheckEntries(false), m_result(0)
{
  processLocate = new KProcess(this);
  connect(processLocate,SIGNAL(readyReadStandardOutput()),this,SLOT(slotreadyReadStandardOutput()));
  connect(processLocate,SIGNAL(readyReadStandardError()),this,SLOT(slotreadyReadStandardError()));
  connect(processLocate,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(slotendProcessLocate(int,QProcess::ExitStatus)));

  // Files with these mime types can be ignored, even if
  // findFormatByFileContent() in some cases may claim that
  // these are text files:
  ignore_mimetypes.append("application/pdf");
  ignore_mimetypes.append("application/postscript");

  // PLEASE update the documentation when you add another
  // file type here:
  ooo_mimetypes.append("application/vnd.sun.xml.writer");
  ooo_mimetypes.append("application/vnd.sun.xml.calc");
  ooo_mimetypes.append("application/vnd.sun.xml.impress");
  // OASIS mimetypes, used by OOo-2.x and KOffice >= 1.4
  //ooo_mimetypes.append("application/vnd.oasis.opendocument.chart");
  //ooo_mimetypes.append("application/vnd.oasis.opendocument.graphics");
  //ooo_mimetypes.append("application/vnd.oasis.opendocument.graphics-template");
  //ooo_mimetypes.append("application/vnd.oasis.opendocument.formula");
  //ooo_mimetypes.append("application/vnd.oasis.opendocument.image");
  ooo_mimetypes.append("application/vnd.oasis.opendocument.presentation-template");
  ooo_mimetypes.append("application/vnd.oasis.opendocument.presentation");
  ooo_mimetypes.append("application/vnd.oasis.opendocument.spreadsheet-template");
  ooo_mimetypes.append("application/vnd.oasis.opendocument.spreadsheet");
  ooo_mimetypes.append("application/vnd.oasis.opendocument.text-template");
  ooo_mimetypes.append("application/vnd.oasis.opendocument.text");
  // KOffice-1.3 mimetypes
  koffice_mimetypes.append("application/x-kword");
  koffice_mimetypes.append("application/x-kspread");
  koffice_mimetypes.append("application/x-kpresenter");
}

KQuery::~KQuery()
{
  while (!m_regexps.isEmpty())
    delete m_regexps.takeFirst();
  m_fileItems.clear();
  if( processLocate->state() == QProcess::Running)
  {
    disconnect( processLocate );
    processLocate->kill();
    processLocate->waitForFinished( 5000 );
    delete processLocate;
  }
}

void KQuery::kill()
{
  if (job)
    job->kill(KJob::EmitResult);
  if (processLocate->state() == QProcess::Running)
    processLocate->kill();
  m_fileItems.clear();
}

void KQuery::start()
{
  m_fileItems.clear();
  if( m_useLocate ) //Use "locate" instead of the internal search method
  {
    bufferLocate.clear();
    m_url.cleanPath();

    processLocate->clearProgram();
    processLocate->setProgram( "locate", QStringList() <<  m_url.path( KUrl::AddTrailingSlash ) );

    processLocate->setOutputChannelMode(KProcess::SeparateChannels);
    processLocate->start();
  }
  else //Use KIO
  {
    if (m_recursive)
      job = KIO::listRecursive( m_url, KIO::HideProgressInfo );
    else
      job = KIO::listDir( m_url, KIO::HideProgressInfo );

    connect(job, SIGNAL(entries(KIO::Job*,KIO::UDSEntryList)),
        SLOT(slotListEntries(KIO::Job*,KIO::UDSEntryList)));
    connect(job, SIGNAL(result(KJob*)), SLOT(slotResult(KJob*)));
    connect(job, SIGNAL(canceled(KJob*)), SLOT(slotCanceled(KJob*)));
  }
}

void KQuery::slotResult( KJob * _job )
{
  if (job != _job) return;
  job = 0;

  m_result=_job->error();
  checkEntries();
}

void KQuery::slotCanceled( KJob * _job )
{
  if (job != _job) return;
  job = 0;

  m_fileItems.clear();

  m_result=KIO::ERR_USER_CANCELED;
  checkEntries();
}

void KQuery::slotListEntries(KIO::Job*, const KIO::UDSEntryList& list)
{
  const KIO::UDSEntryList::ConstIterator end = list.constEnd();
  
  for (KIO::UDSEntryList::ConstIterator it = list.constBegin(); it != end; ++it)
    m_fileItems.enqueue(KFileItem(*it, m_url, true, true));
      
  checkEntries();
}

void KQuery::checkEntries()
{
  if (m_insideCheckEntries) return;
      
  m_insideCheckEntries=true;
  
  metaKeyRx = QRegExp(m_metainfokey);
  metaKeyRx.setPatternSyntax( QRegExp::Wildcard );
  
  m_foundFilesList.clear();

  int processingCount = 0;
  while( !m_fileItems.isEmpty() ) 
  {
    processQuery( m_fileItems.dequeue() );
    processingCount++;
    
    /* This is a workaround. As the kapp->processEvents() call inside processQuery
     * will bring more KIO entries, m_fileItems will increase even inside this loop
     * and that will lead to a big loop, it will take time to report found items to the GUI
     * so we are going to force emit results every 100 files processed */
    if( processingCount==100 )
    {
      processingCount = 0;
      if( m_foundFilesList.size() > 0 )
      {
        emit foundFileList( m_foundFilesList );
        m_foundFilesList.clear();
      }
    }
  }

  if( m_foundFilesList.size() > 0 )
    emit foundFileList( m_foundFilesList );
  
  if (job==0)
    emit result(m_result);
      
  m_insideCheckEntries=false;
}

/* List of files found using slocate */
void KQuery::slotListEntries( QStringList list )
{
  metaKeyRx = QRegExp(m_metainfokey);
  metaKeyRx.setPatternSyntax( QRegExp::Wildcard );

  QStringList::const_iterator it = list.constBegin();
  QStringList::const_iterator end = list.constEnd();

  m_foundFilesList.clear();
  for (; it != end; ++it)
    processQuery( KFileItem( KFileItem::Unknown, KFileItem::Unknown, KUrl(*it)) );

  if( m_foundFilesList.size() > 0 )
    emit foundFileList( m_foundFilesList );
      
}

/* Check if file meets the find's requirements*/
void KQuery::processQuery( const KFileItem &file)
{
  if ( file.name() == "." || file.name() == ".." )
    return;
    
  if ( !m_showHiddenFiles && file.isHidden() )
    return;
  
  bool matched=false;

  QListIterator<QRegExp *> nextItem( m_regexps );
  while ( nextItem.hasNext() )
  {
    QRegExp *reg = nextItem.next();
    matched = matched || ( reg == 0L ) || ( reg->exactMatch( file.url().fileName( KUrl::IgnoreTrailingSlash ) ) ) ;
  }
  if (!matched)
    return;

  // make sure the files are in the correct range
  switch( m_sizemode )
  {
    case 1: // "at least"
      if ( file.size() < m_sizeboundary1 ) return;
      break;
    case 2: // "at most"
      if ( file.size() > m_sizeboundary1 ) return;
      break;
    case 3: // "equal"
      if ( file.size() != m_sizeboundary1 ) return;
      break;
    case 4: // "between"
      if ( (file.size() < m_sizeboundary1) ||
              (file.size() > m_sizeboundary2) ) return;
      break;
    case 0: // "none" -> Fall to default
    default:
            break;
  }

  // make sure it's in the correct date range
  // what about 0 times?
  if ( m_timeFrom && ((uint) m_timeFrom) > file.time(KFileItem::ModificationTime).toTime_t() )
    return;
  if ( m_timeTo && ((uint) m_timeTo) < file.time(KFileItem::ModificationTime).toTime_t() )
    return;

  // username / group match
  if ( (!m_username.isEmpty()) && (m_username != file.user()) )
    return;
  if ( (!m_groupname.isEmpty()) && (m_groupname != file.group()) )
    return;

  // file type
  switch (m_filetype)
  {
    case 0:
      break;
    case 1: // plain file
      if ( !S_ISREG( file.mode() ) )
        return;
      break;
    case 2:
      if ( !file.isDir() )
        return;
      break;
    case 3:
      if ( !file.isLink() )
        return;
      break;
    case 4:
      if ( !S_ISCHR ( file.mode() ) && !S_ISBLK ( file.mode() ) &&
            !S_ISFIFO( file.mode() ) && !S_ISSOCK( file.mode() ) )
            return;
      break;
    case 5: // binary
      if ( (file.permissions() & 0111) != 0111 || file.isDir() )
        return;
      break;
    case 6: // suid
      if ( (file.permissions() & 04000) != 04000 ) // fixme
        return;
      break;
    default:
      if (!m_mimetype.isEmpty() && !m_mimetype.contains(file.mimetype()))
        return;
  }

  // match data in metainfo...
  if ((!m_metainfo.isEmpty())  && (!m_metainfokey.isEmpty()))
  {
      //Avoid sequential files (fifo,char devices)
      if (!file.isRegularFile())
        return;
          
      bool foundmeta=false;
      QString filename = file.url().path();

      if(filename.startsWith( QString("/dev/") ))
        return;

      KFileMetaInfo metadatas(filename);
      QStringList metakeys;
      QString strmetakeycontent;

      metakeys = metadatas.supportedKeys();
      for (QStringList::const_iterator it = metakeys.constBegin(); it != metakeys.constEnd(); ++it )
      {
        if (!metaKeyRx.exactMatch(*it))
          continue;
        strmetakeycontent=metadatas.item(*it).value().toString();
        if(strmetakeycontent.indexOf(m_metainfo)!=-1)
        {
          foundmeta=true;
          break;
        }
      }
      if (!foundmeta)
        return;
  }

  // match contents...
  QString matchingLine;
  if (!m_context.isEmpty())
  {
    //Avoid sequential files (fifo,char devices)
    if (!file.isRegularFile())
        return;
        
    if( !m_search_binary && ignore_mimetypes.indexOf(file.mimetype()) != -1 ) {
      kDebug() << "ignoring, mime type is in exclusion list: " << file.url();
      return;
    }

    bool found = false;
    bool isZippedOfficeDocument=false;
    int matchingLineNumber=0;

    // FIXME: doesn't work with non local files

    QString filename;
    QTextStream* stream=0;
    QFile qf;
    QRegExp xmlTags;
    QByteArray zippedXmlFileContent;

    // KWord's and OpenOffice.org's files are zipped...
    if( ooo_mimetypes.indexOf(file.mimetype()) != -1 ||
        koffice_mimetypes.indexOf(file.mimetype()) != -1 )
    {
      KZip zipfile(file.url().path());
      KZipFileEntry *zipfileEntry;

      if(zipfile.open(QIODevice::ReadOnly))
      {
        const KArchiveDirectory *zipfileContent = zipfile.directory();

        if( koffice_mimetypes.indexOf(file.mimetype()) != -1 )
          zipfileEntry = (KZipFileEntry*)zipfileContent->entry("maindoc.xml");
        else
          zipfileEntry = (KZipFileEntry*)zipfileContent->entry("content.xml"); //for OpenOffice.org

        if(!zipfileEntry) {
          kWarning() << "Expected XML file not found in ZIP archive " << file.url() ;
          return;
        }

        zippedXmlFileContent = zipfileEntry->data();
        xmlTags.setPattern("<.*>");
        xmlTags.setMinimal(true);
        stream = new QTextStream(zippedXmlFileContent, QIODevice::ReadOnly);
        stream->setCodec("UTF-8");
        isZippedOfficeDocument = true;
      } else {
        kWarning() << "Cannot open supposed ZIP file " << file.url() ;
      }
      
    } else if( !m_search_binary && !file.mimetype().startsWith( QString("text/") ) &&
        file.url().isLocalFile() && !file.url().path().startsWith( QString("/dev") ) ) {
      if ( KMimeType::isBinaryData(file.url().path()) ) {
        kDebug() << "ignoring, not a text file: " << file.url();
        return;
      }
    }

    if(!isZippedOfficeDocument) //any other file or non-compressed KWord
    {
      filename = file.url().path();
      if(filename.startsWith(QString("/dev/")))
        return;
      qf.setFileName(filename);
      qf.open(QIODevice::ReadOnly);
      stream=new QTextStream(&qf);
      stream->setCodec(QTextCodec::codecForLocale());
    }

    while ( ! stream->atEnd() )
    {
      QString str = stream->readLine();
      matchingLineNumber++;

      //If the stream ended (readLine().isNull() is true) the file was read completely
      //Do *not* use isEmpty() because that will exit if there is an empty line in the file
      if (str.isNull()) break;
      if(isZippedOfficeDocument)
        str.remove(xmlTags);

      if (m_regexpForContent)
      {
        if (m_regexp.indexIn(str)>=0)
        {
          matchingLine=QString::number(matchingLineNumber)+": "+str;
          found = true;
          break;
        }
      }
      else
      {
        if (str.indexOf(m_context, 0, m_casesensitive?Qt::CaseSensitive:Qt::CaseInsensitive) != -1)
        {
          matchingLine=QString::number(matchingLineNumber)+": "+str;
          found = true;
          break;
        }
      }
      kapp->processEvents();
    }
    
    delete stream;

    if (!found)
      return;
  }
  
  m_foundFilesList.append( QPair<KFileItem,QString>(file, matchingLine) );
}

void KQuery::setContext(const QString & context, bool casesensitive,
  bool search_binary, bool useRegexp)
{
  m_context = context;
  m_casesensitive = casesensitive;
  m_search_binary = search_binary;
  m_regexpForContent=useRegexp;
  if ( !m_regexpForContent )
    m_regexp.setPatternSyntax(QRegExp::Wildcard);
  else
    m_regexp.setPatternSyntax(QRegExp::RegExp);
  if ( casesensitive )
    m_regexp.setCaseSensitivity(Qt::CaseSensitive);
  else
    m_regexp.setCaseSensitivity(Qt::CaseInsensitive);
  if (m_regexpForContent)
     m_regexp.setPattern(m_context);
}

void KQuery::setMetaInfo(const QString &metainfo, const QString &metainfokey)
{
  m_metainfo=metainfo;
  m_metainfokey=metainfokey;
}

void KQuery::setMimeType(const QStringList &mimetype)
{
  m_mimetype = mimetype;
}

void KQuery::setFileType(int filetype)
{
  m_filetype = filetype;
}

void KQuery::setSizeRange(int mode, KIO::filesize_t value1, KIO::filesize_t value2)
{
  m_sizemode = mode;
  m_sizeboundary1 = value1;
  m_sizeboundary2 = value2;
}

void KQuery::setTimeRange(time_t from, time_t to)
{
  m_timeFrom = from;
  m_timeTo = to;
}

void KQuery::setUsername(const QString &username)
{
  m_username = username;
}

void KQuery::setGroupname(const QString &groupname)
{
  m_groupname = groupname;
}

void KQuery::setRegExp(const QString &regexp, bool caseSensitive)
{
  QRegExp *regExp;
  QRegExp sep(";");
  const QStringList strList=regexp.split( sep, QString::SkipEmptyParts);
  //  QRegExp globChars ("[\\*\\?\\[\\]]", TRUE, FALSE);
  while (!m_regexps.isEmpty())
      delete m_regexps.takeFirst();

  //  m_regexpsContainsGlobs.clear();
  for ( QStringList::ConstIterator it = strList.constBegin(); it != strList.constEnd(); ++it ) {
    regExp = new QRegExp((*it),( caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive ), QRegExp::Wildcard);
    //m_regexpsContainsGlobs.append(regExp->pattern().contains(globChars));
    m_regexps.append(regExp);
  }
}

void KQuery::setRecursive(bool recursive)
{
  m_recursive = recursive;
}

void KQuery::setPath(const KUrl &url)
{
  m_url = url;
}

void KQuery::setUseFileIndex(bool useLocate)
{
  m_useLocate=useLocate;
}

void KQuery::setShowHiddenFiles(bool showHidden)
{
  m_showHiddenFiles = showHidden;
}

void KQuery::slotreadyReadStandardError()
{
  KMessageBox::error(NULL, QString::fromLocal8Bit(processLocate->readAllStandardOutput()), i18nc("@title:window", "Error while using locate"));
}

void KQuery::slotreadyReadStandardOutput()
{
  bufferLocate += processLocate->readAllStandardOutput();
}

void KQuery::slotendProcessLocate(int code, QProcess::ExitStatus)
{
  if (code == 0 )
  {
    if( !bufferLocate.isEmpty() )
    {
      QString str = QString::fromLocal8Bit(bufferLocate);
      bufferLocate.clear();
      slotListEntries(str.split('\n', QString::SkipEmptyParts));
    }
  }
  emit result(0);
}

#include "kquery.moc"
