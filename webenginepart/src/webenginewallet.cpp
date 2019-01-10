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

#include "webenginewallet.h"
#include "webenginepage.h"
#include <webenginepart_debug.h>

#include <KWallet>

#include <QSet>
#include <QHash>
#include <QFile>
#include <QPointer>
#include <QScopedPointer>
#include <QJsonDocument>
#include <qwindowdefs.h>

#define QL1S(x)   QLatin1String(x)
#define QL1C(x)   QLatin1Char(x)

// Javascript used to extract/set data from <form> elements.
static const char s_fillableFormElementExtractorJs[] = "(function(){ "
"    function findFormsRecursive(wnd, existingList, path){"
"        findFormsInFrame(wnd, existingList, path);"
"        frameList = wnd.frames;"
"        for(var i = 0; i < frameList.length; ++i) {"
"            var newPath = path.concat(i);"
"            findFormsRecursive(frameList[i], existingList, newPath);"
"        }"
"    }"
"    function findFormsInFrame(frm, existingList, path){"
"        var url = frm.location;"
"        var formList;"
"        try{ formList = frm.document.forms; } "
"        catch(e){"
"          return;"
"        }"
"        if (formList.length > 0) { "
"            for (var i = 0; i < formList.length; ++i) { "
"                var inputList = formList[i].elements; "
"                if (inputList.length < 1) { "
"                    continue; "
"                } "
"                var formObject = new Object; "
"                formObject.url = url;"
"                formObject.name = formList[i].name; "
"                if (typeof(formObject.name) != 'string') { "
"                    formObject.name = String(formList[i].id); "
"                } "
"                formObject.index = i; "
"                formObject.elements = new Array; "
"                for (var j = 0; j < inputList.length; ++j) { "
"                    if (inputList[j].type != 'text' && inputList[j].type != 'email' && inputList[j].type != 'password') { "
"                        continue; "
"                    } "
"                    if (inputList[j].disabled || inputList[j].autocomplete == 'off') { "
"                        continue; "
"                    } "
"                    var element = new Object; "
"                    element.name = inputList[j].name; "
"                    if (typeof(element.name) != 'string' ) { "
"                        element.name = String(inputList[j].id); "
"                    } "
"                    element.value = String(inputList[j].value); "
"                    element.type = String(inputList[j].type); "
"                    element.readonly = Boolean(inputList[j].readOnly); "
"                    formObject.elements.push(element); "
"                } "
"                if (formObject.elements.length > 0) { "
"                    formObject.framePath = path;"
"                    console.log(JSON.stringify(formObject));"
"                    existingList.push(JSON.stringify(formObject)); "
"                } "
"            } "
"        } "
"    }"
"    var forms = new Array;"
"    findFormsRecursive(window, forms, []);"
"    return forms;"
"})()";

//javascript used to fill a single form element
static const char s_javascriptFillInputFragment[] = "var frm = window;"
"    for(var i=0; i < [%1].length; ++i) frm=frm.frames[i];"
"    if (frm.document.forms['%2'] && frm.document.forms['%2'].elements['%3']){"
"        frm.document.forms['%2'].elements['%3'].value='%4';\n"
"    }";


/**
 * Creates key used to store and retrieve form data.
 *
 */
static QString walletKey(const WebEngineWallet::WebForm &form)
{
    QString key = form.url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    key += QL1C('#');
    key += form.name;
    return key;
}

static QUrl urlForFrame(const QUrl &frameUrl, const QUrl &pageUrl)
{
    return (frameUrl.isEmpty() || frameUrl.isRelative() ? pageUrl.resolved(frameUrl) : frameUrl);
}

class WebEngineWallet::WebEngineWalletPrivate
{
public:
    struct FormsData {
        QPointer<WebEnginePage> page;
        WebEngineWallet::WebFormList forms;
    };

    typedef std::function<void(const WebEngineWallet::WebFormList &)> WebWalletCallback;

    WebEngineWalletPrivate(WebEngineWallet *parent);

    void withFormData(WebEnginePage *page, const WebWalletCallback &callback, bool fillform = true, bool ignorepasswd = false);
    WebFormList parseFormData(const QVariant &result, const QUrl &url, bool fillform = true, bool ignorepasswd = false);
    void performFormDataParsing(const QVariant &result, const QUrl &url, WebWalletCallback callback, bool fillform, bool ignorepasswd);
    void fillDataFromCache(WebEngineWallet::WebFormList &formList);
    void saveDataToCache(const QString &key);
    void removeDataFromCache(const WebFormList &formList);
    void openWallet();

    // Private slots...
    void _k_openWalletDone(bool);
    void _k_walletClosed();

