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

// KDE
#include <dom/dom_string.h>
#include <dom/html_document.h>
#include <dom/html_element.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <khtml_part.h> // this plugin applies to a khtml part
#include "config-kttsplugin.h"
#ifdef HAVE_WEBKITKDE
#include <webkitpart.h>
#ifdef HAVE_WEBVIEW
#include <webview.h>
#else
#include <webkitview.h>
#endif
#include <QWebFrame>
#endif
#include <kicon.h>
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
    if (qobject_cast<KHTMLPart*>(parent)) { // should always be true, but let's make sure
        QAction *action = actionCollection()->addAction( "tools_kttsd" );
        action->setIcon( KIcon("text-speak") );
        action->setText( i18n("&Speak Text") );
        connect(action, SIGNAL(triggered(bool) ), SLOT(slotReadOut()));
    }
}

KHTMLPluginKTTSD::~KHTMLPluginKTTSD()
{
}

void KHTMLPluginKTTSD::slotReadOut()
{
    // The parent is assumed to be a KHTMLPart (checked in constructor)
    KParts::ReadOnlyPart* part = static_cast<KParts::ReadOnlyPart *>(parent());

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"))
    {
        QString error;
        if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error)) {
            KMessageBox::error(part->widget(), error, i18n("Starting KTTSD Failed") );
        }
    }
    // Find out if KTTSD supports xhtml (rich speak).
    bool supportsXhtml = false;
    org::kde::KSpeech kttsd( "org.kde.kttsd", "/KSpeech", QDBusConnection::sessionBus() );
    QString talker = kttsd.defaultTalker();
    QDBusReply<int> reply = kttsd.getTalkerCapabilities2(talker);
    if ( !reply.isValid())
        kDebug() << "D-Bus call getTalkerCapabilities2() failed, assuming non-XHTML support.";
    else
    {
        supportsXhtml = reply.value() & KSpeech::tcCanParseHtml;
    }

    QString query;
    bool hasSelection = false;
    KHTMLPart *compPart = dynamic_cast<KHTMLPart *>(part);
    if ( compPart )
    {
        if (supportsXhtml)
        {
            kDebug() << "KTTS claims to support rich speak (XHTML to SSML).";
            if (hasSelection)
                query = compPart->selectedTextAsHTML();
            else
            {
                // TODO: Fooling around with the selection probably has unwanted
                // side effects, but until a method is supplied to get valid xhtml
                // from entire document..
                // query = part->document().toString().string();
                compPart->selectAll();
                query = compPart->selectedTextAsHTML();
                // Restore no selection.
                compPart->setSelection(compPart->document().createRange());
            }
        } else {
            if (hasSelection)
                query = compPart->selectedText();
            else
                query = compPart->htmlDocument().body().innerText().string();
        }
    }
#ifdef HAVE_WEBKITKDE
    else
    {
        WebKitPart *webkitPart = dynamic_cast<WebKitPart *>(part);
        if ( webkitPart )
        {
            if (supportsXhtml)
            {
                kDebug() << "KTTS claims to support rich speak (XHTML to SSML).";
                if (hasSelection)
                    query = webkitPart->view()->page()->currentFrame()->toHtml();
                else
                {
                    // TODO: Fooling around with the selection probably has unwanted
                    // side effects, but until a method is supplied to get valid xhtml
                    // from entire document..
                    // query = part->document().toString().string();
#if 0
                    webkitPart->selectAll();
                    query = webkitPart->view()->page()->currentFrame()->toHtml();
                    // Restore no selection.
                    webkitPart->setSelection(webkitPart->document().createRange());
#endif
                }
            } else {
                if (hasSelection)
                    query = webkitPart->view()->selectedText();
                else
                    query = webkitPart->view()->page()->currentFrame()->toHtml();
            }
        }

    }
#endif
    // kDebug() << "query =" << query;

    reply = kttsd.say(query, KSpeech::soNone);
    if ( !reply.isValid()) {
        KMessageBox::sorry(part->widget(), i18n("The D-Bus call say() failed."),
                            i18n("D-Bus Call Failed"));
    }
}

K_PLUGIN_FACTORY(KHTMLPluginKTTSDFactory,
                 const KService::List offers = KServiceTypeTrader::self()->query("DBUS/Text-to-Speech", "Name == 'KTTSD'");
                 // If KTTSD is not installed, don't create the plugin at all.
                 if (!offers.isEmpty()) {
                     registerPlugin<KHTMLPluginKTTSD>();
                 }
    )
K_EXPORT_PLUGIN( KHTMLPluginKTTSDFactory( "khtmlkttsd" ) )

#include "khtmlkttsd.moc"
