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

#ifndef TIDY_VALIDATOR_H
#define TIDY_VALIDATOR_H

#include <qlist.h>
#include <qstring.h>

struct TidyReport
{
  TidyReport(const QString &m, uint l, uint c)
    : msg(m), line(l), col(c)
  {}

  QString msg;
  uint line;
  uint col;
};

struct ValidationResult
{
  QString frameName;
  QList<TidyReport> errors;
  QList<TidyReport> warnings;
  QList<TidyReport> accesswarns;
};

class TidyValidator
{
public:
  TidyValidator(const QString &fileName);
  TidyValidator(const QByteArray &fileContent);

  int errorCount() const { return d.errors.count(); }
  TidyReport error(int i) const { return d.errors.at(i); }
  QList<TidyReport> errors() const { return d.errors; }
  int warningCount() const { return d.warnings.count(); }
  TidyReport warning(int i) const { return d.warnings.at(i); }
  QList<TidyReport> warnings() const { return d.warnings; }
  int accessibilityWarningCount() const { return d.accesswarns.count(); }
  TidyReport accessibilityWarning(int i) const { return d.accesswarns.at(i); }
  QList<TidyReport> accessibilityWarnings() const { return d.accesswarns; }

  struct Data
  {
    QList<TidyReport> errors;
    QList<TidyReport> warnings;
    QList<TidyReport> accesswarns;
  };
private:
  TidyValidator::Data d;
};

#endif
