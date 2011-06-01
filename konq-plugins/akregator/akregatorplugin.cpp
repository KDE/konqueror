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
#include "pluginbase.h"

#include <kapplication.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kaction.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <konq_popupmenu.h>
#include <kmenu.h>
#include <kgenericfactory.h>
#include <kurl.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kmessagebox.h>

#include <qdir.h>
#include <qobject.h>
#include <qstringlist.h>
#include <kauthorized.h>

using namespace Akregator;

typedef KGenericFactory<AkregatorMenu, KonqPopupMenu> AkregatorMenuFactory;
K_EXPORT_PLUGIN(AkregatorMenuFactory("akregatorkonqplugin"))

AkregatorMenu::AkregatorMenu( KonqPopupMenu * popupmenu, const QStringList& /* list */ )
    : KonqPopupMenuPlugin( popupmenu), PluginBase(), /*m_conf(0),*/ m_part(0)
{
    kDebug() << "AkregatorMenu::AkregatorMenu()";
#if 0
    if ( QByteArray( kapp->name() ) == "kdesktop" && !KAuthorized::authorizeKAction("editable_desktop_icons" ) )
        return;
#endif
    // Do nothing if user has turned us off.
    // TODO: Not yet implemented in aKregator settings.
    
    /*
    m_conf = new KConfig( "akregatorrc" );
    m_conf->setGroup( "AkregatorKonqPlugin" );
    if ( !m_conf->readEntry( "Enable", true ) )
        return;
    */
    KHTMLView* view = 0L;
     
    if (popupmenu && popupmenu->parent() && popupmenu->parent()->inherits("KHTMLView"))
        view = static_cast<KHTMLView*>(popupmenu->parent());
    
    if (view)
        m_part = view->part();
     
    KGlobal::locale()->insertCatalog("akregator_konqplugin");
    m_feedMimeTypes << "application/rss+xml" << "text/rdf" << "application/xml";
    // Get the list of URLs clicked on from Konqi.
    //KFileItemList m_list = popupmenu->fileItemList();
    // See if any are RSS feeds.
    
    KFileItemList list = popupmenu->fileItemList();
    foreach ( const KFileItem &it, list )
    {
	if (isFeedUrl(it))
        {
	    kDebug() << "AkregatorMenu: found feed URL " << it.url().prettyUrl();
            QAction *action = actionCollection()->addAction( "akregatorkonqplugin_mnu");
            action->setText(i18n("Add Feed to Akregator"));
            action->setIcon(KIcon("akregator"));
            connect(action, SIGNAL(triggered()), this, SLOT( slotAddFeed() ));
            //addAction( action );
            addSeparator();
	    m_feedURL = it.url().url();
            break;
        }
    }
}

AkregatorMenu::~AkregatorMenu()
{
    KGlobal::locale()->removeCatalog("akregator_konqplugin");
    //delete m_conf;
}

bool AkregatorMenu::isFeedUrl(const QString &url)
{
    if (url.contains(".htm", false) != 0) return false;
    if (url.contains("rss", false) != 0) return true;
    if (url.contains("rdf", false) != 0) return true;
    if (url.contains("xml", false) != 0) return true;
    return false;
}

bool AkregatorMenu::isFeedUrl(const KFileItem & item)
{
    if ( m_feedMimeTypes.contains( item.mimetype() ) )
        return true;
    else
    {
        QString url = item.url().url();
        // If URL ends in .htm or .html, it is not a feed url.
        return isFeedUrl(url);
    }
    return false;
}

void AkregatorMenu::slotAddFeed()
{
    QString url = m_part ? fixRelativeURL(m_feedURL, m_part->baseURL()) : m_feedURL;
     
    if(akregatorRunning())
      addFeedsViaDBUS(QStringList(url));
    else
        addFeedViaCmdLine(url);
}

#include "akregatorplugin.moc"
