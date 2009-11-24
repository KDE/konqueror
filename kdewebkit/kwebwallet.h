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
    explicit KWebWallet(QObject* parent = 0);

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
     * You must always connect to @ref saveFormDataRequested signal and
     * call this slot or @ref rejectSaveFormDataRequest in
     *
     * @see saveFormDataRequested.
     *
     * @param key      the key for the form data to be saved.
     */
    void acceptSaveFormDataRequest(const QString &key);

    /**
     * Rejects the save form data request associated with @p key.
     *
     * The @p key parameter is the one sent through the @ref saveFormDataRequested
     * signal.
     *
     * @see saveFormDataRequested.
     *
     * @param key      the key for the form data to be saved.
     */
    void rejectSaveFormDataRequest(const QString &key);

Q_SIGNALS:
    /**
     * This signal is emitted as notification of a pending form data save request.
     *
     * This signal is only sent for Asynchronous save requests and requires that you
     * call @ref confirmSaveFormDataRequest aftewards to commit the data to the wallet.
     *
     * @see confirmSaveFormDataRequest
     */
    void saveFormDataRequested(const QString &key, const QUrl &url);

    /**
     * This signal is emitted on completion of a save form data request.
     *
     * @p ok will be set to true if the save form data request for @p url was
     * successfully completed.
     */
    void saveFormDataCompleted(const QUrl &url, bool ok);

protected:
    /**
     * Fill all pending form fill requests associated with the urls in @p list.
     *
     * This function does nothing if there is no pending request that matches
     * any of the urls in the list or there is no cached form data associated
     * with the given url list.
     *
     * @see restoreFormData.
     */
    void fillForms(const KUrl::List &list);

    /**
     * Returns a reference to forms associated with @p url from the pending
     * fill forms request list.
     *
     * This function returns an empty list if there is no pending fill request that
     * matches the given url.
     */
    WebFormList& formsToFill(const KUrl &url);

    /**
     * Returns a reference to forms associated with @p url from the pending
     * save form data request list.
     *
     * This function returns an empty list if there is no pending save request that
     * matches the given url.
     */
    WebFormList& formsToSave(const QString &key);

    /**
     * Returns true when there is data associated with @p form in the
     * persistent cache.
     *
     * @see KWebWallet::WebForm
     */
    virtual bool hasCachedFormData(const WebForm &form) const;

    /**
     * Stores forms data associated with @p keys to a persistent cache.
     *
     *@see KWebWallet::WebFormList
     */
    virtual void saveFormData(const QString &key);

    /**
     * Restores form data from persistent cache
     *
     * If you reimplement this function, use @ref formsForUrl to obtain the
     * parsed form information for a given url. You should also call
     * every form you want to fill out with data from the persistent cache.
     *
     * @see fillFormDataFor
     * @see KWebWallet::WebFormList
     */
    virtual void fillFormData(const KUrl::List &list);

private:
    class KWebWalletPrivate;
    friend class KWebWalletPrivate;
    KWebWalletPrivate * const d;
    

    Q_PRIVATE_SLOT(d, void _k_openWalletDone(bool));
};

#endif // KWEBWALLET_H
