/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqsettings.h"
#include "konq_defaults.h"
#include "kglobalsettings.h"
#include <ksharedconfig.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <kdesktopfile.h>
#include <kdebug.h>
#include <assert.h>
#include <kconfiggroup.h>

//static
KonqFMSettings * KonqFMSettings::s_pSettings = 0L;

//static
KonqFMSettings * KonqFMSettings::settings()
{
  if (!s_pSettings)
  {
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cgs(config, "FMSettings");
    s_pSettings = new KonqFMSettings(cgs);
  }
  return s_pSettings;
}

//static
void KonqFMSettings::reparseConfiguration()
{
  if (s_pSettings)
  {
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup cgs(config, "FMSettings");
    s_pSettings->init(cgs);
  }
}

KonqFMSettings::KonqFMSettings(const KConfigGroup &config)
{
    init(config);
}

KonqFMSettings::~KonqFMSettings()
{
}

void KonqFMSettings::init(const KConfigGroup &config)
{
    m_homeURL = config.readPathEntry("HomeURL", "~");
    m_embedMap = config.config()->entryMap("EmbedSettings");
}

bool KonqFMSettings::shouldEmbed( const QString & mimeType ) const
{
    // First check in user's settings whether to embed or not
    // 1 - in the mimetype file itself
    KMimeType::Ptr mimeTypePtr = KMimeType::mimeType( mimeType );
    bool hasLocalProtocolRedirect = false;
    if ( mimeTypePtr )
    {
        hasLocalProtocolRedirect = !mimeTypePtr->property( "X-KDE-LocalProtocol" ).toString().isEmpty();
        QVariant autoEmbedProp = mimeTypePtr->property( "X-KDE-AutoEmbed" );
        if ( autoEmbedProp.isValid() )
        {
            bool autoEmbed = autoEmbedProp.toBool();
            kDebug(1203) << "X-KDE-AutoEmbed set to " << (autoEmbed ? "true" : "false");
            return autoEmbed;
        } else
            kDebug(1203) << "No X-KDE-AutoEmbed, looking for group";
    }
    // 2 - in the configuration for the group if nothing was found in the mimetype
    QString mimeTypeGroup = mimeType.left(mimeType.indexOf("/"));
    kDebug(1203) << "KonqFMSettings::shouldEmbed : mimeTypeGroup=" << mimeTypeGroup;
    if ( mimeTypeGroup == "inode" || mimeTypeGroup == "Browser" || mimeTypeGroup == "Konqueror" )
        return true; //always embed mimetype inode/*, Browser/* and Konqueror/*
    QMap<QString, QString>::ConstIterator it = m_embedMap.find( QString::fromLatin1("embed-")+mimeTypeGroup );
    if ( it != m_embedMap.end() ) {
        kDebug(1203) << "KonqFMSettings::shouldEmbed: " << it.value();
        return it.value() == QString::fromLatin1("true");
    }
    // 3 - if no config found, use default.
    // Note: if you change those defaults, also change kcontrol/filetypes/typeslistitem.cpp !
    // Embedding is false by default except for image/* and for zip, tar etc.
    if ( mimeTypeGroup == "image" || hasLocalProtocolRedirect )
        return true;
    return false;
}

QString KonqFMSettings::homeUrl() const
{
    return m_homeURL;
}
