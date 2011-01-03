/* This file is part of Validators
 *
 *  It's a merge of the HTML- and the CSSValidator
 *
 *  Copyright (C) 2001 by  Richard Moore <rich@kde.org>
 *                         Andreas Schlapbach <schlpbch@iam.unibe.ch>
 *  Copyright (C) 2008-2009 by  Pino Toscano <pino@kde.org>
 *
 *  for information how to write your own plugin see:
 *    http://developer.kde.org/documentation/tutorials/dot/writing-plugins.html
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 **/

#ifndef __plugin_validators_h
#define __plugin_validators_h

#include "validatorsdialog.h"

#include <qpointer.h>

#include <kparts/plugin.h>

class KAction;
class KActionMenu;
class KUrl;
namespace KIO
{
  class Job;
}
namespace KParts
{
  class ReadOnlyPart;
  class StatusBarExtension;
}
class ClickIconLabel;
struct ValidationResult;

class PluginValidators : public KParts::Plugin
{
  Q_OBJECT
public:
  PluginValidators( QObject* parent,
                    const QVariantList & );
  virtual ~PluginValidators();

  static const char s_boundary[];
  static const char s_CRLF[];

public slots:
  void slotValidateHtmlByUri();
  void slotValidateHtmlByUpload();
  void slotValidateCssByUri();
  void slotValidateCssByUpload();
  void slotValidateLinks();
  void slotConfigure();

private slots:
  void slotStarted( KIO::Job* );
  void slotCompleted();
  void slotContextMenu();
  void slotTidyValidation();
  void slotShowTidyValidationReport();
  void setURLs();

private:
  KActionMenu *m_menu;
  QPointer<ValidatorsDialog> m_configDialog; // |
                                    // +-> Order dependency.
  KParts::ReadOnlyPart* m_part;                // |

  KUrl m_WWWValidatorUrl, m_WWWValidatorUploadUrl;
  KUrl m_CSSValidatorUrl, m_CSSValidatorUploadUrl;
  KUrl m_linkValidatorUrl;

  QAction *m_validateHtmlUri, *m_validateHtmlUpload;
  QAction *m_validateCssUri, *m_validateCssUpload;
  QAction *m_validateLinks;
  QAction *m_localValidation, *m_localValidationReport;

  ClickIconLabel *m_icon;
  KParts::StatusBarExtension *m_statusBarExt;
  QList<ValidationResult *> m_lastResults;

  bool canValidateByUri() const;
  bool canValidateByUpload() const;
  bool canValidateLocally() const;
  QString documentSource() const;
  void validateByUri(const KUrl &url);
  void validateByUpload(const KUrl &url, const QList<QPair<QByteArray, QByteArray> > &formData);
  bool doExternalValidationChecks();
  void addStatusBarIcon();
  void removeStatusBarIcon();
};

#endif
