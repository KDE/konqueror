/* This file is part of the KDE project
   Copyright (C) 1999, 2007 David Faure <faure@kde.org>

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
#include <kprotocolmanager.h>
#include <ksharedconfig.h>

#include <kdesktopfile.h>
#include "konqdebug.h"
#include <kconfiggroup.h>
#include <QMimeDatabase>
#include <QMimeType>

class KonqEmbedSettingsSingleton
{
public:
    KonqFMSettings self;
};
Q_GLOBAL_STATIC(KonqEmbedSettingsSingleton, globalEmbedSettings)

KonqFMSettings *KonqFMSettings::settings()
{
    return &globalEmbedSettings->self;
}

//static
void KonqFMSettings::reparseConfiguration()
{
    if (globalEmbedSettings.exists()) {
        globalEmbedSettings->self.init(true);
    }
}
KonqFMSettings::KonqFMSettings()
{
    init(false);
}

KonqFMSettings::~KonqFMSettings()
{
}

void KonqFMSettings::init(bool reparse)
{
    if (reparse) {
        fileTypesConfig()->reparseConfiguration();
    }
    m_embedMap = fileTypesConfig()->entryMap(QStringLiteral("EmbedSettings"));
}

KSharedConfig::Ptr KonqFMSettings::fileTypesConfig()
{
    if (!m_fileTypesConfig) {
        m_fileTypesConfig = KSharedConfig::openConfig(QStringLiteral("filetypesrc"), KConfig::NoGlobals);
    }
    return m_fileTypesConfig;
}

static bool alwaysEmbedMimeTypeGroup(const QString &mimeType)
{
    if (mimeType.startsWith(QLatin1String("inode")) || mimeType.startsWith(QLatin1String("Browser")) || mimeType.startsWith(QLatin1String("Konqueror"))) {
        return true;    //always embed mimetype inode/*, Browser/* and Konqueror/*
    }
    return false;
}

bool KonqFMSettings::shouldEmbed(const QString &_mimeType) const
{
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(_mimeType);
    if (!mime.isValid()) {
        qCWarning(KONQUEROR_LOG) << "Unknown mimetype" << _mimeType;
        return false; // unknown mimetype!
    }
    const QString mimeType = mime.name();

    // First check in user's settings whether to embed or not
    // 1 - in the filetypesrc config file (written by the configuration module)
    QMap<QString, QString>::const_iterator it = m_embedMap.find(QLatin1String("embed-") + mimeType);
    if (it != m_embedMap.end()) {
        qCDebug(KONQUEROR_LOG) << mimeType << it.value();
        return it.value() == QLatin1String("true");
    }
    // 2 - in the configuration for the group if nothing was found in the mimetype
    if (alwaysEmbedMimeTypeGroup(mimeType)) {
        return true;    //always embed mimetype inode/*, Browser/* and Konqueror/*
    }
    const QString mimeTypeGroup = mimeType.left(mimeType.indexOf('/'));
    it = m_embedMap.find(QLatin1String("embed-") + mimeTypeGroup);
    if (it != m_embedMap.end()) {
        qCDebug(KONQUEROR_LOG) << mimeType << "group setting:" << it.value();
        return it.value() == QLatin1String("true");
    }
    // 2 bis - configuration for group of parent mimetype, if different
    if (mimeType[0].isLower()) {
        QStringList parents;
        parents.append(mimeType);
        while (!parents.isEmpty()) {
            const QString parent = parents.takeFirst();
            if (alwaysEmbedMimeTypeGroup(parent)) {
                return true;
            }
            QMimeType mime = db.mimeTypeForName(parent);
            Q_ASSERT(mime.isValid()); // how could the -parent- be invalid?
            if (mime.isValid()) {
                parents += mime.parentMimeTypes();
            }
        }
    }

    // 3 - if no config found, use default.
    // Note: if you change those defaults, also change keditfiletype/mimetypedata.cpp !
    // Embedding is false by default except for image/*, text/html and for zip, tar etc.
    const bool hasLocalProtocolRedirect = !KProtocolManager::protocolForArchiveMimetype(mimeType).isEmpty();
    if (mimeTypeGroup == QLatin1String("image")
        || mime.inherits(QLatin1String("text/html")) || mime.inherits(QLatin1String("application/xhtml+xml"))
        || mimeTypeGroup == QLatin1String("multipart")
        || hasLocalProtocolRedirect) {
        return true;
    }
    return false;
}
