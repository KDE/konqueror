/* This file is part of the KDE project

    SPDX-FileCopyrightText: 2004 Gary Cramblitt <garycramblitt@comcast.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _AKREGATORPLUGIN_H_
#define _AKREGATORPLUGIN_H_

#include <kabstractfileitemactionplugin.h>

class KFileItem;
class KFileItemListProperties;

namespace Akregator
{

/**
 * Class implementing a plugin which adds an "Add to Akregator" entry to feed files in the file manager
 */
class AkregatorMenu : public KAbstractFileItemActionPlugin
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     */
    AkregatorMenu(QObject *parent, const QVariantList &args);

    /**
     * @brief Destructor
     */
    ~AkregatorMenu() override = default;

    /**
     * @brief Override of `KAbstractFileItemActionPlugin::actions`
     * @param fileItemInfos information about the selected file items.
     * @param parent the parent for the returned actions
     */
    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parent) override;

public slots:
    /**
     * @brief Adds the feed with the given URL to Akregator
     * @param url the url of the feed
     */
    void addFeed(const QString &url);

private:
    /**
     * @brief Whether or not the given `KFileItem` represents a feed URL
     *
     * A `KFileItem` is considered to represent a feed URL if its mimetype is one of those listed in #m_feedMimeTypes.
     * @param item the item describing the URL to check
     * @return `true` if @p item represents a feed and `false` otherwise.
     */
    bool isFeedUrl(const KFileItem &item) const;

private:
    /**
     * @brief A list of suitable mimetypes for feeds
     */
    const QStringList m_feedMimeTypes = {QStringLiteral("application/rss+xml"), QStringLiteral("text/rdf"), QStringLiteral("application/xml")};
};

}

#endif
