/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2008 Eduardo Robles Elvira <edulix@gmail.com>
    SPDX-FileCopyrightText: 2025 Raphael Rosch <kde-dev@insaner.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqsessionmanager.h"
#include "konqmisc.h"
#include "konqmainwindow.h"
#include "konqsessionmanager_interface.h"
#include "konqsessionmanageradaptor.h"
#include "konqviewmanager.h"
#include "konqsettings.h"

#ifdef KActivities_FOUND
#include "activitymanager.h"
#include <PlasmaActivities/Consumer>
#endif

#include "konqdebug.h"
#include <kio/deletejob.h>
#include <KLocalizedString>
#include <KWindowInfo>
#include <KX11Extras>

#include <QUrl>
#include <QIcon>
#include <ksqueezedtextlabel.h>

#include <QPushButton>
#include <QCheckBox>
#include <QFileInfo>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QtAlgorithms>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QSize>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QScrollBar>
#include <QApplication>
#include <QStandardPaths>
#include <QSessionManager>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <KGuiItem>
#include <QScreen>
#include <KSharedConfig>

#include <QClipboard>
#include <QMenu>

#include "konqapplication.h"
#include <QMessageBox>

class KonqSessionManagerPrivate
{
public:
    KonqSessionManagerPrivate()
        : instance(nullptr)
    {
    }

    ~KonqSessionManagerPrivate()
    {
        delete instance;
    }

    KonqSessionManager *instance;
};

Q_GLOBAL_STATIC(KonqSessionManagerPrivate, myKonqSessionManagerPrivate)

static QString windowIdFor(const QString &sessionFile, const QString &windowId)
{
    return (sessionFile + windowId);
}
static QString viewIdFor(const QString &sessionFile, const QString &windowId, const QString &viewId)
{
    return (sessionFile + windowId + viewId);
}

static const QList<KConfigGroup> windowConfigGroups(/*NOT const, we'll use writeEntry*/ KConfig &config)
{
    QList<KConfigGroup> groups;
    KConfigGroup generalGroup(&config, "General");
    const int size = generalGroup.readEntry("Number of Windows", 0);
    for (int i = 0; i < size; i++) {
        groups << KConfigGroup(&config, "Window" + QString::number(i));
    }
    return groups;
}

