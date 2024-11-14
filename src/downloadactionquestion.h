/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef DOWNLOADACTIONQUESTION_H
#define DOWNLOADACTIONQUESTION_H

#include <KService>
#include <KPluginMetaData>

#include <QDialog> //For QDialog::Rejected

#include <memory>

class KConfigGroup;

class DownloadActionQuestionPrivate;

/**
 *
 * @short Class used to decide what should be done when downloading a file
 *
 * The possible actions are:
 * - save the file
 * - open the file in an external application (either the default one or one chosen by the user)
 * - embed the file in Konqueror (either using the default part or one chosen by the user)
 * - do nothing and don't download the file.
 *
 * Users of this class can restrict the kind of action which can be chosen.
 *
 * To decide what to do, this class first looks at the configuration files to see whether the user
 * has chosen an action to always carry out for this mimetype. If this is not the case, it shows
 * a dialog where the user can decide what to do.
 *
 * @note In some situations, the file will always be embedded (if embedding is among the available actions).
 * See autoEmbedMimeType() for more information
 *
 */
class DownloadActionQuestion
{
public:
    /**
     * Constructor, for all kinds of dialogs shown in this class.
     * @param url the URL in question
     * @param mimeType the mimetype of the URL
     * @param skipDefaultHtmlPart if @p mimetype is `text/html`, skip the preferred part and use
     * the second preferred one
     */
    DownloadActionQuestion(QWidget *parent, const QUrl &url, const QString &mimeType, bool skipDefaultHtmlPart = false);

    /**
     * @brief destructor
     */
    ~DownloadActionQuestion();

    /**
     * Sets the suggested filename, shown in the dialog.
     * @param suggestedFileName optional file name suggested by the server (HTTP Content-Disposition)
     */
    void setSuggestedFileName(const QString &suggestedFileName);

    /**
     * @brief Enum describing the possible actions to carry out
     *
     *
     * @internal
     * @note It's important that the entry with the same value as `QDialog::Rejected` is
     * `Cancel`, as these entries are passed to `QDialog::done()` and a value of
     * `QDialog::Rejected` means that the user chose the Cancel button.
     */
    enum class Action {
        Cancel = QDialog::Rejected, //!< Don't download the file and do nothing
        Save = 1, //!< Save the file to disk
        Open = 2, //!< Download the file and open it in an external application
        Embed = 4 //!< Download the file and display it in a part inside Konqueror
    };
    Q_DECLARE_FLAGS(Actions, Action)

    /**
     * Enum giving more information about what to do with the file
     */
    enum EmbedFlags {
        InlineDisposition = 0, //!< The web page stated that the file can be displayed in the web page or as the web page
        AttachmentDisposition = 1, //!< The web page stated that the file should be downloaded
        ForceDialog = 2 //!< Always be display the dialog, regardless of user settings or auto embedding
    };

    /**
     * @brief Asks this object what to do with the URL
     *
     * Calling this function trigger the following algorithm:
     * - if the URL can be embedded automatically, #Embed is returned
     * - if the user has chosen not to ask again for this file type, and the chosen action is allowed, that action is returned
     * - in all other cases, a dialog is displayed so that the user can choose what to do
     *
     * If this function returns #Open, you can use selectedService() to determine which service to use; in the same way,
     * if it returns #Embed, you can determine the part to use by calling selectedPart().
     *
     * @param actions the possible actions which can be chosen. Usually, this will contain Action::Save and at least one of Action::Open
     * or Action::Embed
     * @param flag additional information about how to handle the URL
     * @return the action to perform on the URL. Only the actions in @actions can be returned
     * @note If only Action::Embed is given but there are no parts for the mimetype, Action::Save will be enabled.
     * @note If only Action::Save is given, no dialog will be shown (even if @p flag is ForceDialog). This is because the dialog would
     * only contain the Save and the Cancel button, but since a "Save as" dialog will usually be shown by the caller after the user
     * presses the Save button, the user can cancel the operation using that dialog.
     */
    Action ask(Actions actions, EmbedFlags flag = InlineDisposition);

    /**
     * @overload
     *
     * Convenience overload of ask(Action s, EmbedFlags) which allows all actions. Equivalent to calling
     * @code
     * ask(Action::Save|Action::Embed|Action::Open, flag)
     * @endcode
     */
    Action ask(EmbedFlags flag = InlineDisposition);

    /**
     * @brief Asks the user whether to save the URL or open it in an external application
     *
     * This is a convenience wrapper for ask(Actions, EmbedFlags) which only allows saving and opening. Equivalent to calling
     * @code
     * ask(Action::Save|Action::Open)
     * @endcode
     * @return Action::Save, Action::Open or Action::Cancel.
     */
    Action askOpenOrSave();

    /**
     * @brief Asks the user whether to save the URL or embed it in Konqueror
     *
     * This is a convenience wrapper for ask(Actions, EmbedFlags) which only allows embedding and opening. Equivalent to calling
     * @code
     * ask(Action::Save|Action::Embed, flags)
     * @endcode
     * @return Action::Save, Action::Embed or Action::Cancel.
     */
    Action askEmbedOrSave(EmbedFlags flags = InlineDisposition);

    /**
     * @return the service that was selected during askOpenOrSave,
     * if it returned Open.
     * In all other cases (no associated application, Save or Cancel
     * selected), this returns 0.
     *
     * Requires setFeatures(BrowserOpenOrSaveQuestion::ServiceSelection).
     */

