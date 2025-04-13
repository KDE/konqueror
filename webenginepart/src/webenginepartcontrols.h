/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WEBENGINEPARTCONTROLS_H
#define WEBENGINEPARTCONTROLS_H

#include <QObject>
#include <QWebEngineCertificateError>

#include <kwebenginepartlib_export.h>

#include "cookies/webenginepartcookiejar.h"

class QWebEngineProfile;
class QWebEngineScript;

class SpellCheckerManager;
class WebEnginePartDownloadManager;
class WebEnginePage;
class NavigationRecorder;

namespace KonqWebEnginePart {
    class CertificateErrorDialogManager;
    class Profile;
}

class KWEBENGINEPARTLIB_EXPORT WebEnginePartControls : public QObject
{
    Q_OBJECT

public:

    static WebEnginePartControls *self();

    ~WebEnginePartControls();

    bool isReady() const;

    void setup(KonqWebEnginePart::Profile *profile);

    SpellCheckerManager* spellCheckerManager() const;

    WebEnginePartDownloadManager* downloadManager() const;

    NavigationRecorder* navigationRecorder() const;

    //TODO KF6: change the return value to void
    bool handleCertificateError(const QWebEngineCertificateError &ce, WebEnginePage *page);

    QString httpUserAgent() const;

    QString defaultHttpUserAgent() const;

    /** @brief Disables WebEnginePage lifecycle state management
     *
     * When lifecycle state management is disabled, the page will always be in the `Active` state instead of
     * switching to the `Frozen` state when possible to reduce resource usage.
     *
     * By default, lifecycle state management is enabled.
     *
     * @warning This function is only meant to be called in tests. In tests, usually we don't want widgets of
     * any kind to be visible. However, if a WebEnginePage isn't associated with a visible view, it'll be switched
     * to the `Frozen` state, which (among other things) prevents it from executing javascript code. In turns, this
     * prevents WebEnginePart from emitting the WebEnginePart::completed signal, which could cause tests to fail.
     * Calling this function prevents the page to be `Frozen`.
     * @note This function doesn't cause pages to automatically switch to the `Active` state, so you should call
     * it before creating any page.
     *
     * @param disable whether lifecycle state management should be disabled or enabled. Note that if this is `true`
     * lifecycle state management is _disabled_, while it is _enabled_ when @p disable is `false`
     */
    void disablePageLifecycleStateManagement(bool disable = true) {m_pageLifecycelManagementEnabled = !disable;}

    /**
     * @brief Whether WebEnginePage lifecycle state management is enabled or not
     *
     * Wen lifecycle state management is disabled, instances of WebEnginePage should never change their state to
     * anything other then `Active`.
     *
     * @return whether WebEnginePage lifecycle state management is enabled or not
     */
    bool isPageLifecycleManagementEnabled() const {return m_pageLifecycelManagementEnabled;}

private slots:
    void reparseConfiguration();
    void setHttpUserAgent(const QString &uaString);

signals:
    void userAgentChanged(const QString &uaString);

    /**
     * @brief Informs pages that the user-defined stylesheet has changed
     * @param script the source of the javascript script to run to update the stylesheet
     */
    void updateStyleSheet(const QString &script);
    void updateBackgroundColor(const QColor &color);

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

    /**
     * @brief Inserts in the profile script collection the scripts described in the `:/scripts.json` file
     *
     * The `:/scripts.json` file should contain information about all scripts needed to be injected in each
     * HTML document (for example, the one used by WebEngineWallet to detect forms).
     *
     * The `:scripts.json` file should have the following structure:
     * @code{.json}
     * {
     *      "first script name": {
     *          "file": "path to first script file",
     *          "injectionPoint": injectionPointAsInt,
     *          "worldId": worldIdAsInt,
     *          "runsOnSubFrames": true
     *      },
     *      "second script name": {
     *         ...
     *      },
     *      ...
     * }
     * @endcode
     * Each script is represented by the one of the inner objects. The corresponding key can be anything and
     * represents a name by which the script can be recognized. It's passed to `QWebEngineScript::setName()`.
     * The keys of the object representing a script have the following meaning:
     * - `"file"`: it's the path of the file containing the actual source code of the script. It usually refers
     * to a `qrc` resource
     * - `"injectionPoint"` (optional): it's the script injection point, as described by `QWebEngineScript::InjectionPoint`.
     *  it should be one of the values of the enum. If not given, the default injection point is used. If an invalid value
     *  is given, the behavior is undefined
     * - `"worldId"` (optional): it's the world id where the script should be registered. It's passed to `QWebEngineScript::setWorldId()`
     *  If not given, the default application world is used. If a negative value is given, the behavior is undefined
     * - `"runsOnSubFrames" (optional): if given, it's passed to `QWebEngineScript::setRunsOnSubFrames`
     */
    void registerScripts();

    /**
     * @brief Creates a QWebEngineScript from it's JSON description and its name
     * @param name the name to give to the script (will be passed to QWebEngineScript::setName()
     * @param obj the JSON object describing the script
     * @return the script object. If script creation fails for any reason, the name of the script will be empty
     * @see registerScripts() for the description of the fields in @p object
     */
    static QWebEngineScript scriptFromJson(const QString &name, const QJsonObject &obj);

    /**
     * @brief Applies the user stylesheet according to the user settings
     *
     * Since QtWebEngine doesn't provide special support for using custom stylesheets, custom stylesheets are
     * applied using a script. This script is inserted in the profile script list so that it's automatically
     * applied to new pages. To update existing pages, the updateStyleSheet() signal is emitted with the code
     * of the script.
     *
     * If the user chose to use the default stylesheet while previously a custom one was in use, the script
     * will delete the old stylesheet.
     */
    void updateUserStyleSheetScript();

    KonqWebEnginePart::Profile *m_profile;
    WebEnginePartCookieJar *m_cookieJar;
    SpellCheckerManager *m_spellCheckerManager;
    WebEnginePartDownloadManager *m_downloadManager;
    KonqWebEnginePart::CertificateErrorDialogManager *m_certificateErrorDialogManager;
    NavigationRecorder *m_navigationRecorder;
    QString m_defaultUserAgent;
    static constexpr const char* s_userStyleSheetScriptName{"apply konqueror user stylesheet"};
    bool m_pageLifecycelManagementEnabled = true;
};

#endif // WEBENGINEPARTCONTROLS_H