SessionRestoreDialog::SessionRestoreDialog(const QStringList &sessionFilePaths, QWidget *parent)
    : QDialog(parent)
    , m_sessionItemsCount(0)
    , m_dontShowChecked(false)
{
    setObjectName(QStringLiteral("restoresession"));
    setWindowTitle(i18nc("@title:window", "Restore Session?"));
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(hLayout, 5);

    QIcon icon = QIcon::fromTheme(QLatin1String("dialog-warning"));
    if (!icon.isNull()) {
        QLabel *iconLabel = new QLabel(this);
        iconLabel->setPixmap(icon.pixmap(style()->pixelMetric(QStyle::PM_MessageBoxIconSize)));
        QVBoxLayout *iconLayout = new QVBoxLayout();
        iconLayout->addStretch(1);
        iconLayout->addWidget(iconLabel);
        iconLayout->addStretch(5);
        hLayout->addLayout(iconLayout, 0);
        hLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing));
    }

    const QString text(i18n("Konqueror did not close correctly. Would you like to restore these previous sessions?"));
    QLabel *messageLabel = new QLabel(text, this);
    Qt::TextInteractionFlags flags = (Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    messageLabel->setTextInteractionFlags(flags);
    messageLabel->setWordWrap(true);

    hLayout->addWidget(messageLabel, 5);

    Q_ASSERT(!sessionFilePaths.isEmpty());
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeader(nullptr);
    m_treeWidget->setHeaderHidden(true);
    
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);  // enable right-click
        // handle right-click context menu
    QObject::connect(m_treeWidget, &QTreeWidget::customContextMenuRequested, this, &SessionRestoreDialog::showContextMenu);

    QStyleOptionViewItem styleOption;
    styleOption.initFrom(m_treeWidget);
    QFontMetrics fm(styleOption.font);
    int w = m_treeWidget->width();
    const QRect desktop = screen()->geometry();
    const QString toolTipForSessionList = i18nc("@tooltip:session list", "Uncheck the sessions or windows you do not want to be restored");
    // Collect info from the sessions to restore
    for (const QString &sessionFile: sessionFilePaths) {
        QFileInfo fileInfo(sessionFile);
        QString sessionName = fileInfo.fileName();
        qCDebug(KONQUEROR_LOG) << sessionFile;
        QRegularExpression trailingDigitsRE(R"(\d+$)");
        QTreeWidgetItem *sessionItem = new QTreeWidgetItem(m_treeWidget);
        sessionItem->setText(0, i18nc("@item:treewidget", "Session %1", sessionName));
        sessionItem->setToolTip(0, toolTipForSessionList);
        sessionItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        sessionItem->setCheckState(0, Qt::Checked);
        sessionItem->setExpanded(true);
        
        KConfig config(sessionFile, KConfig::SimpleConfig);
        const QList<KConfigGroup> windowGroups = windowConfigGroups(config);
        for (const KConfigGroup &windowGroup: windowGroups) {
            QTreeWidgetItem *windowItem = nullptr;
            const QString windowId = windowGroup.name();
            // To avoid a recursive search, let's do linear search on Foo_CurrentHistoryItem=1
            for (const QString &key: windowGroup.keyList()) {
                if (key.endsWith(QLatin1String("_CurrentHistoryItem"))) {
                    const QString viewId = key.left(key.length() - qstrlen("_CurrentHistoryItem"));
                    const QString historyIndex = windowGroup.readEntry(key, QString());
                    const QString prefix = "HistoryItem" + viewId + '_' + historyIndex;
                    // Ignore the sidebar views
                    if (windowGroup.readEntry(prefix + "StrServiceName", QString()).startsWith(QLatin1String("konq_sidebar"))) {
                        continue;
                    }
                    const QString url = windowGroup.readEntry(prefix + "Url", QString());
                    const QString title = windowGroup.readEntry(prefix + "Title", QString());
                    qCDebug(KONQUEROR_LOG) << viewId << url << title;
                    const QString displayText = (title.trimmed().isEmpty() ? url : title);
                    if (!displayText.isEmpty()) {
                        if (!windowItem) {
                            windowItem = new QTreeWidgetItem(sessionItem);
                            QRegularExpressionMatch trailingDigitsMatch = trailingDigitsRE.match(windowId);
                            // I assume this is done this way for the benefit of i18n. Otherwise it would be easier to just use what is in the session file
                            windowItem->setText(0, i18nc("@item:treewidget", "Window %1", trailingDigitsMatch.captured(0).toInt())); // FIXME: what if for some reason this is broken and there is no match?
                            windowItem->setToolTip(0, toolTipForSessionList);
                            windowItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                            windowItem->setData(0, ViewIdRole, windowIdFor(sessionFile, windowId));
                            windowItem->setCheckState(0, Qt::Checked);
                            windowItem->setExpanded(true);
                            m_sessionItemsCount++;
                        }
                        QTreeWidgetItem *item = new QTreeWidgetItem(windowItem);
                        item->setText(0, displayText);
                        item->setToolTip(0, url);
                        item->setData(0, ViewIdRole, viewIdFor(sessionFile, windowId, viewId));
                        item->setData(0, UrlRole, url);    // "hidden" data to be pulled by the context menu handler
                        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
                        // item->setIcon(0, QIcon::fromTheme("view-restore")); // could also be replaced with just a bullet point, or nothing
                        w = qMax(w, fm.horizontalAdvance(displayText));
                    }
                }
            }
            if (windowItem) {
                m_checkedSessionItems.insert(sessionItem, sessionItem->childCount());
            }
        }
    }

    const int borderWidth = m_treeWidget->width() - m_treeWidget->viewport()->width() + m_treeWidget->verticalScrollBar()->height();
    w += borderWidth;
    if (w > desktop.width() * 0.85) { // limit treeWidget size to 85% of screen width
        w = qRound(desktop.width() * 0.85);
    }
    m_treeWidget->setMinimumWidth(w);
    mainLayout->addWidget(m_treeWidget, 50);
    m_treeWidget->setSelectionMode(QTreeWidget::NoSelection);
    messageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Do not connect the itemChanged signal until after the treewidget
    // is completely populated to prevent the firing of the itemChanged
    // signal while in the process of adding the original session items.
    connect(m_treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(slotItemChanged(QTreeWidgetItem*,int)));

    QCheckBox *checkbox = new QCheckBox(i18n("Do not ask again"), this);
    connect(checkbox, &QCheckBox::clicked, this, &SessionRestoreDialog::slotClicked);
    mainLayout->addWidget(checkbox);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::No|QDialogButtonBox::Yes);
    mainLayout->addWidget(m_buttonBox);
    QPushButton *yesButton = m_buttonBox->button(QDialogButtonBox::Yes);
    QPushButton *noButton = m_buttonBox->button(QDialogButtonBox::No);
    QPushButton *cancelButton = m_buttonBox->button(QDialogButtonBox::Cancel);

    connect(yesButton, &QPushButton::clicked, this, [this]() { done(QDialogButtonBox::Yes); });
    connect(noButton, &QPushButton::clicked, this, [this]() { done(QDialogButtonBox::No); });
    connect(cancelButton, &QPushButton::clicked, this, [this]() { reject(); });

    KGuiItem::assign(yesButton, KGuiItem(i18nc("@action:button yes", "Restore Session"), QStringLiteral("window-new")));
    KGuiItem::assign(noButton, KGuiItem(i18nc("@action:button no", "Do Not Restore"), QStringLiteral("dialog-close")));
    KGuiItem::assign(cancelButton, KGuiItem(i18nc("@action:button ask later", "Ask Me Later"), QStringLiteral("chronometer")));

    yesButton->setDefault(true);
    yesButton->setFocus();
}