    WId wid;
    WebEngineWallet *q;
    QScopedPointer<KWallet::Wallet> wallet;
    WebEngineWallet::WebFormList pendingRemoveRequests;
    QHash<QUrl, FormsData> pendingFillRequests;
    QHash<QString, WebEngineWallet::WebFormList> pendingSaveRequests;
    QSet<QUrl> confirmSaveRequestOverwrites;
};

WebEngineWallet::WebEngineWalletPrivate::WebEngineWalletPrivate(WebEngineWallet *parent)
    : wid(0), q(parent)
{
}

WebEngineWallet::WebFormList WebEngineWallet::WebEngineWalletPrivate::parseFormData(const QVariant &result, const QUrl &url, bool fillform, bool ignorepasswd)
{
    const QVariantList results(result.toList());
    WebEngineWallet::WebFormList list;
    Q_FOREACH (const QVariant &formVariant, results) {
        QJsonDocument doc = QJsonDocument::fromJson(formVariant.toString().toUtf8());
        const QVariantMap map = doc.toVariant().toMap();
        WebEngineWallet::WebForm form;
        form.url = urlForFrame(QUrl(map[QL1S("url")].toString()), url);
        form.name = map[QL1S("name")].toString();
        form.index = map[QL1S("index")].toString();
        form.framePath = map["framePath"].toStringList().join(",");
        bool formHasPasswords = false;
        const QVariantList elements = map[QL1S("elements")].toList();
        QVector<WebEngineWallet::WebForm::WebField> inputFields;
        Q_FOREACH (const QVariant &element, elements) {
            QVariantMap elementMap(element.toMap());
            const QString name(elementMap[QL1S("name")].toString());
            const QString value(ignorepasswd ? QString() : elementMap[QL1S("value")].toString());
            if (name.isEmpty()) {
                continue;
            }
            if (fillform && elementMap[QL1S("readonly")].toBool()) {
                continue;
            }
            if (elementMap[QL1S("type")].toString().compare(QL1S("password"), Qt::CaseInsensitive) == 0) {
                if (!fillform && value.isEmpty()) {
                    continue;
                }
                formHasPasswords = true;
            }
            inputFields.append(qMakePair(name, value));
        }
        // Only add the input fields on form save requests...
        if (formHasPasswords || fillform) {
            form.fields = inputFields;
        }

        // Add the form to the list if we are saving it or it has cached data.
        if ((fillform && q->hasCachedFormData(form)) || (!fillform  && !form.fields.isEmpty())) {
            list << form;
        }
    }
    return list;
}

void WebEngineWallet::WebEngineWalletPrivate::withFormData(WebEnginePage* page, const WebWalletCallback &callback, bool fillform, bool ignorepasswd)
{
    Q_ASSERT(page);
    QUrl url = page->url();
    auto internalCallback = [this, url, fillform, ignorepasswd, callback](const QVariant &result){
        WebFormList res = parseFormData(result, url, fillform, ignorepasswd);
        callback(res);
    };
    page->runJavaScript(QL1S(s_fillableFormElementExtractorJs), internalCallback);
}

void WebEngineWallet::WebEngineWalletPrivate::fillDataFromCache(WebEngineWallet::WebFormList &formList)
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
        const QString key(walletKey(form));
        if (key != lastKey && wallet->readMap(key, cachedValues) != 0) {
            qCWarning(WEBENGINEPART_LOG) << "Unable to read form data for key:" << key;
            continue;
        }

        for (int i = 0, count = form.fields.count(); i < count; ++i) {
            form.fields[i].second = cachedValues.value(form.fields[i].first);
        }
        lastKey = key;
    }
}

