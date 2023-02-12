/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "uachangerplugin.h"

#include <sys/utsname.h>

#include <QMenu>
#include <QRegExp>

#include <kwidgetsaddons_version.h>
#include <kactionmenu.h>
#include <kservicetypetrader.h>
#include <klocalizedstring.h>
#include <kservice.h>
#include <kconfiggroup.h>
#include <kpluginfactory.h>
#include <kprotocolmanager.h>
#include <kactioncollection.h>
#include <ksharedconfig.h>

#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegate>
#include <kparts/openurlarguments.h>

#include <kio/job.h>
#include <kio/scheduler.h>

K_PLUGIN_CLASS_WITH_JSON(UAChangerPlugin, "uachangerplugin.json")

#define UA_PTOS(x) (*it)->property(x).toString()
#define QFL1(x) QLatin1String(x)

UAChangerPlugin::UAChangerPlugin(QObject *parent,
                                 const QVariantList &)
    : KonqParts::Plugin(parent),
      m_bSettingsLoaded(false), m_part(nullptr), m_config(nullptr)
{
    m_pUAMenu = new KActionMenu(QIcon::fromTheme("preferences-web-browser-identification"),
                                i18n("Change Browser Identification"),
                                actionCollection());
    actionCollection()->addAction("changeuseragent", m_pUAMenu);
    m_pUAMenu->setPopupMode(QToolButton::InstantPopup);
    connect(m_pUAMenu->menu(), &QMenu::aboutToShow, this, &UAChangerPlugin::slotAboutToShow);

    if (parent!=nullptr) {
        m_part = qobject_cast<KParts::ReadOnlyPart *>(parent);
        connect(m_part, &KParts::ReadOnlyPart::started, this, &UAChangerPlugin::slotEnableMenu);
        connect(m_part, QOverload<>::of(&KParts::ReadOnlyPart::completed), this, &UAChangerPlugin::slotEnableMenu);
        connect(m_part, &KParts::ReadOnlyPart::completedWithPendingAction, this, &UAChangerPlugin::slotEnableMenu);
    }
}

UAChangerPlugin::~UAChangerPlugin()
{
    saveSettings();
    slotReloadDescriptions();
}

void UAChangerPlugin::slotReloadDescriptions()
{
    delete m_config;
    m_config = nullptr;
}

void UAChangerPlugin::parseDescFiles()
{
    const KService::List list = KServiceTypeTrader::self()->query("UserAgentStrings");
    if (list.isEmpty()) {
        return;
    }

    m_mapAlias.clear();
    m_lstAlias.clear();
    m_lstIdentity.clear();

    struct utsname utsn;
    uname(&utsn);

    QStringList languageList = KLocalizedString::languages();
    if (!languageList.isEmpty()) {
        const int index = languageList.indexOf(QFL1("C"));
        if (index > -1) {
            if (languageList.contains(QFL1("en"))) {
                languageList.removeAt(index);
            } else {
                languageList[index] = QFL1("en");
            }
        }
    }

    KService::List::ConstIterator it = list.constBegin();
    KService::List::ConstIterator lastItem = list.constEnd();

    for (; it != lastItem; ++it) {
        QString ua  = UA_PTOS("X-KDE-UA-FULL");
        QString tag = UA_PTOS("X-KDE-UA-TAG");

        // The menu groups thing by tag, with the menu name being the X-KDE-UA-NAME by default. We make groups for
        // IE, NS, Firefox, Safari, and Opera, and put everything else into "Other"
        QString menuName;
        MenuGroupSortKey menuKey; // key for the group..
        if (tag != "IE" && tag != "NN" && tag != "FF" && tag != "SAF" && tag != "OPR") {
            tag = "OTHER";
            menuName = i18n("Other");
            menuKey = MenuGroupSortKey(tag, true);
        } else {
            menuName = UA_PTOS("X-KDE-UA-NAME");
            menuKey  = MenuGroupSortKey(tag, false);
        }

        if ((*it)->property("X-KDE-UA-DYNAMIC-ENTRY").toBool()) {
            ua.replace(QFL1("appSysName"), QFL1(utsn.sysname));
            ua.replace(QFL1("appSysRelease"), QFL1(utsn.release));
            ua.replace(QFL1("appMachineType"), QFL1(utsn.machine));
            ua.replace(QFL1("appLanguage"), languageList.join(QFL1(", ")));
            ua.replace(QFL1("appPlatform"), QFL1("X11"));
        }

        if (m_lstIdentity.contains(ua)) {
            continue;    // Ignore dups!
        }

        m_lstIdentity << ua;

        // Compute what to display for our menu entry --- including platform name if it's available,
        // and avoiding repeating the browser name in categories other than 'other'.
        QString platform = QString("%1 %2").arg(UA_PTOS("X-KDE-UA-SYSNAME")).arg(UA_PTOS("X-KDE-UA-SYSRELEASE"));

        QString alias;
        if (platform.trimmed().isEmpty()) {
            if (!menuKey.isOther) {
                alias = i18nc("%1 = browser version (e.g. 2.0)", "Version %1", UA_PTOS("X-KDE-UA-VERSION"));
            } else
                alias = i18nc("%1 = browser name, %2 = browser version (e.g. Firefox, 2.0)",
                              "%1 %2", UA_PTOS("X-KDE-UA-NAME"), UA_PTOS("X-KDE-UA-VERSION"));
        } else {
            if (!menuKey.isOther)
                alias = i18nc("%1 = browser version, %2 = platform (e.g. 2.0, Windows XP)",
                              "Version %1 on %2", UA_PTOS("X-KDE-UA-VERSION"), platform);
            else
                alias = i18nc("%1 = browser name, %2 = browser version, %3 = platform (e.g. Firefox, 2.0, Windows XP)",
                              "%1 %2 on %3", UA_PTOS("X-KDE-UA-NAME"), UA_PTOS("X-KDE-UA-VERSION"), platform);
        }

        m_lstAlias << alias;

        /* sort in this UA Alias alphabetically */
        BrowserGroup ualist = m_mapAlias[menuKey];
        BrowserGroup::Iterator e = ualist.begin();
        while (!alias.isEmpty() && e != ualist.end()) {
            if (m_lstAlias[(*e)] > alias) {
                ualist.insert(e, m_lstAlias.count() - 1);
                alias.clear();
            }
            ++e;
        }

        if (!alias.isEmpty()) {
            ualist.append(m_lstAlias.count() - 1);
        }

        m_mapAlias[menuKey]   = ualist;
        m_mapBrowser[menuKey] = menuName;
    }
}

