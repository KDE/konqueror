
#include <kapplication.h>
#include <kconfig.h>
#include <ksharedconfig.h>
#include <kglobal.h>

#include "history_settings.h"

KonqSidebarHistorySettings::KonqSidebarHistorySettings( QObject *parent )
    : QObject( parent )
{
    m_fontOlderThan.setItalic( true ); // default

    new KonqSidebarHistorySettingsAdaptor( this );
    const QString dbusPath = "/KonqSidebarHistorySettings";
    const QString dbusInterface = "org.kde.Konqueror.SidebarHistorySettings";
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject( dbusPath, this );
    dbus.connect(QString(), dbusPath, dbusInterface, "notifySettingsChanged", this, SLOT(slotSettingsChanged()));
}

#if 0 // huh? copying a QObject?
KonqSidebarHistorySettings::KonqSidebarHistorySettings( const KonqSidebarHistorySettings& s )
    : QObject()
{
    m_valueYoungerThan = s.m_valueYoungerThan;
    m_valueOlderThan = s.m_valueOlderThan;

    m_metricYoungerThan = s.m_metricYoungerThan;
    m_metricOlderThan = s.m_metricOlderThan;

    m_detailedTips = s.m_detailedTips;

    m_fontYoungerThan = s.m_fontYoungerThan;
    m_fontOlderThan = s.m_fontOlderThan;
}
#endif

KonqSidebarHistorySettings::~KonqSidebarHistorySettings()
{
}

void KonqSidebarHistorySettings::readSettings(bool global)
{
    KSharedConfigPtr config;

    if (global)
      config = KGlobal::config();
    else
      config = KSharedConfig::openConfig("konquerorrc");

    KConfigGroup cg( config, "HistorySettings");
    m_valueYoungerThan = cg.readEntry("Value youngerThan", 1 );
    m_valueOlderThan = cg.readEntry("Value olderThan", 2 );

    QString minutes = QLatin1String("minutes");
    QString days = QLatin1String("days");
    QString metric = cg.readEntry("Metric youngerThan", days );
    m_metricYoungerThan = (metric == days) ? DAYS : MINUTES;
    metric = cg.readEntry("Metric olderThan", days );
    m_metricOlderThan = (metric == days) ? DAYS : MINUTES;

    m_detailedTips = cg.readEntry("Detailed Tooltips", true);

    m_fontYoungerThan = cg.readEntry( "Font youngerThan",
					       m_fontYoungerThan );
    m_fontOlderThan   = cg.readEntry( "Font olderThan",
					       m_fontOlderThan );
}

void KonqSidebarHistorySettings::applySettings()
{
    KConfigGroup config(KSharedConfig::openConfig("konquerorrc"), "HistorySettings");

    config.writeEntry("Value youngerThan", m_valueYoungerThan );
    config.writeEntry("Value olderThan", m_valueOlderThan );

    QString minutes = QLatin1String("minutes");
    QString days = QLatin1String("days");
    config.writeEntry("Metric youngerThan", m_metricYoungerThan == DAYS ?
			  days : minutes );
    config.writeEntry("Metric olderThan", m_metricOlderThan == DAYS ?
 	 	 	   days : minutes );

    config.writeEntry("Detailed Tooltips", m_detailedTips);

    config.writeEntry("Font youngerThan", m_fontYoungerThan );
    config.writeEntry("Font olderThan", m_fontOlderThan );

    // notify konqueror instances about the new configuration
    emit notifySettingsChanged();
}

void KonqSidebarHistorySettings::slotSettingsChanged()
{
    readSettings(false);
    emit settingsChanged();
}

#include "history_settings.moc"
