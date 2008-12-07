#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QMap>

class CSSTemplate
{
public:

  CSSTemplate(const QString& templateFilename):m_templateFilename(templateFilename) {}
  bool expandToFile(const QString& outputFilename, const QMap<QString,QString> &dict);
  QString expandToString(const QMap<QString,QString> &dict);

protected:
  void doExpand(QTextStream &is, QTextStream &os, const QMap<QString,QString> &dict);

  QString m_templateFilename;
};


#endif
