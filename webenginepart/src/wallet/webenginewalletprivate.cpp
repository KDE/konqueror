/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "webenginewallet.h"
#include "webenginepage.h"

#include <KWallet>

#include <QSet>
#include <QHash>
#include <QWebEngineScript>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QMessageBox>

#include <algorithm>

class WebEngineWallet::WebEngineWalletPrivate
{
public:
    struct FormsData {
        QPointer<WebEnginePage> page;
        WebEngineWallet::WebFormList forms;
    };

    typedef std::function<void(const WebEngineWallet::WebFormList &)> WebWalletCallback;

    WebEngineWalletPrivate(WebEngineWallet *parent);

    static WebFormList parseFormDetectionResult(const QVariant &jsForms, const QUrl &pageUrl);

    WebFormList formsToFill(const WebFormList &allForms) const;
    WebFormList formsToSave(const WebFormList &allForms) const;
    bool hasAutoFillableFields(const WebFormList &forms) const;

    void fillDataFromCache(WebEngineWallet::WebFormList &formList, bool custom);

    /**
     * @brief Saves form data to the wallet
     *
     * If the wallet already contains data for this key, the data won't be saved immediately. Instead, the
     * WebEngineWallet::saveFormDataRequested signal will be emitted, which will cause the WebEnginePart to
     * ask the user what to do. In this case, this function will do nothing after emitting the signal.
     *
     * @note Callers of this function @b must remove the key from #pendingRemoveRequests if this function returns @b true.
     *
     * @param key the key corresponding to the data to save in #pendingRemoveRequests
     * @return @b true if the data was saved immediately and @a key must be removed from #pendingRemoveRequests and @b false
     * otherwise
     */
    bool saveDataToCache(const QString &key);
    void removeDataFromCache(const WebFormList &formList);
    void openWallet();

    static bool containsCustomForms(const QMap<QString, QString>& map);
    static void detectFormsInPage(WebEnginePage *page, WebWalletCallback callback, bool findLabels=false);
    static bool shouldFieldBeIgnored(const QString &name);

    // Private slots...
    void _k_openWalletDone(bool);
    void _k_walletClosed();

    WId wid;
    WebEngineWallet *q;
    std::unique_ptr<KWallet::Wallet> wallet;
    WebEngineWallet::WebFormList pendingRemoveRequests;
    QHash<QUrl, FormsData> pendingFillRequests;
    QHash<QString, WebFormList> pendingSaveRequests;

    QSet<QUrl> confirmSaveRequestOverwrites;

    ///@brief A list of field names which the user is unlikely to want stored (such as search fields)
    static const char* s_fieldNamesToIgnore[];
};

const char* WebEngineWallet::WebEngineWalletPrivate::s_fieldNamesToIgnore[] = {
    "q", //The search field in Google and DuckDuckGo
    "search", "search_bar", //Other possible names for a search field
    "amount" //A field corresponding to a quantity
};

static QUrl urlForFrame(const QUrl &frameUrl, const QUrl &pageUrl)
{
    return (frameUrl.isEmpty() || frameUrl.isRelative() ? pageUrl.resolved(frameUrl) : frameUrl);
}

WebEngineWallet::WebEngineWalletPrivate::WebEngineWalletPrivate(WebEngineWallet *parent)
    : wid(0), q(parent)
{
}

bool WebEngineWallet::WebEngineWalletPrivate::shouldFieldBeIgnored(const QString& name)
{
    QString lowerName = name.toLower();
    for (uint i = 0; i < sizeof(s_fieldNamesToIgnore)/sizeof(char*); ++i){
        if (lowerName == s_fieldNamesToIgnore[i]) {
            return true;
        }
    }
    return false;
}

WebEngineWallet::WebFormList WebEngineWallet::WebEngineWalletPrivate::parseFormDetectionResult(const QVariant& jsForms, const QUrl& pageUrl)
{
    const QVariantList variantForms(jsForms.toList());
    WebEngineWallet::WebFormList list;
    for (const QVariant &v : variantForms) {
        QJsonObject formMap = QJsonDocument::fromJson(v.toString().toUtf8()).object();
        WebEngineWallet::WebForm form;
        form.url = urlForFrame(QUrl(formMap[QL1S("url")].toString()), pageUrl);
        form.name = formMap[QL1S("name")].toString();
        form.index = formMap[QL1S("index")].toString();
        form.framePath = QVariant(formMap[QL1S("framePath")].toArray().toVariantList()).toStringList().join(",");
        const QVariantList elements = formMap[QL1S("elements")].toArray().toVariantList();
        for(const QVariant &e : elements) {
           QVariantMap elementMap(e.toMap());
           WebForm::WebField field;
           field.type = WebForm::fieldTypeFromTypeName(elementMap[QL1S("type")].toString().toLower());
           if (field.type == WebForm::WebFieldType::Other) {
               continue;
           }
           field.name = elementMap[QL1S("name")].toString();
           if (shouldFieldBeIgnored(field.name)) {
               continue;
           }

           field.id = elementMap[QL1S("id")].toString();
           field.readOnly = elementMap[QL1S("readonly")].toBool();
           field.disabled = elementMap[QL1S("disabled")].toBool();
           field.autocompleteAllowed = elementMap[QL1S("autocompleteAllowed")].toBool();
           field.value = elementMap[QL1S("value")].toString();
           field.label = elementMap[QL1S("label")].toString();
           form.fields.append(field);
        }
        if (!form.fields.isEmpty()) {
            list.append(form);
        }
    }
    return list;
}

