/* This file is part of the KDE project
   Copyright 2007 David Faure <faure@kde.org>
   Copyright 2007 Eduardo Robles Elvira <edulix@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqmisc.h"
#include "konqsettingsxt.h"
#include "konqundomanager.h"
#include "konqundomanageradaptor.h"
#include "konqundomanager_interface.h"
#include <QAction>
#include <QByteArray>
#include <QDBusArgument>
#include <QDBusConnectionInterface>
#include <QDirIterator>
#include <QFile>
#include <QMetaType>
#include <QtDBus/QtDBus>
#include <QTimer>
#include <QVariant>
#include <kio/fileundomanager.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <unistd.h> // getpid

Q_DECLARE_METATYPE(QList<QVariant>)

class KonqClosedWindowsManagerPrivate
{
public:
    KonqClosedWindowsManager instance;
    int m_maxNumClosedItems;
};

K_GLOBAL_STATIC(KonqClosedWindowsManagerPrivate, myKonqClosedWindowsManagerPrivate)


KonqUndoManager::KonqUndoManager(QWidget* parent)
    : QObject(parent)
{
    connect( KIO::FileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    connect( KIO::FileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );

    connect(KonqClosedWindowsManager::self(),
        SIGNAL(addWindowInOtherInstances(KonqUndoManager *, KonqClosedWindowItem *)), this,
        SLOT( slotAddClosedWindowItem(KonqUndoManager *, KonqClosedWindowItem *) ));
    connect(KonqClosedWindowsManager::self(),
        SIGNAL(removeWindowInOtherInstances(KonqUndoManager *, const KonqClosedWindowItem *)), this,
        SLOT( slotRemoveClosedWindowItem(KonqUndoManager *, const KonqClosedWindowItem *) ));
    m_populated = false;
}

KonqUndoManager::~KonqUndoManager()
{
    disconnect( KIO::FileUndoManager::self(), SIGNAL(undoAvailable(bool)),
             this, SLOT(slotFileUndoAvailable(bool)) );
    disconnect( KIO::FileUndoManager::self(), SIGNAL(undoTextChanged(QString)),
             this, SLOT(slotFileUndoTextChanged(QString)) );

    disconnect(KonqClosedWindowsManager::self(),
        SIGNAL(addWindowInOtherInstances(KonqUndoManager *, KonqClosedWindowItem *)), this,
        SLOT( slotAddClosedWindowItem(KonqUndoManager *, KonqClosedWindowItem *) ));
    disconnect(KonqClosedWindowsManager::self(),
        SIGNAL(removeWindowInOtherInstances(KonqUndoManager *, const KonqClosedWindowItem *)), this,
        SLOT( slotRemoveClosedWindowItem(KonqUndoManager *, const KonqClosedWindowItem *) ));

    // Clear the closed item lists but only remove closed windows items
    // in this window
    clearClosedItemsList(true);
}

void KonqUndoManager::populate()
{
    if(m_populated)
        return;
    m_populated = true;
    
    const QList<KonqClosedWindowItem *> closedWindowItemList =
        KonqClosedWindowsManager::self()->closedWindowItemList();

    QListIterator<KonqClosedWindowItem *> i(closedWindowItemList);
    
    // This loop is done backwards because slotAddClosedWindowItem prepends the
    // elements to the list, so if we do it forwards the we would get an inverse
    // order of closed windows
    for(i.toBack(); i.hasPrevious(); )
        slotAddClosedWindowItem(0L, i.previous());
}

void KonqUndoManager::slotFileUndoAvailable(bool)
{
    emit undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    if (KonqClosedWindowsManager::self()->undoAvailable())
        return true;
    else
        return (m_supportsFileUndo && KIO::FileUndoManager::self()->undoAvailable());
}

QString KonqUndoManager::undoText() const
{
    if (!m_closedItemList.isEmpty()) {
        const KonqClosedItem* closedItem = m_closedItemList.first();
        if (closedItem->serialNumber() > KIO::FileUndoManager::self()->currentCommandSerialNumber()) {
            const KonqClosedTabItem* closedTabItem =
                dynamic_cast<const KonqClosedTabItem *>(closedItem);
            if(closedTabItem)
                return i18n("Und&o: Closed Tab");
            else
                return i18n("Und&o: Closed Window");
        }
    } else if(KonqClosedWindowsManager::self()->undoAvailable())
        return i18n("Und&o: Closed Window");
    
    return KIO::FileUndoManager::self()->undoText();
}

void KonqUndoManager::undo()
{
    populate();
    KIO::FileUndoManager* fileUndoManager = KIO::FileUndoManager::self();
    if (!m_closedItemList.isEmpty()) {
        KonqClosedItem* closedItem = m_closedItemList.first();

        // Check what to undo
        if (closedItem->serialNumber() > fileUndoManager->currentCommandSerialNumber()) {
            undoClosedItem(0);
            return;
        }
    }
    fileUndoManager->uiInterface()->setParentWidget(qobject_cast<QWidget*>(parent()));
    fileUndoManager->undo();
}

void KonqUndoManager::slotAddClosedWindowItem(KonqUndoManager *real_sender, KonqClosedWindowItem *closedWindowItem)
{
    if(real_sender == this)
        return;
    
    populate();
    
    if(m_closedItemList.size() >= KonqSettings::maxNumClosedItems())
    {
        const KonqClosedItem* last = m_closedItemList.last();
        const KonqClosedTabItem* lastTab =
            dynamic_cast<const KonqClosedTabItem *>(last);
        m_closedItemList.removeLast();

        // Delete only if it's a tab
        if(lastTab)
            delete lastTab;
    }

    m_closedItemList.prepend(closedWindowItem);
    emit undoTextChanged(i18n("Und&o: Closed Window"));
    emit undoAvailable(true);
    emit closedItemsListChanged();
}

void KonqUndoManager::addClosedWindowItem(KonqClosedWindowItem *closedWindowItem)
{
    populate();
    KonqClosedWindowsManager::self()->addClosedWindowItem(this, closedWindowItem);
}

void KonqUndoManager::slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const KonqClosedWindowItem *closedWindowItem)
{
    if(real_sender == this)
        return;
    
    populate();

    QList<KonqClosedItem *>::iterator it = qFind(m_closedItemList.begin(), m_closedItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if(it != m_closedItemList.end()) {
        m_closedItemList.erase(it);
        emit undoAvailable(this->undoAvailable());
        emit closedItemsListChanged();
    }
}

const QList<KonqClosedItem *>& KonqUndoManager::closedItemsList()
{
    populate();
    return m_closedItemList;
}

void KonqUndoManager::undoClosedItem(int index)
{
    populate();
    Q_ASSERT(!m_closedItemList.isEmpty());
    KonqClosedItem* closedItem = m_closedItemList.at( index );
    m_closedItemList.removeAt(index);

    const KonqClosedTabItem* closedTabItem =
        dynamic_cast<const KonqClosedTabItem *>(closedItem);
    KonqClosedRemoteWindowItem* closedRemoteWindowItem =
        dynamic_cast<KonqClosedRemoteWindowItem *>(closedItem);
    KonqClosedWindowItem* closedWindowItem =
        dynamic_cast<KonqClosedWindowItem *>(closedItem);
    if(closedTabItem)
        emit openClosedTab(*closedTabItem);
    else if(closedRemoteWindowItem) {
        emit openClosedWindow(*closedRemoteWindowItem);
        KonqClosedWindowsManager::self()->removeClosedWindowItem(this, closedRemoteWindowItem);
    } else if(closedWindowItem) {
        emit openClosedWindow(*closedWindowItem);
        KonqClosedWindowsManager::self()->removeClosedWindowItem(this, closedWindowItem);
        closedWindowItem->configGroup().deleteGroup();
    }
    delete closedItem;
    emit undoAvailable(this->undoAvailable());
    emit undoTextChanged(this->undoText());
    emit closedItemsListChanged();
}

void KonqUndoManager::slotClosedItemsActivated(QAction* action)
{
    // Open a closed tab
    const int index = action->data().toInt();
    undoClosedItem(index);
}

void KonqUndoManager::slotFileUndoTextChanged(const QString& text)
{
    // I guess we can always just forward that one?
    emit undoTextChanged(text);
}

quint64 KonqUndoManager::newCommandSerialNumber()
{
    return KIO::FileUndoManager::self()->newCommandSerialNumber();
}

void KonqUndoManager::addClosedTabItem(KonqClosedTabItem* closedTabItem)
{
    populate();
    
    if(m_closedItemList.size() >= KonqSettings::maxNumClosedItems())
    {
        const KonqClosedItem* last = m_closedItemList.last();
        const KonqClosedTabItem* lastTab =
            dynamic_cast<const KonqClosedTabItem *>(last);
        m_closedItemList.removeLast();

        // Delete only if it's a tab
        if(lastTab)
            delete lastTab;
    }

    m_closedItemList.prepend(closedTabItem);
    emit undoTextChanged(i18n("Und&o: Closed Tab"));
    emit undoAvailable(true);
}

void KonqUndoManager::updateSupportsFileUndo(bool enable)
{
    m_supportsFileUndo = enable;
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::clearClosedItemsList(bool onlyInthisWindow)
{
    populate();
// we only DELETE tab items! So we can't do this anymore:
//     qDeleteAll(m_closedItemList);
    QList<KonqClosedItem *>::iterator it = m_closedItemList.begin();
    for (; it != m_closedItemList.end(); ++it)
    {
        KonqClosedItem *closedItem = *it;
        const KonqClosedTabItem* closedTabItem =
            dynamic_cast<const KonqClosedTabItem *>(closedItem);
        const KonqClosedWindowItem* closedWindowItem =
            dynamic_cast<const KonqClosedWindowItem *>(closedItem);

        m_closedItemList.erase(it);
        if(closedTabItem)
            delete closedTabItem;
        else if (closedWindowItem && !onlyInthisWindow) {
            KonqClosedWindowsManager::self()->removeClosedWindowItem(this, closedWindowItem, true);
            delete closedWindowItem;
        }
    }
    emit closedItemsListChanged();
    emit undoAvailable(this->undoAvailable());
}

void KonqUndoManager::undoLastClosedItem()
{
    undoClosedItem(0);
}

KonqClosedWindowsManager::KonqClosedWindowsManager()
{
    qDBusRegisterMetaType<QList<QVariant> >();

    new KonqClosedWindowsManagerAdaptor ( this );

    const QString dbusPath = "/KonqUndoManager";
    const QString dbusInterface = "org.kde.Konqueror.UndoManager";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyClosedWindowItem", this, SLOT(slotNotifyClosedWindowItem(QString,int,QString,QString,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyRemove", this, SLOT(slotNotifyRemove(QString,QString,QDBusMessage)));

    QString filename = "closeditems/" + KonqMisc::encodeFilename(dbus.baseService());
    QString file = KStandardDirs::locateLocal("tmp", filename);
    QFile::remove(file);
    
    KConfigGroup configGroup(KGlobal::config(), "Undo");
    m_numUndoClosedItems = configGroup.readEntry("Number of Closed Windows", 0);
    
    m_konqClosedItemsConfig = 0L;
    m_blockClosedItems = false;
}

KonqClosedWindowsManager::~KonqClosedWindowsManager()
{
    // Do some file cleaning
    removeClosedItemsConfigFiles();

    delete m_konqClosedItemsConfig;
}

KConfig* KonqClosedWindowsManager::config()
{
    readConfig();
    return m_konqClosedItemsConfig;
}

KonqClosedWindowsManager *KonqClosedWindowsManager::self()
{
    return &myKonqClosedWindowsManagerPrivate->instance;
}

void KonqClosedWindowsManager::addClosedWindowItem(KonqUndoManager
*real_sender, KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    readConfig();
    // If we are off the limit, remove the last closed window item
    if(m_closedWindowItemList.size() >=
        KonqSettings::maxNumClosedItems())
    {
        KonqClosedWindowItem* last = m_closedWindowItemList.last();

        emit removeWindowInOtherInstances(0L, last);
        emitNotifyRemove(last);

        m_closedWindowItemList.removeLast();
        delete last;
    }

    m_closedWindowItemList.prepend(closedWindowItem);
    if(!m_blockClosedItems)
    {
        m_numUndoClosedItems++;
        emit addWindowInOtherInstances(real_sender, closedWindowItem);
    }

    if(propagate)
    {
        emitNotifyClosedWindowItem(closedWindowItem);
        
        // if it needs to be propagated means that it's a local window and thus
        // we need to call to saveConfig() to keep updated the kconfig file, so
        // that new konqueror instances can read it correctly updated.
        saveConfig();
    }
}

void KonqClosedWindowsManager::removeClosedWindowItem(KonqUndoManager
*real_sender, const KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    readConfig();
    QList<KonqClosedWindowItem *>::iterator it = qFind(m_closedWindowItemList.begin(),
    m_closedWindowItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if(it != m_closedWindowItemList.end()) {
        m_closedWindowItemList.erase(it);
        m_numUndoClosedItems--;
    }
    emit removeWindowInOtherInstances(real_sender, closedWindowItem);

    if(propagate)
        emitNotifyRemove(closedWindowItem);
}

const QList<KonqClosedWindowItem *>& KonqClosedWindowsManager::closedWindowItemList()
{
    readConfig();
    return m_closedWindowItemList;
}

void KonqClosedWindowsManager::readSettings()
{
    readConfig();
}



static QString dbusService()
{
    return QDBusConnection::sessionBus().baseService();
}

/**
 * Returns whether the DBUS call we are handling was a call from us self
 */
