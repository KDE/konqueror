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
 * Object containing the settings used by the history views
 * (both the sidebar module and the history dialog)
 *
 * The instances of KonqHistorySettings in all konqueror processes
 * synchronize themselves using DBus.
 */
class KONQUERORPRIVATE_EXPORT KonqHistorySettings : public QObject
{
    Q_OBJECT

public:
    enum { MINUTES, DAYS };

    enum class Action {Auto = 0, OpenNewTab = 1, OpenCurrentTab = 2 , OpenNewWindow = 3};

    static KonqHistorySettings *self();
    ~KonqHistorySettings() override;

    void applySettings();

    Action m_defaultAction;

    uint m_valueYoungerThan;
    uint m_valueOlderThan;

    int m_metricYoungerThan;
    int m_metricOlderThan;

    QFont m_fontYoungerThan;
    QFont m_fontOlderThan;

    bool m_detailedTips;
    bool m_sortsByName;

Q_SIGNALS:
    void settingsChanged();

private Q_SLOTS:
    void slotSettingsChanged();

protected:
    Q_DISABLE_COPY(KonqHistorySettings)

Q_SIGNALS:
    // DBus signals
    void notifySettingsChanged();

private:
    void readSettings(bool reparse);

    friend class KonqHistorySettingsSingleton;
    KonqHistorySettings();
};

class KonqHistorySettingsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror.SidebarHistorySettings")
public:
    KonqHistorySettingsAdaptor(KonqHistorySettings *parent)
        : QDBusAbstractAdaptor(parent)
    {
        setAutoRelaySignals(true);
    }

Q_SIGNALS:
    void notifySettingsChanged();
};

#endif // KONQ_HISTORYSETTINGS_H