void WebEngineWallet::WebEngineWalletPrivate::detectFormsInPage(WebEnginePage* page, WebEngineWallet::WebEngineWalletPrivate::WebWalletCallback callback, bool findLabels)
{
    if (!page) {
        return;
    }
    QUrl url = page->url();
    auto realCallBack = [callback, url](const QVariant &jsForms) {
        //If the page is deleted while the javascript code is still running, the callback will be called
        //but the page will be invalid, which will cause a crash. According to the documentation, when this
        //happens, the parameter passed to the callback is invalid: in that case, we do nothing.
        if (!jsForms.isValid()) {
            return;
        }
        WebFormList forms = parseFormDetectionResult(jsForms, url);
        callback(forms);
    };
    page->runJavaScript(QStringLiteral("findFormsInWindow(%1)").arg(findLabels ? "true" : ""), QWebEngineScript::ApplicationWorld, realCallBack);
}

WebEngineWallet::WebFormList WebEngineWallet::WebEngineWalletPrivate::formsToFill(const WebFormList &allForms) const
{
    WebEngineWallet::WebFormList list;
    for (const WebEngineWallet::WebForm &form : allForms) {
        WebEngineWallet::WebForm f(form.withAutoFillableFieldsOnly());
        if (!f.fields.isEmpty()) {
            list.append(f);
        }
    }
    return list;
}

WebEngineWallet::WebFormList WebEngineWallet::WebEngineWalletPrivate::formsToSave(const WebEngineWallet::WebFormList& allForms) const
{
    WebEngineWallet::WebFormList list;
    std::copy_if(allForms.constBegin(), allForms.constEnd(), std::back_inserter(list), [](const WebForm &f){return f.hasPasswords();});
    return list;
}

bool WebEngineWallet::WebEngineWalletPrivate::hasAutoFillableFields(const WebEngineWallet::WebFormList& forms) const
{
    return std::any_of(forms.constBegin(), forms.constEnd(), [](const WebForm &f){return f.hasAutoFillableFields();});
}

void WebEngineWallet::WebEngineWalletPrivate::fillDataFromCache(WebEngineWallet::WebFormList &formList, bool custom)
{
    if (!wallet) {
        qCWarning(WEBENGINEPART_LOG) << "Unable to retrieve form data from wallet";
        return;
    }

    QString lastKey;
    QMap<QString, QString> cachedValues;
    QMutableVectorIterator <WebForm> formIt(formList);

    while (formIt.hasNext()) {
        WebEngineWallet::WebForm &form = formIt.next();
        const QString key(form.walletKey());
        if (key != lastKey && wallet->readMap(key, cachedValues) != 0) {
            qCWarning(WEBENGINEPART_LOG) << "Unable to read form data for key:" << key;
            continue;
        }
        if (!custom) {
            form = form.withAutoFillableFieldsOnly();
        }
        for (int i = 0, count = form.fields.count(); i < count; ++i) {
            form.fields[i].value = cachedValues.value(form.fields[i].name);
        }
        lastKey = key;
    }
}

