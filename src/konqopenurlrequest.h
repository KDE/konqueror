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

/**
 * @brief Class containing information about how Konqueror should open an URL
 */
struct KONQ_TESTS_EXPORT KonqOpenURLRequest {

    KonqOpenURLRequest() = default; //!< Default constructor

    /**
     * @brief Constructor
     *
     * @param url the string that the user entered in the location bar, from which the URL was created.
     * @note @p url isn't the actual URL which will be opened, and it may not even be a valid URL
     * @see typedUrl
     */
    KonqOpenURLRequest(const QString &url) : typedUrl(url) {}

    /**
     * @brief Constructor
     * @param _args information about how to open the URL
     * @param _browserArgs Konqueror-specific information about how to open the URL
     * @param _requestingPart the part which asked to open an URL
     */
    KonqOpenURLRequest(const KParts::OpenUrlArguments &_args, const BrowserArguments &_browserArgs,
                       KParts::ReadOnlyPart *_requestingPart) {
        args = _args;
        browserArgs = _browserArgs;
        requestingPart = _requestingPart;
    };

    /**
     * @brief A string with debug information about the object
     * @return a string with debug information about the object
     */
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
        if (urlActions().isForced()) {
            QString name;
            switch (browserArgs.urlActions().forcedAction()) {
                case Konq::UrlAction::DoNothing:
                    name = QStringLiteral("do nothing");
                    break;
                case Konq::UrlAction::Save:
                    name = QStringLiteral("save");
                    break;
                case Konq::UrlAction::Open:
                    name = QStringLiteral("open");
                    break;
                case Konq::UrlAction::Embed:
                    name = QStringLiteral("embed");
                    break;
                case Konq::UrlAction::Execute:
                    name = QStringLiteral("execute");
                    break;
                case Konq::UrlAction::UnknownAction: //Avoid compiler error
                    break;
            }
            s << name << QStringLiteral(" forced");
        }
        if (tempFile) {
            s << QStringLiteral("tempFile");
        }
        if (userRequestedReload) {
            s << QStringLiteral("userRequestedReload");
        }
        if (forceMimeType) {
            s << QStringLiteral("forceMimeType");
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
    bool tempFile = false; ///< if true, the URL should be deleted after use
    bool userRequestedReload = false; ///< `args.reload` because the user requested it, not a website
    KParts::OpenUrlArguments args; //!< Information on how to open the URL
    BrowserArguments browserArgs; //!< Konqueror-specific information on how to open the URL
    QList<QUrl> filesToSelect; ///< files to select in a konqdirpart
    QString suggestedFileName; ///< The suggested name when saving an URL
    KParts::ReadOnlyPart *requestingPart = nullptr; ///< The part which requested the download of an URL
    Konq::AllowedUrlActions urlActions() const {return browserArgs.urlActions();}
    void setAllowedUrlActions(const Konq::AllowedUrlActions &actions){browserArgs.setAllowedUrlActions(actions);}
    /**
     * @brief Makes embedding the only allowed action
     */
    void forceEmbed() {
        browserArgs.setAllowedUrlActions({Konq::UrlAction::Embed});
    }
    Konq::UrlAction chosenAction = Konq::UrlAction::UnknownAction; //!< The action the user chose to perform on the URL
    /**
     * @brief Whether or not to enforce the mimetype in #args
     *
     * If this is `false`, `QMimeDatabase` will be used (if possible) to determine the actual mimetype of the URL
     * (in case of a remote URL, this will be done after the URL has been downloaded). If this is `true`, instead,
     * the mimetype returned by \link args #args.mimetype\endlink will always be used.
     */
    bool forceMimeType = false; //!< If `true`, the mimetype set in #args will be always used, even if doesn't correspond

    static KonqOpenURLRequest null; //!< An object representing a default (invalid) request
};

#endif
