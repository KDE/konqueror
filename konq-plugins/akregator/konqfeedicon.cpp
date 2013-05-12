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
#include "pluginbase.h"

#include <kdebug.h>
#include <kpluginfactory.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kparts/part.h>
#include <kparts/statusbarextension.h>
#include <KParts/HtmlExtension>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kurllabel.h>
#include <kurl.h>
#include <kicon.h>
#include <kprotocolinfo.h>

#include <qcursor.h>
#include <qobject.h>
#include <qpixmap.h>
#include <qstringlist.h>
#include <QTextDocument>

using namespace Akregator;


K_PLUGIN_FACTORY(KonqFeedIconFactory, registerPlugin<KonqFeedIcon>();)
K_EXPORT_PLUGIN(KonqFeedIconFactory("akregatorkonqfeedicon"))

static KUrl baseUrl(KParts::ReadOnlyPart *part)
{
   KUrl url;
   KParts::HtmlExtension* ext = KParts::HtmlExtension::childObject(part);
   if (ext) {
      url = ext->baseUrl();
   }
   return url;
}

KonqFeedIcon::KonqFeedIcon(QObject *parent, const QVariantList &)
    : KParts::Plugin(parent), PluginBase(), m_part(0), m_feedIcon(0), m_statusBarEx(0), m_menu(0)
{
    KGlobal::locale()->insertCatalog("akregator_konqplugin");

    // make our icon foundable by the KIconLoader
    KIconLoader::global()->addAppDir("akregator");

    KParts::ReadOnlyPart* part = qobject_cast<KParts::ReadOnlyPart*>(parent);
    if (part) {
        KParts::HtmlExtension* ext = KParts::HtmlExtension::childObject(part);
        KParts::SelectorInterface* selectorInterface = qobject_cast<KParts::SelectorInterface*>(ext);
        if (selectorInterface) {
            m_part = part;
            connect(m_part, SIGNAL(completed()), this, SLOT(addFeedIcon()));
            connect(m_part, SIGNAL(completed(bool)), this, SLOT(addFeedIcon()));
            connect(m_part, SIGNAL(started(KIO::Job*)), this, SLOT(removeFeedIcon()));
        }
    }
}

KonqFeedIcon::~KonqFeedIcon()
{
    KGlobal::locale()->removeCatalog("akregator_konqplugin");
    m_statusBarEx = KParts::StatusBarExtension::childObject(m_part);
    if (m_statusBarEx)
    {
        m_statusBarEx->removeStatusBarItem(m_feedIcon);
        // if the statusbar extension is deleted, the icon is deleted as well (being the child of the status bar)
        delete m_feedIcon;
    }
    // the icon is deleted in every case
    m_feedIcon = 0;
    delete m_menu;
    m_menu = 0;
}

bool KonqFeedIcon::feedFound()
{
    // Since attempting to determine feed info for about:blank crashes khtml,
    // lets prevent such look up for local urls (about, file, man, etc...)
    if (KProtocolInfo::protocolClass(m_part->url().protocol()).compare(QLatin1String(":local"), Qt::CaseInsensitive) == 0)
        return false;

    KParts::HtmlExtension* ext = KParts::HtmlExtension::childObject(m_part);
    KParts::SelectorInterface* selectorInterface = qobject_cast<KParts::SelectorInterface*>(ext);
    QString doc;
    if (selectorInterface)
    {
        QList<KParts::SelectorInterface::Element> linkNodes = selectorInterface->querySelectorAll("head > link[rel=\"alternate\"]", KParts::SelectorInterface::EntireContent);
        //kDebug() << linkNodes.length() << "links";
        for (int i = 0; i < linkNodes.count(); i++) {
            const KParts::SelectorInterface::Element element = linkNodes.at(i);

            // TODO parse the attributes directly here, rather than re-assembling
            // and then re-parsing in extractFromLinkTags!
            doc += "<link ";
            Q_FOREACH(const QString& attrName, element.attributeNames()) {
                doc += attrName + "=\"";
                doc += Qt::escape( element.attribute(attrName) ).replace("\"", "&quot;");
                doc += "\" ";
            }
            doc += "/>";
        }
        kDebug() << doc;
    }

    m_feedList = FeedDetector::extractFromLinkTags(doc);
    return m_feedList.count() != 0;
}

