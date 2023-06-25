/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net> 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-exception
*/

#include "konqfeedicon.h"

#include "pluginutil.h"
#include "akregatorplugindebug.h"

#include <kpluginfactory.h>
#include <KLocalizedString>
#include <kiconloader.h>
#include <kparts/part.h>
#include <kparts/statusbarextension.h>
#include <KParts/ReadOnlyPart>
#include <KParts/HtmlExtension>
#include <KParts/SelectorInterface>
#include <kio/job.h>
#include <kurllabel.h>
#include <kprotocolinfo.h>

#include <QApplication>
#include <QStatusBar>
#include <QStyle>

#include <asyncselectorinterface.h>

using namespace Akregator;

K_PLUGIN_CLASS_WITH_JSON(KonqFeedIcon, "akregator_konqfeedicon.json")

static QUrl baseUrl(KParts::ReadOnlyPart *part)
{
    QUrl url;
    KParts::HtmlExtension *ext = KParts::HtmlExtension::childObject(part);
    if (ext) {
        url = ext->baseUrl();
    }
    return url;
}

static QString query() {
    QString s_query = QStringLiteral("head > link[rel='alternate']");
    return s_query;
}

KonqFeedIcon::KonqFeedIcon(QObject *parent, const QVariantList &args)
    : KonqParts::Plugin(parent),
      m_part(nullptr),
      m_feedIcon(nullptr),
      m_statusBarEx(nullptr),
      m_menu(nullptr)
{
    Q_UNUSED(args);

    // make our icon foundable by the KIconLoader
    KIconLoader::global()->addAppDir(QStringLiteral("akregator"));

    KParts::ReadOnlyPart *part = qobject_cast<KParts::ReadOnlyPart *>(parent);
    if (part) {
        KParts::HtmlExtension *ext = KParts::HtmlExtension::childObject(part);
        KParts::SelectorInterface *syncSelectorInterface = qobject_cast<KParts::SelectorInterface *>(ext);
        AsyncSelectorInterface *asyncSelectorInterface = qobject_cast<AsyncSelectorInterface*>(ext);
        if (syncSelectorInterface || asyncSelectorInterface) {
            m_part = part;
            auto slot = syncSelectorInterface ? &KonqFeedIcon::updateFeedIcon : &KonqFeedIcon::updateFeedIconAsync;
            connect(m_part, QOverload<>::of(&KParts::ReadOnlyPart::completed), this, slot);
            connect(m_part, &KParts::ReadOnlyPart::completedWithPendingAction, this, slot);
            connect(m_part, &KParts::ReadOnlyPart::started, this, &KonqFeedIcon::removeFeedIcon);
        }
    }
}

KonqFeedIcon::~KonqFeedIcon()
{
    m_statusBarEx = KParts::StatusBarExtension::childObject(m_part);
    if (m_statusBarEx) {
        m_statusBarEx->removeStatusBarItem(m_feedIcon);
        // if the statusbar extension is deleted, the icon is deleted as well (being the child of the status bar)
        delete m_feedIcon;
    }
    // the icon is deleted in every case
    m_feedIcon = nullptr;
    delete m_menu;
    m_menu = nullptr;
}

bool Akregator::KonqFeedIcon::isUrlUsable() const
{
    // Ensure that it is safe to use the URL, before doing anything else with it
    const QUrl partUrl(m_part->url());
    if (!partUrl.isValid() || partUrl.scheme().isEmpty()) {
        return false;
    }
    // Since attempting to determine feed info for about:blank crashes khtml,
    // lets prevent such look up for local urls (about, file, man, etc...)
    if (KProtocolInfo::protocolClass(partUrl.scheme()).compare(QLatin1String(":local"), Qt::CaseInsensitive) == 0) {
        return false;
    }
    return true;
}

void KonqFeedIcon::contextMenu()
{
    delete m_menu;
    m_menu = new QMenu(m_part->widget());
    if (m_feedList.count() == 1) {
        m_menu->setTitle(m_feedList.first().title());
        m_menu->addAction(QIcon::fromTheme("bookmark-new"), i18n("Add Feed to Akregator"), this, &KonqFeedIcon::addAllFeeds);
    } else {
        m_menu->setTitle(i18n("Add Feeds to Akregator"));
        int id = 0;
        for (FeedDetectorEntryList::Iterator it = m_feedList.begin(); it != m_feedList.end(); ++it) {
            QAction *action = m_menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), (*it).title(), this, &KonqFeedIcon::addFeed);
            action->setData(QVariant::fromValue(id));
            id++;
        }
        //disconnect(m_menu, SIGNAL(activated(int)), this, SLOT(addFeed(int)));
        m_menu->addSeparator();
        m_menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add All Found Feeds to Akregator"), this, &KonqFeedIcon::addAllFeeds);
    }
    m_menu->popup(QCursor::pos());
}

