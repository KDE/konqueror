/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

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
#include <KParts/NavigationExtension>
#include <kio/job.h>
#include <kurllabel.h>
#include <kprotocolinfo.h>
#include <KCharsets>

#include <QApplication>
#include <QStatusBar>
#include <QStyle>
#include <QClipboard>
#include <QWidgetAction>
#include <QInputDialog>

#include <htmlextension.h>
#include <browserarguments.h>
#include <browserextension.h>

using namespace Akregator;
using namespace KonqInterfaces;

K_PLUGIN_CLASS_WITH_JSON(KonqFeedIcon, "akregator_konqfeedicon.json")

static QUrl baseUrl(KParts::ReadOnlyPart *part)
{
    QUrl url;
    HtmlExtension *ext = HtmlExtension::childObject(part);
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
        HtmlExtension *ext = HtmlExtension::childObject(part);
        SelectorInterface *selectorInterface = qobject_cast<SelectorInterface*>(ext);
        if (selectorInterface) {
            m_part = part;
            auto slot = &KonqFeedIcon::updateFeedIcon;
            connect(m_part, QOverload<>::of(&KParts::ReadOnlyPart::completed), this, slot);
            connect(m_part, &KParts::ReadOnlyPart::completedWithPendingAction, this, slot);
            connect(m_part, &KParts::ReadOnlyPart::started, this, &KonqFeedIcon::removeFeedIcon);
        }
    }
}

KonqFeedIcon::~KonqFeedIcon()
{
    //When the part is destroyed, this becomes nullptr before this destructor is called
    if (m_part) {
        m_statusBarEx = KParts::StatusBarExtension::childObject(m_part);
        if (m_statusBarEx) {
            m_statusBarEx->removeStatusBarItem(m_feedIcon);
        }
    }
    delete m_feedIcon;
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

QAction * Akregator::KonqFeedIcon::actionTitleForFeed(const QString &title, QWidget* parent)
{
    QLabel *l = new QLabel(title);
    l->setAlignment(Qt::AlignCenter);
    QWidgetAction *wa = new QWidgetAction(parent);
    wa->setDefaultWidget(l);
    return wa;
}

QMenu * Akregator::KonqFeedIcon::createMenuForFeed(const Feed& feed, QWidget* parent, bool addSection)
{
    QMenu *menu = new QMenu(feed.title(), parent);
    if (addSection) {
        menu->addAction(actionTitleForFeed(feed.title(), menu));
        menu->addSeparator();
    }
    menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add feed to Akregator"), menu, [feed, this](){addFeedToAkregator(feed.url());});
    menu->addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy feed URL to clipboard"), menu, [feed, this](){copyFeedUrlToClipboard(feed.url());});
    menu->addAction(QIcon::fromTheme(QStringLiteral("document-open")), i18n("Open feed URL"), menu, [feed, this](){openFeedUrl(feed.url(), feed.mimeType());});
    return menu;
}

void KonqFeedIcon::contextMenu()
{
    delete m_menu;
    if (m_feedList.count() == 1) {
        m_menu = createMenuForFeed(m_feedList.first(), m_part->widget(), true);
    } else {
        m_menu = new QMenu(m_part->widget());
        m_menu->addAction(actionTitleForFeed(i18nc("@title:menu title for the feeds menu", "Feeds"), m_menu));
        m_menu->addSeparator();
        for (const Feed &f : m_feedList) {
            m_menu->addMenu(createMenuForFeed(f, m_menu));
            m_menu->addSeparator();
        }
        m_menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add All Found Feeds to Akregator"), this, &KonqFeedIcon::addAllFeeds);
    }
    m_menu->popup(QCursor::pos());
}

void Akregator::KonqFeedIcon::updateFeedIcon()
{
    if (!isUrlUsable() || m_feedIcon) {
        return;
    }

    SelectorInterface *asyncIface = qobject_cast<SelectorInterface*>(HtmlExtension::childObject(m_part));
    if (!asyncIface) {
        return;
    }

    auto callback = [this](const QList<Element> &nodes) {
        fillFeedList(nodes);
        if (!m_feedList.isEmpty()) {
            addFeedIcon();
        }
    };
    asyncIface->querySelectorAll(query(), SelectorInterface::EntireContent, callback);
}

void Akregator::KonqFeedIcon::fillFeedList(const QList<Element> &linkNodes)
{
    QString doc;
    for (const Element &e : linkNodes) {
        QString rel = e.attribute(QStringLiteral("rel")).toLower();
        if (!rel.endsWith(QStringLiteral("alternate")) && !rel.endsWith(QStringLiteral("feed")) && !rel.endsWith(QStringLiteral("service.feed"))) {
            continue;
        }

        static const QStringList acceptableMimeTypes =  {
            QStringLiteral("application/rss+xml"),
            QStringLiteral("application/rdf+xml"),
            QStringLiteral("application/atom+xml"),
            QStringLiteral("application/xml")
        };
        QString mimeType = e.attribute("type").toLower();
        if (!acceptableMimeTypes.contains(mimeType)) {
            continue;
        }
        QString url = KCharsets::resolveEntities(e.attribute(QStringLiteral("href")));
        if (url.isEmpty()) {
            continue;
        }
        url = PluginUtil::fixRelativeURL(url, baseUrl(m_part));

        QString title = KCharsets::resolveEntities(e.attribute(QStringLiteral("title")));
        if (title.isEmpty()) {
            title = url;
        }
        m_feedList.append(Feed(url, title, mimeType));
    }
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

// from akregatorplugin.cpp
void KonqFeedIcon::addAllFeeds()
{
    QStringList list;
    std::transform(m_feedList.constBegin(), m_feedList.constEnd(), std::back_inserter(list), [](const Feed &f){return f.url();});
    PluginUtil::addFeeds(list);
}

void Akregator::KonqFeedIcon::addFeedToAkregator(const QString& url)
{
    PluginUtil::addFeeds({url});
}

void Akregator::KonqFeedIcon::copyFeedUrlToClipboard(const QString& url)
{
    QApplication::clipboard()->setText(url);
}

void Akregator::KonqFeedIcon::openFeedUrl(const QString& url, const QString &mimeType)
{
    KParts::NavigationExtension *ext = KParts::NavigationExtension::childObject(m_part);
    if (!ext) {
        return;
    }
    KParts::OpenUrlArguments args;
    args.setMimeType(mimeType);
    BrowserArguments bargs;
    bargs.setNewTab(true);

    if (auto browserExtension = qobject_cast<BrowserExtension *>(ext)) {
        emit browserExtension->browserOpenUrlRequest(QUrl(url), args, bargs);
    } else {
        emit ext->openUrlRequest(QUrl(url));
    }

}

#include "konqfeedicon.moc"
