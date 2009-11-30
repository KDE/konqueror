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

#include "kwebwallet.h"

#include <kwallet.h>
#include <kdebug.h>

#include <QtCore/QHash>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtWebKit/QWebPage>
#include <QtWebKit/QWebFrame>
#include <qwindowdefs.h>

#define QL1S(x)   QLatin1String(x)
#define QL1C(x)   QLatin1Char(x)

/**
 * Creates key used to store and retreive form data.
 *
 */
static QString walletKey(KWebWallet::WebForm form, bool useIndexOnEmptyName = false)
{
    QString key = form.url.toString(QUrl::RemoveQuery|QUrl::RemoveFragment);
    key += QL1C('#');
    if (form.name.isEmpty() && useIndexOnEmptyName)
        key += form.index;
    else
        key += form.name;

    return key;
}

static QString escapeValue (const QString& _value)
{
    QString value (_value);
    value.replace(QL1C('\\'), QL1S("\\\\"));
    value.replace(QL1C('\"'), QL1S("\\\""));
    return value;
}

class KWebWallet::KWebWalletPrivate
{  
public:
    struct FormsData
    {
        QPointer<QWebFrame> frame;
        KWebWallet::WebFormList forms;
    };
  
    KWebWalletPrivate(KWebWallet* parent);
    KWebWallet::WebFormList parseFormData(QWebFrame* frame, bool fillform = true, bool ignorepasswd = false);
    void fillDataFromCache(KWebWallet::WebFormList &formList);
    void saveDataToCache(const QString &key);
    void removeDataFromCache(const WebFormList &formList);

    // Private Q_SLOT...
    void _k_openWalletDone(bool);    

    WId wid;
    KWebWallet *q;
    QPointer<KWallet::Wallet> wallet;
    KWebWallet::WebFormList pendingRemoveRequests;
    QHash<KUrl, FormsData> pendingFillRequests;
    QHash<QString, KWebWallet::WebFormList> pendingSaveRequests;
};

KWebWallet::KWebWalletPrivate::KWebWalletPrivate(KWebWallet *parent)
                              :wid (0), q(parent)
{
}

KWebWallet::WebFormList KWebWallet::KWebWalletPrivate::parseFormData(QWebFrame *frame, bool fillform, bool ignorepasswd)
{
    KWebWallet::WebFormList list;
    const QString fileName = (fillform ? QL1S(":/resources/parseFormNames.js"):QL1S(":/resources/parseForms.js"));
    QFile file(fileName);

    if (file.open(QFile::ReadOnly)) {
        const QVariant r = frame->evaluateJavaScript(file.readAll());
        QListIterator<QVariant> formIt (r.toList());
        while (formIt.hasNext()) {
            KWebWallet::WebForm form;
            form.url = frame->url();
            const QVariantMap map = formIt.next().toMap();
            form.name = map.value(QL1S("name")).toString();
            form.index = map.value(QL1S("index")).toString();
            QListIterator<QVariant> elementIt (map.value(QL1S("elements")).toList());
            while (elementIt.hasNext()) {
                const QVariantMap elementMap = elementIt.next().toMap();
                if (elementMap[QL1S("autocomplete")].toString() == QL1S("off") ||
                    (ignorepasswd && elementMap.value(QL1S("type")).toString() == QL1S("password"))) {
                    continue;
                } else {
                    KWebWallet::WebForm::WebField field = qMakePair(elementMap.value(QL1S("name")).toString(),
                                                                    elementMap.value(QL1S("value")).toString());
                    form.fields << field;
                }
            }

            if ((fillform && q->hasCachedFormData(form)) || !form.fields.isEmpty())
                list << form;
        }
    }
    return list;
}

void KWebWallet::KWebWalletPrivate::fillDataFromCache(KWebWallet::WebFormList &formList)
{
    Q_ASSERT(wallet);
    Q_ASSERT(q);

    QMap<QString, QString> cachedValues;
    QMutableListIterator <WebForm> formIt (formList);

    while (formIt.hasNext()) {
        KWebWallet::WebForm &form = formIt.next();
        if (wallet->readMap(walletKey(form), cachedValues) == 0) {
            QMapIterator<QString, QString> valuesIt (cachedValues);
            while (valuesIt.hasNext()) {
                valuesIt.next();
                form.fields << qMakePair(valuesIt.key(), valuesIt.value());
            }
        }
    }
}

