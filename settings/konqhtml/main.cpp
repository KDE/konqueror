/*
    main.cpp

    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
    SPDX-FileCopyrightText: 2000 Daniel Molkentin <molkentin@kde.org>

    Requires the Qt widget libraries, available at no cost at
    http://www.troll.no/

    SPDX-License-Identifier: GPL-2.0-or-later
*/

// Own
#include "main.h"


// Qt
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDBusConnection>
#include <QDBusMessage>

// KDE
#include <kaboutdata.h>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

// Local
#include "jsopts.h"
#include "javaopts.h"
#include "appearance.h"
#include "htmlopts.h"
#include "filteropts.h"
#include "generalopts.h"

KJSParts::KJSParts(QObject *parent, const KPluginMetaData &md, const QVariantList &)
    : KCModule(parent, md)
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);

    QVBoxLayout *layout = new QVBoxLayout(widget());
    tab = new QTabWidget(widget());
    layout->addWidget(tab);

    // ### the groupname is duplicated in KJSParts::save
    java = new KJavaOptions(mConfig, QStringLiteral("Java/JavaScript Settings"), widget());
    tab->addTab(java->widget(), i18n("&Java"));
#if QT_VERSION_MAJOR < 6
    connect(java, QOverload<bool>::of(&KJavaOptions::changed), this, &KJSParts::markAsChanged);
#else
    connect(java, &KJavaOptions::needsSaveChanged, this, &KJSParts::needsSaveChanged);
#endif

    javascript = new KJavaScriptOptions(mConfig, QStringLiteral("Java/JavaScript Settings"), widget());
    tab->addTab(javascript->widget(), i18n("Java&Script"));
#if QT_VERSION_MAJOR < 6
    connect(javascript, QOverload<bool>::of(&KJavaScriptOptions::changed), this, &KJSParts::markAsChanged);
#else
    connect(javascript, &KJavaScriptOptions::needsSaveChanged, this, &KJSParts::needsSaveChanged);
#endif
}

void KJSParts::load()
{
    javascript->load();
    java->load();
}

void KJSParts::save()
{
    javascript->save();
    java->save();

    // delete old keys after they have been migrated
    if (javascript->_removeJavaScriptDomainAdvice
            || java->_removeJavaScriptDomainAdvice) {
        mConfig->group("Java/JavaScript Settings").deleteEntry("JavaScriptDomainAdvice");
        javascript->_removeJavaScriptDomainAdvice = false;
        java->_removeJavaScriptDomainAdvice = false;
    }

    mConfig->sync();

    // Send signal to konqueror
    // Warning. In case something is added/changed here, keep kfmclient in sync
    QDBusMessage message =
        QDBusMessage::createSignal(QStringLiteral("/KonqMain"), QStringLiteral("org.kde.Konqueror.Main"), QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void KJSParts::defaults()
{
    javascript->defaults();
    java->defaults();
}
