/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "activitymanager.h"

#include "konqviewmanager.h"
#include "konqmainwindow.h"
#include "konqapplication.h"

#include <QStandardPaths>

#if QT_VERSION_MAJOR < 6
#include <KActivities/Consumer>
#else //QT_VERSION_MAJOR
#include <PlasmaActivities/Consumer>
#endif //QT_VERSION_MAJOR

#include <KX11Extras>
#include <KWindowInfo>
#include <KSharedConfig>
#include <KConfigGroup>

#include <QTimer>

ActivityManager::ActivityManager(QObject* parent) : QObject(parent), m_activitiesConsumer(new KActivities::Consumer(this))
{
    connect(m_activitiesConsumer, &KActivities::Consumer::runningActivitiesChanged, this, &ActivityManager::handleRunningActivitiesChange);
    connect(m_activitiesConsumer, &KActivities::Consumer::activityRemoved, this, &ActivityManager::removeActivityState);
    connect(KX11Extras::self(), &KX11Extras::windowChanged, this, &ActivityManager::handleWindowChanged);
}

ActivityManager::~ActivityManager()
{
}

QString ActivityManager::activitiesConfigPath()
{
    static QString s_actitivitiesConfigPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QLatin1String("/activitiesrc");
    return s_actitivitiesConfigPath;
}

QString ActivityManager::activitiesGroupName()
{
    static QString s_activitiesGroupName = QLatin1String("Activities");
    return s_activitiesGroupName;
}

void ActivityManager::closeWindowBecauseNotInRunningActivities(KonqMainWindow* window)
{
    disconnect(window, &KonqMainWindow::closing, this, &ActivityManager::removeWindowFromActivities);

    //If this is the last window, don't close it, because it would interfere with activities management.
    //Closing the last window would close the whole application, so the global activity manager wouldn't
    //restart Konqueror when switching to the activity the window was moved to.
    //TODO activities: check what happens with preloaded windows
    QList<KonqMainWindow*>* allWindows = KonqMainWindow::mainWindowList();
    if (allWindows && allWindows->length() > 1) {
        window->close();
    }
}

void ActivityManager::handleRunningActivitiesChange(const QStringList& runningActivities)
{
    QList<KonqMainWindow*> *windows = KonqMainWindow::mainWindowList();
    if (!windows) {
        return;
    }

    auto inRunningActivities = [runningActivities](const QStringList &activities) {
        //If activities is empty, it means that the window should be shown in all activities
        return activities.isEmpty() ||
            std::any_of(activities.constBegin(), activities.constEnd(), [runningActivities](const QString &a){return runningActivities.contains(a);});
    };

    QHash<KonqMainWindow*, QStringList> toClose;
    for (KonqMainWindow *w : *windows) {
        KWindowInfo info(w->winId(), NET::Properties(), NET::WM2Activities);
        QStringList activities = info.activities();
        if (!inRunningActivities(activities)) {
            if (!w->isPreloaded()) {
                toClose.insert(w, activities);
            }
        }
    }

    QStringList titlesToClose;
    QList<KonqMainWindow*> keys = toClose.keys();
    std::transform(keys.constBegin(), keys.constEnd(), std::back_inserter(titlesToClose), [](KonqMainWindow *mw){return mw->windowTitle();});

    saveWindowsActivityInfo(toClose);

    QStringList existingUuidsList;
    std::transform(windows->constBegin(), windows->constEnd(), std::back_inserter(existingUuidsList), [](KonqMainWindow *w){return w->uuid();});
    QSet<QString> existingUuids(existingUuidsList.constBegin(), existingUuidsList.constEnd());

    QStringList expUuids;
    KConfigGroup grp = KSharedConfig::openConfig(activitiesConfigPath())->group(activitiesGroupName());
    for (const QString &act : runningActivities) {
        QStringList uuids = grp.readEntry(act, QStringList{});
        expUuids.append(uuids);
    }
    makeUnique(expUuids);

    QStringList uuidsToRestore;
    std::copy_if(expUuids.constBegin(), expUuids.constEnd(), std::back_inserter(uuidsToRestore), [existingUuids](const QString &u){return !existingUuids.contains(u);});

    if (!uuidsToRestore.isEmpty()) {
        m_restoringStoppedActivity = true;
        // In theory, this should be set by KonquerorApplication::performStart. This is just for safety, to avoid the risk that m_restoringStoppedActivity remains set to true
        QTimer::singleShot(2000, [this](){m_restoringStoppedActivity = false;});
    }

    for (const QString &uuid : existingUuids) {
        restoreWindowFromActivityState(uuid);
    }

    for (auto it = toClose.begin(); it != toClose.end(); ++it) {
        closeWindowBecauseNotInRunningActivities(it.key());
    }
}

