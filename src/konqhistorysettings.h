/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_HISTORYSETTINGS_H
#define KONQ_HISTORYSETTINGS_H

#include <konqprivate_export.h>

#include <QFont>
#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusConnection>

/**
 * @brief Object containing the settings used by the history views
 *
 * These settings are used by both the sidebar module and the history dialog
 *
 * The instances of KonqHistorySettings in all konqueror processes
 * synchronize themselves using DBus.
 *
 * This is a singleton class whose only instance can be accessed using self().
 *
 * Settings are stored in `konquerorrc`.
 */
class KONQUERORPRIVATE_EXPORT KonqHistorySettings : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Enum describing how to interpret the "newer than" and "older than"
     * user settings
     */
    enum {
        MINUTES, //!< The settings are expressed in minutes
        DAYS //!< The settings are expressed in days
    };

    /**
     * @brief Enum describing the action to carry out when the user activates an
     * item in the history
     */
    enum class Action {
        Auto = 0, //!< Open the item in the current tab if it's empty and in a new tab otherwise
        OpenNewTab = 1, //!< Always open the item in a new tab
        OpenCurrentTab = 2, //!< Always open the item in the current tab
        OpenNewWindow = 3 //!< Always open the item in a new window
    };

    /**
     * @brief The only instance of this class
     * @return the only instance of this class
     */
    static KonqHistorySettings *self();

    ~KonqHistorySettings() override; //!< Destructor

    /**
     * @brief Writes the settings to the configuration file
     *
     * It notifies other Konqueror instances that settings have changed.
     */
    void applySettings();

    Action m_defaultAction; //!< The action to carry out when the user activates an history item

    /**
     * @brief The maximum age for an entry to be shown in the special font #m_fontYoungerThan
     *
     * This number can represent days or minutes, according to #m_metricYoungerThan
     */
    uint m_valueYoungerThan;

    /**
     * @brief The minimum age for an entry to be shown in the special font #m_fontOlderThan
     *
     * This number can represent days or minutes, according to #m_metricOlderThan
     */
    uint m_valueOlderThan;

    /**
     * @brief How to interpret m_valueYoungerThan
     *
     * It can be either MINUTES or DAYS
     */
    int m_metricYoungerThan;

    /**
     * @brief How to interpret m_metricOlderThan
     *
     * It can be either MINUTES or DAYS
     */
    int m_metricOlderThan;

    QFont m_fontYoungerThan; //!< The font to use for entries more recent than #m_valueYoungerThan
    QFont m_fontOlderThan; //!< The font to use for entries more recent than #m_valueOlderThan

    bool m_detailedTips; //!< Whether or not to show detailed tooltips
    bool m_sortsByName; //!< Whether or not to sort history entries by name

Q_SIGNALS:
    /**
     * @brief Signal emitted when settings have changed
     */
    void settingsChanged();

private Q_SLOTS:

    /**
     * @brief Slot called when settings have changed
     *
     * This is connected to the DBus signal notifySettingsChanged() and emits the
     * ordinary signal settingsChanged().
     */
    void slotSettingsChanged();

protected:
    Q_DISABLE_COPY(KonqHistorySettings)

Q_SIGNALS:
    // DBus signals
    /**
     * @brief DBus signal telling Konqueror instances that settings have changed
     */
    void notifySettingsChanged();

private:
    /**
     * @brief Reads settings from the configuration file
     *
     * @param reparse whether or not to call `KConfig::reparseConfiguration()` on
     * the configuration object before reading settings from it. It should usually
     * be `true` except when called from the constructor.
     */
    void readSettings(bool reparse);

    friend class KonqHistorySettingsSingleton;

    /**
     * @brief Constructor
     *
     * It reads the settings from the configuration file
     */
    KonqHistorySettings();
};

/**
 * @brief DBus adaptor for KonqHistorySettings
 */
class KonqHistorySettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.SidebarHistorySettings")
public:

    /**
     * @brief Constructor
     *
     * The new instance will automatically rely signals from the KonqHistorySettings
     * object
     *
     * @param parent the KonqHistorySettings to be wrapped by the new instance
     */
    KonqHistorySettingsAdaptor(KonqHistorySettings *parent)
        : QDBusAbstractAdaptor(parent)
    {
        setAutoRelaySignals(true);
    }

Q_SIGNALS:
    /**
     * @brief DBus signal emitted when settings have changed
     */
    void notifySettingsChanged();
};

#endif // KONQ_HISTORYSETTINGS_H
