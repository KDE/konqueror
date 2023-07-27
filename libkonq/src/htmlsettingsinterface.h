/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2010 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef HTMLSETTINGSINTERFACE_H
#define HTMLSETTINGSINTERFACE_H

#include <libkonq_export.h>

#include <QObject>

class QString;

/**
 * @short An interface for modifying the settings of browser engines.
 *
 *  This interface provides a generic means for querying or changing the
 *  settings of browser engines that implement it.
 *
 *  To use this class simply cast an instance of the HTMLExtension object
 *  using qobject_cast<KParts::HtmlSettingsInterface>.
 *
 *  Example:
 *  @code
 *  KParts::HTMLExtension* extension = KParts::HTMLExtension::childObject(part);
 *  KParts::HtmlSettingsInterface* settings = qobject_cast&lt;KParts::HtmlSettingsInterface&gt;(extension);
 *  const bool autoLoadImages = settings->attribute(KParts::AutoLoadImages);
 *  @endcode
 *
 *  @since 4.8.1
 */
class LIBKONQ_EXPORT HtmlSettingsInterface
{
public:
    /**
     * Settings attribute types.
     */
    enum HtmlSettingsType {
        AutoLoadImages,
        DnsPrefetchEnabled,
        JavaEnabled,
        JavascriptEnabled,
        MetaRefreshEnabled,
        PluginsEnabled,
        PrivateBrowsingEnabled,
        OfflineStorageDatabaseEnabled,
        OfflineWebApplicationCacheEnabled,
        LocalStorageEnabled,
        UserDefinedStyleSheetURL,
    };

    /**
     * This enum specifies whether Java/JavaScript execution is allowed.
     *
     * @since 4.8.2
     */
    enum JavaScriptAdvice {
        JavaScriptDunno = 0,
        JavaScriptAccept,
        JavaScriptReject,
    };

    /**
     * This enum specifies the policy for window.open
     *
     * @since 4.8.2
     */
    enum JSWindowOpenPolicy {
        JSWindowOpenAllow = 0,
        JSWindowOpenAsk,
        JSWindowOpenDeny,
        JSWindowOpenSmart,
    };

    /**
     * This enum specifies the policy for window.status and .defaultStatus
     *
     * @since 4.8.2
     */
    enum JSWindowStatusPolicy {
        JSWindowStatusAllow = 0,
        JSWindowStatusIgnore,
    };

    /**
     * This enum specifies the policy for window.moveBy and .moveTo
     *
     * @since 4.8.2
     */
    enum JSWindowMovePolicy {
        JSWindowMoveAllow = 0,
        JSWindowMoveIgnore,
    };

    /**
     * This enum specifies the policy for window.resizeBy and .resizeTo
     *
     * @since 4.8.2
     */
    enum JSWindowResizePolicy {
        JSWindowResizeAllow = 0,
        JSWindowResizeIgnore,
    };

    /**
     * This enum specifies the policy for window.focus
     *
     * @since 4.8.2
     */
    enum JSWindowFocusPolicy {
        JSWindowFocusAllow = 0,
        JSWindowFocusIgnore,
    };

    /**
     * Destructor
     */
    virtual ~HtmlSettingsInterface()
    {
    }

    /**
     * Returns the value of the browser engine's attribute @p type.
     */
    virtual QVariant htmlSettingsProperty(HtmlSettingsType type) const = 0;

    /**
     * Sets the value of the browser engine's attribute @p type to @p value.
     */
    virtual bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant &value) = 0;

    /**
     * A convenience function that returns the javascript advice for @p text.
     *
     * If text is not either "accept" or "reject", this function returns
     * @ref JavaScriptDunno.
     *
     *  @since 4.8.2
     */
    static JavaScriptAdvice textToJavascriptAdvice(const QString &text);

    /**
     * A convenience function Returns the text for the given JavascriptAdvice @p advice.
     *
     * If @p advice is not either JavaScriptAccept or JavaScriptReject, this
     * function returns a NULL string.
     *
     *  @since 4.8.2
     */
    static const char *javascriptAdviceToText(JavaScriptAdvice advice);

    /**
     * A convenience function that splits @p text into @p domain, @p javaAdvice
     * and @p jScriptAdvice.
     *
     * If @p text is empty or does not contain the proper delimiter (':'), this
     * function will set @p domain to @p text and the other two parameters to
     * JavaScriptDunno.
     *
     *  @since 4.8.2
     */
    static void splitDomainAdvice(const QString &text, QString &domain, JavaScriptAdvice &javaAdvice, JavaScriptAdvice &javaScriptAdvice);
};

Q_DECLARE_INTERFACE(HtmlSettingsInterface, "org.kde.KParts.HtmlSettingsInterface")

#endif /* HTMLSETTINGSINTERFACE_H */