void ActivityManager::saveWindowsActivityInfo(const QHash<KonqMainWindow *, QStringList>& windowsWithActivities)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(activitiesConfigPath());

    QStringList windowsUuids;

    QMultiHash <QString, QString> activities;
    for (auto it = windowsWithActivities.constBegin(); it != windowsWithActivities.constEnd(); ++it) {
        KonqMainWindow *w = it.key();
        windowsUuids.append(w->uuid());
        if (w->isPreloaded()) {
            continue;
        }
        KConfigGroup grp = config->group(w->uuid());
        w->saveProperties(grp);
        for (const QString & act : it.value()) {
            activities.insert(act, w->uuid());
        }
    }

    //Saving the windows belonging to each activity requires care:
    // - we can't just write the value of the entry in activities corresponding to each activity because there may
    //  be windows belonging to that activity which aren't included in windowsWithActivities and which must be preserved
    // - we can't just append the existing entries for each activity to the corresponding entry in activities because if
    //  one of the windows in windowsWithActivities used to belong to an activity but doesn't belong to it anymore, it
    //  wouldn't be removed (since it was in the existing entry and thus would remain)
    // What we do is the following:
    // - read the existing windows for each activity from the configuration file
    // - remove from this list all the windows in windowsWithActivities (whose uuids are in windowsUuids)
    // - append the remaining existing entries to the new ones (in uuids)
    KConfigGroup grp = config->group(activitiesGroupName());
    for (const QString & act : activities.uniqueKeys()) {
        QStringList uuids = activities.values(act);
        QStringList existingUuids = grp.readEntry(act, QStringList{});
        for (const QString &u : windowsUuids) {
            existingUuids.removeOne(u);
        }
        uuids.append(existingUuids);
        makeUnique(uuids);
        grp.writeEntry(act, uuids);
    }

    // grp = config->group("Windows UUID");
    // for (KonqMainWindow *w : windowsWithActivities.keys()) {
    //     grp.writeEntry(w->uuid(), w->windowTitle());
    // }

    config->sync();
}

void ActivityManager::handleWindowChanged(WId id, NET::Properties, NET::Properties2 prop2)
{
    if (!(prop2 & NET::WM2Activities)) {
        return;
    }
    KonqMainWindow *w = qobject_cast<KonqMainWindow*>(QWidget::find(id));
    if (!w) {
        return;
    }

    KWindowInfo info(id, NET::Properties(), NET::WM2Activities);
    QStringList activities = info.activities();
    //activities will be empty if the window should be shown in all activities. In this case,
    //there's nothing we need to do
    if (activities.isEmpty()) {
        return;
    }
    QStringList runningActivities = m_activitiesConsumer->runningActivities();
    auto isRunning = [runningActivities](const QString &act){return runningActivities.contains(act);};
    if (std::any_of(activities.constBegin(), activities.constEnd(), isRunning)) {
        return;
    }

    QHash<KonqMainWindow*, QStringList> hash;
    hash.insert(w, activities);
    saveWindowsActivityInfo(hash);
    closeWindowBecauseNotInRunningActivities(w);
}

void ActivityManager::removeWindowFromActivities(KonqMainWindow* window)
{
    QString uuid = window->uuid();
    KSharedConfig::Ptr config = KSharedConfig::openConfig(activitiesConfigPath());

    config->deleteGroup(uuid);

    KConfigGroup grp = config->group(activitiesGroupName());
    QStringList activities = grp.keyList();
    for (const QString &act : activities) {
        QStringList windows = grp.readEntry(act, QStringList{});
        //Remove the window uuid from the list of windows for the activity. If removeOne returns false, it means
        //the uuid wasn't in the the list, so there's no need to write it back
        if (windows.removeOne(uuid)) {
            grp.writeEntry(act, windows);
        }
    }
    config->sync();
}

void ActivityManager::removeActivityState(const QString& id)
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig(activitiesConfigPath());

    KConfigGroup activitiesGrp = config->group(activitiesGroupName());

    //We need to find out all windows which only belongs to the activity id
    //and remove all information about them
    QStringList activities = activitiesGrp.keyList();
    QStringList uuids = activitiesGrp.readEntry(id, QStringList{});
    QStringList otherActivitiesUuids;
    for (const QString &act : activities) {
        if (act != id) {
            otherActivitiesUuids.append(activitiesGrp.readEntry(act, QStringList{}));
        }
    }
    makeUnique(otherActivitiesUuids);
    for (const QString &uuid : uuids) {
        if (!otherActivitiesUuids.contains(uuid)) {
            config->deleteGroup(uuid);
        }
    }

    activitiesGrp.deleteEntry(id);

    config->sync();
}

KonqMainWindow* ActivityManager::restoreWindowFromActivityState(const QString& uuid)
{
    //WARNING: for efficiency reasons, this method assumes no window with the given uuid exists. It's up to the caller to make sure of that

    KSharedConfig::Ptr conf = KSharedConfig::openConfig(activitiesConfigPath());
    KConfigGroup windowGrp = conf->group(uuid);
    KConfigGroup activitiesGrp = conf->group(activitiesGroupName());

    if (!windowGrp.exists()) {
        return nullptr;
    }
    KonqMainWindow *w = KonqViewManager::openSavedWindow(windowGrp);
    if (!w) {
        return nullptr;
    }

    // QStringList activities;
    // QStringList activitiesEntries = activitiesGrp.keyList();
    // auto activityHasWindow = [uuid, activitiesGrp] (const QString &act) {
    //     return activitiesGrp.readEntry(act, QStringList{}).contains(uuid);
    // };
    // std::copy_if(activitiesEntries.constBegin(), activitiesEntries.constEnd(), std::back_inserter(activities), activityHasWindow);
    // KX11Extras::setOnActivities(w->winId(), activities);
    w->show();

    //Don't keep information about the window, since they would become outdated
    conf->deleteGroup(uuid);
    conf->sync();

    return w;
}

void ActivityManager::registerMainWindow(KonqMainWindow* window)
{
    connect(window, &KonqMainWindow::closing, this, &ActivityManager::removeWindowFromActivities);
}

void ActivityManager::makeUnique(QStringList& lst)
{
    if (lst.isEmpty()) {
        return;
    }
    lst.sort();
    auto last = std::unique(lst.begin(), lst.end());
    lst.erase(last, lst.end());
}
