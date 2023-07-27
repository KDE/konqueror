/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "htmlsettingsinterface.h"

#include <KLazyLocalizedString>

const char *HtmlSettingsInterface::javascriptAdviceToText(HtmlSettingsInterface::JavaScriptAdvice advice)
{
    // NOTE: The use of kli18n below is intended to allow GUI code to call
    // i18n on the returned text without affecting use of untranslated text in
    // config files.
    switch (advice) {
    case JavaScriptAccept:
        return kli18n("Accept").untranslatedText();
    case JavaScriptReject:
        return kli18n("Reject").untranslatedText();
    default:
        break;
    }

    return nullptr;
}

HtmlSettingsInterface::JavaScriptAdvice HtmlSettingsInterface::textToJavascriptAdvice(const QString &text)
{
    JavaScriptAdvice ret = JavaScriptDunno;

    if (!text.isEmpty()) {
        if (text.compare(QLatin1String("accept"), Qt::CaseInsensitive) == 0) {
            ret = JavaScriptAccept;
        } else if (text.compare(QLatin1String("reject"), Qt::CaseInsensitive) == 0) {
            ret = JavaScriptReject;
        }
    }

    return ret;
}

void HtmlSettingsInterface::splitDomainAdvice(const QString &adviceStr,
                                              QString &domain,
                                              HtmlSettingsInterface::JavaScriptAdvice &javaAdvice,
                                              HtmlSettingsInterface::JavaScriptAdvice &javaScriptAdvice)
{
    const QString tmp(adviceStr);
    const int splitIndex = tmp.indexOf(QLatin1Char(':'));

    if (splitIndex == -1) {
        domain = adviceStr.toLower();
        javaAdvice = JavaScriptDunno;
        javaScriptAdvice = JavaScriptDunno;
    } else {
        domain = tmp.left(splitIndex).toLower();
        const QString adviceString = tmp.mid(splitIndex + 1, tmp.length());
        const int splitIndex2 = adviceString.indexOf(QLatin1Char(':'));
        if (splitIndex2 == -1) {
            // Java advice only
            javaAdvice = textToJavascriptAdvice(adviceString);
            javaScriptAdvice = JavaScriptDunno;
        } else {
            // Java and JavaScript advice
            javaAdvice = textToJavascriptAdvice(adviceString.left(splitIndex2));
            javaScriptAdvice = textToJavascriptAdvice(adviceString.mid(splitIndex2 + 1, adviceString.length()));
        }
    }
}
