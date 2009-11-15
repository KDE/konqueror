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

#include <QtCore/QObject>
#include <QtGlobal>

class QWebFrame;
class QWebPage;
class QUrl;

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
     * Flags that determine how form data is saved to the wallet.
     */
    enum SaveType {
      Synchronous=0,
      Asynchronous
    };

    /**
     * Constructs a KWebWallet with @p parent as its parent.
     */
    explicit KWebWallet(QObject* parent = 0);

    /**
     * Destructor
     */
    virtual ~KWebWallet();

    /**
     * Save the form data from @p frame.
     *
     * If @p type is set to Asynchronous, the default, then this function will
     * not block during the saving process. Instead it queues the data to be
     * saved and emits @ref saveFormDataRequested. The queued form data will
     * then be sent to the wallet for permanent storage if and only if the
     * @ref confirmSaveFormDataRequest slot is invoked with the appropriate key.
     * To determine whether or not the save request succeeded, you must connect
     * to the @ref saveFormDataCompleted signal.
     *
     * If @p type is Synchronous, this function will block until the data is
     * saved and return the appropriate status.
     *
     * @param frame   the frame from which the form data is saved.
     * @param type    if Asynchronous, the default, this function will not
     *                block while saving form data.
     *
     * @return true on success or when @p type it Asynchronous, false otherwise.
     */
    bool saveFormData(QWebFrame *frame, SaveType type = Asynchronous);

public Q_SLOTS:
    /**
     * Restores the form data for @p frame.
     *
     * @param frame the web frame for which data should be restored.
     * @return true if form data was sucessfully restored.
     */
    void restoreFormData(QWebFrame *frame);

    /**
     * Confirms the form data to be saved to
     *
     * @param key the token provided through the @ref saveFormDataRequested signal.
     * @return true if form data was sucessfully restored.
     */
    void confirmSaveFormDataRequest(const QString &key);

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
     * This signal is emitted when a form data save request is completed.
     * @p ok will indicate whether or not the save request for @p url was
     * completed successfully.
     *
     * Note that this signal is only emitted if the request was Asynchronous.
     * See @ref saveFormData for details.
     */
    void saveFormDataCompleted(const QUrl &url, bool ok);

protected:
    /**
     * Saves the form data information in @p frame to KDE's wallet.
     */
    virtual void doSaveFormData(QWebFrame *frame);

    /**
     * Restores form data from KDE's wallet to @ frame.
     */
    virtual void doRestoreFormData(QWebFrame *frame);

private:
    class KWebWalletPrivate;
    KWebWalletPrivate * const d;
};

#endif // KWEBWALLET_H
