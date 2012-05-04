/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2012 Dawit Alemayehu <adawit@kde.org>
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

#include "webpluginfactory.h"

#include "webpage.h"
#include "kwebkitpart.h"
#include "settings/webkitsettings.h"

#include <KDE/KDebug>
#include <KDE/KConfigGroup>
#include <KDE/KSharedConfig>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>
#include <kparts/scriptableextension.h>

#include <QtWebKit/QWebFrame>
#include <QtWebKit/QWebView>

#define QL1S(x)  QLatin1String(x)


WebPluginFactory::WebPluginFactory (KWebKitPart* parent)
    : KWebPluginFactory (parent)
    , mPart (parent)
{
}

QObject* WebPluginFactory::create (const QString& _mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    QString mimeType (_mimeType.trimmed());
    if (mimeType.isEmpty()) {
        extractGuessedMimeType (url, &mimeType);
    }

    KParts::ReadOnlyPart* part = 0;
    QWebView* view = 0;

    if (WebKitSettings::self()->isInternalPluginHandlingDisabled() || !excludedMimeType(mimeType)) {
        view = mPart->view();
        QWebFrame* frame = (view ? view->page()->currentFrame() : 0);
        part = createPartInstanceFrom(mimeType, argumentNames, argumentValues, view, frame);
    }

    kDebug() << "Asked for" << mimeType << "plugin, got" << part;

    if (part) {
        connect (part->browserExtension(), SIGNAL (openUrlNotify()),
                 mPart->browserExtension(), SIGNAL (openUrlNotify()));

        connect (part->browserExtension(), SIGNAL (openUrlRequest (KUrl, KParts::OpenUrlArguments, KParts::BrowserArguments)),
                 mPart->browserExtension(), SIGNAL (openUrlRequest (KUrl, KParts::OpenUrlArguments, KParts::BrowserArguments)));

        // Check if this part is scriptable
        KParts::ScriptableExtension* scriptExt = KParts::ScriptableExtension::childObject(part);
        if (!scriptExt) {
            // Try to fall back to LiveConnectExtension compat
            KParts::LiveConnectExtension* lc = KParts::LiveConnectExtension::childObject(part);
            if (lc) {
                scriptExt = KParts::ScriptableExtension::adapterFromLiveConnect(part, lc);
            }
        }

        if (scriptExt) {
            scriptExt->setHost(KParts::ScriptableExtension::childObject(mPart));
        }

        QMap<QString, QString> metaData = part->arguments().metaData();
        QString urlStr = url.toString (QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);
        metaData.insert ("PropagateHttpHeader", "true");
        metaData.insert ("referrer", urlStr);
        metaData.insert ("cross-domain", urlStr);
        metaData.insert ("main_frame_request", "TRUE");
        metaData.insert ("ssl_activate_warnings", "TRUE");

        KWebPage *page = (view ? qobject_cast<KWebPage*>(view->page()) : 0);

        if (page) {
            const QString scheme = page->currentFrame()->url().scheme();
            if (page && (QString::compare (scheme, QL1S ("https"), Qt::CaseInsensitive) == 0 ||
                         QString::compare (scheme, QL1S ("webdavs"), Qt::CaseInsensitive) == 0))
                metaData.insert ("ssl_was_in_use", "TRUE");
            else
                metaData.insert ("ssl_was_in_use", "FALSE");
        }

        KParts::OpenUrlArguments openUrlArgs = part->arguments();
        openUrlArgs.metaData() = metaData;
        openUrlArgs.setMimeType(mimeType);
        part->setArguments(openUrlArgs);
        QMetaObject::invokeMethod(part, "openUrl", Qt::QueuedConnection, Q_ARG(KUrl, KUrl(url)));
        return part->widget();
    }

    return 0;
}