bool isSenderOfSignal( const QDBusMessage& msg )
{
    return dbusService() == msg.service();
}

bool isSenderOfSignal( const QString& service )
{
    return dbusService() == service;
}

void KonqClosedWindowsManager::emitNotifyClosedWindowItem(
    const KonqClosedWindowItem *closedWindowItem)
{
    emit notifyClosedWindowItem( closedWindowItem->title(),
        closedWindowItem->numTabs(),
        closedWindowItem->configGroup().config()->name(),
        closedWindowItem->configGroup().name() );
}

void KonqClosedWindowsManager::emitNotifyRemove(
    const KonqClosedWindowItem *closedWindowItem)
{
    const KonqClosedRemoteWindowItem* closedRemoteWindowItem =
        dynamic_cast<const KonqClosedRemoteWindowItem *>(closedWindowItem);

    // Here we do this because there's no need to call to configGroup() if it's
    // a remote window item, and it would be error prone to be so, because
    // it could give us a null pointer and konqueror would crash
    if(closedRemoteWindowItem)
        emit notifyRemove(  closedRemoteWindowItem->remoteConfigFileName(),
            closedRemoteWindowItem->remoteGroupName() );
    else
        emit notifyRemove(  closedWindowItem->configGroup().config()->name(),
            closedWindowItem->configGroup().name() );
}

