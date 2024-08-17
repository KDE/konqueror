/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000-2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konqopenurlrequest_h
#define __konqopenurlrequest_h

#include "konqprivate_export.h"

#include <QStringList>

#include <KParts/NavigationExtension>

#include "browserarguments.h"

namespace KParts {
    class ReadOnlyPart;
}

struct KONQ_TESTS_EXPORT KonqOpenURLRequest {

    KonqOpenURLRequest() = default;

    KonqOpenURLRequest(const QString &url) : typedUrl(url) {}

    KonqOpenURLRequest(const KParts::OpenUrlArguments &_args, const BrowserArguments &_browserArgs,
                       KParts::ReadOnlyPart *_requestingPart) {
        args = _args;
        browserArgs = _browserArgs;
        requestingPart = _requestingPart;
    };

    QString debug() const
    {
#ifndef NDEBUG
        QStringList s;
        if (!browserArgs.frameName.isEmpty()) {
            s << "frameName=" + browserArgs.frameName;
        }
        if (browserArgs.newTab()) {
            s << QStringLiteral("newTab");
        }
        if (!nameFilter.isEmpty()) {
            s << "nameFilter=" + nameFilter;
        }
        if (!typedUrl.isEmpty()) {
            s << "typedUrl=" + typedUrl;
        }
        if (!serviceName.isEmpty()) {
            s << "serviceName=" + serviceName;
        }
        if (followMode) {
            s << QStringLiteral("followMode");
        }
        if (newTabInFront) {
            s << QStringLiteral("newTabInFront");
        }
        if (openAfterCurrentPage) {
            s << QStringLiteral("openAfterCurrentPage");
        }
        if (forceAutoEmbed) {
            s << QStringLiteral("forceAutoEmbed");
        }
        if (tempFile) {
            s << QStringLiteral("tempFile");
        }
        if (userRequestedReload) {
            s << QStringLiteral("userRequestedReload");
        }
        return "[" + s.join(QStringLiteral(" ")) + "]";
#else
        return QString();
#endif
    }

    QString typedUrl; ///< empty if URL wasn't typed manually
    QString nameFilter; ///< like `*.cpp`, extracted from the URL
    QString serviceName; ///< to force the use of a given part (e.g. khtml or kwebkitpart)
    bool followMode = false; ///< `true` if following another view - avoids loops
    bool newTabInFront = false; ///< new tab in front or back (when browserArgs.newTab() == true)
    bool openAfterCurrentPage = false; ///< open the URL after the current tab
    bool forceAutoEmbed = false; ///< if `true`, override the user's FMSettings for embedding
    bool tempFile = false; ///< if true, the URL should be deleted after use
    bool userRequestedReload = false; ///< `args.reload` because the user requested it, not a website
    KParts::OpenUrlArguments args;
    BrowserArguments browserArgs;
    QList<QUrl> filesToSelect; ///< files to select in a konqdirpart
    QString suggestedFileName; ///< The suggested name when saving an URL
    KParts::ReadOnlyPart *requestingPart = nullptr; ///< The part which requested the download of an URL
    bool forceOpen = false; ///< Whether the URL should be opened in an external application, regardless of settings

    static KonqOpenURLRequest null;
};

#endif
