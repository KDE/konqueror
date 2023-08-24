/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "konqutils.h"

#include <QJsonObject>

QStringList Konq::serviceTypes(const KPluginMetaData& md)
{
    //TODO KF6: ensure that this entry will still exist
    return md.rawData().value(QLatin1String("KPlugin")).toObject().value(QLatin1String("ServiceTypes")).toVariant().toStringList();
}

//Code copied from kparts/browserrun.cpp (KF5.109) written by David Faure <faure@kde.org>
QUrl Konq::makeErrorUrl(int error, const QString& errorText, const QUrl& initialUrl)
{
    /*
     * The format of the error:/ URL is error:/?query#url,
     * where two variables are passed in the query:
     * error = int kio error code, errText = QString error text from kio
     * The sub-url is the URL that we were trying to open.
     */
    QUrl newURL(QStringLiteral("error:/?error=%1&errText=%2").arg(error).arg(QString::fromUtf8(QUrl::toPercentEncoding(errorText))));

    QString cleanedOrigUrl = initialUrl.toString();
    QUrl runURL(cleanedOrigUrl);
    if (runURL.isValid()) {
        runURL.setPassword(QString()); // don't put the password in the error URL
        cleanedOrigUrl = runURL.toString();
    }

    newURL.setFragment(cleanedOrigUrl);
    return newURL;
}
