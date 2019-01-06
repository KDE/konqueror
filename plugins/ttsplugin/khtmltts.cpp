/***************************************************************************
  Copyright (C) 2002 by George Russell <george.russell@clara.net>
  Copyright (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
  Copyright (C) 2015 Jeremy Whiting <jpwhiting@kde.org>
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
#include "khtmltts.h"

// Qt
#include <QAction>
#include <QIcon>
#include <QTextToSpeech>

// KDE
#include <KActionCollection>
#include <KLocalizedString>
#include <KParts/ReadOnlyPart>
#include <kparts/textextension.h>
#include <kpluginfactory.h>

KHTMLPluginTTS::KHTMLPluginTTS(QObject *parent, const QVariantList &)
    : Plugin(parent)
{
    KParts::TextExtension *textExt = KParts::TextExtension::childObject(parent);
    if (textExt && qobject_cast<KParts::ReadOnlyPart *>(parent)) {
        QAction *action = actionCollection()->addAction(QStringLiteral("tools_tts"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("text-speak")));
        action->setText(i18n("&Speak Text"));
        connect(action, SIGNAL(triggered(bool)), SLOT(slotReadOut()));
    }
}

KHTMLPluginTTS::~KHTMLPluginTTS()
{
}

void KHTMLPluginTTS::slotReadOut()
{
    KParts::TextExtension *textExt = KParts::TextExtension::childObject(parent());
    QString query;
    const KParts::TextExtension::Format format = KParts::TextExtension::PlainText;
    if (textExt->hasSelection()) {
        query = textExt->selectedText(format);
    } else {
        query = textExt->completeText(format);
    }

    QTextToSpeech tts;
    tts.say(query);
}

K_PLUGIN_FACTORY(KHTMLPluginTTSFactory, registerPlugin<KHTMLPluginTTS>();)

#include "khtmltts.moc"