void UAChangerPlugin::slotEnableMenu()
{
    m_currentURL = m_part->url();

    // This plugin works on local files, http[s], and webdav[s].
    const QString proto = m_currentURL.scheme();
    if (m_currentURL.isLocalFile() ||
            proto.startsWith("http") || proto.startsWith("webdav")) {
        if (!m_pUAMenu->isEnabled()) {
            m_pUAMenu->setEnabled(true);
        }
    } else {
        m_pUAMenu->setEnabled(false);
    }
}

void UAChangerPlugin::slotAboutToShow()
{
    if (!m_config) {
        m_config = new KConfig("kio_httprc");
        parseDescFiles();
    }

    if (!m_bSettingsLoaded) {
        loadSettings();
    }

    if (m_pUAMenu->menu()->actions().isEmpty()) { // need to create the actions
        m_defaultAction = new QAction(i18n("Default Identification"), this);
        m_defaultAction->setCheckable(true);
        connect(m_defaultAction, &QAction::triggered, this, &UAChangerPlugin::slotDefault);
        m_pUAMenu->menu()->addAction(m_defaultAction);

        m_pUAMenu->menu()->addSeparator();

        m_actionGroup = new QActionGroup(m_pUAMenu->menu());
        AliasConstIterator map = m_mapAlias.constBegin();
        for (; map != m_mapAlias.constEnd(); ++map) {
            QMenu *browserMenu = m_pUAMenu->menu()->addMenu(m_mapBrowser.value(map.key()));
            BrowserGroup::ConstIterator e = map.value().begin();
            for (; e != map.value().end(); ++e) {
                QAction *action = new QAction(m_lstAlias[*e], m_actionGroup);
                action->setCheckable(true);
                action->setData(*e);
                browserMenu->addAction(action);
            }
        }
        connect(m_actionGroup, &QActionGroup::triggered, this, &UAChangerPlugin::slotItemSelected);

        m_pUAMenu->menu()->addSeparator();

        /* useless here, imho..
           m_pUAMenu->menu()->insertItem( i18n("Reload Identifications"), this,
           SLOT(slotReloadDescriptions()),
           0, ++count );*/

        m_applyEntireSiteAction = new QAction(i18n("Apply to Entire Site"), this);
        m_applyEntireSiteAction->setCheckable(true);
        m_applyEntireSiteAction->setChecked(m_bApplyToDomain);
        connect(m_applyEntireSiteAction, &QAction::triggered, this, &UAChangerPlugin::slotApplyToDomain);
        m_pUAMenu->menu()->addAction(m_applyEntireSiteAction);

        m_pUAMenu->menu()->addAction(i18n("Configure..."), this, &UAChangerPlugin::slotConfigure);
    }

    // Reflect current settings in the actions

    QString host = m_currentURL.isLocalFile() ? QFL1("localhost") : m_currentURL.host();
    m_currentUserAgent = KProtocolManager::userAgentForHost(host);
    //qDebug() << "User Agent: " << m_currentUserAgent;
    m_defaultAction->setChecked(m_currentUserAgent == KProtocolManager::defaultUserAgent());

    m_applyEntireSiteAction->setChecked(m_bApplyToDomain);
    Q_FOREACH (QAction *action, m_actionGroup->actions()) {
        const int id = action->data().toInt();
        action->setChecked(m_lstIdentity[id] == m_currentUserAgent);
    }

}

