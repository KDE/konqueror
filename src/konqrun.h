/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KONQRUN_H
#define KONQRUN_H

#include <kparts/browserrun.h>
#include <QPointer>
#include <kservice.h>
#include "konqopenurlrequest.h"
#include <QUrl>

#include <KIO/Global>

class KonqMainWindow;
class KonqView;

class KonqRun : public KParts::BrowserRun
{
    Q_OBJECT
public:
    /**
     * Create a KonqRun instance, associated to the main view and an
     * optional child view.
     */
    KonqRun(KonqMainWindow *mainWindow, KonqView *childView,
            const QUrl &url, const KonqOpenURLRequest &req = KonqOpenURLRequest(),
            bool trustedSource = false);

    ~KonqRun() override;

    /**
     * Returns true if we found the mimetype for the given url.
     */
    bool wasMimeTypeFound() const
    {
        return m_bFoundMimeType;
    }

    KonqView *childView() const;

    const QString &typedUrl() const
    {
        return m_req.typedUrl;
    }

    QUrl mailtoURL() const
    {
        return m_mailto;
    }

protected:
    void foundMimeType(const QString &_type) override;
    void handleError(KJob *job) override;
    void init() override;
    void scanFile() override;

    /**
     * Displays an error page appropriate to the given error code
     *
     * @param error the error code
     * @param stringUrl the string representation of the URL which caused the error
     */
    void switchToErrorUrl(KIO::Error error, const QString &stringUrl);

protected Q_SLOTS:
    void slotRedirection(KIO::Job *, const QUrl &);

private:
    bool tryOpenView(const QString &mimeType, bool associatedAppIsKonqueror);
    bool usingWebEngine() const;

    QPointer<KonqMainWindow> m_pMainWindow;
    QPointer<KonqView> m_pView;
    bool m_bFoundMimeType;
    KonqOpenURLRequest m_req;
    QUrl m_mailto;
    bool m_inlineErrors;
};

#endif // KONQRUN_H
