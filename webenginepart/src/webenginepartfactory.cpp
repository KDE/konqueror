/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "webenginepartfactory.h"
#include "webenginepart_ext.h"
#include "webenginepart.h"

#include <kparts_version.h>
#include <KPluginMetaData>

#include <QWidget>

WebEngineFactory::~WebEngineFactory()
{
    // qCDebug(WEBENGINEPART_LOG) << this;
}

#if QT_VERSION_MAJOR < 6
QObject *WebEngineFactory::create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList &args, const QString&)
#else
QObject *WebEngineFactory::create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList &args)
#endif
{
    Q_UNUSED(iface);
    Q_UNUSED(args);

    connect(parentWidget, &QObject::destroyed, this, &WebEngineFactory::slotDestroyed);

    // NOTE: The code below is what makes it possible to properly integrate QtWebEngine's PORTING_TODO
    // history management with any KParts based application.
    QByteArray histData (m_historyBufContainer.value(parentWidget));
    if (!histData.isEmpty()) histData = qUncompress(histData);
    WebEnginePart* part = new WebEnginePart(parentWidget, parent, metaData(), histData);
    WebEngineNavigationExtension* ext = qobject_cast<WebEngineNavigationExtension*>(part->navigationExtension());
    if (ext) {
        connect(ext, QOverload<QObject *, const QByteArray &>::of(&WebEngineNavigationExtension::saveHistory),
                this, &WebEngineFactory::slotSaveHistory);
    }
    return part;
}

void WebEngineFactory::slotSaveHistory(QObject* widget, const QByteArray& buffer)
{
    // qCDebug(WEBENGINEPART_LOG) << "Caching history data from" << widget;
    m_historyBufContainer.insert(widget, buffer);
}

void WebEngineFactory::slotDestroyed(QObject* object)
{
    // qCDebug(WEBENGINEPART_LOG) << "Removing cached history data of" << object;
    m_historyBufContainer.remove(object);
}