void Akregator::KonqFeedIcon::updateFeedIconAsync()
{
    if (!isUrlUsable() || m_feedIcon) {
        return;
    }

    AsyncSelectorInterface *asyncIface = qobject_cast<AsyncSelectorInterface*>(KParts::HtmlExtension::childObject(m_part));
    if (!asyncIface) {
        return;
    }

    auto callback = [this](const QList<Element> &nodes) {
        fillFeedList(nodes);
        if (!m_feedList.isEmpty()) {
            addFeedIcon();
        }
    };
    asyncIface->querySelectorAllAsync(query(), KParts::SelectorInterface::EntireContent, callback);
}

void KonqFeedIcon::updateFeedIcon()
{
    if (!isUrlUsable() || m_feedIcon) {
        return;
    }

    KParts::HtmlExtension *ext = KParts::HtmlExtension::childObject(m_part);
    KParts::SelectorInterface *syncInterface = qobject_cast<KParts::SelectorInterface *>(ext);
    QList<KParts::SelectorInterface::Element> linkNodes = syncInterface->querySelectorAll(query(), KParts::SelectorInterface::EntireContent);
    fillFeedList(linkNodes);
    if (m_feedList.isEmpty()) {
        return;
    }
    addFeedIcon();
}

void Akregator::KonqFeedIcon::fillFeedList(const QList<Element> &linkNodes)
{
    QString doc;
    for (int i = 0; i < linkNodes.count(); i++) {
        const KParts::SelectorInterface::Element element = linkNodes.at(i);

        // TODO parse the attributes directly here, rather than re-assembling
        // and then re-parsing in extractFromLinkTags!
        doc += QLatin1String("<link ");
        for (const QString &attrName : element.attributeNames()) {
            doc += attrName + "=\"";
            doc += element.attribute(attrName).toHtmlEscaped().replace(QLatin1String("\""), QLatin1String("&quot;"));
            doc += QLatin1String("\" ");
        }
        doc += QLatin1String("/>");
    }
    qCDebug(AKREGATORPLUGIN_LOG) << doc;
    m_feedList = FeedDetector::extractFromLinkTags(doc);
}

void Akregator::KonqFeedIcon::addFeedIcon()
{
    m_statusBarEx = KParts::StatusBarExtension::childObject(m_part);
    if (!m_statusBarEx) {
        return;
    }

    m_feedIcon = new KUrlLabel(m_statusBarEx->statusBar());

    // from khtmlpart's ualabel
    m_feedIcon->setFixedHeight(qApp->style()->pixelMetric(QStyle::PM_SmallIconSize));
    m_feedIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_feedIcon->setUseCursor(false);

    m_feedIcon->setPixmap(KIconLoader::global()->loadIcon(QStringLiteral("feed"), KIconLoader::User));
    m_feedIcon->setToolTip(i18n("Subscribe to site updates (using news feed)"));

    m_statusBarEx->addStatusBarItem(m_feedIcon, 0, true);

    connect(m_feedIcon, QOverload<>::of(&KUrlLabel::leftClickedUrl), this, &KonqFeedIcon::contextMenu);
}

void KonqFeedIcon::removeFeedIcon()
{
    m_feedList.clear();
    if (m_feedIcon && m_statusBarEx) {
        m_statusBarEx->removeStatusBarItem(m_feedIcon);
        delete m_feedIcon;
        m_feedIcon = nullptr;
    }

    // Close the popup if it's open, otherwise we crash
    delete m_menu;
    m_menu = nullptr;
}

void KonqFeedIcon::addFeed()
{
    bool ok = false;
    const int id = sender() ? qobject_cast<QAction *>(sender())->data().toInt(&ok) : -1;
    if (!ok || id == -1) {
        return;
    }
    PluginUtil::addFeeds(QStringList(PluginUtil::fixRelativeURL(m_feedList[id].url(), baseUrl(m_part))));
}

// from akregatorplugin.cpp
void KonqFeedIcon::addAllFeeds()
{
    QStringList list;
    foreach (const FeedDetectorEntry &it, m_feedList) {
        list.append(PluginUtil::fixRelativeURL(it.url(), baseUrl(m_part)));
    }
    PluginUtil::addFeeds(list);
}

#include "konqfeedicon.moc"
