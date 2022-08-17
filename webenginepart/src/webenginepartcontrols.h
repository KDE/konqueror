/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WEBENGINEPARTCONTROLS_H
#define WEBENGINEPARTCONTROLS_H

#include <QObject>
#include <QWebEngineCertificateError>

class QWebEngineProfile;
class WebEnginePartCookieJar;
class SpellCheckerManager;
class WebEnginePartDownloadManager;
class WebEnginePage;
class NavigationRecorder;

namespace KonqWebEnginePart {
    class CertificateErrorDialogManager;
}

class WebEnginePartControls : public QObject
{
    Q_OBJECT

public:

    static WebEnginePartControls *self();

    ~WebEnginePartControls();

    bool isReady() const;

    void setup(QWebEngineProfile *profile);

    SpellCheckerManager* spellCheckerManager() const;

    WebEnginePartDownloadManager* downloadManager() const;

    NavigationRecorder* navigationRecorder() const;

    bool handleCertificateError(const QWebEngineCertificateError &ce, WebEnginePage *page);

private slots:
    void reparseConfiguration();

private:

    WebEnginePartControls();

    /**
     * @brief Constructs an `Accept-Language` http header from the language settings
     *
     * The accepted languages are determined according to the following sources:
     * - the language settings in the application itself (using the `Application Language...` menu entry)
     * - the language settings in `SystemSettings`
     * - the locale returned by `QLocale::system()`
     *
     * These sources are tried in order: the first to return a valid value is used. If all return an invalid
     * value (which shouldn't happen), an empty string is returned.
     * @return a suitable `AcceptLanguage` header according to the user preferences
     */
    QString determineHttpAcceptLanguageHeader() const;

    QWebEngineProfile *m_profile;
    WebEnginePartCookieJar *m_cookieJar;
    SpellCheckerManager *m_spellCheckerManager;
    WebEnginePartDownloadManager *m_downloadManager;
    KonqWebEnginePart::CertificateErrorDialogManager *m_certificateErrorDialogManager;
    NavigationRecorder *m_navigationRecorder;
};

#endif // WEBENGINEPARTCONTROLS_H