void KWebWallet::KWebWalletPrivate::saveDataToCache(const QString &key)
{
    Q_ASSERT(wallet);

    int count = 0;
    const QUrl url = pendingSaveRequests.value(key).first().url;
    const KWebWallet::WebFormList list = pendingSaveRequests.take(key);
    QListIterator<KWebWallet::WebForm> formIt (list);

    while (formIt.hasNext()) {
        QMap<QString, QString> values;
        const KWebWallet::WebForm form = formIt.next();
        QListIterator<KWebWallet::WebForm::WebField> fieldIt (form.fields);
        while (fieldIt.hasNext()) {
            const KWebWallet::WebForm::WebField field = fieldIt.next();
            values.insert(field.first, field.second);
        }

        if (wallet->writeMap(walletKey(form), values) == 0)
            count++;
    }

    emit q->saveFormDataCompleted(url, (count == list.count()));
}

void KWebWallet::KWebWalletPrivate::removeDataFromCache(const WebFormList &formList)
{
    Q_ASSERT(wallet);

    QListIterator<WebForm> formIt (formList);
    while (formIt.hasNext()) {
        wallet->removeEntry(walletKey(formIt.next()));
    }
}

void KWebWallet::KWebWalletPrivate::_k_openWalletDone(bool ok)
{ 
    Q_ASSERT (wallet);

    if (ok &&
        (wallet->hasFolder(KWallet::Wallet::FormDataFolder()) ||
         wallet->createFolder(KWallet::Wallet::FormDataFolder())) &&
        wallet->setFolder(KWallet::Wallet::FormDataFolder())) {

        // Do pending fill requests...
        if (!pendingFillRequests.isEmpty()) {
            KUrl::List urlList;
            QMutableHashIterator<KUrl, FormsData> requestIt (pendingFillRequests);
            while (requestIt.hasNext()) {
               requestIt.next();
               KWebWallet::WebFormList list = requestIt.value().forms;
               fillDataFromCache(list);
               q->fillWebForm(requestIt.key(), list);
            }

            pendingFillRequests.clear();
        }

         // Do pending save requests...
        if (!pendingSaveRequests.isEmpty()) {
            QListIterator<QString> keysIt (pendingSaveRequests.keys());
            while (keysIt.hasNext())
                saveDataToCache(keysIt.next());

            pendingSaveRequests.clear();
        }

        // Do pending remove requests...
        if (!pendingRemoveRequests.isEmpty()) {
            removeDataFromCache(pendingRemoveRequests);
            pendingRemoveRequests.clear();
        }
    } else {
        // Delete the wallet if opening the wallet failed or we were unable
        // to change to the folder we wanted to change to.
        delete wallet;
    }
}

KWebWallet::KWebWallet(QObject *parent, qlonglong wid)
           :QObject(parent), d(new KWebWalletPrivate(this))
{
    if (!wid) {
        // If wid is 0, make the best effort the discern it from our parent.
        QWebPage *page = qobject_cast<QWebPage*>(parent);
        if (page) {
            QWidget *widget = page->view();
            if (widget && widget->window())
                wid = widget->window()->winId();
        }
    }

    d->wid = wid;
}

KWebWallet::~KWebWallet()
{
    delete d->wallet;
    delete d;
}

KWebWallet::WebFormList KWebWallet::formsWithCachedData(QWebFrame* frame, bool recursive) const
{
    WebFormList list = d->parseFormData(frame);

    if (recursive) {
        QListIterator <QWebFrame *> framesIt (frame->childFrames());
        while (framesIt.hasNext()) {
            list << d->parseFormData(framesIt.next());
        }
    }

    return list;
}

void KWebWallet::fillFormData(QWebFrame *frame, bool recursive)
{
    if (frame) {
        KUrl::List urlList;
        WebFormList formsList = d->parseFormData(frame);
        if (!formsList.isEmpty()) {
          const QUrl url (frame->url());
          if (d->pendingFillRequests.contains(url)) {
              kWarning() << "Duplicate request rejected!!!";
          } else {
              KWebWalletPrivate::FormsData data;
              data.frame = frame;
              data.forms << formsList;
              d->pendingFillRequests.insert(url, data);
              urlList << url;
          }
        }

        if (recursive) {
            QListIterator<QWebFrame*> frameIt (frame->childFrames());
            while (frameIt.hasNext()) {
                QWebFrame *childFrame = frameIt.next();
                formsList = d->parseFormData(childFrame);
                if (!formsList.isEmpty()) {
                    const QUrl url (childFrame->url());
                    if (d->pendingFillRequests.contains(url)) {
                        kWarning() << "Duplicate request rejected!!!";
                    } else {
                        KWebWalletPrivate::FormsData data;
                        data.frame = frame;
                        data.forms << formsList;
                        d->pendingFillRequests.insert(url, data);
                        urlList << url;
                    }
                }
            }
        }

        if (!urlList.isEmpty())
            fillFormDataFromCache(urlList);
    }
}

