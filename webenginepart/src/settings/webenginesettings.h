/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef WEBENGINESETTINGS_H
#define WEBENGINESETTINGS_H

class KConfig;
class KConfigGroup;

#include <QColor>
#include <QStringList>
#include <QPair>

#include <KParts/HtmlExtension>
#include <KParts/HtmlSettingsInterface>

struct KPerDomainSettings;
class WebEngineSettingsPrivate;

/**
 * Settings for the HTML view.
 */
class WebEngineSettings
{
public:

    enum KAnimationAdvice {
        KAnimationDisabled=0,
        KAnimationLoopOnce,
        KAnimationEnabled
    };

    enum KSmoothScrollingMode {
        KSmoothScrollingDisabled=0,
        KSmoothScrollingWhenEfficient,
        KSmoothScrollingEnabled
    };

    /**
     * Called by constructor and reparseConfiguration
     */
    void init();

    /**
     * Destructor. Don't delete any instance by yourself.
     */
    virtual ~WebEngineSettings();

    void computeFontSizes(int logicalDpi);
    bool zoomToDPI() const;
    void setZoomToDPI(bool b);

    // Automatic page reload/refresh...
    bool autoPageRefresh() const;

    bool isOpenMiddleClickEnabled();

    // Java and JavaScript
    bool isJavaEnabled( const QString& hostname = QString() ) const;
    bool isJavaScriptEnabled( const QString& hostname = QString() ) const;
    bool isJavaScriptDebugEnabled( const QString& hostname = QString() ) const;
    bool isJavaScriptErrorReportingEnabled( const QString& hostname = QString() ) const;
    bool isPluginsEnabled( const QString& hostname = QString() ) const;
    bool isLoadPluginsOnDemandEnabled() const;
    bool isInternalPluginHandlingDisabled() const;

    // AdBlocK Filtering
    bool isAdFiltered( const QString &url ) const;
    bool isAdFilterEnabled() const;
    bool isHideAdsEnabled() const;
    void addAdFilter( const QString &url );
    QString adFilteredBy( const QString &url, bool *isWhiteListed = nullptr ) const;

    // Access Keys
    bool accessKeysEnabled() const;

    // Favicons
    bool favIconsEnabled() const;

    KParts::HtmlSettingsInterface::JSWindowOpenPolicy windowOpenPolicy( const QString& hostname = QString() ) const;
    KParts::HtmlSettingsInterface::JSWindowMovePolicy windowMovePolicy( const QString& hostname = QString() ) const;
    KParts::HtmlSettingsInterface::JSWindowResizePolicy windowResizePolicy( const QString& hostname = QString() ) const;
    KParts::HtmlSettingsInterface::JSWindowStatusPolicy windowStatusPolicy( const QString& hostname = QString() ) const;
    KParts::HtmlSettingsInterface::JSWindowFocusPolicy windowFocusPolicy( const QString& hostname = QString() ) const;

    QString settingsToCSS() const;
    QString userStyleSheet() const;

    // Form completion
    bool isFormCompletionEnabled() const;
    int maxFormCompletionItems() const;

    // Meta refresh/redirect (http-equiv)
    bool isAutoDelayedActionsEnabled () const;

    // CookieJar...
    bool isCookieJarEnabled() const;
    
    // Cross domain cookies
    bool acceptCrossDomainCookies() const;

    // Password storage...
    bool isNonPasswordStorableSite(const QString &host) const;
    void addNonPasswordStorableSite(const QString &host);
    void removeNonPasswordStorableSite(const QString &host);
    bool askToSaveSitePassword() const;

    // Mixed content
    bool alowActiveMixedContent() const;
    bool allowMixedContentDisplay() const;

    // Global config object stuff.
    static WebEngineSettings* self();

private:
    /**
     * Read settings from @p config.
     * @param config is a pointer to KConfig object.
     * @param reset if true, settings are always set; if false,
     *  settings are only set if the config file has a corresponding key.
     */
    void init( KConfig * config, bool reset = true );

    // Behavior settings
    bool changeCursor() const;
    bool underlineLink() const;
    bool hoverLink() const;
    bool allowTabulation() const;
    bool autoSpellCheck() const;
    KAnimationAdvice showAnimations() const;
    KSmoothScrollingMode smoothScrolling() const;
    bool zoomTextOnly() const;

    // Font settings
    QString stdFontName() const;
    QString fixedFontName() const;
    QString serifFontName() const;
    QString sansSerifFontName() const;
    QString cursiveFontName() const;
    QString fantasyFontName() const;

    // these two can be set. Mainly for historical reasons (the method in KHTMLPart exists...)
    void setStdFontName(const QString &n);
    void setFixedFontName(const QString &n);

    int minFontSize() const;
    int mediumFontSize() const;

    bool jsErrorsEnabled() const;
    void setJSErrorsEnabled(bool enabled);

    const QString &encoding() const;

    bool followSystemColors() const;

    // Color settings
    const QColor& textColor() const;
    const QColor& baseColor() const;
    const QColor& linkColor() const;
    const QColor& vLinkColor() const;

    // Autoload images
    bool autoLoadImages() const;
    bool unfinishedImageFrame() const;

     /**
      * reads from @p config's current group, forcing initialization
      * if @p reset is true.
      * @param config is a pointer to KConfig object.
      * @param reset true if initialization is to be forced.
      * @param global true if the global domain is to be read.
      * @param pd_settings will be initialised with the computed (inherited)
      *     settings.
      */
    void readDomainSettings(const KConfigGroup &config, bool reset,
                            bool global, KPerDomainSettings &pd_settings);


    QList< QPair< QString, QChar > > fallbackAccessKeysAssignments() const;

    // Whether to show passive popup when windows are blocked
    void setJSPopupBlockerPassivePopup(bool enabled);
    bool jsPopupBlockerPassivePopup() const;

 
    QString lookupFont(int i) const;

    void initWebEngineSettings();
    void initCookieJarSettings();
    void initNSPluginSettings();

    /**
     * @internal Constructor
     */
    WebEngineSettings();

    WebEngineSettingsPrivate* const d;
};

#endif
