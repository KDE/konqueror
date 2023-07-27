/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BROWSEROPENORSAVEQUESTION_H
#define BROWSEROPENORSAVEQUESTION_H

#include "libkonq_export.h"

#include <KService>
#include <memory>

class BrowserOpenOrSaveQuestionPrivate;

/**
 *
 * @short This class shows the dialog that asks the user whether to
 * save a url or open a url in another application.
 *
 * It also has the variant which asks "save or embed" (e.g. into konqueror).
 *
 */
class LIBKONQ_EXPORT BrowserOpenOrSaveQuestion
{
public:
    /**
     * Constructor, for all kinds of dialogs shown in this class.
     * @param url the URL in question
     * @param mimeType the mimetype of the URL
     */
    BrowserOpenOrSaveQuestion(QWidget *parent, const QUrl &url, const QString &mimeType);
    ~BrowserOpenOrSaveQuestion();

    /**
     * Sets the suggested filename, shown in the dialog.
     * @param suggestedFileName optional file name suggested by the server (HTTP Content-Disposition)
     */
    void setSuggestedFileName(const QString &suggestedFileName);

    /**
     * Set of features that should be enabled in this dialog.
     * This allows to add features before making all applications ready for those features
     * (e.g. applications need to read selectedService() otherwise the dialog should not
     * show the service selection button)
     * @see Features
     */
    enum Feature {
        BasicFeatures = 0, /**< Only the basic save, open, embed, cancel button */
        ServiceSelection = 1, /**< Shows "Open With..." with the associated applications for the mimetype */
    };
    /**
     * Stores a combination of #Feature values.
     */
    Q_DECLARE_FLAGS(Features, Feature)

    /**
     * Enables the given features in the dialog
     */
    void setFeatures(Features features);

    enum Result { Save, Open, Embed, Cancel };

    /**
     * Ask the user whether to save or open a url in another application.
     * @return Save, Open or Cancel.
     */
    Result askOpenOrSave();

    /**
     * @since 5.65
     */
    enum AskEmbedOrSaveFlags {
        InlineDisposition = 0,
        AttachmentDisposition = 1,
    };

    /**
     * Ask the user whether to save or open a url in another application.
     * @param flags set to AttachmentDisposition if suggested by the server
     * This is used because by default text/html files are opened embedded in browsers, not saved.
     * But if the server said "attachment", it means the user is download a file for saving it.
     * @return Save, Embed or Cancel.
     */
    Result askEmbedOrSave(int flags = 0);

    // TODO askOpenEmbedOrSave

    /**
     * @return the service that was selected during askOpenOrSave,
     * if it returned Open.
     * In all other cases (no associated application, Save or Cancel
     * selected), this returns 0.
     *
     * Requires setFeatures(BrowserOpenOrSaveQuestion::ServiceSelection).
     */
    KService::Ptr selectedService() const;

private:
    std::unique_ptr<BrowserOpenOrSaveQuestionPrivate> const d;
    Q_DISABLE_COPY(BrowserOpenOrSaveQuestion)
};

#endif /* BROWSEROPENORSAVEQUESTION_H */
