/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef WEBENGINEWALLET_H
#define WEBENGINEWALLET_H

#include <QObject>
#include <QString>
#include <QList>
#include <QPair>
#include <QUrl>
#include <QWidget>
#include <QtGlobal>
#include <QDebug>
#include <QWebEnginePage>
#include <QMap>

#include "settings/webenginesettings.h"

class WebEnginePage;
class WebEnginePart;
class QWebEngineScript;

/**
 * @brief Class which allows WebEnginePart to cache form data in `KWallet`
 *
 */
class WebEngineWallet : public QObject
{
    Q_OBJECT

public:

    /**
     * Holds data from a HTML &lt;form&gt; element.
     */
    struct WebForm {

        /**
         * @brief Enum describing a field type
         */
        enum class WebFieldType {
            Text, ///< The field has type @e text
            Password, ///< The field has type @e password
            Email, ///< The field has type @e email
            Other ///< The field has a type not described by other values
        };

        /**
         * @brief The WebFieldType corresponding to a given field type name
         *
         * @param name the name of the field type. It must be lowercase
         * @return The WebForm::WebFieldType value corresponding to @p name. If @p name is not @e text, @e password or @e email, WebFieldType::Other is returned
         * @note For efficiency, this function requires @p name to be lowercase
         */
        static WebFieldType fieldTypeFromTypeName(const QString &name);

        /**
         * @brief A string representation of the type of a web field
         *
         * @param type the type of the field
         * @param localized whether or not the returned type should be localized
         * @return a string with the name of the type @type. If @p localized is @b true, `i18n` will be used to localize the string
         */
        static QString fieldNameFromType(WebFieldType type, bool localized = false);

        /**
         * @brief Struct describing a field inside of a web form
         */
        struct WebField {
            QString name; ///< The name of the field
            QString id; ///< The id of the field
            WebFieldType type; ///< The type of the field
            bool readOnly; ///< Whether the field is read-only
            bool disabled; ///< Whether the field is disabled
            bool autocompleteAllowed; ///< Whether the autocomplete attribute of the field is on or off
            QString value; ///< The value of the field
            QString label; ///< The HTML label associated with the field

        /**
         * @brief Whether the field can be automatically filled or not
         *
         * A field can be automatically filled if all these conditions are true:
         * - it is @b not read-only
         * - it is @b not disabled
         * - autocomplete is @b on
         *
         * @return @b true if the field can be automatically filled and @b false otherwise
         * @note The user can request that a field is cached (and so filled) even if this function returns false
         */
            bool isAutoFillable() const {return !readOnly && !disabled && autocompleteAllowed;}
        };

        /**
         * @brief Removes all fields which aren't automatically fillable from the forms
         *
         * After calling this function, \link fields \endlink will only contain fields for which WebField::isAutoFillable() returns @b true.
         * If none of the fields is automatically fillable, #fields will be empty.
         */
        void deleteNotAutoFillableFields();

        /**
         * @brief Returns a copy of this object which only contains auto fillable fields
         *
         * @return a copy of this object which only contains fields for which WebField::isAutoFillable() returns @b true
         */
        WebForm withAutoFillableFieldsOnly() const;

        /**
         * @brief Whether the form contains fields of type @e password
         *
         * @return @b true if at least one of the #fields has type @e password and @b false otherwise
         */
        bool hasPasswords() const;

        /**
         * @brief Whether the form has autofillable fields
         *
         * @return @b true if at least one of the #fields can be automatically filled according to WebField::isAutoFillable() and @b false otherwise
         */
        bool hasAutoFillableFields() const;

        /**
         * @brief Whether any of the writable fields in the form have non-empty values
         *
         * @return @b true if at least one of the #fields is not read-only and has a non-empty value
         */
        bool hasFieldsWithWrittenValues() const;

        /**
         * @brief Creates a WebEngineSettings::WebFormInfo from this object
         *
         * @return a WebEngineSettings::WebFormInfo having the same name, frame path and fields names as this object
         */
        WebEngineSettings::WebFormInfo toSettingsInfo() const;

        /// @brief The URL the form was found at.
        QUrl url;

        ///@brief The name attribute of the form.
        QString name;

        /// @brief The position of the form on the web page, relative to other forms.
        QString index;

        /** @brief The path of the frame the form belongs to relative to the toplevel window (in the javascript sense).
         *
         * This is stored as a string containing a javascript array (it is passed as is to javascript code, so no need to store it in C++ format
         */
        QString framePath;

        /// @brief The name and value attributes of each input element in the form.
        QVector<WebField> fields;
    };

    /**
     * @brief A list of web forms
     */
    typedef QVector<WebForm> WebFormList;

    /**
     * @brief Constructs a WebEngineWebWallet
     *
     * @p parent is usually the WebEnginePage this wallet is being used for.
     *
     * The @p wid parameter is used to tell the KWallet manager which window
     * is requesting access to the wallet.
     *
     * @param parent  the owner of this wallet
     * @param wid     the window ID of the window the web page will be
     *                embedded in
     */
    explicit WebEngineWallet(WebEnginePart *parent = nullptr, WId wid = 0);

