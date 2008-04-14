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
#include <khtmldefaults.h>
#include <QWebSettings>
#include <KDE/KConfigGroup>
#include <KDE/KGlobalSettings>
#include <KDE/KGlobal>
#include <QLinkedList>
#include <KDE/KDebug>
#include <assert.h>

WebKitGlobal *WebKitGlobal::s_self = 0;
unsigned long int WebKitGlobal::s_refcnt = 0;
static QLinkedList<WebKitPart*> *s_parts = 0;

WebKitGlobal::WebKitGlobal()
{
    assert(!s_self);
    s_self = this;
    ref();
}

WebKitGlobal::~WebKitGlobal()
{
    if ( s_self == this )
    {
        if (s_parts) {
            assert(s_parts->isEmpty());
            delete s_parts;
        }
        s_parts = 0;
    }
    else
        deref();
}



void WebKitGlobal::ref()
{
    if ( !s_refcnt && !s_self )
    {
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
    if ( !--s_refcnt && s_self )
    {
        delete s_self;
        s_self = 0;
    }
}

void WebKitGlobal::registerPart( WebKitPart *part )
{
    if ( !s_parts )
        s_parts = new QLinkedList<WebKitPart*>;

    if ( !s_parts->contains( part ) ) {
        s_parts->append( part );
        ref();
    }
}

void WebKitGlobal::deregisterPart( WebKitPart *part )
{
    assert( s_parts );

    if ( s_parts->removeAll( part ) ) {
        if ( s_parts->isEmpty() ) {
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
