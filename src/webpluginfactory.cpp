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
#define QL1C(x)  QLatin1Char(x)

static QWebView* webViewFrom(QWidget* widget)
{
    QWidget* parent = widget;
    QWebView *view = 0;
    while (parent) {
        if (QWebView *aView = qobject_cast<QWebView*>(parent)) {
            view = aView;
            break;
        }
        parent = parent->parentWidget();
    }

    return view;
}

FakePluginWidget::FakePluginWidget (uint id, const QUrl& url, const QString& mimeType, QWidget* parent)
                 :QWidget(parent)
                 ,m_swapping(false)
                 ,m_updateScrollPosition(false)
                 ,m_mimeType(mimeType)
                 ,m_id(id)
{
    QHBoxLayout* horizontalLayout = new QHBoxLayout;
    setLayout(horizontalLayout);

    QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    horizontalLayout->addSpacerItem(horizontalSpacer);

    QPushButton* startPluginButton = new QPushButton(this);
    startPluginButton->setText(i18n("Start Plugin"));
    horizontalLayout->addWidget(startPluginButton);

    horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    horizontalLayout->addSpacerItem(horizontalSpacer);

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
    QWebView *view = webViewFrom(parentWidget());
    if (!view)
        return;

    // WORKAROUND: For some reason, when we load on demand plugins the scroll
    // position gets utterly screwed up and reset to the beginning of the
    // document. This is an effort to workaround that issue.
    connect(view->page(), SIGNAL(scrollRequested(int,int,QRect)),
            this, SLOT(updateScrollPoisition(int,int,QRect)), Qt::QueuedConnection);

    hide();
    m_swapping = true;

    QList<QWebFrame*> frames;
    frames.append(view->page()->mainFrame());

    QString selector (QLatin1String("applet:not([type]),embed:not([type]),object:not([type]),applet[type=\""));
    selector += m_mimeType;
    selector += QLatin1String("\"],embed[type=\"");
    selector += m_mimeType;
    selector += QLatin1String("\"],object[type=\"");
    selector += m_mimeType;
    selector += QLatin1String("\"]");

    while (!frames.isEmpty()) {
        bool loaded = false;
        QWebFrame *frame = frames.takeFirst();
        QWebElement docElement = frame->documentElement();
        QWebElementCollection elements = docElement.findAll(selector);

        Q_FOREACH (QWebElement element, elements) {
            if (loadAll || element.evaluateJavaScript(QLatin1String("this.swapping")).toBool()) {
                QWebElement substitute = element.clone();
                emit pluginLoaded(m_id);
                m_updateScrollPosition = true;
                element.replace(substitute);
                deleteLater();
                if (!loadAll) {
                    loaded = true;
                    break;  // Found the one plugin we wanted to start so exit loop.
                }
            }
        }
        if (loaded && !loadAll) {
            break;      // Loading only one item, exit the outer loop as well...
        }
        frames += frame->childFrames();
    }

    m_swapping = false;
}


void FakePluginWidget::showContextMenu(const QPoint&)
{
    // TODO: Implement context menu, e.g. load all and configure plugins.
}

void FakePluginWidget::updateScrollPoisition (int dx, int dy, const QRect& rect)
{
    if (m_updateScrollPosition) {
        QWebView* view = webViewFrom(parentWidget());
        if (view)
            view->page()->mainFrame()->setScrollPosition(QPoint(dx, dy));
    }

    Q_UNUSED(rect);
}


WebPluginFactory::WebPluginFactory (KWebKitPart* part, QObject* parent)
    : KWebPluginFactory (parent)
    , mPart (part)
{
}

static uint pluginId(const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues)
{
    static const char* properties[] = {"src","data","width","height","type","id","name",0};

    QString signature = url.toString();
    for (int i = 0; properties[i]; ++i) {
        const int index = argumentNames.indexOf(properties[i]);
        if (index > -1) {
            signature += argumentNames.at(index);
            signature += QL1C('=');
            signature += argumentValues.at(index);
        }
    }

    return qHash(signature);
}

QObject* WebPluginFactory::create (const QString& _mimeType, const QUrl& url, const QStringList& argumentNames, const QStringList& argumentValues) const
{
    //kDebug() << _mimeType << url << argumentNames;
    QString mimeType (_mimeType.trimmed());
    if (mimeType.isEmpty()) {
        extractGuessedMimeType (url, &mimeType);
    }

    const bool noPluginHandling = WebKitSettings::self()->isInternalPluginHandlingDisabled();

    if (!noPluginHandling && WebKitSettings::self()->isLoadPluginsOnDemandEnabled()) {
        const uint id = pluginId(url, argumentNames, argumentValues);
        if (!mPluginsLoadedOnDemand.contains(id)) {
            FakePluginWidget* widget = new FakePluginWidget(id, url, mimeType);
            connect(widget, SIGNAL(pluginLoaded(uint)), this, SLOT(loadedPlugin(uint)));
            return widget;
        }
    }

    Q_ASSERT(mPart); // should never happen!!
    KParts::ReadOnlyPart* part = 0;
    QWebView* view = (mPart ? mPart->view() : 0);

    if (noPluginHandling || !excludedMimeType(mimeType)) {
        QWebFrame* frame = (view ? view->page()->currentFrame() : 0);
        if (frame) {
            part = createPartInstanceFrom(mimeType, argumentNames, argumentValues, view, frame);
        }
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