void WebEngineWallet::WebEngineWalletPrivate::saveDataToCache(const QString &key)
{
    // Make sure the specified keys exists before acting on it. See BR# 270209.
    if (!pendingSaveRequests.contains(key)) {
        return;
    }

    bool success = false;
    const QUrl url = pendingSaveRequests.value(key).first().url;

    if (wallet) {
        int count = 0;
        const WebEngineWallet::WebFormList list = pendingSaveRequests.value(key);
        QVectorIterator<WebEngineWallet::WebForm> formIt(list);

        while (formIt.hasNext()) {
            QMap<QString, QString> values, storedValues;
            const WebEngineWallet::WebForm form = formIt.next();
            const QString accessKey = walletKey(form);
            if (confirmSaveRequestOverwrites.contains(url)) {
                confirmSaveRequestOverwrites.remove(url);
                const int status = wallet->readMap(accessKey, storedValues);
                if (status == 0 && storedValues.count()) {
                    QVectorIterator<WebEngineWallet::WebForm::WebField> fieldIt(form.fields);
                    while (fieldIt.hasNext()) {
                        const WebEngineWallet::WebForm::WebField field = fieldIt.next();
                        if (storedValues.contains(field.first) &&
                                storedValues.value(field.first) != field.second) {
                            emit q->saveFormDataRequested(key, url);
                            return;
                        }
                    }
                    // If we got here it means the new credential is exactly
                    // the same as the one already cached ; so skip the
                    // re-saving part...
                    success = true;
                    continue;
                }
            }
            QVectorIterator<WebEngineWallet::WebForm::WebField> fieldIt(form.fields);
            while (fieldIt.hasNext()) {
                const WebEngineWallet::WebForm::WebField field = fieldIt.next();
                values.insert(field.first, field.second);
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

        pendingSaveRequests.remove(key);
    } else {
        qCWarning(WEBENGINEPART_LOG) << "NULL Wallet instance!";
    }

    emit q->saveFormDataCompleted(url, success);
}

void WebEngineWallet::WebEngineWalletPrivate::openWallet()
{
    if (!wallet.isNull()) {
        return;
    }

    wallet.reset(KWallet::Wallet::openWallet(KWallet::Wallet::NetworkWallet(),
                 wid, KWallet::Wallet::Asynchronous));

    if (wallet.isNull()) {
        return;
    }

    connect(wallet.data(), SIGNAL(walletOpened(bool)), q, SLOT(_k_openWalletDone(bool)));
    connect(wallet.data(), SIGNAL(walletClosed()), q, SLOT(_k_walletClosed()));
}

void WebEngineWallet::WebEngineWalletPrivate::removeDataFromCache(const WebFormList &formList)
{
    if (!wallet) {
        qCWarning(WEBENGINEPART_LOG) << "NULL Wallet instance!";
        return;
    }

    QVectorIterator<WebForm> formIt(formList);
    while (formIt.hasNext()) {
        wallet->removeEntry(walletKey(formIt.next()));
    }
}

void WebEngineWallet::WebEngineWalletPrivate::_k_openWalletDone(bool ok)
{
    Q_ASSERT(wallet);

    if (ok &&
            (wallet->hasFolder(KWallet::Wallet::FormDataFolder()) ||
             wallet->createFolder(KWallet::Wallet::FormDataFolder())) &&
            wallet->setFolder(KWallet::Wallet::FormDataFolder())) {

        // Do pending fill requests...
        if (!pendingFillRequests.isEmpty()) {
            QMutableHashIterator<QUrl, FormsData> requestIt(pendingFillRequests);
            while (requestIt.hasNext()) {
                requestIt.next();
                WebEngineWallet::WebFormList list = requestIt.value().forms;
                fillDataFromCache(list);
                q->fillWebForm(requestIt.key(), list);
            }

            pendingFillRequests.clear();
        }

        // Do pending save requests...
        if (!pendingSaveRequests.isEmpty()) {
            QListIterator<QString> keysIt(pendingSaveRequests.keys());
            while (keysIt.hasNext()) {
                saveDataToCache(keysIt.next());
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
        delete wallet.take();
    }
}

void WebEngineWallet::WebEngineWalletPrivate::_k_walletClosed()
{
    if (wallet) {
        wallet.take()->deleteLater();
    }

    emit q->walletClosed();
}

WebEngineWallet::WebEngineWallet(QObject *parent, WId wid)
    : QObject(parent), d(new WebEngineWalletPrivate(this))
{
    d->wid = wid;
}

WebEngineWallet::~WebEngineWallet()
{
    delete d;
}

void WebEngineWallet::fillFormDataCallback(WebEnginePage* page, const WebEngineWallet::WebFormList& formsList)
{
    QList<QUrl> urlList;
    if (!formsList.isEmpty()) {
        const QUrl url(page->url());
        if (d->pendingFillRequests.contains(url)) {
            qCWarning(WEBENGINEPART_LOG) << "Duplicate request rejected!";
        } else {
            WebEngineWalletPrivate::FormsData data;
            data.page = QPointer<WebEnginePage>(page);
            data.forms << formsList;
            d->pendingFillRequests.insert(url, data);
            urlList << url;
        }
    }

    if (!urlList.isEmpty()) {
        fillFormDataFromCache(urlList);
    }

}

void WebEngineWallet::fillFormData(WebEnginePage *page)
{
    if (!page) return;
    auto callback = [this, page](const WebFormList &forms){
        fillFormDataCallback(page, forms);
    };
    d->withFormData(page, callback);
}

static void createSaveKeyFor(WebEnginePage *page, QString *key)
{
    QUrl pageUrl(page->url());
    pageUrl.setPassword(QString());

    QString keyStr = pageUrl.toString();

    *key = QString::number(qHash(keyStr), 16);
}

void WebEngineWallet::saveFormData(WebEnginePage *page, bool ignorePasswordFields)
{
    if (!page) {
        return;
    }

    QString key;
    createSaveKeyFor(page, &key);
    if (d->pendingSaveRequests.contains(key)) {
        return;
    }

    QUrl url = page->url();
    auto callback = [this, key, url](const WebFormList &list){saveFormDataCallback(key, url, list);};
    d->withFormData(page, callback, false, ignorePasswordFields);
}

void WebEngineWallet::saveFormDataCallback(const QString &key, const QUrl& url, const WebEngineWallet::WebFormList& formsList)
{

    if (formsList.isEmpty()) {
        return;
    }
    
    WebFormList list(formsList);

    d->pendingSaveRequests.insert(key, list);

    QMutableVectorIterator<WebForm> it(list);
    while (it.hasNext()) {
        const WebForm form(it.next());
        if (hasCachedFormData(form)) {
            it.remove();
        }
    }

    if (list.isEmpty()) {
        d->confirmSaveRequestOverwrites.insert(url);
        saveFormDataToCache(key);
        return;
    }

    emit saveFormDataRequested(key, url);
}

void WebEngineWallet::removeFormData(WebEnginePage *page)
{
    if (page) {
        auto callback = [this](const WebFormList &list){removeFormDataFromCache(list);};
        d->withFormData(page, callback);
    }
}

void WebEngineWallet::removeFormDataCallback(const WebFormList& list)
{
    removeFormDataFromCache(list);
}


void WebEngineWallet::removeFormData(const WebFormList &forms)
{
    d->pendingRemoveRequests << forms;
    removeFormDataFromCache(forms);
}

void WebEngineWallet::acceptSaveFormDataRequest(const QString &key)
{
    saveFormDataToCache(key);
}

void WebEngineWallet::rejectSaveFormDataRequest(const QString &key)
{
    d->pendingSaveRequests.remove(key);
}

void WebEngineWallet::fillWebForm(const QUrl &url, const WebEngineWallet::WebFormList &forms)
{
    QPointer<WebEnginePage> page = d->pendingFillRequests.value(url).page;
    if (!page) {
        return;
    }

    QString script;
    bool wasFilled = false;

    Q_FOREACH (const WebEngineWallet::WebForm &form, forms) {
        Q_FOREACH (const WebEngineWallet::WebForm::WebField &field, form.fields) {
            QString value = field.second;
            value.replace(QL1C('\\'), QL1S("\\\\"));
            script+= QString(s_javascriptFillInputFragment)
                      .arg(form.framePath)
                      .arg((form.name.isEmpty() ? form.index : form.name))
                      .arg(field.first).arg(value);
        }
    }
    if (!script.isEmpty()) {
        wasFilled = true;
        auto callback = [wasFilled, this](const QVariant &){emit fillFormRequestCompleted(wasFilled);};
        page.data()->runJavaScript(script, callback);
    }
}

WebEngineWallet::WebFormList WebEngineWallet::formsToFill(const QUrl &url) const
{
    return d->pendingFillRequests.value(url).forms;
}

WebEngineWallet::WebFormList WebEngineWallet::formsToSave(const QString &key) const
{
    return d->pendingSaveRequests.value(key);
}

bool WebEngineWallet::hasCachedFormData(const WebForm &form) const
{
    return !KWallet::Wallet::keyDoesNotExist(KWallet::Wallet::NetworkWallet(),
            KWallet::Wallet::FormDataFolder(),
            walletKey(form));
}

void WebEngineWallet::fillFormDataFromCache(const QList<QUrl> &urlList)
{
    if (d->wallet) {
        QListIterator<QUrl> urlIt(urlList);
        while (urlIt.hasNext()) {
            const QUrl url = urlIt.next();
            WebFormList list = formsToFill(url);
            d->fillDataFromCache(list);
            fillWebForm(url, list);
        }
        d->pendingFillRequests.clear();
    }
    d->openWallet();
}

void WebEngineWallet::saveFormDataToCache(const QString &key)
{
    if (d->wallet) {
        d->saveDataToCache(key);
        return;
    }
    d->openWallet();
}

void WebEngineWallet::removeFormDataFromCache(const WebFormList &forms)
{
    if (d->wallet) {
        d->removeDataFromCache(forms);
        d->pendingRemoveRequests.clear();
        return;
    }
    d->openWallet();
}

#include "moc_webenginewallet.cpp"