bool WebEngineWallet::WebEngineWalletPrivate::saveDataToCache(const QString &key)
{
    // Make sure the specified keys exists before acting on it. See BR# 270209.
    if (!pendingSaveRequests.contains(key)) {
        return false;
    }

    if (!wallet) {
        qCWarning(WEBENGINEPART_LOG) << "NULL Wallet instance!";
        return false;
    }

    bool success = false;

    int count = 0;
    const WebEngineWallet::WebFormList list = pendingSaveRequests.value(key);
    const QUrl url = list.first().url;
    QMap <QString, QString> values, storedValues;

    QVectorIterator<WebEngineWallet::WebForm> formIt(list);

    while (formIt.hasNext()) {
        QMap<QString, QString> values, storedValues;
        WebEngineWallet::WebForm form = formIt.next();
        const QString accessKey = form.walletKey();
        const int status = wallet->readMap(accessKey, storedValues);
        if (status == 0 && !storedValues.isEmpty()) {
            if (confirmSaveRequestOverwrites.contains(url)) {
                confirmSaveRequestOverwrites.remove(url);
                if (!storedValues.isEmpty()) {
                    auto fieldChanged = [&storedValues](const WebForm::WebField field){
                        return !storedValues.contains(field.name) || storedValues.value(field.name) != field.value;
                    };
                    if (std::any_of(form.fields.constBegin(), form.fields.constEnd(), fieldChanged)) {
                        emit q->saveFormDataRequested(key, url);
                        return false;
                    }
                    // If we got here it means the new credential is exactly
                    // the same as the one already cached ; so skip the
                    // re-saving part...
                    success = true;
                    continue;
                }
            }
        }
        QVectorIterator<WebEngineWallet::WebForm::WebField> fieldIt(form.fields);
        while (fieldIt.hasNext()) {
            const WebEngineWallet::WebForm::WebField field = fieldIt.next();
            values.insert(field.name, field.value);
        }

        if (wallet->writeMap(accessKey, values) == 0) {
            count++;
        } else {
            qCWarning(WEBENGINEPART_LOG) << "Unable to write form data to wallet";
        }
    }

    if (list.isEmpty() || count > 0) {
        success = true;
    }

    emit q->saveFormDataCompleted(url, success);
    return true;
}

void WebEngineWallet::WebEngineWalletPrivate::openWallet()
{
    if (wallet) {
        return;
    }

    wallet.reset(KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                 wid, KWallet::Wallet::Asynchronous));

    if (!wallet) {
        return;
    }

    // FIXME: See if possible to use new Qt5 connect syntax
    connect(wallet.get(), SIGNAL(walletOpened(bool)), q, SLOT(_k_openWalletDone(bool)));
    connect(wallet.get(), SIGNAL(walletClosed()), q, SLOT(_k_walletClosed()));
}

void WebEngineWallet::WebEngineWalletPrivate::removeDataFromCache(const WebFormList &formList)
{
    if (!wallet) {
        qCWarning(WEBENGINEPART_LOG) << "NULL Wallet instance!";
        return;
    }

    QVectorIterator<WebForm> formIt(formList);
    while (formIt.hasNext()) {
        wallet->removeEntry(formIt.next().walletKey());
    }
}

void WebEngineWallet::WebEngineWalletPrivate::_k_openWalletDone(bool ok)
{
    Q_ASSERT(wallet);

    if (ok &&
            (wallet->hasFolder(KWallet::Wallet::FormDataFolder()) ||
             wallet->createFolder(KWallet::Wallet::FormDataFolder())) &&
            wallet->setFolder(KWallet::Wallet::FormDataFolder())) {

        emit q->walletOpened();

        // Do pending fill requests...
        if (!pendingFillRequests.isEmpty()) {
            QMutableHashIterator<QUrl, FormsData> requestIt(pendingFillRequests);
            while (requestIt.hasNext()) {
                requestIt.next();
                WebEngineWallet::WebFormList list = requestIt.value().forms;
                fillDataFromCache(list, WebEngineSettings::self()->hasPageCustomizedCacheableFields(customFormsKey(requestIt.key())));
                q->fillWebForm(requestIt.key(), list);
            }

            pendingFillRequests.clear();
        }

        // Do pending save requests...
        //NOTE: don't increment the iterator inside the for because it's done inside the cycle, depending on the value returned
        //by saveDataToCache
        for (QHash<QString, WebFormList>::iterator it = pendingSaveRequests.begin(); it != pendingSaveRequests.end();) {
            bool removeEntry = saveDataToCache(it.key());
            //Only remove the entry if it could be saved without user confirmation
            if (removeEntry) {
                it = pendingSaveRequests.erase(it);
            } else {
                ++it;
            }
        }

        // Do pending remove requests...
        if (!pendingRemoveRequests.isEmpty()) {
            removeDataFromCache(pendingRemoveRequests);
            pendingRemoveRequests.clear();
        }
    } else {
        // Delete the wallet if opening the wallet failed or we were unable
        // to change to the folder we wanted to change to.
        wallet.reset();
    }
}

void WebEngineWallet::WebEngineWalletPrivate::_k_walletClosed()
{
    if (wallet) {
        KWallet::Wallet *w = wallet.release();
        w->deleteLater();
    }

    emit q->walletClosed();
}