void KonqFeedIcon::contextMenu()
{
    delete m_menu;
    m_menu = new KMenu(m_part->widget());
    if(m_feedList.count() == 1)
    {
        m_menu->addTitle(m_feedList.first().title());
        m_menu->addAction(SmallIcon("bookmark-new"), i18n("Add Feed to Akregator"), this, SLOT(addFeeds()) );
    }
    else
    {
        m_menu->addTitle(i18n("Add Feeds to Akregator"));
        int id = 0;
        for(FeedDetectorEntryList::Iterator it = m_feedList.begin(); it != m_feedList.end(); ++it)
        {
            QAction *action = m_menu->addAction(KIcon("bookmark-new"), (*it).title(), this, SLOT(addFeed()));
            action->setData(qVariantFromValue(id));
            id++;
        }
        //disconnect(m_menu, SIGNAL(activated(int)), this, SLOT(addFeed(int)));
        m_menu->addSeparator();
        m_menu->addAction(KIcon("bookmark-new"), i18n("Add All Found Feeds to Akregator"), this, SLOT(addFeeds()));
    }
    m_menu->popup(QCursor::pos());
}

void KonqFeedIcon::addFeedIcon()
{
    if(!feedFound() || m_feedIcon)
        return;

    m_statusBarEx = KParts::StatusBarExtension::childObject(m_part);
    if(!m_statusBarEx)
      return;

    m_feedIcon = new KUrlLabel(m_statusBarEx->statusBar());

    // from khtmlpart's ualabel
    m_feedIcon->setFixedHeight(KIconLoader::global()->currentSize(KIconLoader::Small));
    m_feedIcon->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_feedIcon->setUseCursor(false);

    m_feedIcon->setPixmap(KIconLoader::global()->loadIcon("feed", KIconLoader::User));

    m_feedIcon->setToolTip( i18n("Subscribe to site updates (using news feed)"));

    m_statusBarEx->addStatusBarItem(m_feedIcon, 0, true);

    connect(m_feedIcon, SIGNAL(leftClickedUrl()), this, SLOT(contextMenu()));
}

void KonqFeedIcon::removeFeedIcon()
{
    m_feedList.clear();
    if (m_feedIcon && m_statusBarEx)
    {
        m_statusBarEx->removeStatusBarItem(m_feedIcon);
        delete m_feedIcon;
        m_feedIcon = 0;
    }

    // Close the popup if it's open, otherwise we crash
    delete m_menu;
    m_menu = 0;
}

void KonqFeedIcon::addFeed()
{
    bool ok = false;
    const int id = sender() ? qobject_cast<QAction*>(sender())->data().toInt(&ok) : -1;
    if(!ok || id == -1) return;
    if(akregatorRunning())
        addFeedsViaDBUS(QStringList(fixRelativeURL(m_feedList[id].url(), baseUrl(m_part))));
    else
        addFeedViaCmdLine(fixRelativeURL(m_feedList[id].url(), baseUrl(m_part)));
}

// from akregatorplugin.cpp
void KonqFeedIcon::addFeeds()
{
    if(akregatorRunning())
    {
      	QStringList list;
        for ( FeedDetectorEntryList::Iterator it = m_feedList.begin(); it != m_feedList.end(); ++it )
            list.append(fixRelativeURL((*it).url(), baseUrl(m_part)));
        addFeedsViaDBUS(list);
    }
    else {
        kDebug() << "KonqFeedIcon::addFeeds(): use command line";
        KProcess proc;
        proc << "akregator" << "-g" << i18n("Imported Feeds");

        for ( FeedDetectorEntryList::Iterator it = m_feedList.begin(); it != m_feedList.end(); ++it ) {
            proc << "-a" << fixRelativeURL((*it).url(), baseUrl(m_part));
        }

        proc.startDetached();

    }
}

#include "konqfeedicon.moc"
// vim: ts=4 sw=4 et
