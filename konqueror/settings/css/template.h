#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include <QtCore/QString>
#include <QtCore/QMap>

class CSSTemplate
{
public:

  CSSTemplate(QString fname) : _filename(fname) {}
  bool expand(const QString &destname, const QMap<QString,QString> &dict);

protected:
  QString _filename;

};


#endif
