/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
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
#ifndef KWEBWALLET_H
#define KWEBWALLET_H

#include <kdewebkit_export.h>

#include <kurl.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtGlobal>

class QWebFrame;
class QWebPage;


/**
 * @short A class that provides KWallet integration for QtWebKit.
 *
 * @author Dawit Alemayehu <adawit @ kde.org>
 * @since 4.4
 */
class KDEWEBKIT_EXPORT KWebWallet : public QObject
{
    Q_OBJECT

public:

    /**
     * Structure to hold data from HTML <form> element.
     *
     * @p url    holds the origination url of a form.
     * @p name   holds the name attribute of a form.
     * @p index  holds the order of a form on a web page.
     * @p fields holds the name and value attributes of each <input> in a form.
     */
    struct WebForm
    {
       /**
        * A typedef for storing the name and value attributes of HTML <input>
        * elements.
        */
        typedef QPair<QString, QString> WebField;

        QUrl url;
        QString name;
        QString index;
        QList<WebField> fields;
    };

    typedef QList<WebForm> WebFormList;

    /**
     * Constructs a KWebWallet with @p parent as its parent.
     */
    explicit KWebWallet(QObject* parent = 0, qlonglong wid = 0);

    /**
     * Destructor
     */
    virtual ~KWebWallet();

    /**
     * Attempts to save the form data from @p frame and its children frames.
     *
     * If @p recursive is set to true, the default, then form data from all
     * the child frames of @p frame will be saved. Set @p ignorePasswordFields
     * to true if you do not want data from password fields to not be saved.
     *
     * You must connect to the @ref saveFormDataRequested signal and call either
     * @ref rejectSaveFormDataRequest or @ref acceptSaveFormDataRequest signals
     * in order to complete the save request. Otherwise, you request will simply
     * be ignored.
     *
     * @see saveFormDataRequested
     */
    void saveFormData(QWebFrame *frame, bool recursive = true, bool ignorePasswordFields = false);

    /**
     * Attempts to fill forms contained in @p frame with cached data.
     *
     * If @p recursive is set to true, the default, then this function will
     * attempt to fill out forms in the specified frame and all its children
     * frames.
     *
     * @see restoreFormData
     */
    void fillFormData(QWebFrame *frame, bool recursive = true);

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
     * Unless you connect to this signal and and call the
     * @ref acceptSaveFormDataRequest or @ref rejectSaveFormDataRequest slots,
     * the save request will never be acted upon.
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

protected:
    /**
     * Returns a list of forms for @p url that are waiting to be filled.
     *
     * This function returns an empty list if there is no pending requests
     * for filling forms associated with @p url.
     */
    WebFormList formsToFill(const KUrl &url) const;

    /**
     * Returns a list of for @p key that are waiting to be saved.
     *
     * This function returns an empty list if there are no pending requests
     * for saving forms associated with @p key.
     */
    WebFormList formsToSave(const QString &key) const;

    /**
     * Returns true when there is data associated with @p form in the
     * persistent cache.
     */
    virtual bool hasCachedFormData(const WebForm &form) const;

    /**
     * Fills the QWebFrame that is showing @p url with data from @p forms.
     *
     * @see fillFormData.
     */
    void fillForm(const KUrl &url, const WebFormList &forms);

    /**
     * Fills form data from persistent cache.
     *
     * If you reimplement this function, call @ref formsToFill to obtain
     * the list of forms pending to be filled. Once you fill the list with
     * the cached data from the persistent storage, you must call @p fillForm
     * to fill out the actual web forms.
     *
     * @see formsToFill
     */
    virtual void fillFormData(const KUrl::List &list);

    /**
     * Stores form data associated with @p key to a persistent cache.
     *
     * If you reimplement this function, call @ref formsToSave to obtain the
     * list of forms data pending to be saved to persistent cache.
     *
     *@see formsToSave
     */
    virtual void saveFormData(const QString &key);

private:
    class KWebWalletPrivate;
    friend class KWebWalletPrivate;
    KWebWalletPrivate * const d;
    

    Q_PRIVATE_SLOT(d, void _k_openWalletDone(bool))
};

#endif // KWEBWALLET_H
