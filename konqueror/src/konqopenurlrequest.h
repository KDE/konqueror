/* This file is part of the KDE project
   Copyright (C) 2000-2007 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konqopenurlrequest_h
#define __konqopenurlrequest_h

#include "konqprivate_export.h"

#include <QtCore/QStringList>

#include <kparts/browserextension.h>

struct KONQ_TESTS_EXPORT KonqOpenURLRequest {

  KonqOpenURLRequest() :
      followMode(false), newTab(false), newTabInFront(false),
      openAfterCurrentPage(false), forceAutoEmbed(false),
      tempFile(false), userRequestedReload(false) {}

  KonqOpenURLRequest( const QString & url ) :
    typedUrl(url), followMode(false), newTab(false), newTabInFront(false),
    openAfterCurrentPage(false), forceAutoEmbed(false),
    tempFile(false), userRequestedReload(false) {}

  QString debug() const {
#ifndef NDEBUG
      QStringList s;
      if ( !browserArgs.frameName.isEmpty() )
          s << "frameName=" + browserArgs.frameName;
      if ( !nameFilter.isEmpty() )
          s << "nameFilter=" + nameFilter;
      if ( !typedUrl.isEmpty() )
          s << "typedUrl=" + typedUrl;
      if ( followMode )
          s << "followMode";
      if ( newTab )
          s << "newTab";
      if ( newTabInFront )
          s << "newTabInFront";
      if ( openAfterCurrentPage )
          s << "openAfterCurrentPage";
      if ( forceAutoEmbed )
          s << "forceAutoEmbed";
      if ( tempFile )
          s << "tempFile";
      if ( userRequestedReload )
          s << "userRequestedReload";
      return "[" + s.join(" ") + "]";
#else
      return QString();
#endif
  }

  QString typedUrl; // empty if URL wasn't typed manually
  QString nameFilter; // like *.cpp, extracted from the URL
  bool followMode; // true if following another view - avoids loops
  bool newTab; // open url in new tab
  bool newTabInFront; // new tab in front or back
  bool openAfterCurrentPage;
  bool forceAutoEmbed; // if true, override the user's FMSettings for embedding
  bool tempFile; // if true, the url should be deleted after use
  bool userRequestedReload; // args.reload because the user requested it, not a website
  KParts::OpenUrlArguments args;
  KParts::BrowserArguments browserArgs;
  QStringList filesToSelect; // files to select in a konqdirpart

  static KonqOpenURLRequest null;
};

#endif