SessionRestoreDialog::~SessionRestoreDialog()
{
}

bool SessionRestoreDialog::isEmpty() const
{
    return m_treeWidget->topLevelItemCount() == 0;
}

QStringList SessionRestoreDialog::discardedWindowList() const
{
    return m_discardedWindowList;
}

bool SessionRestoreDialog::isDontShowChecked() const
{
    return m_dontShowChecked;
}

void SessionRestoreDialog::slotClicked(bool checked)
{
    m_dontShowChecked = checked;
}

void SessionRestoreDialog::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item) return;

    QString copyThis = item->data(0, UrlRole).toString();

    // Only present the tooltip if it's a view with a URL
    if (!copyThis.isEmpty()) {
        QMenu context_menu;
        int screenWidth = m_treeWidget->screen()->geometry().width();
        QFontMetrics metrics(m_treeWidget->font());
        QString clipboardPrompt = i18nc("@tooltip:copy url to clipboard", "Copy url to clipboard:  %1", copyThis);
        QString clipboardPromptFit = metrics.elidedText(clipboardPrompt, Qt::ElideMiddle, screenWidth);

        QAction *copyAction = context_menu.addAction(clipboardPromptFit);
        connect(copyAction, &QAction::triggered, this, [copyThis]() {
            QApplication::clipboard()->setText(copyThis);
        });

        context_menu.exec(m_treeWidget->mapToGlobal(pos));
    }
}

