// -*- c++ -*-

/*
    SPDX-FileCopyrightText: 2003 Richard J. Moore <rich@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "autorefresh.h"
#include <kparts/part.h>

#include <kiconloader.h>
#include <kmessagebox.h>
#include <KLocalizedString>
#include <QTimer>
#include <kselectaction.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <KParts/ReadOnlyPart>

AutoRefresh::AutoRefresh(QObject *parent, const QVariantList & /*args*/)
    : KonqParts::Plugin(parent)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AutoRefresh::slotRefresh);

    refresher = actionCollection()->add<KSelectAction>(QStringLiteral("autorefresh"));
    refresher->setText(i18n("&Auto Refresh"));
    refresher->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    connect(refresher, SIGNAL(triggered(QAction*)), this, SLOT(slotIntervalChanged()));
    QStringList sl;
    sl << i18n("None");
    sl << i18n("Every 15 Seconds");
    sl << i18n("Every 30 Seconds");
    sl << i18n("Every Minute");
    sl << i18n("Every 5 Minutes");
    sl << i18n("Every 10 Minutes");
    sl << i18n("Every 15 Minutes");
    sl << i18n("Every 30 Minutes");
    sl << i18n("Every 60 Minutes");
    sl << i18n("Every 2 Hours");
    sl << i18n("Every 6 Hours");

    refresher->setItems(sl);
    refresher->setCurrentItem(0);
}

AutoRefresh::~AutoRefresh()
{
}

void AutoRefresh::slotIntervalChanged()
{
    int idx = refresher->currentItem();
    int timeout = 0;
    switch (idx) {
    case 1:
        timeout = (15 * 1000);
        break;
    case 2:
        timeout = (30 * 1000);
        break;
    case 3:
        timeout = (60 * 1000);
        break;
    case 4:
        timeout = (5 * 60 * 1000);
        break;
    case 5:
        timeout = (10 * 60 * 1000);
        break;
    case 6:
        timeout = (15 * 60 * 1000);
        break;
    case 7:
        timeout = (30 * 60 * 1000);
        break;
    case 8:
        timeout = (60 * 60 * 1000);
        break;
    case 9:
        timeout = (2 * 60 * 60 * 1000);
        break;
    case 10:
        timeout = (6 * 60 * 60 * 1000);
        break;
    default:
        break;
    }
    timer->stop();
    if (timeout) {
        timer->start(timeout);
    }
}

void AutoRefresh::slotRefresh()
{
    KParts::ReadOnlyPart *part = qobject_cast< KParts::ReadOnlyPart * >(parent());
    if (!part) {
        QString title = i18nc("@title:window", "Cannot Refresh Source");
        QString text = i18n("<qt>This plugin cannot auto-refresh the current part.</qt>");

        KMessageBox::error(nullptr, text, title);
    } else {
        // Get URL
        QUrl url = part->url();
        part->openUrl(url);
    }
}

K_PLUGIN_CLASS_WITH_JSON(AutoRefresh, "autorefresh.json")

#include "autorefresh.moc"

