/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 * Copyright (C) 2018 Stefano Crocco <stefano.crocco@alice.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
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

class WebEnginePage;

class WebEngineWallet : public QObject
{
    Q_OBJECT

public:

    /**
     * Holds data from a HTML &lt;form&gt; element.
     */
    struct WebForm {
        /**
         * A typedef for storing the name and value attributes of HTML &lt;input&gt;
         * elements.
         */
        typedef QPair<QString, QString> WebField;

        /** The URL the form was found at. */
        QUrl url;
        /** The name attribute of the form. */
        QString name;
        /** The position of the form on the web page, relative to other forms. */
        QString index;
        /** The path of the frame the form belongs to relative to the toplevel window (in the javascript sense).
         *
         * This is stored as a string containing a javascript array (it is passed as is to javascript code, so no need to store it in C++ format
         */
        QString framePath;
        /** The name and value attributes of each input element in the form. */
        QVector<WebField> fields;
    };

    /**
     * A list of web forms
     */
    typedef QVector<WebForm> WebFormList;

    /**
     * Constructs a WebEngineWebWallet
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
    explicit WebEngineWallet(QObject *parent = nullptr, WId wid = 0);

    /**
     * Destructor
     */
    ~WebEngineWallet() override;

    /**
     * Attempts to save the form data from @p page and its children frames.
     *
     * You must connect to the @ref saveFormDataRequested signal and call either
     * @ref rejectSaveFormDataRequest or @ref acceptSaveFormDataRequest signals
     * in order to complete the save request. Otherwise, you request will simply
     * be ignored.
     *
     * Note that this function is asynchronous, as it requires running javascript code
     * on the page using QWebEnginePage::runJavaScript. This function only requests
     * for the form data to be saved when QWebEnginePage::runJavaScript finishes.
     * The actual saving is done by @ref saveFormDataCallback
     */
    void saveFormData(WebEnginePage *page, bool ignorePasswordFields = false);

    /**
     * Attempts to fill forms contained in @p page with cached data.
     *
     * Note that this function is asynchronous, as it requires running javascript code
     * on the page using QWebEnginePage::runJavaScript. This function only requests
     * for the form data to be filled when QWebEnginePage::runJavaScript finishes.
     * The actual filling is done by fillFormDataCallback
     */
    void fillFormData(WebEnginePage *page);

    /**
     * Removes the form data specified by @p forms from the persistent storage.
     *
     * Note that this function will remove all cached data for forms found in @p page.
     *
     * Note that this function is asynchronous, as it requires running javascript code
     * on the page using QWebEnginePage::runJavaScript. This function only requests
     * for the form data to be removed when QWebEnginePage::runJavaScript finishes.
     * The actual removing is done by removeFormDataCallback
     */
    void removeFormData(WebEnginePage *page);

    /**
     * Removes the form data specified by @p forms from the persistent storage.
     *
     * @see formsWithCachedData
     */
    void removeFormData(const WebFormList &forms);

public Q_SLOTS:
    /**
     * Accepts the save form data request associated with @p key.
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
     * Rejects the save form data request associated with @p key.
     *
     * The @p key parameter is the one sent through the @ref saveFormDataRequested
     * signal.
     *
     * @see saveFormDataRequested.
     */
    void rejectSaveFormDataRequest(const QString &key);

Q_SIGNALS:
    /**
     * This signal is emitted whenever a save form data request is received.
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
     * This signal is emitted whenever a save form data request is completed.
     *
     * @p ok will be set to true if the save form data request for @p url was
     * completed successfully.
     *
     * @see saveFormDataRequested
     */
    void saveFormDataCompleted(const QUrl &url, bool ok);

    /**
     * This signal is emitted whenever a fill form data request is completed.
     *
     * @p ok will be set to true if any forms were successfully filled with
     * cached data from the persistent storage.
     *
     * @see fillFormData
     * @since 4.5
     */
    void fillFormRequestCompleted(bool ok);

    /**
     * This signal is emitted whenever the current wallet is closed.
     */
    void walletClosed();

protected:
    /**
     * Returns a list of forms for @p url that are waiting to be filled.
     *
     * This function returns an empty list if there is no pending requests
     * for filling forms associated with @p url.
     */
    WebFormList formsToFill(const QUrl &url) const;

    /**
     * Returns a list of for @p key that are waiting to be saved.
     *
     * This function returns an empty list if there are no pending requests
     * for saving forms associated with @p key.
     */
    WebFormList formsToSave(const QString &key) const;

    /**
     * Returns forms to be removed from persistent storage.
     */
    WebFormList formsToDelete() const;

    /**
     * Returns true when there is data associated with @p form in the
     * persistent storage.
     */
    bool hasCachedFormData(const WebForm &form) const;

    /**
     * Fills the web forms in frame that point to @p url with data from @p forms.
     *
     * @see fillFormDataFromCache.
     */
    void fillWebForm(const QUrl &url, const WebFormList &forms);

    /**
     * Fills form data from persistent storage.
     *
     * If you reimplement this function, call @ref formsToFill to obtain
     * the list of forms pending to be filled. Once you fill the list with
     * the cached data from the persistent storage, you must call @p fillWebForm
     * to fill out the actual web forms.
     *
     * @see formsToFill
     */
    void fillFormDataFromCache(const QList<QUrl> &list);

    /**
     * Stores form data associated with @p key to a persistent storage.
     *
     * If you reimplement this function, call @ref formsToSave to obtain the
     * list of form data pending to be saved to persistent storage.
     *
     *@see formsToSave
     */
    void saveFormDataToCache(const QString &key);

    /**
     * Removes all cached form data associated with @p forms from persistent storage.
     *
     * If you reimplement this function, call @ref formsToDelete to obtain the
     * list of form data pending to be removed from persistent storage.
     *
     *@see formsToDelete
     */
    void removeFormDataFromCache(const WebFormList &forms);

private:

    /**
     * Callback used by @ref fillFormData to insert form data
     *
     * This function is called as a callback from @ref fillFormData after
     * @ref QWebEnginePage::runJavaScript has finished
     */
    void fillFormDataCallback(WebEnginePage *page, const WebFormList &formsList);

    /**
     * Callback used by @ref saveFormData to save data
     *
     * This function is called as a callback from @ref saveFormData after
     * @ref QWebEnginePage::runJavaScript has finished
     */
    void saveFormDataCallback(const QString &key, const QUrl &url, const WebFormList &formslist);

    /**
     * Callback used by @ref removeFormData to remove data
     *
     * This function is called as a callback from @ref removeFormData after
     * @ref QWebEnginePage::runJavaScript has finished
     */
    void removeFormDataCallback(const WebFormList &list);

private:
    class WebEngineWalletPrivate;
    friend class WebEngineWalletPrivate;
    WebEngineWalletPrivate *const d;

    Q_PRIVATE_SLOT(d, void _k_openWalletDone(bool))
    Q_PRIVATE_SLOT(d, void _k_walletClosed())
};


#endif // WEBENGINEWALLET_H