void SessionRestoreDialog::slotItemChanged(QTreeWidgetItem *item, int column)
{
    Q_ASSERT(item);
    const int itemChildCount = item->childCount();
    QTreeWidgetItem *parentItem = item;

    const bool blocked = item->treeWidget()->blockSignals(true);

    const int maxDepthToDisplay = 1; // 0 = session, 1 = window, 2 = views (not containers)
    int depth = 0;
    for (QTreeWidgetItem *tmpItem = item; tmpItem->parent();  tmpItem = tmpItem->parent()) {
        depth++;
    }

    if (depth < maxDepthToDisplay && itemChildCount > 0) {	// toggle child items
        for (int i = 0; i < itemChildCount; ++i) {
            QTreeWidgetItem *childItem = item->child(i);
            if (childItem && childItem->checkState(column) != item->checkState(column)) {
                childItem->setCheckState(column, item->checkState(column));
                switch (childItem->checkState(column)) {
                case Qt::Checked:
                    m_sessionItemsCount++;
                    m_discardedWindowList.removeAll(childItem->data(column, ViewIdRole).toString());
                    m_checkedSessionItems[item]++;
                    break;
                case Qt::Unchecked:
                    m_sessionItemsCount--;
                    m_discardedWindowList.append(childItem->data(column, ViewIdRole).toString());
                    m_checkedSessionItems[item]--;
                    break;
                default:
                    break;
                }
            }
        }
    } 
    if (depth > 0) {	// toggle parent item
        parentItem = item->parent();
        switch (item->checkState(column)) {
        case Qt::Checked:
            m_sessionItemsCount++;
            m_discardedWindowList.removeAll(item->data(column, ViewIdRole).toString());
            m_checkedSessionItems[parentItem]++;
            break;
        case Qt::Unchecked:
            m_sessionItemsCount--;
            m_discardedWindowList.append(item->data(column, ViewIdRole).toString());
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
        break;
    case Qt::Unchecked:
        if (numCheckSessions > 0) {
            parentItem->setCheckState(column, Qt::Checked);
        }
    default:
        break;
    }

    m_buttonBox->button(QDialogButtonBox::Yes)->setEnabled(m_sessionItemsCount>0);
    item->treeWidget()->blockSignals(blocked);
}

void SessionRestoreDialog::saveDontShow(const QString &dontShowAgainName, int result)
{
    if (dontShowAgainName.isEmpty()) {
        return;
    }

    KConfigGroup::WriteConfigFlags flags = KConfig::Persistent;
    if (dontShowAgainName[0] == ':') {
        flags |= KConfigGroup::Global;
    }

    KConfigGroup cg(KSharedConfig::openConfig().data(), "Notification Messages");
    cg.writeEntry(dontShowAgainName, result == QDialogButtonBox::Yes, flags);
    cg.sync();
}

bool SessionRestoreDialog::shouldBeShown(const QString &dontShowAgainName, int *result)
{
    if (dontShowAgainName.isEmpty()) {
        return true;
    }

    KConfigGroup cg(KSharedConfig::openConfig().data(), "Notification Messages");
    const QString dontAsk = cg.readEntry(dontShowAgainName, QString()).toLower();

    if (dontAsk == QLatin1String("yes") || dontAsk == QLatin1String("true")) {
        if (result) {
            *result = QDialogButtonBox::Yes;
        }
        return false;
    }

    if (dontAsk == QLatin1String("no") || dontAsk == QLatin1String("false")) {
        if (result) {
            *result = QDialogButtonBox::No;
        }
        return false;
    }

    return true;
}

KonqSessionManager::KonqSessionManager()
    : m_autosaveDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + "autosave")
    , m_autosaveEnabled(false) // so that enableAutosave works
    , m_createdOwnedByDir(false)
    , m_sessionConfig(nullptr)
#ifdef KActivities_FOUND
    , m_activityManager(new ActivityManager(this))
#endif
{
    // Initialize dbus interfaces
    new KonqSessionManagerAdaptor(this);

    const QString dbusPath = QStringLiteral("/KonqSessionManager");
    const QString dbusInterface = QStringLiteral("org.kde.Konqueror.SessionManager");

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(dbusPath, this);
    m_baseService = KonqMisc::encodeFilename(dbus.baseService());
    dbus.connect(QString(), dbusPath, dbusInterface, QStringLiteral("saveCurrentSession"), this, SLOT(slotSaveCurrentSession(QString)));

    // Initialize the timer
    const int interval = Konq::Settings::autoSaveInterval();
    if (interval > 0) {
        m_autoSaveTimer.setInterval(interval * 1000);
        connect(&m_autoSaveTimer, &QTimer::timeout, this, &KonqSessionManager::autoSaveSession);
    }
    enableAutosave();

    connect(qApp, &QGuiApplication::commitDataRequest, this, &KonqSessionManager::slotCommitData);
    connect(qApp, &QGuiApplication::lastWindowClosed, this, &KonqSessionManager::saveSessionAtExit);
}