void KonqClosedWindowsManager::slotNotifyClosedWindowItem(
    const QString& title, const int& numTabs, const QString& configFileName,
    const QString& configGroup, const QString& service )
{
    if ( isSenderOfSignal( service ) )
        return;

    // Create a new ClosedWindowItem and add it to the list
    KonqClosedWindowItem* closedWindowItem = new KonqClosedRemoteWindowItem(
        title, configGroup, configFileName,
        KIO::FileUndoManager::self()->newCommandSerialNumber(), numTabs,
        service);

    // Add it to all the windows but don't propogate over dbus,
    // as it already comes from dbus)
    addClosedWindowItem(0L, closedWindowItem, false);
}

void KonqClosedWindowsManager::slotNotifyClosedWindowItem(
    const QString& title, const int& numTabs, const QString& configFileName,
    const QString& configGroup, const QDBusMessage& msg )
{
     slotNotifyClosedWindowItem(title, numTabs, configFileName, configGroup,
        msg.service() );
}

void KonqClosedWindowsManager::slotNotifyRemove(
    const QString& configFileName, const QString& configGroup,
    const QDBusMessage& msg )
{
    if ( isSenderOfSignal( msg ) )
        return;

    // Find the window item. It can be either remote or local
    KonqClosedWindowItem* closedWindowItem =
        findClosedRemoteWindowItem(configFileName, configGroup);
    if(!closedWindowItem)
    {
        closedWindowItem = findClosedLocalWindowItem(configFileName, configGroup);
        if(!closedWindowItem)
            return;
    }

    // Remove it in all the windows but don't propogate over dbus,
    // as it already comes from dbus)
    removeClosedWindowItem(0L, closedWindowItem, false);
}

