/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef KONQ_HISTORYSETTINGS_H
#define KONQ_HISTORYSETTINGS_H

#include <QFont>
#include <QtCore/QObject>
#include <QtDBus/QtDBus>

/**
 * Object containing the settings used by the history views
 * (both the sidebar module and the history dialog)
 *
 * The instances of KonqHistorySettings in all konqueror processes
 * synchronize themselves using DBus.
 */
class KonqHistorySettings : public QObject
{
    Q_OBJECT

public:
    enum { MINUTES, DAYS };

    static KonqHistorySettings* self();
    virtual ~KonqHistorySettings();

    void applySettings();

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
    Q_DISABLE_COPY( KonqHistorySettings )

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
    KonqHistorySettingsAdaptor( KonqHistorySettings* parent )
        : QDBusAbstractAdaptor( parent ) {
        setAutoRelaySignals( true );
    }

Q_SIGNALS:
    void notifySettingsChanged();
};

#endif // KONQ_HISTORYSETTINGS_H