KonqSessionManager::~KonqSessionManager()
{
    if (m_sessionConfig) {
        QFile::remove(m_sessionConfig->name());
    }
    delete m_sessionConfig;
}

void KonqSessionManager::restoreSessionSavedAtLogout()
{
    askUserToRestoreAutosavedAbandonedSessions();

    m_preloadedWindowsNumber.clear();
    int n = 1;
    while (KonqMainWindow::canBeRestored(n)) {
        const QString className = KXmlGuiWindow::classNameOfToplevel(n);

        //The !m_preloadedWindowsNumber.contains(n) check avoid restoring preloaded windows
        if (className == QLatin1String("KonqMainWindow") && !m_preloadedWindowsNumber.contains(n)) {
            KonqMainWindow * mw = new KonqMainWindow();
            mw->restore(n);

            //m_preloadedWindowsNumber is set from the readGlobalProperties of the first (n==1) window.
            //This means that the first window is always restored, even if it was preloaded (because readGlobalProperties
            //is called from restore). For the first window, we need to check whether it was preloaded, and in that case
            //delete it afterwards
            if (n == 1 && m_preloadedWindowsNumber.contains(1)) {
                mw->deleteLater();
            }
        } else  {
            qCWarning(KONQUEROR_LOG) << "Unknown class" << className << "in session saved data!";
        }
        ++n;
    }
    m_preloadedWindowsNumber.clear();
}


// Don't restore preloaded konquerors
void KonqSessionManager::slotCommitData(QSessionManager &sm)
{
    QList<KonqMainWindow*> const *windows = KonqMainWindow::mainWindowList();
    if (std::all_of(windows->constBegin(), windows->constEnd(), [](KonqMainWindow *w){return w->isPreloaded();})) {
        sm.setRestartHint(QSessionManager::RestartNever);
    }
}

void KonqSessionManager::disableAutosave()
{
    if (!m_autosaveEnabled) {
        return;
    }

    m_autosaveEnabled = false;
    m_autoSaveTimer.stop();
    if (m_sessionConfig) {
        QFile::remove(m_sessionConfig->name());
        delete m_sessionConfig;
        m_sessionConfig = nullptr;
    }
}

void KonqSessionManager::enableAutosave()
{
    if (m_autosaveEnabled) {
        return;
    }

    // Create the config file for autosaving current session
    QString filename = QLatin1String("autosave/") + m_baseService;
    const QString filePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1Char('/') + filename;

    delete m_sessionConfig;
    m_sessionConfig = new KConfig(filePath, KConfig::SimpleConfig);
    //qCDebug(KONQUEROR_LOG) << "config filename:" << m_sessionConfig->name();

    m_autosaveEnabled = true;
    m_autoSaveTimer.start();
}

void KonqSessionManager::deleteOwnedSessions()
{
    // Not dealing with the sessions about to remove anymore
    if (m_createdOwnedByDir && QDir(dirForMyOwnedSessionFiles()).removeRecursively()) {
        m_createdOwnedByDir = false;
    }
}

KonqSessionManager *KonqSessionManager::self()
{
    if (!myKonqSessionManagerPrivate->instance) {
        myKonqSessionManagerPrivate->instance = new KonqSessionManager();
    }

    return myKonqSessionManagerPrivate->instance;
}

void KonqSessionManager::autoSaveSession()
{
    // FIXME: Should really be checking if the session has changed before saving, to prevent needless I/O activity
    if (!m_autosaveEnabled) {
        return;
    }

    const bool isActive = m_autoSaveTimer.isActive();
    if (isActive) {
        m_autoSaveTimer.stop();
    }

    saveCurrentSessionToFile(m_sessionConfig);
    m_sessionConfig->sync();
    m_sessionConfig->markAsClean();

    // Now that we have saved current session it's safe to remove our owned_by
    // directory
    deleteOwnedSessions();

    if (isActive) {
        m_autoSaveTimer.start();
    }
}

void KonqSessionManager::saveCurrentSessions(const QString &path)
{
    emit saveCurrentSession(path);
}

