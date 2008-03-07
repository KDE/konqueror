/***************************************************************************
  Copyright (C) 2002 by George Russell <george.russell@clara.net>
  Copyright (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// Own
#include "khtmlkttsd.h"

// Qt
#include <QtCore/QBuffer>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QtGui/QMessageBox>

// KDE
#include <dom/dom_string.h>
#include <dom/html_document.h>
#include <dom/html_element.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kdebug.h>
#include <khtml_part.h> // this plugin applies to a khtml part
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <kservicetypetrader.h>
#include <kspeech.h>
#include <ktoolinvocation.h>

#include "kspeechinterface.h"

KHTMLPluginKTTSD::KHTMLPluginKTTSD( QObject* parent, const QVariantList& )
    : Plugin( parent )
{
    // If KTTSD is not installed, hide action.
    KService::List offers = KServiceTypeTrader::self()->query("DBUS/Text-to-Speech", "Name == 'KTTSD'");
    if (offers.count() > 0)
    {
        QAction *action = actionCollection()->addAction( "tools_kttsd" );
        action->setIcon( KIcon("preferences-desktop-text-to-speech") );
        action->setText( i18n("&Speak Text") );
        connect(action, SIGNAL(triggered(bool) ), SLOT(slotReadOut()));
    }
    else
        kDebug() << "KHTMLPLuginKTTSD::KHTMLPluginKTTSD: KTrader did not find KTTSD.";
}

KHTMLPluginKTTSD::~KHTMLPluginKTTSD()
{
}

void KHTMLPluginKTTSD::slotReadOut()
{
    // The parent is assumed to be a KHTMLPart
    if ( !parent()->inherits("KHTMLPart") )
       QMessageBox::warning( 0, i18n( "Cannot Read source" ),
                                i18n( "You cannot read anything except web pages with\n"
                                      "this plugin, sorry." ));
    else
    {
        if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"))
        {
            QString error;
            if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error))
                QMessageBox::warning(0, i18n( "Starting KTTSD Failed"), error );
        }
        // Find out if KTTSD supports xhtml (rich speak).
        bool supportsXhtml = false;
        org::kde::KSpeech kttsd( "org.kde.kttsd", "/KSpeech", QDBusConnection::sessionBus() );
        QString talker = kttsd.defaultTalker();
        QDBusReply<int> reply = kttsd.getTalkerCapabilities2(talker);
        if ( !reply.isValid())
            QMessageBox::warning( 0, i18n( "D-Bus Call Failed" ),
                                     i18n( "The D-Bus call getTalkerCapabilities2 failed." ));
        else
        {
            supportsXhtml = reply.value() & KSpeech::tcCanParseHtml;
        }

        KHTMLPart *part = (KHTMLPart *) parent();

        QString query;
        if (supportsXhtml)
        {
            kDebug() << "KTTS claims to support rich speak (XHTML to SSML).";
            if (part->hasSelection())
                query = part->selectedTextAsHTML();
            else
            {
                // TODO: Fooling around with the selection probably has unwanted
                // side effects, but until a method is supplied to get valid xhtml
                // from entire document..
                // query = part->document().toString().string();
                part->selectAll();
                query = part->selectedTextAsHTML();
                // Restore no selection.
                part->setSelection(part->document().createRange());
            }
        } else {
            if (part->hasSelection())
                query = part->selectedText();
            else
                query = part->htmlDocument().body().innerText().string();
        }
        // kDebug() << "KHTMLPluginKTTSD::slotReadOut: query = " << query;

        reply = kttsd.say(query, KSpeech::soNone);
        if ( !reply.isValid())
            QMessageBox::warning( 0, i18n( "D-Bus Call Failed" ),
                                     i18n( "The D-Bus call say() failed." ));
    }
}

K_PLUGIN_FACTORY( KHTMLPluginKTTSDFactory, registerPlugin< KHTMLPluginKTTSD >(); )
K_EXPORT_PLUGIN( KHTMLPluginKTTSDFactory( "khtmlkttsd" ) )

#include "khtmlkttsd.moc"
