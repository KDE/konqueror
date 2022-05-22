/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "uachangerplugin.h"

#include <sys/utsname.h>

#include <QMenu>
#include <QRegExp>
#include <QWebEngineProfile>
#include <QApplication>
#include <QMetaObject>

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

#include "webenginepartcontrols.h"

K_PLUGIN_CLASS_WITH_JSON(UAChangerPlugin, "uachangerplugin.json")

#define UA_PTOS(x) (*it)->property(x).toString()
#define QFL1(x) QLatin1String(x)

UAChangerPlugin::UAChangerPlugin(QObject *parent,
                                 const QVariantList &)
    : KonqParts::Plugin(parent), m_part(nullptr)
{
    m_pUAMenu = new KActionMenu(QIcon::fromTheme("preferences-web-browser-identification"),
                                i18nc("@title:menu Changes the browser identification", "Change Browser Identification"),
                                actionCollection());
    actionCollection()->addAction("changeuseragent", m_pUAMenu);
    m_pUAMenu->setPopupMode(QToolButton::InstantPopup);
    connect(m_pUAMenu->menu(), &QMenu::aboutToShow, this, &UAChangerPlugin::slotAboutToShow);
    initMenu();
}

UAChangerPlugin::~UAChangerPlugin()
{
}

void UAChangerPlugin::slotAboutToShow()
{
    clearMenu();

    KConfigGroup grp = KSharedConfig::openConfig("useragenttemplatesrc")->group("Templates");
    TemplateMap templates = grp.entryMap();
    fillMenu(templates);

    const QString currentUA = QWebEngineProfile::defaultProfile()->httpUserAgent();
    QList<QAction*> actions = m_actionGroup->actions();
    auto isCurrentUA = [currentUA](QAction *a){return currentUA == a->data().toString();};
    auto found = std::find_if(actions.constBegin(), actions.constEnd(), isCurrentUA);
    if (found != actions.constEnd()) {
        (*found)->setChecked(true);
    } else {
        m_defaultAction->setChecked(true);
    }
}

void UAChangerPlugin::initMenu()
{
    m_actionGroup = new QActionGroup(m_pUAMenu->menu());
    m_defaultAction = new QAction(i18nc("@action:inmenu Uses the default browser identification", "Default Identification"), this);
    m_defaultAction->setCheckable(true);
    m_pUAMenu->menu()->addAction(m_defaultAction);
    m_actionGroup->addAction(m_defaultAction);

    connect(m_actionGroup, &QActionGroup::triggered, this, &UAChangerPlugin::slotItemSelected);
}

void UAChangerPlugin::fillMenu(const TemplateMap &templates)
{
    m_pUAMenu->menu()->addSeparator();
    for (auto it = templates.constBegin(); it != templates.constEnd(); ++it) {
        QAction *a = new QAction(it.key());
        a->setData(it.value());
        m_pUAMenu->addAction(a);
        m_actionGroup->addAction(a);
        a->setCheckable(true);
    }
}

void UAChangerPlugin::clearMenu()
{
    QList<QAction*> actions = m_actionGroup->actions();
    for (QAction *a : actions) {
        if (a != m_defaultAction) {
            a->deleteLater();
        }
    }
}

void UAChangerPlugin::slotItemSelected(QAction *action)
{
    WebEnginePartControls *ctrls = WebEnginePartControls::self();
    QString uaString = action->data().toString();
    if (action == m_defaultAction) {
        KConfigGroup grp = KSharedConfig::openConfig()->group("UserAgent");
        bool useCustomUserAgent = grp.readEntry("UseCustomUserAgent", false);
        QString defaultUA = ctrls->defaultHttpUserAgent();
        if (useCustomUserAgent) {
            uaString = grp.readEntry("CustomUserAgent", defaultUA);
        } else {
            uaString = defaultUA;
        }
    }
    ctrls->setHttpUserAgent(uaString);
}

#include "uachangerplugin.moc"
