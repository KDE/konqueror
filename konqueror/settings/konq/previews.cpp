/* This file is part of the KDE libraries
    Copyright (C) 2002 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
//
// File previews configuration
//

// Own
#include "previews.h"

// Qt
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QBoxLayout>
#include <QtGui/QTreeWidget>
#include <QtDBus/QtDBus>

// KDE
#include <klocale.h>
#include <knuminput.h>
#include <kprotocolmanager.h>
#include <kconfiggroup.h>

// Local
#include "konqkcmfactory.h"

//-----------------------------------------------------------------------------

class PreviewCheckListItem : public QTreeWidgetItem
{
  public:
    PreviewCheckListItem( QTreeWidget *parent, const QString &text )
      : QTreeWidgetItem( parent, QStringList() << text )
    {}

    PreviewCheckListItem( QTreeWidgetItem *parent, const QString &text )
      : QTreeWidgetItem( parent, QStringList() << text )
    {
        setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    }
};

KPreviewOptions::KPreviewOptions( QWidget *parent, const QVariantList & )
    : KCModule( KonqKcmFactory::componentData(), parent )
{
    QVBoxLayout *lay = new QVBoxLayout(this);

    QLabel *label = new QLabel( i18n("<p>Allow previews, \"Folder Icons Reflect Contents\", and "
                                     "retrieval of meta-data on protocols:</p>"), this );
    label->setWordWrap(true);
    lay->addWidget(label);

    setQuickHelp( i18n("<h1>Preview Options</h1> Here you can modify the behavior "
                "of Konqueror when it shows the files in a folder."
                "<h2>The list of protocols:</h2> check the protocols over which "
                "previews should be shown; uncheck those over which they should not. "
                "For instance, you might want to show previews over SMB if the local "
                "network is fast enough, but you might disable it for FTP if you often "
                "visit very slow FTP sites with large images."
                "<h2>Maximum File Size:</h2> select the maximum file size for which "
                "previews should be generated. For instance, if set to 1 MB (the default), "
                "no preview will be generated for files bigger than 1 MB, for speed reasons."));

    // Listview containing checkboxes for all protocols that support listing
    QTreeWidget *listView = new QTreeWidget( this );
    listView->setHeaderLabel( i18n( "Select Protocols" ) );
    listView->setRootIsDecorated( false );

    QHBoxLayout *hbox = new QHBoxLayout();
    lay->addItem( hbox );
    hbox->addWidget( listView );
    hbox->addStretch();

    PreviewCheckListItem *localItems = new PreviewCheckListItem( listView,
        i18n( "Local Protocols" ) );
    localItems->setCheckState( 0, Qt::Unchecked );
    PreviewCheckListItem *inetItems = new PreviewCheckListItem( listView,
        i18n( "Internet Protocols" ) );
    inetItems->setCheckState( 0, Qt::Unchecked );

    QStringList protocolList = KProtocolInfo::protocols();
    protocolList.sort();
    QStringList::Iterator it = protocolList.begin();

    KUrl url;
    url.setPath("/");

    for ( ; it != protocolList.end() ; ++it )
    {
        url.setProtocol( *it );
        if ( KProtocolManager::supportsListing( url ) )
        {
            PreviewCheckListItem *item;
            if ( KProtocolInfo::protocolClass( *it ) == ":local" )
                item = new PreviewCheckListItem( localItems, ( *it ) );
            else
                item = new PreviewCheckListItem( inetItems, ( *it ) );

            m_items.append( item );
        }
    }

    listView->setItemExpanded( localItems, true );
    listView->setItemExpanded( inetItems, true );

    listView->setWhatsThis(
                     i18n("This option makes it possible to choose when the file previews, "
                          "smart folder icons, and meta-data in the File Manager should be activated.\n"
                          "In the list of protocols that appear, select which ones are fast "
                          "enough for you to allow previews to be generated.") );

    label = new QLabel( i18n( "&Maximum file size:" ), this );
    lay->addWidget( label );

    m_maxSize = new KDoubleNumInput( this );
    m_maxSize->setSuffix( i18n(" MB") );
    m_maxSize->setRange( 0.02, 10, 0.02, true );
    m_maxSize->setDecimals( 1 );
    label->setBuddy( m_maxSize );
    lay->addWidget( m_maxSize );
    connect( m_maxSize, SIGNAL( valueChanged(double) ), SLOT( changed() ) );

    m_boostSize = new QCheckBox(i18n("&Increase size of previews relative to icons"), this);
    connect( m_boostSize, SIGNAL( toggled(bool) ), SLOT( changed() ) );
    lay->addWidget(m_boostSize);

    m_useFileThumbnails = new QCheckBox(i18n("&Use thumbnails embedded in files"), this);
    connect( m_useFileThumbnails, SIGNAL( toggled(bool) ), SLOT( changed() ) );

    lay->addWidget(m_useFileThumbnails);

    m_useFileThumbnails->setWhatsThis(
                i18n("Select this to use thumbnails that are found inside some "
                "file types (e.g. JPEG). This will increase speed and reduce "
                "disk usage. Deselect it if you have files that have been processed "
                "by programs which create inaccurate thumbnails, such as ImageMagick.") );

    lay->addStretch();

    connect(listView, SIGNAL(itemChanged(QTreeWidgetItem *, int)), SLOT(itemChanged(QTreeWidgetItem *)));
}

// Default: 1 MB
#define DEFAULT_MAXSIZE (1024*1024)

void KPreviewOptions::load(bool useDefaults)
{
    // *** load and apply to GUI ***
    KGlobal::config()->setReadDefaults(useDefaults);
    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    foreach( PreviewCheckListItem *item, m_items ) {
        QString protocol( item->text(0) );
        if ( ( protocol == "file" ) && ( !group.hasKey ( protocol ) ) ) {
          // file should be enabled in case is not defined because if not so
          // than preview's lost when size is changed from default one
          item->setCheckState( 0, Qt::Checked );
        } else {
          item->setCheckState( 0, group.readEntry( protocol, false ) ? Qt::Checked : Qt::Unchecked);
        }
    }
    // config key is in bytes (default value 1MB), numinput is in MB
    m_maxSize->setValue( ((double)group.readEntry( "MaximumSize", DEFAULT_MAXSIZE )) / (1024*1024) );

    m_boostSize->setChecked( group.readEntry( "BoostSize", false /*default*/) );
    m_useFileThumbnails->setChecked( group.readEntry( "UseFileThumbnails", true /*default*/) );
    KGlobal::config()->setReadDefaults(false);
}

