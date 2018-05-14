/* This file is part of the KDE project

   Copyright (C) 2004 Gary Cramblitt <garycramblitt@comcast.net>

   Adapted from kdeutils/ark/konqplugin by
        Georg Robbers <Georg.Robbers@urz.uni-hd.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "akregatorplugin.h"

#include "pluginutil.h"
#include "akregatorplugindebug.h"

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kfileitem.h>
#include <kfileitemlistproperties.h>

#include <qdebug.h>
#include <qaction.h>

using namespace Akregator;

K_PLUGIN_FACTORY_WITH_JSON(AkregatorMenuFactory, "akregator_konqplugin.json", registerPlugin<AkregatorMenu>();)

AkregatorMenu::AkregatorMenu(QObject *parent, const QVariantList &args)
  : KAbstractFileItemActionPlugin(parent)
{
    Q_UNUSED(args);

#if 0
    if (QByteArray(kapp->name()) == "kdesktop" && !KAuthorized::authorizeKAction("editable_desktop_icons")) {
        return;
    }
#endif
    // Do nothing if user has turned us off.
    // TODO: Not yet implemented in aKregator settings.

    /*
    m_conf = new KConfig( "akregatorrc" );
    m_conf->setGroup( "AkregatorKonqPlugin" );
    if ( !m_conf->readEntry( "Enable", true ) )
        return;
    */

    m_feedMimeTypes << "application/rss+xml" << "text/rdf" << "application/xml";
}


QList<QAction *> AkregatorMenu::actions(const KFileItemListProperties &fileItemInfos, QWidget *parent)
{
    Q_UNUSED(parent);

    QList<QAction *> acts;
    const KFileItemList items = fileItemInfos.items();
    foreach (const KFileItem &item, items) {
        if (isFeedUrl(item)) {
            qCDebug(AKREGATORPLUGIN_LOG) << "found feed" << item.url();

            QAction *action = new QAction(this);
            action->setText(i18nc("@action:inmenu", "Add Feed to Akregator"));
            action->setIcon(QIcon::fromTheme("akregator"));
            action->setData(item.url());
            connect(action, &QAction::triggered, this, &AkregatorMenu::slotAddFeed);
            acts.append(action);
        }
    }

    return acts;
}


static bool isFeedUrl(const QString &urlPath)
{
    // If URL ends in .htm or .html, it is not a feed url.
    if (urlPath.endsWith(".htm", Qt::CaseInsensitive) || urlPath.endsWith(".html", Qt::CaseInsensitive)) {
        return false;
    }
    if (urlPath.contains("rss", Qt::CaseInsensitive)) {
        return true;
    }
    if (urlPath.contains("rdf", Qt::CaseInsensitive)) {
        return true;
    }
    return false;
}

bool AkregatorMenu::isFeedUrl(const KFileItem &item) const
{
    if (m_feedMimeTypes.contains(item.mimetype())) {
        return true;
    } else {
        return ::isFeedUrl(item.url().path());
    }
    return false;
}

void AkregatorMenu::slotAddFeed()
{
    QAction *action = qobject_cast<QAction *>(sender());
    Q_ASSERT(action!=nullptr);

    QString url = action->data().toUrl().url();
    qCDebug(AKREGATORPLUGIN_LOG) << "for feed url" << url;
    PluginUtil::addFeeds(QStringList(url));
}

#include "akregatorplugin.moc"
