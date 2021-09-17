/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2017 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQ_CLIENT_REQUEST_H
#define KONQ_CLIENT_REQUEST_H

#include <QScopedPointer>

class KonqClientRequestPrivate;
class QUrl;

/**
 * KonqClientRequest talks to a running Konqueror process, or starts a new one,
 * in order to create a new window or tab for a URL.
 *
 * Usage: each instance of KonqClientRequest is a separate request.
 */
class KonqClientRequest
{
public:
    /**
     * Creates a KonqClientRequest instance
     */
    KonqClientRequest();
    /**
     * Destroys this KonqClientRequest instance
     */
    ~KonqClientRequest();

    /**
     * Sets the URL to open (mandatory)
     */
    void setUrl(const QUrl& url);
    /**
     * Sets whether to open the URL in a new tab (optional, defaults to false)
     */
    void setNewTab(bool newTab);
    /**
     * Sets whether the URL is a temp file that should be deleted after usage (optional, defaults to false)
     */
    void setTempFile(bool tempFile);
    /**
     * Sets the MIME type of the URL (optional)
     */
    void setMimeType(const QString &mimeType);

    /**
     * This is the main method, call it to trigger the opening of the URL,
     * once you have called all the relevant setters.
     */
    bool openUrl();

private:
    Q_DISABLE_COPY(KonqClientRequest)

    QScopedPointer<KonqClientRequestPrivate> d;
};

#endif
