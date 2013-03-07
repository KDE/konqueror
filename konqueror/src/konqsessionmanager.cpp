/*
   This file is part of the KDE project
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

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

#include "konqsessionmanager.h"
#include "konqmisc.h"
#include "konqmainwindow.h"
#include "konqsessionmanager_interface.h"
#include "konqsessionmanageradaptor.h"
#include "konqviewmanager.h"
#include "konqsettingsxt.h"

#include <kglobal.h>
#include <kdebug.h>
#include <kio/deletejob.h>
#include <kstandarddirs.h>
#include <kvbox.h>
#include <khbox.h>
#include <klocale.h>
#include <kdialog.h>
#include <kurl.h>
#include <ktempdir.h>
#include <ksqueezedtextlabel.h>

#include <QPushButton>
#include <QCheckBox>
#include <QFileInfo>
#include <QPoint>
#include <QtDBus/QtDBus>
#include <QtAlgorithms>
#include <QDirIterator>
#include <QtCore/QDir>
#include <QDBusArgument>
#include <QFile>
#include <QSize>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QScrollArea>
#include <QScrollBar>


class KonqSessionManagerPrivate
{
public:
    KonqSessionManagerPrivate()
        : instance(0)
    {
    }

    ~KonqSessionManagerPrivate()
    {
        delete instance;
    }

    KonqSessionManager *instance;
};

K_GLOBAL_STATIC(KonqSessionManagerPrivate, myKonqSessionManagerPrivate)

static QString viewIdFor(const QString& sessionFile, const QString& viewId)
{
    return (sessionFile + viewId);
}

static const QList<KConfigGroup> windowConfigGroups(const KConfig& config)
{
    QList<KConfigGroup> groups;
    KConfigGroup generalGroup(&config, "General");
    const int size = generalGroup.readEntry("Number of Windows", 0);
    for(int i = 0; i < size; i++) {
        groups << KConfigGroup(&config, "Window" + QString::number(i));
    }
    return groups;
}

SessionRestoreDialog::SessionRestoreDialog(const QStringList& sessionFilePaths, QWidget* parent)
        : KDialog(parent, 0)
         ,m_sessionItemsCount(0)
         ,m_dontShowChecked(false)
{
    setCaption(i18nc("@title:window", "Restore Session?"));
    setButtons(KDialog::Yes | KDialog::No | KDialog::Cancel);
    setObjectName(QLatin1String("restoresession"));
    setButtonGuiItem(KDialog::Yes, KGuiItem(i18nc("@action:button yes", "Restore Session"), "window-new"));
    setButtonGuiItem(KDialog::No, KGuiItem(i18nc("@action:button no","Do Not Restore"), "dialog-close"));
    setButtonGuiItem(KDialog::Cancel, KGuiItem(i18nc("@action:button ask later","Ask Me Later"), "chronometer"));
    setDefaultButton(KDialog::Yes);
    setButtonFocus(KDialog::Yes);
    setModal(true);

    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(KDialog::spacingHint() * 2); // provide extra spacing
    mainLayout->setMargin(0);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setMargin(0);
    hLayout->setSpacing(-1); // use default spacing
    mainLayout->addLayout(hLayout,5);

    KIcon icon (QLatin1String("dialog-warning"));
    if (!icon.isNull()) {
        QLabel *iconLabel = new QLabel(mainWidget);
        QStyleOption option;
        option.initFrom(mainWidget);
        iconLabel->setPixmap(icon.pixmap(mainWidget->style()->pixelMetric(QStyle::PM_MessageBoxIconSize, &option, mainWidget)));
        QVBoxLayout *iconLayout = new QVBoxLayout();
        iconLayout->addStretch(1);
        iconLayout->addWidget(iconLabel);
        iconLayout->addStretch(5);
        hLayout->addLayout(iconLayout,0);
    }

    const QString text (i18n("Konqueror did not close correctly. Would you like to restore these previous sessions?"));
    QLabel *messageLabel = new QLabel(text, mainWidget);
    Qt::TextInteractionFlags flags = (Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    messageLabel->setTextInteractionFlags(flags);
    messageLabel->setWordWrap(true);

    hLayout->addSpacing(KDialog::spacingHint());
    hLayout->addWidget(messageLabel,5);

    QTreeWidget* treeWidget = 0;
    if (!sessionFilePaths.isEmpty()) {
        treeWidget = new QTreeWidget(mainWidget);
        treeWidget->setHeader(0);
        treeWidget->setHeaderHidden(true);
        treeWidget->setToolTip(i18nc("@tooltip:session list", "Uncheck the sessions you do not want to be restored"));

        QStyleOptionViewItem styleOption;
        styleOption.initFrom(treeWidget);
        QFontMetrics fm(styleOption.font);
        int w = treeWidget->width();
        const QRect desktop = KGlobalSettings::desktopGeometry(this);

        // Collect info from the sessions to restore
        Q_FOREACH(const QString& sessionFile, sessionFilePaths) {
            kDebug() << sessionFile;
            QTreeWidgetItem* windowItem = 0;
            const KConfig config(sessionFile, KConfig::SimpleConfig);
            const QList<KConfigGroup> groups = windowConfigGroups(config);
            Q_FOREACH(const KConfigGroup& group, groups) {
                // To avoid a recursive search, let's do linear search on Foo_CurrentHistoryItem=1
                Q_FOREACH(const QString& key, group.keyList()) {
                    if (key.endsWith(QLatin1String("_CurrentHistoryItem"))) {
                        const QString viewId = key.left(key.length() - qstrlen("_CurrentHistoryItem"));
                        const QString historyIndex = group.readEntry(key, QString());
                        const QString prefix = "HistoryItem" + viewId + '_' + historyIndex;
                        // Ignore the sidebar views
                        if (group.readEntry(prefix + "StrServiceName", QString()).startsWith("konq_sidebar"))
                            continue;
                        const QString url = group.readEntry(prefix + "Url", QString());
                        const QString title = group.readEntry(prefix + "Title", QString());
                        kDebug() << viewId << url << title;
                        const QString displayText = (title.trimmed().isEmpty() ? url : title);
                        if (!displayText.isEmpty()) {
                            if (!windowItem) {
                                 windowItem = new QTreeWidgetItem(treeWidget);
                                 const int index = sessionFilePaths.indexOf(sessionFile) + 1;
                                 windowItem->setText(0, i18nc("@item:treewidget", "Window %1", index));
                                 windowItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                                 windowItem->setCheckState(0, Qt::Checked);
                                 windowItem->setExpanded(true);
                            }
                            QTreeWidgetItem* item = new QTreeWidgetItem (windowItem);
                            item->setText(0, displayText);
                            item->setData(0, Qt::UserRole, viewIdFor(sessionFile, viewId));
                            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                            item->setCheckState(0, Qt::Checked);
                            w = qMax(w, fm.width(displayText));
                            m_sessionItemsCount++;
                        }
                    }
                }
            }

            if (windowItem) {
                m_checkedSessionItems.insert(windowItem, windowItem->childCount());
            }
        }

        const int borderWidth = treeWidget->width() - treeWidget->viewport()->width() + treeWidget->verticalScrollBar()->height();
        w += borderWidth;
        if (w > desktop.width() * 0.85) { // limit treeWidget size to 85% of screen width
            w = qRound(desktop.width() * 0.85);
        }
        treeWidget->setMinimumWidth(w);
        mainLayout->addWidget(treeWidget, 50);
        treeWidget->setSelectionMode(QTreeWidget::NoSelection);
        messageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    }

    // Do not connect the itemChanged signal until after the treewidget
    // is completely populated to prevent the firing of the itemChanged
    // signal while in the process of adding the original session items.
    if (treeWidget && treeWidget->topLevelItemCount() > 0) {
        connect(treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
                this, SLOT(slotItemChanged(QTreeWidgetItem*,int)));
    }

    QCheckBox* checkbox = new QCheckBox(i18n("Do not ask again"), mainWidget);
    connect(checkbox, SIGNAL(clicked(bool)), this, SLOT(slotClicked(bool)));
    mainLayout->addWidget(checkbox);

    setMainWidget(mainWidget);
}

SessionRestoreDialog::~SessionRestoreDialog()
{
}

QStringList SessionRestoreDialog::discardedSessionList() const
{
    return m_discardedSessionList;
}

bool SessionRestoreDialog::isDontShowChecked() const
{
    return m_dontShowChecked;
}

void SessionRestoreDialog::slotClicked(bool checked)
{
     m_dontShowChecked = checked;
}

static void setCheckState(QTreeWidgetItem* item, int column, Qt::CheckState state)
{
    const bool blocked = item->treeWidget()->blockSignals(true);
    item->setCheckState(column, state);
    item->treeWidget()->blockSignals(blocked);
}

void SessionRestoreDialog::slotItemChanged(QTreeWidgetItem* item, int column)
{
    Q_ASSERT(item);

    const int itemChildCount = item->childCount();
    QTreeWidgetItem* parentItem = 0;

    const bool blocked = item->treeWidget()->blockSignals(true);
    if (itemChildCount > 0) {
        parentItem = item;
        for (int i = 0; i < itemChildCount; ++i) {
            QTreeWidgetItem* childItem = item->child(i);
            if (childItem) {
                childItem->setCheckState(column, item->checkState(column));
                switch (childItem->checkState(column)) {
                case Qt::Checked:
                    m_sessionItemsCount++;
                    m_discardedSessionList.removeAll(childItem->data(column, Qt::UserRole).toString());
                    m_checkedSessionItems[item]++;
                    break;
                case Qt::Unchecked:
                    m_sessionItemsCount--;
                    m_discardedSessionList.append(childItem->data(column, Qt::UserRole).toString());
                    m_checkedSessionItems[item]--;
                    break;
                default:
                    break;
                }
            }
        }
    } else {
        parentItem = item->parent();
        switch (item->checkState(column)) {
        case Qt::Checked:
            m_sessionItemsCount++;
            m_discardedSessionList.removeAll(item->data(column, Qt::UserRole).toString());
            m_checkedSessionItems[parentItem]++;
            break;
        case Qt::Unchecked:
            m_sessionItemsCount--;
            m_discardedSessionList.append(item->data(column, Qt::UserRole).toString());
            m_checkedSessionItems[parentItem]--;
            break;
        default:
            break;
        }
    }

    const int numCheckSessions = m_checkedSessionItems.value(parentItem);
    switch (parentItem->checkState(column)) {
      case Qt::Checked:
          if (numCheckSessions == 0) {
              parentItem->setCheckState(column, Qt::Unchecked);
          }
      case Qt::Unchecked:
          if (numCheckSessions > 0) {
              parentItem->setCheckState(column, Qt::Checked);
          }
      default:
          break;
    }

    enableButton(KDialog::Yes, m_sessionItemsCount > 0);
    item->treeWidget()->blockSignals(blocked);
}

void SessionRestoreDialog::saveDontShow(const QString& dontShowAgainName, int result)
{
    if (dontShowAgainName.isEmpty()) {
        return;
    }

    KConfigGroup::WriteConfigFlags flags = KConfig::Persistent;
    if (dontShowAgainName[0] == ':') {
        flags |= KConfigGroup::Global;
    }

    KConfigGroup cg(KGlobal::config().data(), "Notification Messages");
    cg.writeEntry( dontShowAgainName, result==Yes, flags );
    cg.sync();
}

bool SessionRestoreDialog::shouldBeShown(const QString& dontShowAgainName, int* result)
{
    if (dontShowAgainName.isEmpty()) {
        return true;
    }

    KConfigGroup cg(KGlobal::config().data(), "Notification Messages");
    const QString dontAsk = cg.readEntry(dontShowAgainName, QString()).toLower();

    if (dontAsk == "yes" || dontAsk == "true") {
        if (result) {
            *result = Yes;
        }
        return false;
    }

    if (dontAsk == "no" || dontAsk == "false") {
        if (result) {
            *result = No;
        }
        return false;
    }

    return true;
}

KonqSessionManager::KonqSessionManager()
    : m_autosaveDir(KStandardDirs::locateLocal("appdata", "autosave"))
      , m_autosaveEnabled(false) // so that enableAutosave works
{
    // Initialize dbus interfaces
    new KonqSessionManagerAdaptor ( this );

    const QString dbusPath = "/KonqSessionManager";
    const QString dbusInterface = "org.kde.Konqueror.SessionManager";

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    m_baseService = KonqMisc::encodeFilename(dbus.baseService());
    dbus.connect(QString(), dbusPath, dbusInterface, "saveCurrentSession", this, SLOT(slotSaveCurrentSession(QString)));

    // Initialize the timer
    const int interval = KonqSettings::autoSaveInterval();
    if (interval > 0) {
        m_autoSaveTimer.setInterval(interval*1000);
        connect( &m_autoSaveTimer, SIGNAL(timeout()), this,
            SLOT(autoSaveSession()) );
    }
    enableAutosave();
}

KonqSessionManager::~KonqSessionManager()
{
}

void KonqSessionManager::disableAutosave()
{
    if(!m_autosaveEnabled)
        return;

    m_autosaveEnabled = false;
    m_autoSaveTimer.stop();
    QFile::remove(m_autoSavedSessionConfig);
}

void KonqSessionManager::enableAutosave()
{
    if(m_autosaveEnabled)
        return;

    // Create the config file for autosaving current session
    QString filename = "autosave/" + m_baseService;
    m_autoSavedSessionConfig = KStandardDirs::locateLocal("appdata", filename);
    QFile::remove(m_autoSavedSessionConfig);

    m_autosaveEnabled = true;
    m_autoSaveTimer.start();
}

void KonqSessionManager::deleteOwnedSessions()
{
    // Not dealing with the sessions about to remove anymore
    KTempDir::removeDir(dirForMyOwnedSessionFiles());
}

KonqSessionManager* KonqSessionManager::self()
{
    if(!myKonqSessionManagerPrivate->instance)
        myKonqSessionManagerPrivate->instance = new KonqSessionManager();

    return myKonqSessionManagerPrivate->instance;
}

void KonqSessionManager::autoSaveSession()
{
    if(!m_autosaveEnabled)
        return;

    const bool isActive = m_autoSaveTimer.isActive();
    if(isActive)
        m_autoSaveTimer.stop();

    saveCurrentSessionToFile(m_autoSavedSessionConfig);

    // Now that we have saved current session it's safe to remove our owned_by
    // directory
    deleteOwnedSessions();

    if(isActive)
        m_autoSaveTimer.start();
}

void KonqSessionManager::saveCurrentSessions(const QString & path)
{
    emit saveCurrentSession(path);
}

void KonqSessionManager::slotSaveCurrentSession(const QString & path)
{
    const QString filename = path + '/' + m_baseService;
    saveCurrentSessionToFile(filename);
}

void KonqSessionManager::saveCurrentSessionToFile(const QString& sessionConfigPath)
{
    QFile::remove(sessionConfigPath);
    KConfig sessionConfig(sessionConfigPath, KConfig::SimpleConfig, "appdata");

    QList<KonqMainWindow*> *mainWindows = KonqMainWindow::mainWindowList();
    unsigned int counter = 0;

    if(!mainWindows || mainWindows->isEmpty())
        return;

    foreach ( KonqMainWindow* window, *mainWindows )
    {
        KConfigGroup configGroup(&sessionConfig, "Window" +
            QString::number(counter));
        window->saveProperties(configGroup);
        counter++;
    }
    KConfigGroup configGroup(&sessionConfig, "General");
    configGroup.writeEntry("Number of Windows", counter);
}

QString KonqSessionManager::autosaveDirectory() const
{
    return m_autosaveDir;
}

QStringList KonqSessionManager::takeSessionsOwnership()
{
    // Tell to other konqueror instances that we are the one dealing with
    // these sessions
    QDir dir(dirForMyOwnedSessionFiles());
    QDir parentDir(m_autosaveDir);

    if(!dir.exists())
        parentDir.mkdir("owned_by" + m_baseService);

    QDirIterator it(m_autosaveDir, QDir::Writable|QDir::Files|QDir::Dirs|
        QDir::NoDotAndDotDot);

    QStringList sessionFilePaths;
    QDBusConnectionInterface *idbus = QDBusConnection::sessionBus().interface();

    while (it.hasNext())
    {
        it.next();
        // this is the case where another konq started to restore that session,
        // but crashed immediately. So we try to restore that session again
        if(it.fileInfo().isDir())
        {
            // The remove() removes the "owned_by" part
            if(!idbus->isServiceRegistered(
                KonqMisc::decodeFilename(it.fileName().remove(0, 8))))
            {
                QDirIterator it2(it.filePath(), QDir::Writable|QDir::Files);
                while (it2.hasNext())
                {
                    it2.next();
                    // take ownership of the abandoned file
                    const QString newFileName = dirForMyOwnedSessionFiles() +
                                                '/' + it2.fileName();
                    QFile::rename(it2.filePath(), newFileName);
                    sessionFilePaths.append(newFileName);
                }
                // Remove the old directory
                KTempDir::removeDir(it.filePath());
            }
        } else { // it's a file
            if(!idbus->isServiceRegistered(KonqMisc::decodeFilename(it.fileName())))
            {
                // and it's abandoned: take its ownership
                const QString newFileName = dirForMyOwnedSessionFiles() + '/' +
                                            it.fileName();
                QFile::rename(it.filePath(), newFileName);
                sessionFilePaths.append(newFileName);
            }
        }
    }

    return sessionFilePaths;
}

void KonqSessionManager::restoreSessions(const QStringList &sessionFilePathsList,
    bool openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    foreach ( const QString& sessionFilePath, sessionFilePathsList )
    {
        restoreSession(sessionFilePath, openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSessions(const QString &sessionsDir, bool
    openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    QDirIterator it(sessionsDir, QDir::Readable|QDir::Files);

    while (it.hasNext())
    {
        QFileInfo fi(it.next());
        restoreSession(fi.filePath(), openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSession(const QString &sessionFilePath, bool
    openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    if (!QFile::exists(sessionFilePath))
        return;

    const KConfig config(sessionFilePath, KConfig::SimpleConfig);
    const QList<KConfigGroup> groups = windowConfigGroups(config);
    Q_FOREACH(const KConfigGroup& configGroup, groups) {
        if(!openTabsInsideCurrentWindow)
            KonqViewManager::openSavedWindow(configGroup)->show();
        else
            parent->viewManager()->openSavedWindow(configGroup, true);
    }
}

static void removeDiscardedSessions(const QStringList& sessionFiles, const QStringList& discardedSessions)
{
    if (discardedSessions.isEmpty()) {
        return;
    }

    Q_FOREACH(const QString& sessionFile, sessionFiles) {
        const KConfig config(sessionFile, KConfig::SimpleConfig);
        QList<KConfigGroup> groups = windowConfigGroups(config);
        for (int i = 0, count = groups.count(); i < count; ++i) {
            KConfigGroup& group = groups[i];
            const QString rootItem = group.readEntry("RootItem", "empty");
            const QString viewsKey (rootItem + QLatin1String("_Children"));
            QStringList views = group.readEntry(viewsKey, QStringList());
            QMutableStringListIterator it (views);
            while (it.hasNext()) {
                if (discardedSessions.contains(viewIdFor(sessionFile, it.next()))) {
                    it.remove();
                }
            }
            group.writeEntry(viewsKey, views);
        }
    }
}

bool KonqSessionManager::askUserToRestoreAutosavedAbandonedSessions()
{
    const QStringList sessionFilePaths = takeSessionsOwnership();
    if(sessionFilePaths.isEmpty())
        return false;

    disableAutosave();

    int result;
    QStringList discardedSessionList;
    const QLatin1String dontAskAgainName ("Restore session when konqueror didn't close correctly");

    if (SessionRestoreDialog::shouldBeShown(dontAskAgainName, &result)) {
        SessionRestoreDialog* restoreDlg = new SessionRestoreDialog(sessionFilePaths);
        result = restoreDlg->exec();
        discardedSessionList = restoreDlg->discardedSessionList();
        if (restoreDlg->isDontShowChecked()) {
            SessionRestoreDialog::saveDontShow(dontAskAgainName, result);
        }
        delete restoreDlg;
    }

    switch (result) {
        case KDialog::Yes:
            // Remove the discarded session list files.
            removeDiscardedSessions(sessionFilePaths, discardedSessionList);
            restoreSessions(sessionFilePaths);
            enableAutosave();
            return true;
        case KDialog::No:
            deleteOwnedSessions();
            enableAutosave();
            return false;
        default:
            // Remove the ownership of the currently owned files
            QDirIterator it(dirForMyOwnedSessionFiles(),
                QDir::Writable|QDir::Files);

            while (it.hasNext()) {
                it.next();
                // remove ownership of the abandoned file
                QFile::rename(it.filePath(), m_autosaveDir + '/' + it.fileName());
            }
            // Remove the owned_by directory
            KTempDir::removeDir(dirForMyOwnedSessionFiles());
            enableAutosave();
            return false;
    }
}

#include "konqsessionmanager.moc"
