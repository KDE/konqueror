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

KJSParts::KJSParts(QWidget *parent, const QVariantList &)
    : KCModule(parent)
{
    mConfig = KSharedConfig::openConfig(QStringLiteral("konquerorrc"), KConfig::NoGlobals);
    KAboutData *about =
        new KAboutData(QStringLiteral("kcmkonqhtml"), i18n("Konqueror Browsing Control Module"),
                       QLatin1String(""), QLatin1String(""), KAboutLicense::GPL,
                       i18n("(c) 1999 - 2001 The Konqueror Developers"));

    about->addAuthor(i18n("Waldo Bastian"), QLatin1String(""), QStringLiteral("bastian@kde.org"));
    about->addAuthor(i18n("David Faure"), QLatin1String(""), QStringLiteral("faure@kde.org"));
    about->addAuthor(i18n("Matthias Kalle Dalheimer"), QLatin1String(""), QStringLiteral("kalle@kde.org"));
    about->addAuthor(i18n("Lars Knoll"), QLatin1String(""), QStringLiteral("knoll@kde.org"));
    about->addAuthor(i18n("Dirk Mueller"), QLatin1String(""), QStringLiteral("mueller@kde.org"));
    about->addAuthor(i18n("Daniel Molkentin"), QLatin1String(""), QStringLiteral("molkentin@kde.org"));
    about->addAuthor(i18n("Wynn Wilkes"), QLatin1String(""), QStringLiteral("wynnw@caldera.com"));

    about->addCredit(i18n("Leo Savernik"), i18n("JavaScript access controls\n"
                     "Per-domain policies extensions"),
                     QStringLiteral("l.savernik@aon.at"));

    setAboutData(about);

    QVBoxLayout *layout = new QVBoxLayout(this);
    tab = new QTabWidget(this);
    layout->addWidget(tab);

    // ### the groupname is duplicated in KJSParts::save
    java = new KJavaOptions(mConfig, QStringLiteral("Java/JavaScript Settings"), this);
    tab->addTab(java, i18n("&Java"));
    connect(java, QOverload<bool>::of(&KJavaOptions::changed), this, &KJSParts::markAsChanged);

    javascript = new KJavaScriptOptions(mConfig, QStringLiteral("Java/JavaScript Settings"), this);
    tab->addTab(javascript, i18n("Java&Script"));
    connect(javascript, QOverload<bool>::of(&KJavaScriptOptions::changed), this, &KJSParts::markAsChanged);
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

QString KJSParts::quickHelp() const
{
    return i18n("<h2>JavaScript</h2>On this page, you can configure "
                "whether JavaScript programs embedded in web pages should "
                "be allowed to be executed by Konqueror."
                "<h2>Java</h2>On this page, you can configure "
                "whether Java applets embedded in web pages should "
                "be allowed to be executed by Konqueror."
                "<br /><br /><b>Note:</b> Active content is always a "
                "security risk, which is why Konqueror allows you to specify very "
                "fine-grained from which hosts you want to execute Java and/or "
                "JavaScript programs.");
}
