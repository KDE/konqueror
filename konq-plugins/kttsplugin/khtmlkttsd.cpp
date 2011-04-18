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

// Local
#include "kspeechinterface.h"

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kicon.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kparts/part.h>
#include <kparts/textextension.h>
#include <kpluginfactory.h>
#include <kservicetypetrader.h>
#include <kspeech.h>
#include <ktoolinvocation.h>

KHTMLPluginKTTSD::KHTMLPluginKTTSD( QObject* parent, const QVariantList& )
    : Plugin( parent )
{
    KParts::TextExtension* textExt = KParts::TextExtension::childObject(parent);
    if (textExt && qobject_cast<KParts::ReadOnlyPart *>(parent)) {
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
    // The parent is a KParts::ReadOnlyPart (checked in constructor)
    KParts::ReadOnlyPart* part = static_cast<KParts::ReadOnlyPart *>(parent());

    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered("org.kde.kttsd"))
    {
        QString error;
        if (KToolInvocation::startServiceByDesktopName("kttsd", QStringList(), &error)) {
            KMessageBox::error(part->widget(), error, i18n("Starting Jovie Text-to-Speech Service Failed") );
            return;
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
        if (supportsXhtml)
            kDebug() << "KTTS claims to support rich speak (XHTML to SSML).";
    }

    KParts::TextExtension* textExt = KParts::TextExtension::childObject(parent());
    QString query;
    const KParts::TextExtension::Format format = supportsXhtml ? KParts::TextExtension::HTML : KParts::TextExtension::PlainText;
    if (textExt->hasSelection()) {
        query = textExt->selectedText(format);
    } else {
        query = textExt->completeText(format);
    }

    // kDebug() << "query =" << query;

    reply = kttsd.say(query, KSpeech::soNone);
    if ( !reply.isValid()) {
        KMessageBox::sorry(part->widget(), i18n("The D-Bus call say() failed."),
                            i18n("D-Bus Call Failed"));
    }
}

K_PLUGIN_FACTORY(KHTMLPluginKTTSDFactory,
                 const KService::Ptr kttsd = KService::serviceByDesktopName("kttsd");
                 // If KTTSD is not installed, don't create the plugin at all.
                 if (kttsd) {
                     registerPlugin<KHTMLPluginKTTSD>();
                 }
    )
K_EXPORT_PLUGIN( KHTMLPluginKTTSDFactory( "khtmlkttsd" ) )

#include "khtmlkttsd.moc"