KonqClosedRemoteWindowItem* KonqClosedWindowsManager::findClosedRemoteWindowItem(
    const QString& configFileName,
    const QString& configGroup)
{
    readConfig();
    
    KonqClosedRemoteWindowItem* closedRemoteWindowItem = 0L;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.begin();
        it != m_closedWindowItemList.end(); ++it)
    {
        closedRemoteWindowItem = dynamic_cast<KonqClosedRemoteWindowItem *>(*it);

        if(closedRemoteWindowItem &&
            closedRemoteWindowItem->equalsTo(configFileName, configGroup))
            return closedRemoteWindowItem;
    }

    return closedRemoteWindowItem;
}

KonqClosedWindowItem* KonqClosedWindowsManager::findClosedLocalWindowItem(
    const QString& configFileName,
    const QString& configGroup)
{
    readConfig();
    KonqClosedWindowItem* closedWindowItem = 0L;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.begin();
        it != m_closedWindowItemList.end(); ++it)
    {
        closedWindowItem = *it;
        KonqClosedRemoteWindowItem* closedRemoteWindowItem =
            dynamic_cast<KonqClosedRemoteWindowItem *>(closedWindowItem);

        if(!closedRemoteWindowItem && closedWindowItem &&
            closedWindowItem->configGroup().config()->name() == configFileName &&
            closedWindowItem->configGroup().name() == configGroup)
            return closedWindowItem;
    }

    return closedWindowItem;
}

