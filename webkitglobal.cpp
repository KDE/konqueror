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
#include <khtmldefaults.h>
#include <QWebSettings>
#include <KDE/KConfigGroup>
#include <KDE/KGlobalSettings>
#include <KDE/KGlobal>

WebKitGlobal::WebKitGlobal()
{
    initGlobalSettings();
}

WebKitGlobal::~WebKitGlobal()
{
    //TODO
}

void WebKitGlobal::initGlobalSettings()
{
    QString userStyleSheet;
    KConfigGroup cgHtml( KGlobal::config(), "HTML Settings" );
    if ( cgHtml.readEntry( "UserStyleSheetEnabled", false ) == true ) {
        if ( cgHtml.hasKey( "UserStyleSheet" ) )
            userStyleSheet = cgHtml.readEntry( "UserStyleSheet", "" );
    }
    if ( !userStyleSheet.isEmpty() )
    {
        QWebSettings::globalSettings()->setUserStyleSheetUrl( QUrl( userStyleSheet ) );
    }
    if ( cgHtml.hasKey( "AutoLoadImages" ) )
        QWebSettings::globalSettings()->setAttribute(QWebSettings::AutoLoadImages, cgHtml.readEntry( "AutoLoadImages", true ) );

    QWebSettings::globalSettings()->setFontFamily(QWebSettings::StandardFont , cgHtml.readEntry( "StandardFont", KGlobalSettings::generalFont().family() ) );
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont ,cgHtml.readEntry( "FixedFont", KGlobalSettings::fixedFont().family() ) );
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SerifFont ,cgHtml.readEntry( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT ) );
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::SansSerifFont , cgHtml.readEntry( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT ) );
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::CursiveFont , cgHtml.readEntry( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT ) );
    QWebSettings::globalSettings()->setFontFamily(QWebSettings::FixedFont , cgHtml.readEntry( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT ) );
}
