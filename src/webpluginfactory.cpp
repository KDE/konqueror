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
#include <KDE/KLocalizedString>
#include <KDE/KParts/ReadOnlyPart>
#include <KDE/KParts/BrowserExtension>
#include <kparts/scriptableextension.h>

#include <QHBoxLayout>
#include <QSpacerItem>
#include <QPushButton>

#include <QWebFrame>
#include <QWebView>
#include <QWebElement>

#define QL1S(x)  QLatin1String(x)


FakePluginWidget::FakePluginWidget (uint id, const QUrl& url, const QString& mimeType, QWidget* parent)
                 :QWidget(parent)
                 ,m_swapping(false)
                 ,m_mimeType(mimeType)
                 ,m_id(id)
{
    QHBoxLayout* horizontalLayout = new QHBoxLayout;

    QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    horizontalLayout->addSpacerItem(horizontalSpacer);

    QPushButton* startPluginButton = new QPushButton(this);
    startPluginButton->setText(i18n("Start Plugin"));
    horizontalLayout->addWidget(startPluginButton);

    horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    horizontalLayout->addSpacerItem(horizontalSpacer);

    setLayout(horizontalLayout);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(startPluginButton, SIGNAL(clicked()), this, SLOT(load()));
    setToolTip(url.toString());
}

void FakePluginWidget::loadAll()
{
    load (true);
}

void FakePluginWidget::load (bool loadAll)
{
    QWidget *parent = parentWidget();
    QWebView *view = 0;
    while (parent) {
        view = qobject_cast<QWebView*>(parent);
        if (view) {
          break;
        }
        parent = parent->parentWidget();
    }

    if (!view) {
        return;
    }

    const QString selector = QLatin1String("object[type=\"%1\"],embed[type=\"%1\"]");
    kDebug() << selector.arg(m_mimeType);

    hide();
    m_swapping = true;

    QList<QWebFrame*> frames;
    frames.append(view->page()->mainFrame());

    while (!frames.isEmpty()) {
        QWebFrame *frame = frames.takeFirst();
        QWebElement docElement = frame->documentElement();
        QWebElementCollection elements = docElement.findAll(selector.arg(m_mimeType));

        QWebElement element;
        foreach (element, elements) {
            if (loadAll || element.evaluateJavaScript(QLatin1String("this.swapping")).toBool()) {
                QWebElement substitute = element.clone();
                substitute.setAttribute(QLatin1String("type"), m_mimeType);
                element.replace(substitute);
                deleteLater();
                emit pluginLoaded(m_id);
                if (!loadAll) {
                    break;  // Found the one plugin we wanted to start so exit loop.
                }
            }
        }
        frames += frame->childFrames();
    }

    m_swapping = false;
}


void FakePluginWidget::showContextMenu(const QPoint&)
{
}


WebPluginFactory::WebPluginFactory (KWebKitPart* parent)
    : KWebPluginFactory (parent)
    , mPart (parent)
{
}

static uint pluginId(const QUrl& url, const QStringList& argNames, const QStringList& argValues)
{
    QString id = url.toString();
    id += argNames.join(QL1S(","));
    id += argValues.join(QL1S(","));

    return qHash(id);
}

QObject* WebPluginFactory::create (const QString& _mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    kDebug() << _mimeType << url << argumentNames;
    QString mimeType (_mimeType.trimmed());
    if (mimeType.isEmpty()) {
        extractGuessedMimeType (url, &mimeType);
    }

    QWebView* view = mPart->view();
    const bool noPluginHandling = WebKitSettings::self()->isInternalPluginHandlingDisabled();

    if (!noPluginHandling && WebKitSettings::self()->isLoadPluginsOnDemandEnabled()) {
        const uint id = pluginId(url, argumentNames, argumentValues);
        if (!mPluginsLoadedOnDemand.contains(id)) {
            FakePluginWidget* widget = new FakePluginWidget(id, url, mimeType, view);
            connect(widget, SIGNAL(pluginLoaded(uint)), this, SLOT(loadedPlugin(uint)));
            return widget;
        }
    }

    KParts::ReadOnlyPart* part = 0;

    if (noPluginHandling || !excludedMimeType(mimeType)) {
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

void WebPluginFactory::loadedPlugin (uint id)
{
    mPluginsLoadedOnDemand << id;
}

void WebPluginFactory::resetPluginOnDemandList()
{
    mPluginsLoadedOnDemand.clear();
}