void KonqSessionManager::slotSaveCurrentSession(const QString &path)
{
    const QString filename = path + '/' + m_baseService;
    saveCurrentSessionToFile(filename);
}

void KonqSessionManager::saveCurrentSessionToFile(const QString &sessionConfigPath, KonqMainWindow *mainWindow)
{
    QFile::remove(sessionConfigPath);
    KConfig config(sessionConfigPath, KConfig::SimpleConfig);

    QList<KonqMainWindow *> mainWindows;
    if (mainWindow) {
        mainWindows << mainWindow;
    }
    saveCurrentSessionToFile(&config, mainWindows);
}

void KonqSessionManager::saveCurrentSessionToFile(KConfig *config, const QList<KonqMainWindow *> &theMainWindows)
{
    QList<KonqMainWindow *> mainWindows = theMainWindows;

    if (mainWindows.isEmpty() && KonqMainWindow::mainWindowList()) {
        mainWindows = *KonqMainWindow::mainWindowList();
    }

    unsigned int counter = 0;

    if (mainWindows.isEmpty()) {
        return;
    }

    for (KonqMainWindow *window: mainWindows) {
        if (!window->isPreloaded()) {
            KConfigGroup configGroup(config, "Window" + QString::number(counter));
            window->saveProperties(configGroup);
            KWindowInfo info(window->winId(), NET::Properties(), NET::WM2Activities);
            configGroup.writeEntry("Activities", info.activities());
            counter++;
        }
    }

    KConfigGroup configGroup(config, "General");
    configGroup.writeEntry("Number of Windows", counter);
}

