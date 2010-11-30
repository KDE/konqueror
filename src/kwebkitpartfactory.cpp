/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "kwebkitpartfactory.h"
#include "kwebkitpart.h"

#include <KDE/KApplication>
#include <KDE/KStandardDirs>
#include <KDE/KTemporaryFile>
#include <KDE/KParts/GenericFactory>

#include <QtGui/QWidget>

KWebKitFactory::KWebKitFactory()
               :m_discardSessionFiles(true)

{
    kDebug() << this;
    KApplication *app = qobject_cast<KApplication*>(qApp);
    if (app)
        connect(app, SIGNAL(saveYourself()), SLOT(slotSaveYourself()));
    else
        kWarning() << "Invoked from a non-KDE application... Session management will NOT work properly!";
}

KWebKitFactory::~KWebKitFactory()
{
    kDebug() << this;
}

QObject *KWebKitFactory::create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString& keyword)
{
    Q_UNUSED(iface);
    Q_UNUSED(keyword);
    Q_UNUSED(args);

    // NOTE: The code below is what makes it possible to properly integrate QtWebKit's
    // history management with any KParts based application.
    QString tempFileName;
    KTemporaryFile tempFile;
    tempFile.setFileTemplate(KStandardDirs::locateLocal("data", QLatin1String("kwebkitpart/autosave/XXXXXX")));
    tempFile.setSuffix(QLatin1String(""));
    if (tempFile.open())
        tempFileName = tempFile.fileName();

    if (parentWidget) {
        m_sessionFileLookup.insert(parentWidget, tempFileName);
        connect (parentWidget, SIGNAL(destroyed(QObject*)), SLOT(slotDestroyed(QObject *)));
    } else {
        kWarning() << "No parent widget specified... Session management will FAIL to work properly!";
    }


    return new KWebKitPart(parentWidget, parent, QStringList() << tempFileName);
}


void KWebKitFactory::slotSaveYourself()
{
    m_discardSessionFiles = false;
}

void KWebKitFactory::slotDestroyed(QObject * obj)
{
    //kDebug() << "Discard the session history file of" << obj << "?" << m_discardSessionFiles;
    if (m_discardSessionFiles) {
        const QString sessionFile =  m_sessionFileLookup.take(obj);
        disconnect (obj, SIGNAL(destroyed(QObject*)), this, SLOT(slotDestroyed(QObject *)));
        //kDebug() << "Discarding session history File" << sessionFile;
        if (!QFile::remove(sessionFile))
            kWarning() << "Failed to discard the session history file";
    }
}

K_EXPORT_PLUGIN(KWebKitFactory)

#include "kwebkitpartfactory.moc"