void UAChangerPlugin::slotConfigure()
{
    KService::Ptr service = KService::serviceByDesktopName("useragent");
    if (service) {
        KIO::ApplicationLauncherJob *job = new KIO::ApplicationLauncherJob(service);
        job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, m_part->widget()));
        job->start();
    }
}

void UAChangerPlugin::slotItemSelected(QAction *action)
{
    const int id = action->data().toInt();
    if (m_lstIdentity[id] == m_currentUserAgent) {
        return;
    }

    m_currentUserAgent = m_lstIdentity[id];
    QString host = m_currentURL.isLocalFile() ? QFL1("localhost") : filterHost(m_currentURL.host());

    KConfigGroup grp = m_config->group(host.toLower());
    grp.writeEntry("UserAgent", m_currentUserAgent);
    //qDebug() << "Writing out UserAgent=" << m_currentUserAgent << "for host=" << host;
    grp.sync();

    // Reload the page with the new user-agent string
    reloadPage();
}

void UAChangerPlugin::slotDefault()
{
    if (m_currentUserAgent == KProtocolManager::defaultUserAgent()) {
        return;    // don't flicker!
    }
    // We have no choice but delete all higher domain level settings here since it
    // affects what will be matched.
    QStringList partList = m_currentURL.host().split(QLatin1Char(' '),
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                                                     Qt::SkipEmptyParts);
#else
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                                                     QString::SkipEmptyParts);
#else
                                                     Qt::SkipEmptyParts);
#endif
#endif
    if (!partList.isEmpty()) {
        partList.removeFirst();

        QStringList domains;
        // Remove the exact name match...
        domains << m_currentURL.host();

        while (partList.count()) {
            if (partList.count() == 2)
                if (partList[0].length() <= 2 && partList[1].length() == 2) {
                    break;
                }

            if (partList.count() == 1) {
                break;
            }

            domains << partList.join(QFL1("."));
            partList.removeFirst();
        }

        KConfigGroup grp(m_config, QString());
        for (QStringList::Iterator it = domains.begin(); it != domains.end(); it++) {
            //qDebug () << "Domain to remove: " << *it;
            if (grp.hasGroup(*it)) {
                grp.deleteGroup(*it);
            } else if (grp.hasKey(*it)) {
                grp.deleteEntry(*it);
            }
        }
    } else if (m_currentURL.isLocalFile() && m_config->hasGroup("localhost")) {
        m_config->deleteGroup("localhost");
    }

    m_config->sync();

    // Reset some internal variables and inform the http io-slaves of the changes.
    m_currentUserAgent = KProtocolManager::defaultUserAgent();

    // Reload the page with the default user-agent
    reloadPage();
}

void UAChangerPlugin::reloadPage()
{
    // Inform running http(s) io-slaves about the change...
    KIO::Scheduler::emitReparseSlaveConfiguration();

    KParts::OpenUrlArguments args = m_part->arguments();
    args.setReload(true);
    m_part->setArguments(args);
    m_part->openUrl(m_currentURL);
}

QString UAChangerPlugin::filterHost(const QString &hostname)
{
    QRegExp rx;

    // Check for IPv4 address
    rx.setPattern("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
    if (rx.exactMatch(hostname)) {
        return hostname;
    }

    // Check for IPv6 address here...
    rx.setPattern("^\\[.*\\]$");
    if (rx.exactMatch(hostname)) {
        return hostname;
    }

    // Return the TLD if apply to domain or
    return (m_bApplyToDomain ? findTLD(hostname) : hostname);
}

QString UAChangerPlugin::findTLD(const QString &hostname)
{
    // As per the documentation for QUrl::topLevelDomain(), the "entire site"
    // for a hostname is considered to be the TLD suffix as returned by that
    // function, prefixed by the hostname component immediately before it.
    // For example, QUrl::topLevelDomain("http://www.kde.org") gives ".org"
    // so the returned result is "kde.org".

    QUrl u;
    u.setScheme("http");
    u.setHost(hostname);					// gives http://hostname/

    const QString tld = u.topLevelDomain(QUrl::EncodeUnicode);
    if (tld.isEmpty()) return hostname;				// name has no valid TLD

    const QString prefix = hostname.chopped(tld.length());	// remaining prefix of name
    const int idx = prefix.lastIndexOf(QLatin1Char('.'));
    const QString prev = prefix.mid(idx+1);			// works even if no '.'
    return prev+tld;
}

void UAChangerPlugin::saveSettings()
{
    if (!m_bSettingsLoaded) {
        return;
    }

    KConfig cfg("uachangerrc", KConfig::NoGlobals);
    KConfigGroup grp = cfg.group("General");

    grp.writeEntry("applyToDomain", m_bApplyToDomain);
}

void UAChangerPlugin::loadSettings()
{
    KConfig cfg("uachangerrc", KConfig::NoGlobals);
    KConfigGroup grp = cfg.group("General");

    m_bApplyToDomain = grp.readEntry("applyToDomain", true);
    m_bSettingsLoaded = true;
}

void UAChangerPlugin::slotApplyToDomain()
{
    m_bApplyToDomain = !m_bApplyToDomain;
}

#include "uachangerplugin.moc"
