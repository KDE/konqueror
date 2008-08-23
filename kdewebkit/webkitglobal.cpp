/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
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

void WebKitGlobal::initGlobalSettings()
{
    kDebug();
    if (!settings()->userStyleSheet().isEmpty()) {
        QWebSettings::globalSettings()->setUserStyleSheetUrl(QUrl(settings()->userStyleSheet()));
    }
    QWebSettings::globalSettings()->setAttribute(QWebSettings::AutoLoadImages, settings()->autoLoadImages());
    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptEnabled, settings()->isJavaScriptEnabled());
    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavaEnabled, settings()->isJavaEnabled());
    QWebSettings::globalSettings()->setAttribute(QWebSettings::PluginsEnabled, settings()->isPluginsEnabled());
    QWebSettings::globalSettings()->setAttribute(QWebSettings::JavascriptCanOpenWindows,
                                                 settings()->windowOpenPolicy() != WebKitSettings::KJSWindowOpenDeny);

    QWebSettings::globalSettings()->setFontFamily(QWebSettings::StandardFont, settings()->stdFontName());
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont, settings()->fixedFontName());
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SerifFont, settings()->serifFontName());
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SansSerifFont, settings()->sansSerifFontName());
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::CursiveFont, settings()->cursiveFontName());
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FantasyFont, settings()->fantasyFontName());

    QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, settings()->minFontSize());
    QWebSettings::globalSettings()->setFontSize(QWebSettings::DefaultFontSize, settings()->mediumFontSize());
}

WebKitSettings *WebKitGlobal::settings()
{
    return s_webKitSettings;
}

const KComponentData &WebKitGlobal::componentData()
{
    Q_ASSERT(s_self);

    if (!s_componentData) {
        s_about = new KAboutData("webkitkde", 0, ki18n("Webkit HTML Component"),
                                 /*version*/ "0.1", ki18n(/*shortDescription*/ ""),
                                 KAboutData::License_LGPL,
                                 ki18n("(c) 2008, Urs Wolfer\n"
                                       "(c) 2007 Trolltech ASA"));

        s_about->addAuthor(ki18n("Laurent Montel"), KLocalizedString(), "montel@kde.org");
        s_about->addAuthor(ki18n("Michael Howell"), KLocalizedString(), "mhowell123@gmail.com");
        s_about->addAuthor(ki18n("Urs Wolfer"), KLocalizedString(), "uwolfer@kde.org");
        s_about->addAuthor(ki18n("Dirk Mueller"), KLocalizedString(), "mueller@kde.org");
        s_componentData = new KComponentData(s_about);
    }

    return *s_componentData;
}
