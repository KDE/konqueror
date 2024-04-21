/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef WEBENGINEPART_PROFILE_H
#define WEBENGINEPART_PROFILE_H

#include <QWebEngineProfile>

namespace KonqWebEnginePart
{
/**
 * @brief Profile for pages created by Konqueror
 * @note If the profile is not off-the-record, according to the documentation, it
 * "should be destroyed on or before application exit". To ensure this, it's
 * recommended to make it a child of the application object.
 */
class Profile : public QWebEngineProfile
{
    Q_OBJECT

public:

    /**
     * Constructor
     *
     * @param storageName the name of the location where to store data
     * @param parent the parent object
     */
    Profile(const QString& storageName, QObject* parent = nullptr);
    ~Profile(); ///< Destructor

    /**
     * @brief A default instance of this class
     *
     * This works as Qt5 `QtWebEngineProfile::defaultProfile()`
     * @return A default instance of this class
     */
    static Profile* defaultProfile();
};

}

#endif // WEBENGINEPART_PROFILE_H
