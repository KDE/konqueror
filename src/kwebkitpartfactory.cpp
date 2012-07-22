/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "kwebkitpartfactory.h"
#include "kwebkitpart_ext.h"
#include "kwebkitpart.h"

#include <KDE/KDebug>

#include <QWidget>

KWebKitFactory::~KWebKitFactory()
{
    // kDebug() << this;
}

QObject *KWebKitFactory::create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString& keyword)
{
    Q_UNUSED(iface);
    Q_UNUSED(keyword);
    Q_UNUSED(args);

    kDebug() << parentWidget << parent;
    connect(parentWidget, SIGNAL(destroyed(QObject*)), this, SLOT(slotDestroyed(QObject*)));

    // NOTE: The code below is what makes it possible to properly integrate QtWebKit's
    // history management with any KParts based application.
    QByteArray histData (m_historyBufContainer.value(parentWidget));
    if (!histData.isEmpty()) histData = qUncompress(histData);
    KWebKitPart* part = new KWebKitPart(parentWidget, parent, histData);
    WebKitBrowserExtension* ext = qobject_cast<WebKitBrowserExtension*>(part->browserExtension());
    if (ext) {
        connect(ext, SIGNAL(saveHistory(QObject*,QByteArray)), this, SLOT(slotSaveHistory(QObject*,QByteArray)));
    }
    return part;
}

void KWebKitFactory::slotSaveHistory(QObject* widget, const QByteArray& buffer)
{
    // kDebug() << "Caching history data from" << widget;
    m_historyBufContainer.insert(widget, buffer);
}

void KWebKitFactory::slotDestroyed(QObject* object)
{
    // kDebug() << "Removing cached history data of" << object;
    m_historyBufContainer.remove(object);
}


K_EXPORT_PLUGIN(KWebKitFactory)

#include "kwebkitpartfactory.moc"
