/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "libkonq_utils.h"

#include <KBookmarkManager>
#include <QStandardPaths>

#include <QFileDialog>

using namespace Konq;

KBookmarkManager * Konq::userBookmarksManager()
{
    static const QString bookmarksFile = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror/bookmarks.xml");
    static KBookmarkManager s_manager(bookmarksFile);
    return &s_manager;
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

QString Konq::askDownloadLocation(const QString& suggestedFileName, QWidget* parent, const QString& startingDir)
{
    QFileDialog dlg(parent);
    dlg.setAcceptMode(QFileDialog::AcceptSave);
    dlg.setOption(QFileDialog::DontConfirmOverwrite, false);
    dlg.selectFile(suggestedFileName);
    dlg.setDirectory(!startingDir.isEmpty() ? startingDir : QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    if (dlg.exec() == QDialog::Rejected) {
        return QString();
    }
    return dlg.selectedUrls().first().path();
}