void KPreviewOptions::load()
{
    load(false);
}

void KPreviewOptions::defaults()
{
    load(true);
}

void KPreviewOptions::save()
{
    KConfigGroup group( KGlobal::config(), "PreviewSettings" );
    foreach( PreviewCheckListItem *item, m_items ) {
        QString protocol( item->text(0) );
        group.writeEntry( protocol, item->checkState(0) == Qt::Checked, KConfigBase::Normal | KConfigBase::Global );
    }
    // config key is in bytes, numinput is in MB
    group.writeEntry( "MaximumSize", qRound( m_maxSize->value() *1024*1024 ), KConfigBase::Normal | KConfigBase::Global );
    group.writeEntry( "BoostSize", m_boostSize->isChecked(), KConfigBase::Normal | KConfigBase::Global );
    group.writeEntry( "UseFileThumbnails", m_useFileThumbnails->isChecked(), KConfigBase::Normal | KConfigBase::Global );
    group.sync();

    // Send signal to all konqueror instances
    QDBusMessage message =
        QDBusMessage::createSignal("/KonqMain", "org.kde.Konqueror.Main", "reparseConfiguration");
    QDBusConnection::sessionBus().send(message);
}

void KPreviewOptions::changed()
{
    emit KCModule::changed(true);
}

void KPreviewOptions::itemChanged(QTreeWidgetItem *item)
{
    if (!item->parent()) { // a root item
        for (int i = 0; i < item->childCount(); i++)
            item->child(i)->setCheckState(0, item->checkState(0));
    }
    changed();
}

#include "previews.moc"
