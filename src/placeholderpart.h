// This file is part of the KDE project
// SPDX-FileCopyrightText: <year> Stefano Crocco <stefano.crocco@alice.it>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef KONQ_PLACEHOLDERPART_H
#define KONQ_PLACEHOLDERPART_H

#include "konqutils.h"

#include <KParts/ReadOnlyPart>

namespace Konq {

/**
 * @brief A part which displays an empty widget
 *
 * The only scope of this class is to support delayed loading of views, which is
 * used by session loading to avoid having to load many URLs all at once (which
 * is especially slow for web pages). In this case, views create instances of
 * this part instead of the part they would use. This part provides the same
 * interface of a `KParts::ReadOnlyPart`, but both its implementations of openFile()
 * and openUrl() do nothing and its widget is an empty `QWidget`
*/
class PlaceholderPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param parentWidget the part's widget parent widget
     * @param parent the part's parent
     */
    PlaceholderPart(QWidget *parentWidget, QObject *parent=nullptr);

    /**
     * @brief Destructor
     */
    ~PlaceholderPart() override;

    /**
     * @brief Struct containing information about delayed loading
     *
     * This contains all the part-specific information stored in a session config file, so that
     * it can be stored away when the session is loaded and used later, when the view becomes visible.
     *
     * @note History information isn't included here because it's stored in the view itself
     */
    struct DelayedLoadingData {
        ViewType type; //!< The type of the view. Currently, it should be a real mimetype
        QString serviceName; //!< serviceName the part plugin id
        bool openUrl; //!< Whether to open an URL after creating the part
        QUrl url; //!< The URL to load after creating the part
        bool lockedLocation; //!Whether the location should be locked
    };

    /**
     * @brief Stores information about the part the PlaceholderPart stands for
     * @param data The data to store
     */
    void setDelayedLoadingData(const DelayedLoadingData &data){m_delayedLoadingData = data;}

    /**
     * @brief information about the part the PlaceholderPart stands for
     */
    DelayedLoadingData delayedLoadingData() const {return m_delayedLoadingData;}

public slots:
    /**
     * @brief Override of `KParts::ReadOnlyPart::openUrl()`
     *
     * This implementation of `KParts::ReadOnlyPart::openUrl()` doesn't actually open the URL:
     * it only calls `setUrl` and emits the `setWindowCaption` signal
     * @param url the URL to open
     * @return `false` if @p url is invalid and `true` otherwise
     */
    bool openUrl(const QUrl & url) override;

protected:
    /**
     * @brief Override of `KParts::ReadOnlyPart::openFile()`
     *
     * This function does nothing
     * @return `true` if the file exists and `false` otherwise
     */
    bool openFile() override;

private:

   static KPluginMetaData defaultMetadata();

    QWidget *m_widget; //!< The part's widget
    DelayedLoadingData m_delayedLoadingData; //!< Information about the part the PlaceholderPart stands for
};

}

#endif //KONQ_PLACEHOLDERPART_H