void KonqSessionManager::saveSessionAtExit()
{
    if (qApp->isSavingSession()) {
        return;
    }
    bool saveSessions = Konq::Settings::restoreLastState();
    if (!saveSessions) {
        return;
    }
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.isEmpty()) {
        const QList<KonqMainWindow*> windows = KonqMainWindow::mainWindows();
        if (windows.count() == 1 && windows.at(0)->isPreloaded())  {
            return;
        }
        saveCurrentSessionToFile(QDir(path).absoluteFilePath("last_session"));
    }
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

    if (!dir.exists()) {
        m_createdOwnedByDir = parentDir.mkdir("owned_by" + m_baseService);
    }

    QDirIterator it(m_autosaveDir, QDir::Writable | QDir::Files | QDir::Dirs |
                    QDir::NoDotAndDotDot);

    QStringList sessionFilePaths;
    QDBusConnectionInterface *idbus = QDBusConnection::sessionBus().interface();

    while (it.hasNext()) {
        it.next();
        // this is the case where another konq started to restore that session,
        // but crashed immediately. So we try to restore that session again
        if (it.fileInfo().isDir()) {
            // The remove() removes the "owned_by" part
            if (!idbus->isServiceRegistered(
                        KonqMisc::decodeFilename(it.fileName().remove(0, 8)))) {
                QDirIterator it2(it.filePath(), QDir::Writable | QDir::Files);
                while (it2.hasNext()) {
                    it2.next();
                    // take ownership of the abandoned file
                    const QString newFileName = dirForMyOwnedSessionFiles() +
                                                '/' + it2.fileName();
                    QFile::rename(it2.filePath(), newFileName);
                    sessionFilePaths.append(newFileName);
                }
                // Remove the old directory
                QDir(it.filePath()).removeRecursively();
            }
        } else { // it's a file
            if (!idbus->isServiceRegistered(KonqMisc::decodeFilename(it.fileName()))) {
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
    for (const QString &sessionFilePath: sessionFilePathsList) {
        restoreSession(sessionFilePath, openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSessions(const QString &sessionsDir, bool
        openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    QDirIterator it(sessionsDir, QDir::Readable | QDir::Files);

    while (it.hasNext()) {
        QFileInfo fi(it.next());
        restoreSession(fi.filePath(), openTabsInsideCurrentWindow, parent);
    }
}

void KonqSessionManager::restoreSession(const QString &sessionFilePath, bool
                                        openTabsInsideCurrentWindow, KonqMainWindow *parent)
{
    if (!QFile::exists(sessionFilePath)) {
        return;
    }

    KConfig config(sessionFilePath, KConfig::SimpleConfig);
    const QList<KConfigGroup> groups = windowConfigGroups(config);
    for (const KConfigGroup &configGroup: groups) {
        if (!configGroup.exists()) { // because if the session file has the wrong number of windows, it would launch empty windows for the missing number of windows
            // qWarning() << "restoreSession cleaning empty window group"; // should a warning be issued?
            continue;
        }
        if (!openTabsInsideCurrentWindow) {
            KonqViewManager::openSavedWindow(configGroup)->show();
        } else {
            parent->viewManager()->openSavedWindow(configGroup, true);
        }
    }
}

bool KonqSessionManager::restoreSessionSavedAtExit()
{
    if (!Konq::Settings::restoreLastState()) {
        return false;
    }

    QString lastSessionPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, QStringLiteral("last_session"));
    if (lastSessionPath.isEmpty()) {
        return false;
    }

    restoreSession(lastSessionPath);
    QFile(lastSessionPath).remove();

    return !KonqMainWindow::mainWindows().isEmpty();
}

static void removeDiscardedWindows(const QStringList &sessionFiles, const QStringList &discardedWindows)
{
    if (discardedWindows.isEmpty()) {
        return;
    }

    for (const QString &sessionFile: sessionFiles) {
        KConfig config(sessionFile, KConfig::SimpleConfig);
        QList<KConfigGroup> groups = windowConfigGroups(config);
        for (int i = 0, count = groups.count(); i < count; ++i) {
            KConfigGroup &group = groups[i];
            const QString windowName = group.name();
            const QString windowId = windowIdFor(sessionFile, windowName);
            if (discardedWindows.contains(windowId)) {
                group.deleteGroup();
            }
        }
    }
}

bool KonqSessionManager::askUserToRestoreAutosavedAbandonedSessions()
{
    const QStringList sessionFilePaths = takeSessionsOwnership();
    if (sessionFilePaths.isEmpty()) {
        return false;
    }

    disableAutosave();

    int result;
    QStringList discardedWindowList;
    const QLatin1String dontAskAgainName("Restore session when konqueror didn't close correctly");

    if (SessionRestoreDialog::shouldBeShown(dontAskAgainName, &result)) {
        SessionRestoreDialog *restoreDlg = new SessionRestoreDialog(sessionFilePaths);
        if (restoreDlg->isEmpty()) {
            result = QDialogButtonBox::No;
        } else {
            result = restoreDlg->exec();
            discardedWindowList = restoreDlg->discardedWindowList();
            if (restoreDlg->isDontShowChecked()) {
                SessionRestoreDialog::saveDontShow(dontAskAgainName, result);
            }
        }
        delete restoreDlg;
    }

    switch (result) {
    case QDialogButtonBox::Yes:
        // Remove the discarded session list files.
        removeDiscardedWindows(sessionFilePaths, discardedWindowList);
        restoreSessions(sessionFilePaths);
        enableAutosave();
        return true;
    case QDialogButtonBox::No:
        deleteOwnedSessions();
        enableAutosave();
        return false;
    default:
        // Remove the ownership of the currently owned files
        QDirIterator it(dirForMyOwnedSessionFiles(),
                        QDir::Writable | QDir::Files);

        while (it.hasNext()) {
            it.next();
            // remove ownership of the abandoned file
            QFile::rename(it.filePath(), m_autosaveDir + '/' + it.fileName());
        }
        // Remove the owned_by directory
        QDir(dirForMyOwnedSessionFiles()).removeRecursively();
        enableAutosave();
        return false;
    }
}

void KonqSessionManager::setPreloadedWindowsNumber(const QList<int> &numbers)
{
    m_preloadedWindowsNumber = numbers;
}

void KonqSessionManager::registerMainWindow(KonqMainWindow* window)
{
#ifdef KActivities_FOUND
    m_activityManager->registerMainWindow(window);
#else
    Q_UNUSED(window);
#endif
}

#ifdef KActivities_FOUND
ActivityManager * KonqSessionManager::activityManager()
{
    return m_activityManager;
}
#endif
