
// Own
#include "template.h"

// Qt
#include <QtCore/QFile>
#include <QtCore/QTextStream>


bool CSSTemplate::expandToFile(const QString& outputFilename, const QMap<QString,QString> &dict)
{
  QFile inf(m_templateFilename);
  if (!inf.open(QIODevice::ReadOnly)) return false;
  QTextStream is(&inf);
  
  QFile outf(outputFilename);
  if (!outf.open(QIODevice::WriteOnly)) return false;
  QTextStream os(&outf);

  doExpand(is, os, dict);

  inf.close();
  outf.close();
  return true;
}

QString CSSTemplate::expandToString(const QMap<QString,QString> &dict)
{
  QFile inf(m_templateFilename);
  if (!inf.open(QIODevice::ReadOnly)) return QString();
  QTextStream is(&inf);

  QString out;
  QTextStream os(&out);

  doExpand(is, os, dict);

  inf.close();

  return out;
}

// bool CSSTemplate::expand(const QString &destname, const QMap<QString,QString> &dict)
void CSSTemplate::doExpand(QTextStream &is, QTextStream &os, const QMap<QString,QString> &dict)
{
  QString line;
  while (!is.atEnd())
    {
      line = is.readLine();

      int start = line.indexOf('$');
      if (start >= 0)
	{
	  int end = line.indexOf('$', start+1);
	  if (end >= 0)
            {
	      QString expr = line.mid(start+1, end-start-1);
	      QString res = dict[expr];

	      line.replace(start, end-start+1, res);
	    }
	}
      os << line << endl;
    }  
}
