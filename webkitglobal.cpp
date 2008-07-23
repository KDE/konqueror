/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "webkitglobal.h"
#include "webkitpart.h"
#include "settings/webkitsettings.h"
#include "settings/webkitdefaults.h"

#include <QLinkedList>
#include <QWebSettings>

#include <KDE/KConfigGroup>
#include <KDE/KGlobalSettings>
#include <KDE/KGlobal>
#include <KDE/KDebug>
#include <KDE/KLocale>
#include <KDE/KAboutData>

WebKitGlobal *WebKitGlobal::s_self = 0;
unsigned long int WebKitGlobal::s_refcnt = 0;
static QLinkedList<WebKitPart*> *s_parts = 0;
KAboutData *WebKitGlobal::s_about = 0;
KComponentData *WebKitGlobal::s_componentData = 0;
K_GLOBAL_STATIC(WebKitSettings, s_webKitSettings)

WebKitGlobal::WebKitGlobal()
{
    Q_ASSERT(!s_self);
    s_self = this;
    ref();
}

WebKitGlobal::~WebKitGlobal()
{
    if (s_self == this) {
        delete s_componentData;
        delete s_about;
        if (s_parts) {
            Q_ASSERT(s_parts->isEmpty());
            delete s_parts;
        }
        s_parts = 0;
        s_componentData = 0;
        s_about = 0;
    } else
        deref();
}



void WebKitGlobal::ref()
{
    if (!s_refcnt && !s_self) {
        // we can't use a staticdeleter here, because that would mean
        // that the WebKitGlobal instance gets deleted from within a qPostRoutine, called
        // from the QApplication destructor. That however is too late, because
        // we want to destruct a KComponentData object, which involves destructing
        // a KConfig object, which might call KGlobal::dirs() (in sync()) which
        // probably is not going to work ;-)
        // well, perhaps I'm wrong here, but as I'm unsure I try to stay on the
        // safe side ;-) -> let's use a simple reference counting scheme
        // (Simon)
        WebKitGlobal *webkitglobal = new WebKitGlobal; // does initial ref()
        webkitglobal->initGlobalSettings();
    } else {
        ++s_refcnt;
    }
}

void WebKitGlobal::deref()
{
    if (!--s_refcnt && s_self) {
        delete s_self;
        s_self = 0;
    }
}

void WebKitGlobal::registerPart(WebKitPart *part)
{
    if (!s_parts)
        s_parts = new QLinkedList<WebKitPart*>;

    if (!s_parts->contains(part)) {
        s_parts->append(part);
        ref();
    }
}

void WebKitGlobal::deregisterPart(WebKitPart *part)
{
    Q_ASSERT(s_parts);

    if (s_parts->removeAll(part)) {
        if (s_parts->isEmpty()) {
            delete s_parts;
            s_parts = 0;
        }
        deref();
    }
}

void WebKitGlobal::initGlobalSettings()
{
    kDebug();
    QString userStyleSheet;
    KConfigGroup cgHtml(KGlobal::config(), "HTML Settings");
    if (cgHtml.readEntry("UserStyleSheetEnabled", false) == true) {
        if (cgHtml.hasKey("UserStyleSheet"))
            userStyleSheet = cgHtml.readEntry("UserStyleSheet", "");
    }
    if (!userStyleSheet.isEmpty()) {
        QWebSettings::globalSettings()->setUserStyleSheetUrl(QUrl(userStyleSheet));
    }
    if (cgHtml.hasKey("AutoLoadImages"))
        QWebSettings::globalSettings()->setAttribute(QWebSettings::AutoLoadImages, cgHtml.readEntry("AutoLoadImages", true));

    QWebSettings::globalSettings()->setFontFamily(QWebSettings::StandardFont, cgHtml.readEntry("StandardFont", KGlobalSettings::generalFont().family()));
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont, cgHtml.readEntry("FixedFont", KGlobalSettings::fixedFont().family()));
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SerifFont, cgHtml.readEntry("SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT));
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SansSerifFont, cgHtml.readEntry("SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT));
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::CursiveFont, cgHtml.readEntry("CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT));
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont, cgHtml.readEntry("FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT));

    QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, cgHtml.readEntry("MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE));
    QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFontSize, cgHtml.readEntry("MediumFontSize", 12));
}

WebKitSettings *WebKitGlobal::settings()
{
    return s_webKitSettings;
}

const KComponentData &WebKitGlobal::componentData()
{
    Q_ASSERT(s_self);

    if (!s_componentData) {
        s_about = new KAboutData("webkitpart", 0, ki18n("Webkit HTML Component"),
                                 /*version*/ "1.0", ki18n(/*shortDescription*/ ""),
                                 KAboutData::License_LGPL,
                                 ki18n("Copyright (c) 2007 Trolltech ASA"));

        s_about->addAuthor(ki18n("Laurent Montel"), KLocalizedString(), "montel@kde.org");
        s_about->addAuthor(ki18n("Urs Wolfer"), KLocalizedString(), "uwolfer@kde.org");
        s_about->addAuthor(ki18n("Dirk Mueller"), KLocalizedString(), "mueller@kde.org");
        s_componentData = new KComponentData(s_about);
    }

    return *s_componentData;
}
