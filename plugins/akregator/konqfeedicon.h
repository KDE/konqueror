/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>
    SPDX-FileCopyrightText: 2023 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-exception
*/

#ifndef KONQFEEDICON_H
#define KONQFEEDICON_H

#include <QPointer>
#include <konq_kpart_plugin.h>
#include <QMenu>

#include "asyncselectorinterface.h"

/**
@author Teemu Rytilahti
*/
class KUrlLabel;

namespace KParts
{
class StatusBarExtension;
class ReadOnlyPart;
}

namespace Akregator
{
    /**
     * Class representing a feed
     */
    class Feed
    {
    public:
        /**
         * @brief Default constructor
         */
        Feed() {}

        /**
         * @brief Constructor
         * @param url the feed URL
         * @param title the feed title
         * @param mimeType the feed mime type
         */
        Feed(const QString &url, const QString &title, const QString &mimeType) : m_url(url), m_title(title), m_mimeType(mimeType) {}

        /**
         * @brief The feed URL
         * @return the feed URL
         */
        const QString &url() const
        {
            return m_url;
        }

        /**
         * @brief The feed title
         * @return the feed title
         */
        const QString &title() const
        {
            return m_title;
        }

        /**
         * @brief The feed mime type
         * @return the feed mime type
         */
        const QString &mimeType() const
        {
            return m_mimeType;
        }

    private:
        QString m_url; ///<The feed URL
        QString m_title; ///<The feed title
        QString m_mimeType; ///<The feed mime type
    };

    typedef QList<Feed> FeedList;

/**
 * @brief Class handling detection of feeds in a page and displaying of the feeds icon in the statusbar.
 *
 * Currently, this class needs to support two different APIs for the part:
 * - the old, _synchronous_, KParts::SelectorInterface interface implemented by KHtmlPart and KWebkitPart
 * - the new, _asynchronous_ AsyncSelectorInterface interface implemented by WebEnginePart
 * The only difference in the two cases is in the slot called to update the feeds icon after a page finished
 * loading:
 * - updateFeedIcon() is used for the _synchronous_ API
 * - updateFeedIconAsync() is used for the _asynchronous_ API.
 */
class KonqFeedIcon : public KonqParts::Plugin
{
    Q_OBJECT

public:

    /**
     * @brief Constructor
     */
    KonqFeedIcon(QObject *parent, const QVariantList &args);

    /**
     * @brief Destructor
     */
    ~KonqFeedIcon() override;

private:

    /**
     * @brief Adds the feed icon to the status bar
     */
    void addFeedIcon();

    /**
     * @brief Whether the part current URL is suitable for looking for feeds
     *
     * Unusable URLs are: invalid URLs, URLs with empty scheme and URLs corresponding to local protocols
     * (according to `KProtocolInfo::protocolClass`)
     * @return `true` if the part's current URL can be searched for feeds and `false` otherwise
     */
    bool isUrlUsable() const;

    QMenu* createMenuForFeed(const Feed &feed, QWidget *parent, bool addSection = false);

    static QAction* actionTitleForFeed(const QString &title, QWidget *parent);

    typedef AsyncSelectorInterface::Element Element;
    /**
     * @brief Fills the feeds list from the list of `link` elements found in a page
     * @param linkNodes the list of link nodes contained in the page
     * @note Not all elements in @p linkNodes actually represents a feed: FeedDetector::extractFromLinkTags() will
     * be used to select the ones to use
     */
    void fillFeedList(const QList<Element> &linkNodes);

    /**
     * @brief The part associated with this instance of the plugin
     */
    QPointer<KParts::ReadOnlyPart> m_part;

    /**
     * @brief The widget displaying the feed icon
     */
    KUrlLabel *m_feedIcon;

    /**
     * @brief The status bar extension
     */
    KParts::StatusBarExtension *m_statusBarEx;

    /**
     * @brief The list of feeds found in the page
     */
    FeedList m_feedList;

    /**
     * @brief The popup menu used to add feeds to Akregator
     */
    QPointer<QMenu> m_menu;

private slots:

    /**
     * @brief Displays the context menu
     */
    void contextMenu();

    /**
     * @brief Looks for feed links in the page using the _asynchronous_ interface and adds the feed widget to the status bar if needed
     * @note This function is _asynchronous_
     * @warning This should only be called after making sure the part supports the AsyncSelectorInterface interface
     */
    void updateFeedIconAsync();

    /**
     * @brief Removes the feed widget from the status bar
     */
    void removeFeedIcon();

    /**
     * @brief Adds all the feeds in the current page to Akgregator
     */
    void addAllFeeds();

    void copyFeedUrlToClipboard(const QString &url);

    void openFeedUrl(const QString &url, const QString &mimeType);

    /**
     * @brief Adds the feed with the given URL to Akregator
     * @param url the feed url
     */
    void addFeedToAkregator(const QString &url);
};

}
#endif
