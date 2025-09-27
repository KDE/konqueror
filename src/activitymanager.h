/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ACTIVITYMANAGER_H
#define ACTIVITYMANAGER_H

#include <QObject>

#include <KX11Extras>

#ifdef KActivities_FOUND
//Functions allowing to query about running activities has been removed from Plasma 6.5.0
//since activities are always considered to be running. Code making use of that
//functionality needs to be skipped
#include <PlasmaActivities/Version>
#if PLASMA_ACTIVITIES_VERSION < QT_VERSION_CHECK(6, 4, 90)
#define ACTIVITIES_CAN_BE_STOPPED
#endif
#endif

class KonqMainWindow;
namespace KActivities {
  class Consumer;
};

/**
 * @brief Class which handles closing and restoring windows according to the current activity
 *
 * In particular, this class:
 * - closes windows when all the activities they belong to are stopped (only for Plasma versions less than 6.5)
 * - stores information about windows which are closed because they only belong to stopped activities (only for Plasma versions less than 6.5)
 * - creates windows belonging to activities which are restarted (only for Plasma versions less than 6.5)
 * - removes information about deleted activities
 * - removes information about windows closed by the user
 *
 * All of this is made keeping a configuration file called `activitiesrc` in `./local/share/konqueror`.
 * This file contains:
 * - a list of all windows associated with each activity
 * - a list of information about windows belonging only to stopped activities (which means no window object exists for them)
 *  so they can be restored when the activities are started again
 */
class ActivityManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     *
     * @param parent the parent object
     */
    ActivityManager(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ActivityManager();

    /**
     * @brief Makes the necessary connections with a new KonqMainWindow
     *
     * @warning Never call this function more than once for the same window
     * @param window the window to connect to
     */
    void registerMainWindow(KonqMainWindow *window);

    bool restoringStoppedActivity() const {return m_restoringStoppedActivity;}

    void restoringStoppedActivityDone() {m_restoringStoppedActivity = false;}

private slots:

    /**
     * @brief Removes all information about a window when it's closed
     *
     * This removes both information to restore the window and information about which
     * activities the window used to belong to
     * @param window the window
     */
    void removeWindowFromActivities(KonqMainWindow *window);

#ifdef ACTIVITIES_CAN_BE_STOPPED
    /**
     * @brief Performs the operations needed to keep windows in sync with running activities
     *
     * In particular, this method:
     * - finds out which windows should be closed because all the activities they belong to are closed
     * - saves information about the windows to close
     * - closes the windows which only belong to stopped activities
     * - checks whether there are windows which belong to running activities but which don't exist and creates them
     * @note this method ignores preloaded windows (if any)
     * @param runningActivities the list of identifiers of all running activities
     */
    void handleRunningActivitiesChange(const QStringList &runningActivities);
#endif

    /**
     * @brief Removes information about a deleted activity
     *
     * This means:
     * - removing the list of windows belonging to the activity
     * - removing all information about those windows only belonging to the activity
     * @param id the id of activity to remove
     */
    void removeActivityState(const QString &id);

private:

    /**
     * @brief Stores information about windows and activities
     *
     * This method stores the information necessary to create the windows and updates the list of windows belonging
     * to each activity with the information contained in @p windowsWithActivities.
     *
     * @note This method doesn't replace the list of windows belonging to each activity, but "merge" that list with the information
     * from @p windowsWithActivities. In particular, for each activity:
     * - existing windows not included in @p windowsWithActivities remain untouched
     * - windows in @p windowsWithActivities are removed from activities which aren't listed in @p windowsWithActivities and added to
     *  activities listed in @p windowsWithActivities.
     *
     * @param windowsWithActivities a hash containing the list of activities each window belong to. The windows are
     * the keys while the list of activities are the values
     */
    void saveWindowsActivityInfo(const QHash<KonqMainWindow*, QStringList> &windowsWithActivities);

    /**
     * @brief Reacts to change in the activity settings for a given window
     *
     * If the window used to belong to a running activity but doesn't do so anymore, information about it is saved using saveWindowsActivityInfo()
     * and the window is closed.
     *
     * If the window belongs to an activity which is currently running, nothing is done because the activity system takes care of managing windows
     * itself.
     *
     * @note This method assumes that it can't be called for a windows which only belonged to stopped activities. The rationale for this assumption
     * is that windows belonging only to stopped activities should be closed, and so they can't have a window id.
     * @param id the window id of the changed window
     * @param prop unused
     * @param prop2 a list of the properties of type `NET::Properties2` which have changed. This method does nothing if this doesn't include `WM2Activities`
     */
    void handleWindowChanged(WId id, NET::Properties prop, NET::Properties2 prop2);

#ifdef ACTIVITIES_CAN_BE_STOPPED
    /**
     * @brief Closes a window in a way which works correctly with activities management
     *
     * When a window needs to be closed because all the activities it belongs to have been stopped, ActivityManager must call this method instead of
     * directly calling KonqMainWindow::close() to ensure that the window is closed in a way which allows to correctly restore it when the activities
     * are started again. In particular, this means:
     * - ensuring that removeActivityState() isn't called, otherwise the data needed to restore the window would be deleted
     * - not actually closing the window if it's the last window in the application. In this cause, closing the window would also close the application,
     *  preventing the global activity manager from recording that Konqueror needs to be started when one of the activities the window belongs to is
     *  restarted.
     *
     * @param window the window to close
     */
    void closeWindowBecauseNotInRunningActivities(KonqMainWindow *window);

    /**
     * @brief Creates a window using the information from the activity configuration file
     *
     * The window is moved to the appropriate activities.
     *
     * @param uuid the uuid of the window to restore
     * @return the restored window
     */
    KonqMainWindow* restoreWindowFromActivityState(const QString &uuid);
#endif

    /**
     * @return The path of the configuration file where activities information is stored
     */
    static QString activitiesConfigPath();

    /**
     * @return The name of the configuration group containing the windows for each activity
     */
    static QString activitiesGroupName();

    /**
     * @brief Removes duplicate entries from a list
     *
     * @note The elements of the list may change order
     * @param lst the list
     */
    void makeUnique(QStringList &lst);

private:

    /**
     * @brief The object informing about the changes about running activities and deleted activities
     */
    KActivities::Consumer *m_activitiesConsumer;

    /**
     * @brief Whether we're in the process of restoring a stopped activity
     *
     *
     */
    bool m_restoringStoppedActivity = false;
};

#endif // ACTIVITYMANAGER_H
