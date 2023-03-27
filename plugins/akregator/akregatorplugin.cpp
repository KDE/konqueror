/* This file is part of the KDE project

    SPDX-FileCopyrightText: 2004 Gary Cramblitt <garycramblitt@comcast.net>

    Adapted from kdeutils/ark/konqplugin by:
    SPDX-FileCopyrightText: Georg Robbers <Georg.Robbers@urz.uni-hd.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "akregatorplugin.h"

#include "pluginutil.h"
#include "akregatorplugindebug.h"

#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kfileitem.h>
#include <kfileitemlistproperties.h>

#include <QDebug>
#include <QAction>

using namespace Akregator;

K_PLUGIN_FACTORY_WITH_JSON(AkregatorMenuFactory, "akregator_konqplugin.json", registerPlugin<AkregatorMenu>();)

AkregatorMenu::AkregatorMenu(QObject *parent, const QVariantList &args)
  : KAbstractFileItemActionPlugin(parent)
{
    Q_UNUSED(args);

#if 0
    if (QByteArray(kapp->name()) == "kdesktop" && !KAuthorized::authorizeAction("editable_desktop_icons")) {
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