    /**
     * @brief Destructor
     */
    ~WebEngineWallet() override;

    /**
     * @brief Whether the wallet is open or not
     * @return @b true if the wallet is open and @b false otherwise
     */
    bool isOpen() const;

    /**
     * @brief Attempts to save the form data from @p page and its children frames.
     *
     * You must connect to the @ref saveFormDataRequested signal and call either
     * @ref rejectSaveFormDataRequest or @ref acceptSaveFormDataRequest signals
     * in order to complete the save request. Otherwise, you request will simply
     * be ignored.
     *
     * Note that this function is asynchronous, as it requires running javascript code
     * on the page using QWebEnginePage::runJavaScript. This function only requests
     * for the form data to be saved when QWebEnginePage::runJavaScript finishes.
     */
    void saveFormData(WebEnginePage *page, const WebFormList &allForms, bool force = false);

    /**
     * @brief Attempts to fill forms contained in @p page with cached data.
     *
     * Note that this function is asynchronous, as it requires running javascript code
     * on the page using QWebEnginePage::runJavaScript. This function only requests
     * for the form data to be filled when QWebEnginePage::runJavaScript finishes.
     */
    void fillFormData(WebEnginePage *page, const WebFormList &allForms);

    /**
     * @brief Removes the form data specified by @p forms from the persistent storage.
     *
     * Note that this function will remove all cached data for forms found in @p page.
     *
     * Note that this function is asynchronous, as it requires running javascript code
     * on the page using QWebEnginePage::runJavaScript. This function only requests
     * for the form data to be removed when QWebEnginePage::runJavaScript finishes.
     */
    void removeFormData(WebEnginePage *page);

    /**
     * @brief Removes the form data specified by @p forms from the persistent storage.
     *
     * @param forms The forms to remove
     * @see formsWithCachedData
     */
    void removeFormData(const WebFormList &forms);
    
    /**
     * @brief the forms to save for the given key
     *
     * @param key the key associated with the page whose forms should be returned. It's the same key passed by the saveFormDataRequested() signal
     * @return the forms associated with the page corresponding to the given key
     */
    WebFormList pendingSaveData(const QString &key);

    /**
     * @brief Whether the user has customized which forms should be cached for the given URL
     *
     * @param url the URL to check
     * @return @b true if the user has customized the forms to cache for @p URL and false if the forms to cache for @p URL are detected automatically
     */
    static bool hasCustomizedCacheableForms(const QUrl &url);

public Q_SLOTS:
    /**
     * @brief Accepts the save form data request associated with @p key.
     *
     * The @p key parameter is the one sent through the @ref saveFormDataRequested
     * signal.
     *
     * You must always call this function or @ref rejectSaveFormDataRequest in
     * order to complete the save form data request. Otherwise, the request will
     * simply be ignored.
     *
     * @see saveFormDataRequested.
     */
    void acceptSaveFormDataRequest(const QString &key);

    /**
     * @brief Rejects the save form data request associated with @p key.
     *
     * The @p key parameter is the one sent through the @ref saveFormDataRequested
     * signal.
     *
     * @see saveFormDataRequested.
     */
    void rejectSaveFormDataRequest(const QString &key);

    /**
     * @brief Detects the form in the given WebEnginePage and fills its form with cached data
     *
     * It usually is called in response to the <a href="https://doc.qt.io/qt-5/qwebenginepage.html#loadFinished">WebEnginePage::loadFinished</a> signal.
     *
     * @param page the WebEnginePage whose forms should be filled
     */
    void detectAndFillPageForms(WebEnginePage *page);

    /**
     * @brief Caches the contents of the fields
     *
     * @param page the page whose forms should be saved. If @b nullptr, nothing is done
     */
    void saveFormsInPage(WebEnginePage *page);

    /**
     * @brief Displays a dialog where the user can choose which fields in the current page should be cached
     *
     * If the user asks for it, besides saving the choice of fields, this function also immediately stores the field contents in the cache.
     * @note If the fields contents are cached immediately, the old cached contents is overwritten without confirmation.
     *
     * @param page the page for which to choose the forms to cache
     * @param widget the widget to use as the dialog parent
     */
    void customizeFieldsToCache(WebEnginePage *page, QWidget *widget = nullptr);

    /**
     * @brief Remove the customized list of forms to cache for a given URL
     *
     * @param url the URL of the page whose customization should be removed
     * @note This function doesn't remove the cached information but only the list of which forms should be cached for the given URL
     */
    void removeCustomizationForPage(const QUrl &url);

    /**
     * @brief Immediately caches the form data of the given page
     *
     * @param page the page whose form data should be cached
     */
    void savePageDataNow(WebEnginePage *page);

Q_SIGNALS:
    /**
     * @brief This signal is emitted whenever a save form data request is received.
     *
     * Unless you connect to this signal and call @ref acceptSaveFormDataRequest
     * or @ref rejectSaveFormDataRequest slots, the save form data requested through
     * @ref saveFormData will simply be ignored.
     *
     * @p key is a value that uniquely identifies the save request and @p url
     * is the address for which the form data is being saved.
     *
     * @see acceptSaveFormDataRequest
     * @see rejectSaveFormDataRequest
     */
    void saveFormDataRequested(const QString &key, const QUrl &url);