void KWebWallet::saveFormData(QWebFrame *frame, bool recursive, bool ignorePasswordFields)
{
    WebFormList list = d->parseFormData(frame, false, ignorePasswordFields);
    if (recursive) {
        QListIterator<QWebFrame*> frameIt (frame->childFrames());
        while (frameIt.hasNext()) {
            list << d->parseFormData(frameIt.next(), false, ignorePasswordFields);
        }
    }

    if (!list.isEmpty()) {
        const QString key = QString::number(qHash(frame->url().toString() + frame->frameName()), 16);
        d->pendingSaveRequests.insert(key, list);
        emit saveFormDataRequested(key, frame->url());
    }
}

void KWebWallet::removeFormData(QWebFrame *frame, bool recursive)
{
    removeFormDataFromCache(formsWithCachedData(frame, recursive));
}

void KWebWallet::removeFormData(const WebFormList &forms)
{
    d->pendingRemoveRequests << forms;
    removeFormDataFromCache(forms);
}

void KWebWallet::acceptSaveFormDataRequest(const QString &key)
{
    saveFormDataToCache(key);
}

void KWebWallet::rejectSaveFormDataRequest(const QString & key)
{
    d->pendingSaveRequests.remove(key);
}

void KWebWallet::fillWebForm(const KUrl &url, const KWebWallet::WebFormList &forms)
{
    QWebFrame *frame = d->pendingFillRequests.value(url).frame;
    if (frame) {
        QListIterator<WebForm> formIt (forms);
        while (formIt.hasNext()) {
            const WebForm form = formIt.next();
            QString formName = form.name;
            if (formName.isEmpty())
                formName = form.index;

            kDebug() << "Filling out form:" << formName;
            QListIterator<WebForm::WebField> fieldIt (form.fields);
            while (fieldIt.hasNext()) {
                const WebForm::WebField field = fieldIt.next();
                const QString script = QString (QL1S("if(document.forms[\"%1\"].elements[\"%2\"] && "
                                                     "!document.forms[\"%1\"].elements[\"%2\"].disabled && "
                                                     "!document.forms[\"%1\"].elements[\"%2\"].readonly) "
                                                     "document.forms[\"%1\"].elements[\"%2\"].value=\"%3\";"))
                                       .arg(formName).arg(field.first).arg(escapeValue(field.second));
                frame->evaluateJavaScript(script);
                kDebug() << "Executed script:" << script;
            }
        }
    }
}

KWebWallet::WebFormList KWebWallet::formsToFill(const KUrl &url) const
{
    return d->pendingFillRequests.value(url).forms;
}

KWebWallet::WebFormList KWebWallet::formsToSave(const QString &key) const
{
    return d->pendingSaveRequests.value(key);
}

bool KWebWallet::hasCachedFormData(const WebForm &form) const
{
    return !KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(),
                                             KWallet::Wallet::FormDataFolder(),
                                             walletKey(form));
}

void KWebWallet::fillFormDataFromCache(const KUrl::List &urlList)
{
    if (!d->wallet) {
        d->wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                d->wid, KWallet::Wallet::Asynchronous);
        connect(d->wallet, SIGNAL(walletOpened(bool)),
                this, SLOT(_k_openWalletDone(bool)));
        return;
    }

    QListIterator<KUrl> urlIt (urlList);
    while (urlIt.hasNext()) {
        const KUrl url = urlIt.next();
        WebFormList list = formsToFill(url);
        d->fillDataFromCache(list);
        fillWebForm(url, list);
    }

    d->pendingFillRequests.clear();
}

void KWebWallet::saveFormDataToCache(const QString &key)
{
    if (!d->wallet) {
        d->wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                d->wid, KWallet::Wallet::Asynchronous);
        connect(d->wallet, SIGNAL(walletOpened(bool)),
                this, SLOT(_k_openWalletDone(bool)));
        return;
    }

    d->saveDataToCache(key);
    d->pendingSaveRequests.clear();
}

void KWebWallet::removeFormDataFromCache(const WebFormList &forms)
{
    if (!d->wallet) {
        d->wallet = KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                                                d->wid, KWallet::Wallet::Asynchronous);
        connect(d->wallet, SIGNAL(walletOpened(bool)),
                this, SLOT(_k_openWalletDone(bool)));
        return;
    }

    d->removeDataFromCache(forms);
    d->pendingRemoveRequests.clear();
}

#include "kwebwallet.moc"
