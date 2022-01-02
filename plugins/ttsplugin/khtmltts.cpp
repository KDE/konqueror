/*
    SPDX-FileCopyrightText: 2002 George Russell <george.russell@clara.net>
    SPDX-FileCopyrightText: 2003-2004 Olaf Schmidt <ojschmidt@kde.org>
    SPDX-FileCopyrightText: 2015 Jeremy Whiting <jpwhiting@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
    : KonqParts::Plugin(parent)
{
    KParts::TextExtension *textExt = KParts::TextExtension::childObject(parent);
    if (textExt && qobject_cast<KParts::ReadOnlyPart *>(parent)) {
        m_tts = std::unique_ptr<QTextToSpeech>(new QTextToSpeech);

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

    m_tts->say(query);
}

K_PLUGIN_CLASS_WITH_JSON(KHTMLPluginTTS, "khtmltts.json")

#include "khtmltts.moc"