    /**
     * @brief The service to use to open the URL
     *
     * @return the service representing the selected application if ask() returned #Open and an application was chosen
     * and `nullptr` otherwise
     */
    KService::Ptr selectedService() const;

    /**
     * @brief The metadata of the part to use to embed the URL in Konqueror
     * @return the object representing the metatata of the part to use if ask() returned #Embed an an invalid `KPluginMetaData`
     * otherwise
     */
    KPluginMetaData selectedPart() const;

    /**
     * @brief Whether the user was shown a dialog to decide what to do
     *
     * @return `true` if the user was shown a dialog to decide what to do and `false` if the answer was determined automatically
     */
    bool dialogShown() const;

private:

    /**
     * @brief Whether to automatically embed the URL, regardless of user settings
     *
     * An URL is automatically embedded in the following cases:
     * - it's a local file
     * - it's an image
     * - its mimetype inherits one of the following: `text/html`, `application/xml`, `inode/directory`,
     * `multipart/x-mixed-replace`, `multipart/replace`
     *
     * @param flags more information about the download request
     * @return whether or not to automatically embed the URL. If @p flags is #AttachmentDisposition
     * or #ForceDialog, it always return `false`
     */
    bool autoEmbedMimeType(EmbedFlags flags);

    /**
     * @brief Determines what to do for the given mimetype
     *
     * The action to carry out is determined according to:
     * - whether the user chose to embed or open the mimetype
     * - whether or not the user chose to be asked whether to embed/open or save
     * - the available actions
     * - the value of @p flag
     *
     * The action is decided according to the following algorithm:
     * - if @p actions is exactly Action::Save (no other actions allowed), then Action::Save is
     *   always returned
     * - if @p flag is #ForceDialog or if there aren't available actions, `std::nullopt` is returned
     * - if the user chose to automatically save the URL and `Action::Save` is available, `Action::Save` is returned
     * - if `Action::Save` is not available, `Action::Embed` or `Action::Open` is returned (according to the user configuration)
     *  if that action is available
     * - in all other cases, `std::nullopt` is returned, so that the user can decide what to do
     *
     * @note There are a few spacial cases:
     * - if the URL represents a local file, `Action::Save` is considered not available, unless there aren't other
     * available actions
     * - if there aren't applications able to handle the mimetype, `Open` is considered not to be available
     *
     * @param requestedActions the actions requested by the caller. It differs from `d->availableActions` if no parts
     *  exist for the mimetype
     * @param flag more information about how to handle the URL
     * @param cg the configuration group to read the settings from
     * @warning You can only call this method after calling DownloadActionQuestionPrivate::setup, as it needs several
     * fields which are created there
     * @return the action to perform or `std::nullopt` in case the dialog must be shown
     */
    std::optional<Action> determineAction(Actions requestedActions, DownloadActionQuestion::EmbedFlags flag, const KConfigGroup& cg);

    /**
     * @brief The key for the config file option determining whether to display a dialog for the given mode
     * @param action the action for which to determine the key. It can only be \link BrowserOpenOrSaveQuestion::Action::Open Open\endlink
     * or Action::Embed.
     * @return the name of the key or an empty string if @p action is Action::Save or Action::Cancel
     */
    QString dontAskAgainKey(Action action) const;

private:

    /**
     * @brief Returns the automatic action chosen by the user for the mimetype and action type
     *
     * Depending from the value of @p action, this function reads the `askEmbedOrSave` or the `askSave` entry from
     * the given configuration group and returns the action to automatically perform or `std::nullopt` if the user
     * chose to be shown the dialog instead.
     *
     * @param cg the configuration group to read the settings from
     * @param action the action to read the settings from. It must be either Action::Open or Action::Embed
     * @return a `std::optional` with value Action::Save if the user chose to automatically save the URL,
     * a `std::optional` with the same value as @p action if the user chose to automatically embed or open
     * the URL and `std::nullopt` if the user chose to be shown the dialog every time
     */
    std::optional<Action> readDontAskAgainEntry(const KConfigGroup &cg, Action action) const;

private:
    std::unique_ptr<DownloadActionQuestionPrivate> const d; //!< The `d` pointer
    Q_DISABLE_COPY(DownloadActionQuestion)

#ifdef BUILD_DOWNLOAD_ACTION_QUESTION_TESTS
    friend class DownloadActionQuestionTest;

    /**
     * @brief Allow tests to access `d->setup()`
     * @warning To only be used by tests
     *
     * This function takes the same arguments as DownloadActionQuestionPrivate::setup()
     */
    void setupDialog(Actions actions);

    /**
     * @brief Allow tests to access the dialog used by this object
     * @warning To only be used by tests
     *
     * @return #d
     */
    QDialog* dialog();

    /**
     * @brief Allow tests to specify the parts to use instead of those returned by `KParts::PartLoader::partsForMimeType()`
     *
     * @param parts the metadata of the parts to use
     */
    void setParts(const QList<KPluginMetaData> &parts);

    /**
     * @brief Allow tests to specify the services to use instead of those returned by `KFileItemActions::associatedApplications()`
     *
     * @param apps the list of KService::Ptr to use
     */
    void setApps(const KService::List &apps);
#endif

};

Q_DECLARE_OPERATORS_FOR_FLAGS(DownloadActionQuestion::Actions)

/**
 * @brief Overload of operator `~` for Action
 *
 * It treats the action as an int and returns its bitwise negation
 */
int operator~(DownloadActionQuestion::Action action);

QDebug operator<<(QDebug dbg, DownloadActionQuestion::Action action);

#endif /* DOWNLOADACTIONQUESTION_H*/