void KonqClosedWindowsManager::removeClosedItemsConfigFiles()
{
    QString dir = KStandardDirs::locateLocal("tmp", "closeditems/");
    QDBusConnectionInterface *idbus = QDBusConnection::sessionBus().interface();
    QDirIterator it(dir, QDir::Writable|QDir::Files);
    while (it.hasNext())
    {
        // Only remove the files for those konqueror instances not running anymore
        QString filename = it.next();
        if(!idbus->isServiceRegistered(KonqMisc::decodeFilename(it.fileName())))
            QFile::remove(filename);
    }
}


void KonqClosedWindowsManager::saveConfig()
{
    readConfig();
    
    // Create / overwrite the saved closed windows list
    QString filename = "closeditems_saved";
    QString file = KStandardDirs::locateLocal("appdata", filename);
    QFile::remove(file);

    KConfig *config = new KConfig(filename, KConfig::SimpleConfig, "appdata");

    // Populate the config file
    KonqClosedWindowItem* closedWindowItem = 0L;
    uint counter = m_closedWindowItemList.size()-1;
    for (QList<KonqClosedWindowItem *>::const_iterator it = m_closedWindowItemList.begin();
        it != m_closedWindowItemList.end(); ++it, --counter)
    {
        closedWindowItem = *it;
        KConfigGroup configGroup(config, "Closed_Window" + QString::number(counter));
        configGroup.writeEntry("title", closedWindowItem->title());
        configGroup.writeEntry("numTabs", closedWindowItem->numTabs());
        closedWindowItem->configGroup().copyTo(&configGroup);
    }
    
    KConfigGroup configGroup(KGlobal::config(), "Undo");
    configGroup.writeEntry("Number of Closed Windows", m_closedWindowItemList.size());
    configGroup.sync();

    delete config;
}

void KonqClosedWindowsManager ::readConfig()
{
    if(m_konqClosedItemsConfig)
        return;

    QString filename = "closeditems_saved";
    QString file = KStandardDirs::locateLocal("appdata", filename);

    m_konqClosedItemsConfig = new KConfig(file, KConfig::SimpleConfig);
    
    // If the config file doesn't exists, there's nothing to read
    if(!QFile::exists(file))
        return;
    
    m_blockClosedItems = true;
    for(int i = 0; i < m_numUndoClosedItems; i++)
    {
        // For each item, create a new ClosedWindowItem
        KConfigGroup configGroup(m_konqClosedItemsConfig, "Closed_Window" + 
            QString::number(i));
        
        // The number of closed items was not correctly set, fix it and save the
        // correct number.
        if(!configGroup.exists())
        {
            m_numUndoClosedItems = i;
            KConfigGroup configGroup(KGlobal::config(), "Undo");
            configGroup.writeEntry("Number of Closed Windows",
                m_closedWindowItemList.size());
            configGroup.sync();
            break;
        }
        
        QString title = configGroup.readEntry("title", i18n("no name"));
        int numTabs = configGroup.readEntry("numTabs", 0);

        KonqClosedWindowItem* closedWindowItem = new KonqClosedWindowItem(
            title,  KIO::FileUndoManager::self()->newCommandSerialNumber(),
            numTabs);
        configGroup.copyTo(&closedWindowItem->configGroup());
        configGroup.writeEntry("foo", 0);
        closedWindowItem->configGroup().config()->sync();

        // Add the item only to this window
        addClosedWindowItem(0L, closedWindowItem, false);
    }
    
    m_blockClosedItems = false;
}

bool KonqClosedWindowsManager::undoAvailable() const
{
    return m_numUndoClosedItems > 0;
}

#include "konqundomanager.moc"
