/* This file is part of Validators
 *
 *  Copyright (C) 2008 by  Pino Toscano <pino@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include "tidy_validator.h"

#include "settings.h"

#include <qfile.h>

#include <kdebug.h>

#include <buffio.h>
#include <tidy.h>

#include <config-konq-validator.h>

static Bool tidy_report_filter(TidyDoc tdoc, TidyReportLevel lvl,
                               uint line, uint col, ctmbstr msg)
{
  TidyValidator::Data* d = reinterpret_cast<TidyValidator::Data *>(tidyGetAppData(tdoc));
  Q_ASSERT(d);
  switch (lvl)
  {
    case TidyInfo:
      break;
    case TidyWarning:
      d->warnings.append(TidyReport(QString::fromLocal8Bit(msg), line, col));
      break;
    case TidyAccess:
      d->accesswarns.append(TidyReport(QString::fromLocal8Bit(msg), line, col));
      break;
    case TidyError:
      d->errors.append(TidyReport(QString::fromLocal8Bit(msg), line, col));
      break;
    default: ;
  }
  return yes;
}


TidyValidator::TidyValidator(const QString &fileName)
{
  TidyBuffer errbuf;
  int rc = -1;
  TidyDoc tdoc = tidyCreate();
#ifdef HAVE_TIDY_ULONG_VERSION
  tidySetAppData(tdoc, (ulong)&d);
#else
  tidySetAppData(tdoc, &d);
#endif
  tidyBufInit( &errbuf );
  tidySetErrorBuffer(tdoc, &errbuf);
  tidySetReportFilter(tdoc, tidy_report_filter);
  tidyOptSetInt(tdoc, TidyAccessibilityCheckLevel, ValidatorsSettings::accessibilityLevel());
  rc = tidyParseFile(tdoc, QFile::encodeName(fileName));

  tidyBufFree(&errbuf);
  tidyRelease(tdoc);
}

TidyValidator::TidyValidator(const QByteArray &fileContent)
{
  TidyBuffer errbuf;
  int rc = -1;
  TidyDoc tdoc = tidyCreate();
#ifdef HAVE_TIDY_ULONG_VERSION
  tidySetAppData(tdoc, (ulong)&d);
#else
  tidySetAppData(tdoc, &d);
#endif
  tidyBufInit(&errbuf);
  tidySetErrorBuffer(tdoc, &errbuf);
  tidySetReportFilter(tdoc, tidy_report_filter);
  tidyOptSetInt(tdoc, TidyAccessibilityCheckLevel, ValidatorsSettings::accessibilityLevel());
  rc = tidyParseString(tdoc, fileContent);

  tidyBufFree(&errbuf);
  tidyRelease(tdoc);
}
