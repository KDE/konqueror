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

#include "konqundomanager.h"
#include "konqundomanageradaptor.h"
#include "konqundomanager_interface.h"
#include <QAction>
#include <QByteArray>
#include <QDBusArgument>
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
    bool m_amIalone;
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
    populate();
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
    const QList<KonqClosedWindowItem *> closedWindowItemList =
        KonqClosedWindowsManager::self()->closedWindowItemList();

    foreach(KonqClosedWindowItem *closedWindowItem, closedWindowItemList)
        slotAddClosedWindowItem(0L, closedWindowItem);
}

void KonqUndoManager::slotFileUndoAvailable(bool)
{
    emit undoAvailable(this->undoAvailable());
}

bool KonqUndoManager::undoAvailable() const
{
    if (!m_closedItemList.isEmpty())
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
    }
    return KIO::FileUndoManager::self()->undoText();
}

void KonqUndoManager::undo()
{
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

    if(m_closedItemList.size() >= KonqClosedWindowsManager::self()->maxNumClosedItems())
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
    KonqClosedWindowsManager::self()->addClosedWindowItem(this, closedWindowItem);
}

void KonqUndoManager::slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const KonqClosedWindowItem *closedWindowItem)
{
    if(real_sender == this)
        return;

    QList<KonqClosedItem *>::iterator it = qFind(m_closedItemList.begin(), m_closedItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if(it != m_closedItemList.end()) {
        m_closedItemList.erase(it);
        emit undoAvailable(this->undoAvailable());
        emit closedItemsListChanged();
    }
}

const QList<KonqClosedItem *>& KonqUndoManager::closedTabsList() const
{
    return m_closedItemList;
}

void KonqUndoManager::undoClosedItem(int index)
{
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
    if(m_closedItemList.size() >= KonqClosedWindowsManager::self()->maxNumClosedItems())
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
    QTimer::singleShot(0, this, SLOT(readSettings()));

    qDBusRegisterMetaType<QList<QVariant> >();

    new KonqClosedWindowsManagerAdaptor ( this );

    const QString dbusPath = "/KonqUndoManager";
    const QString dbusInterface = "org.kde.Konqueror.UndoManager";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyClosedWindowItem", this, SLOT(slotNotifyClosedWindowItem(QString,int,QString,QString,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "notifyRemove", this, SLOT(slotNotifyRemove(QString,QString,QDBusMessage)));
    dbus.connect(QString(), dbusPath, dbusInterface, "requestLocalClosedWindowItems", this, SLOT(slotRequestLocalClosedWindowItems(QDBusMessage)));

    QString filename = "closeditems/closeditems_" + QString::number(getpid());
    QString file = KStandardDirs::locateLocal("appdata", filename);
    QFile::remove(file);

    m_konqClosedItemsConfig = new KConfig(filename, KConfig::SimpleConfig, "appdata");

}

KonqClosedWindowsManager::~KonqClosedWindowsManager()
{
    saveConfig();

    delete m_konqClosedItemsConfig;
}

KConfig* KonqClosedWindowsManager::config()
{
    return m_konqClosedItemsConfig;
}

KonqClosedWindowsManager *KonqClosedWindowsManager::self()
{
    return &myKonqClosedWindowsManagerPrivate->instance;
}

void KonqClosedWindowsManager::addClosedWindowItem(KonqUndoManager
*real_sender, KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    // If we are off the limit, remove the last closed window item
    if(m_closedWindowItemList.size() >=
        maxNumClosedItems())
    {
        KonqClosedWindowItem* last = m_closedWindowItemList.last();

        emit removeWindowInOtherInstances(0L, last);
        emitNotifyRemove(last);

        m_closedWindowItemList.removeLast();
        delete last;
    }

    m_closedWindowItemList.prepend(closedWindowItem);
    emit addWindowInOtherInstances(real_sender, closedWindowItem);

    if(propagate)
        emitNotifyClosedWindowItem(closedWindowItem);
}

void KonqClosedWindowsManager::removeClosedWindowItem(KonqUndoManager
*real_sender, const KonqClosedWindowItem *closedWindowItem, bool propagate)
{
    QList<KonqClosedWindowItem *>::iterator it = qFind(m_closedWindowItemList.begin(),
    m_closedWindowItemList.end(), closedWindowItem);

    // If the item was found, remove it from the list
    if(it != m_closedWindowItemList.end()) {
        m_closedWindowItemList.erase(it);
    }
    emit removeWindowInOtherInstances(real_sender, closedWindowItem);

    if(propagate)
        emitNotifyRemove(closedWindowItem);
}

const QList<KonqClosedWindowItem *>& KonqClosedWindowsManager::closedWindowItemList()
{
    return m_closedWindowItemList;
}

int KonqClosedWindowsManager::maxNumClosedItems()
{
    return m_maxNumClosedItems;
}

void KonqClosedWindowsManager::setMaxNumClosedItems(int max)
{
    m_maxNumClosedItems = qMax(1, max);
}

void KonqClosedWindowsManager::readSettings()
{
    KSharedConfigPtr config = KGlobal::config();

    KConfigGroup configGroup( config, "UndoManagerSettings");
    m_maxNumClosedItems = configGroup.readEntry("Maximum number of Closed Items", 20 );
    m_maxNumClosedItems = qMax(1, m_maxNumClosedItems);

    m_amIalone = true;
    populate();

    // If in 0.5 seconds no other konqueror instance answer, we will assume that
    // we're alone and therefore instead of getting the undo closed window list
    // via dbus, we'll read it from disk
    QTimer::singleShot(500, this, SLOT(readConfig()));
}



void KonqClosedWindowsManager::applySettings()
{
    KConfigGroup configGroup(KSharedConfig::openConfig("konquerorrc"), "UndoManagerSettings");

    configGroup.writeEntry("Value youngerThan", m_maxNumClosedItems );
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

void KonqClosedWindowsManager::slotRequestLocalClosedWindowItems(
    const QDBusMessage& msg)
{
    if ( isSenderOfSignal( msg ) )
        return;

    emitPong(msg.service());


    if ( closedWindowItemList().empty() )
        return;

    QList<QVariant> windowItems;
    KonqClosedWindowItem *closedWindowItem;
    for (QList<KonqClosedWindowItem *>::const_iterator it = closedWindowItemList().begin();
        it != closedWindowItemList().end(); ++it)
    {
        closedWindowItem = *it;
        KonqClosedRemoteWindowItem* closedRemoteWindowItem =
            dynamic_cast<KonqClosedRemoteWindowItem *>(closedWindowItem);

        if(!closedRemoteWindowItem && closedWindowItem)
        {
            // Add to the list
            QByteArray data;
            QDataStream stream( &data, QIODevice::WriteOnly );
            stream << closedWindowItem->title();
            stream << closedWindowItem->numTabs();
            stream << closedWindowItem->configGroup().config()->name();
            stream << closedWindowItem->configGroup().name();
            QVariant variant(data);
            windowItems.append(variant);
        }
    }

    if(!windowItems.empty())
    {
        org::kde::Konqueror::UndoManager interface(msg.service(),
            "/KonqUndoManager", QDBusConnection::sessionBus(), this);

        if(!interface.isValid())
            return;

        interface.localClosedWindowItems(windowItems, dbusService());
    }
}

void KonqClosedWindowsManager::emitPong(const QString & service)
{
    org::kde::Konqueror::UndoManager interface(service,
            "/KonqUndoManager", QDBusConnection::sessionBus(), this);

    if(!interface.isValid())
        return;

    interface.pong();
}

void KonqClosedWindowsManager::localClosedWindowItems(
    const QList<QVariant>& windowItems, const QString& service)
{

    QListIterator<QVariant> it(windowItems);
    for (it.toBack(); it.hasPrevious(); )
    {
        QString title, configFileName, configGroup;
        int numTabs;
        QByteArray data = (it.previous()).toByteArray();
        QDataStream stream( const_cast<QByteArray *>( &data ), QIODevice::ReadOnly );
        stream >> title >> numTabs >> configFileName >> configGroup;
        slotNotifyClosedWindowItem(title, numTabs, configFileName, configGroup,
            service);
    }

}

void KonqClosedWindowsManager::pong()
{
    m_amIalone = false;
}

KonqClosedRemoteWindowItem* KonqClosedWindowsManager::findClosedRemoteWindowItem(
    const QString& configFileName,
    const QString& configGroup)
{
    KonqClosedRemoteWindowItem* closedRemoteWindowItem = 0L;
    for (QList<KonqClosedWindowItem *>::const_iterator it = closedWindowItemList().begin();
        it != closedWindowItemList().end(); ++it)
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
    KonqClosedWindowItem* closedWindowItem = 0L;
    for (QList<KonqClosedWindowItem *>::const_iterator it = closedWindowItemList().begin();
        it != closedWindowItemList().end(); ++it)
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

void KonqClosedWindowsManager::populate()
{
    emit requestLocalClosedWindowItems();
}

void KonqClosedWindowsManager::removeClosedItemsConfigFiles()
{
    QString dir= KStandardDirs::locateLocal("appdata", "closeditems/");

    QDirIterator it(dir, QDir::Writable|QDir::Files);
    while (it.hasNext())
    {
        QFile::remove(it.next());
    }
}


void KonqClosedWindowsManager::saveConfig()
{
    // Create / overwrite the saved closed windows list
    QString filename = "closeditems_saved";
    QString file = KStandardDirs::locateLocal("appdata", filename);
    QFile::remove(file);

    KConfig *config = new KConfig(filename, KConfig::SimpleConfig, "appdata");

    // Populate the config file
    KonqClosedWindowItem* closedWindowItem = 0L;
    uint counter = closedWindowItemList().size()-1;
    for (QList<KonqClosedWindowItem *>::const_iterator it = closedWindowItemList().begin();
        it != closedWindowItemList().end(); ++it, --counter)
    {
        closedWindowItem = *it;
        KConfigGroup configGroup(config, "Closed_Window" + QString::number(counter));
        configGroup.writeEntry("title", closedWindowItem->title());
        configGroup.writeEntry("numTabs", closedWindowItem->numTabs());
        closedWindowItem->configGroup().copyTo(&configGroup);
    }

    KConfigGroup configGroup(config, "General");
    configGroup.writeEntry("Number of Closed Windows", closedWindowItemList().size());

    delete config;
}

void KonqClosedWindowsManager::readConfig()
{
    // We will read the configuration file *only* if we are alone, which means
    // only if we couldn't get the closed windows list via dbus because no
    // dbus instance answered/exists
    if(!m_amIalone)
        return;

    // Simplification: we assume there was no time to open and close
    // a konqueror window in 500ms of konqueror life time and thus there
    // shouldn't be any file in closeditem/* to do some houskeeping everytime
    // konqueror starts
    removeClosedItemsConfigFiles();

    QString filename = "closeditems_saved";
    QString file = KStandardDirs::locateLocal("appdata", filename);

    // If the config file doesn't exists, there's nothing to read
    if(!QFile::exists(file))
        return;

    KConfig config(filename, KConfig::SimpleConfig, "appdata");
    KConfigGroup generalGroup(&config, "General");
    int numClosedWindows = generalGroup.readEntry("Number of Closed Windows", 0);

    for(int i = 0; i < numClosedWindows; i++)
    {
        // For each item, create a new ClosedWindowItem
        KConfigGroup configGroup(&config, "Closed_Window" + QString::number(i));
        QString title = configGroup.readEntry("title", i18n("no name"));
        int numTabs = configGroup.readEntry("numTabs", 0);

        KonqClosedWindowItem* closedWindowItem = new KonqClosedWindowItem(
            title,  KIO::FileUndoManager::self()->newCommandSerialNumber(),
            numTabs);
        configGroup.copyTo(&closedWindowItem->configGroup());
        configGroup.writeEntry("foo", 0);
        closedWindowItem->configGroup().config()->sync();

        // Add the item to all the windows and propagate over dbus
        addClosedWindowItem(0L, closedWindowItem, true);
    }
}

#include "konqundomanager.moc"
