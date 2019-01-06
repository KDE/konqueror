/*
    This file is part of Akregator.

    Copyright (C) 2004 Teemu Rytilahti <tpr@d5k.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "konqfeedicon.h"

#include "feeddetector.h"
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

#include <qstatusbar.h>

using namespace Akregator;

K_PLUGIN_FACTORY(KonqFeedIconFactory, registerPlugin<KonqFeedIcon>();)

static QUrl baseUrl(KParts::ReadOnlyPart *part)
{
    QUrl url;
    KParts::HtmlExtension *ext = KParts::HtmlExtension::childObject(part);
    if (ext) {
        url = ext->baseUrl();
    }
    return url;
}

KonqFeedIcon::KonqFeedIcon(QObject *parent, const QVariantList &args)
    : KParts::Plugin(parent),
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
        KParts::SelectorInterface *selectorInterface = qobject_cast<KParts::SelectorInterface *>(ext);
        if (selectorInterface) {
            m_part = part;
            connect(m_part, QOverload<>::of(&KParts::ReadOnlyPart::completed), this, &KonqFeedIcon::addFeedIcon);
            connect(m_part, QOverload<bool>::of(&KParts::ReadOnlyPart::completed), this, &KonqFeedIcon::addFeedIcon);
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

bool KonqFeedIcon::feedFound()
{
    // Ensure that it is safe to use the URL, before doing anything else with it
    const QUrl partUrl(m_part->url());
    if (!partUrl.isValid()) {
        return false;
    }
    // Since attempting to determine feed info for about:blank crashes khtml,
    // lets prevent such look up for local urls (about, file, man, etc...)
    if (KProtocolInfo::protocolClass(partUrl.scheme()).compare(QLatin1String(":local"), Qt::CaseInsensitive) == 0) {
        return false;
    }

    KParts::HtmlExtension *ext = KParts::HtmlExtension::childObject(m_part);
    KParts::SelectorInterface *selectorInterface = qobject_cast<KParts::SelectorInterface *>(ext);
    QString doc;
    if (selectorInterface) {
        QList<KParts::SelectorInterface::Element> linkNodes = selectorInterface->querySelectorAll(QStringLiteral("head > link[rel=\"alternate\"]"), KParts::SelectorInterface::EntireContent);
        for (int i = 0; i < linkNodes.count(); i++) {
            const KParts::SelectorInterface::Element element = linkNodes.at(i);

            // TODO parse the attributes directly here, rather than re-assembling
            // and then re-parsing in extractFromLinkTags!
            doc += QLatin1String("<link ");
            Q_FOREACH (const QString &attrName, element.attributeNames()) {
                doc += attrName + "=\"";
                doc += element.attribute(attrName).toHtmlEscaped().replace(QLatin1String("\""), QLatin1String("&quot;"));
                doc += QLatin1String("\" ");
            }
            doc += QLatin1String("/>");
        }
        qCDebug(AKREGATORPLUGIN_LOG) << doc;
    }

    m_feedList = FeedDetector::extractFromLinkTags(doc);
    return m_feedList.count() != 0;
}

void KonqFeedIcon::contextMenu()
{
    delete m_menu;
    m_menu = new QMenu(m_part->widget());
    if (m_feedList.count() == 1) {
        m_menu->setTitle(m_feedList.first().title());
        m_menu->addAction(SmallIcon("bookmark-new"), i18n("Add Feed to Akregator"), this, SLOT(addAllFeeds()));
    } else {
        m_menu->setTitle(i18n("Add Feeds to Akregator"));
        int id = 0;
        for (FeedDetectorEntryList::Iterator it = m_feedList.begin(); it != m_feedList.end(); ++it) {
            QAction *action = m_menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), (*it).title(), this, SLOT(addFeed()));
            action->setData(qVariantFromValue(id));
            id++;
        }
        //disconnect(m_menu, SIGNAL(activated(int)), this, SLOT(addFeed(int)));
        m_menu->addSeparator();
        m_menu->addAction(QIcon::fromTheme(QStringLiteral("bookmark-new")), i18n("Add All Found Feeds to Akregator"), this, SLOT(addAllFeeds()));
    }
    m_menu->popup(QCursor::pos());
}

void KonqFeedIcon::addFeedIcon()
{
    if (!feedFound() || m_feedIcon) {
        return;
    }

    m_statusBarEx = KParts::StatusBarExtension::childObject(m_part);
    if (!m_statusBarEx) {
        return;
    }

    m_feedIcon = new KUrlLabel(m_statusBarEx->statusBar());

    // from khtmlpart's ualabel
    m_feedIcon->setFixedHeight(KIconLoader::global()->currentSize(KIconLoader::Small));
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