    /**
     * @brief This signal is emitted whenever a save form data request is completed.
     *
     * @p ok will be set to true if the save form data request for @p url was
     * completed successfully.
     *
     * @see saveFormDataRequested
     */
    void saveFormDataCompleted(const QUrl &url, bool ok);

    /**
     * @brief This signal is emitted whenever a fill form data request is completed.
     *
     * @p ok will be set to true if any forms were successfully filled with
     * cached data from the persistent storage.
     *
     * @see fillFormData
     * @since 4.5
     */
    void fillFormRequestCompleted(bool ok);

    /**
     * @brief Signal emitted from detectAndFillPageForms() after form detection has finished.
     *
     * @param url the URL of the page
     * @param found whether the page contains any form of type @e text, @e, email or @e password
     * @param autoFillableFound whether the page contains any form which can be automatically filled
     *
     * @see detectAndFillPageForms()
     * @see WebForm::WebField::isAutoFillable()
     */
    void formDetectionDone(const QUrl& url, bool found, bool autoFillableFound);

    /**
     * @brief This signal is emitted whenever the current wallet is closed.
     */
    void walletClosed();

    /**
     * @brief Signal emitted when the wallet is opened
     */
    void walletOpened();

protected:
    /**
     * @brief Returns a list of forms for @p url that are waiting to be filled.
     *
     * This function returns an empty list if there is no pending requests
     * for filling forms associated with @p url.
     *
     * @return a list of forms for @p url that are waiting to be filled.
     */
    WebFormList formsToFill(const QUrl &url) const;

    /**
     * @brief Returns forms to be removed from persistent storage.
     *
     * @return a list of forms to be removed from persistent storage.
     */
    WebFormList formsToDelete() const;

    /**
     * The key under which the custom list of fields to cache for a page should be saved
     *
     * @param url the URL of the page
     * @see WebEngineSettings::setCustomizedCacheableFieldsForPage()
     */
    static QString customFormsKey(const QUrl &url);

    /**
     * @brief Whether or not there's data associate with @p form in the persistent storage
     *
     * @return @b true when there is data associated with @p form in the
     * persistent storage and @b false otherwise.
     */
    bool hasCachedFormData(const WebForm &form) const;

    /**
     * @return Fills the web forms in frame that point to @p url with data from @p forms.
     *
     * @param url the URL of the page
     * @param forms the forms to fill the page with
     * @see fillFormDataFromCache.
     */
    void fillWebForm(const QUrl &url, const WebFormList &forms);

    /**
     * @brief Fills form data from persistent storage.
     *
     * If you reimplement this function, call @ref formsToFill to obtain
     * the list of forms pending to be filled. Once you fill the list with
     * the cached data from the persistent storage, you must call @p fillWebForm
     * to fill out the actual web forms.
     *
     * @see formsToFill
     * @param list the list of URLs to fill
     */
    void fillFormDataFromCache(const QList<QUrl> &list);

    /**
     * @brief Stores form data associated with @p key to a persistent storage.
     *
     * If you reimplement this function, call @ref formsToSave to obtain the
     * list of form data pending to be saved to persistent storage.
     *
     * @param key the string to retrieve the data
     * @see formsToSave
     */
    void saveFormDataToCache(const QString &key);

    /**
     * @brief Removes all cached form data associated with @p forms from persistent storage.
     *
     * If you reimplement this function, call @ref formsToDelete to obtain the
     * list of form data pending to be removed from persistent storage.
     *
     * @param forms the forms to delete
     * @see formsToDelete
     */
    void removeFormDataFromCache(const WebFormList &forms);

    /**
     * @brief Enum to distinguish between filling and saving operations with forms
     */
    enum class CacheOperation {
        Fill, ///< Forms are being filled
        Save ///< Forms are being cached
    };

    /**
     * @brief Selects from the given list of forms for an URL the fields to cache
     *
     * Depending on the user choices, this function can return either the result of calling WebForm::withAutoFillableFieldsOnly() on each form
     * or the customized fields list chosen by the user.
     *
     * @param url the URL of the page
     * @param allForms the list of all forms contained in the page
     * @param op whether the function should return the forms to fill or those to save
     * @return a list of all cacheable forms in the page according to the user's settings
     */
    WebFormList cacheableForms(const QUrl &url, const WebFormList &allForms, CacheOperation op) const;

private:
    class WebEngineWalletPrivate;
    friend class WebEngineWalletPrivate;
    WebEngineWalletPrivate *const d;

    Q_PRIVATE_SLOT(d, void _k_openWalletDone(bool))
    Q_PRIVATE_SLOT(d, void _k_walletClosed())
};

QDebug operator<<(QDebug dbg, const WebEngineWallet::WebForm form);
QDebug operator<<(QDebug dbg, const WebEngineWallet::WebForm::WebFieldType type);
QDebug operator<<(QDebug dbg, const WebEngineWallet::WebForm::WebField field);

#endif // WEBENGINEWALLET_H
