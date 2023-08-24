/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "webenginewallet.h"
#include "webenginepage.h"
#include "utils.h"
#include "webenginecustomizecacheablefieldsdlg.h"
#include "webenginepart.h"

#include <webenginepart_debug.h>

#include <KWallet>
#include <KLocalizedString>

#include <QFile>
#include <QPointer>
#include <qwindowdefs.h>
#include <QWebEngineScript>

#include <algorithm>

#include "webenginewalletprivate.cpp"

#define QL1S(x)   QLatin1String(x)
#define QL1C(x)   QLatin1Char(x)

WebEngineSettings::WebFormInfo WebEngineWallet::WebForm::toSettingsInfo() const
{
    QStringList fieldNames;
    fieldNames.reserve(fields.size());
    std::transform(fields.constBegin(), fields.constEnd(), std::back_inserter(fieldNames), [](const WebForm::WebField &f){return f.name;});
    return WebEngineSettings::WebFormInfo{name, framePath, fieldNames};
}

QString WebEngineWallet::customFormsKey(const QUrl& url)
{
    return url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
}

bool WebEngineWallet::hasCustomizedCacheableForms(const QUrl& url){
    return WebEngineSettings::self()->hasPageCustomizedCacheableFields(customFormsKey(url));
}

void WebEngineWallet::WebForm::deleteNotAutoFillableFields()
{
    auto p = std::remove_if(fields.begin(), fields.end(), [](const WebField &f){return !f.isAutoFillable();});
    fields.erase(p, fields.end());
}

WebEngineWallet::WebForm WebEngineWallet::WebForm::withAutoFillableFieldsOnly() const
{
    WebForm form{url, name, index, framePath, {}};
    std::copy_if(fields.constBegin(), fields.constEnd(), std::back_inserter(form.fields), [](const WebField &f){return f.isAutoFillable();});
    return form;
}

bool WebEngineWallet::WebForm::hasPasswords() const
{
    return std::any_of(fields.constBegin(), fields.constEnd(), [](const WebField &f){return f.type == WebFieldType::Password;});
}

bool WebEngineWallet::WebForm::hasAutoFillableFields() const
{
    return std::any_of(fields.constBegin(), fields.constEnd(), [](const WebField &f){return !f.disabled && !f.readOnly && f.autocompleteAllowed;});
}

bool WebEngineWallet::WebForm::hasFieldsWithWrittenValues() const
{
    return std::any_of(fields.constBegin(), fields.constEnd(), [](const WebField &f){return !f.readOnly && !f.value.isEmpty();});
}

WebEngineWallet::WebForm::WebFieldType WebEngineWallet::WebForm::fieldTypeFromTypeName(const QString& name)
{
        static QMap<QString, WebFieldType> s_typeNameMap{
            std::make_pair("text", WebForm::WebFieldType::Text),
            std::make_pair("password", WebForm::WebFieldType::Password),
            std::make_pair("email", WebForm::WebFieldType::Email)
        };
        return s_typeNameMap.value(name, WebFieldType::Other);
}

QString WebEngineWallet::WebForm::fieldNameFromType(WebEngineWallet::WebForm::WebFieldType type, bool localized)
{
    switch(type) {
        case WebFieldType::Text: return localized ? i18nc("Web field with type 'text'", "text") : "text";
        case WebFieldType::Password: return localized ? i18nc("Web field with type 'password'", "password") : "password";
        case WebFieldType::Email: return localized ? i18nc("Web field with type 'e-mail'", "e-mail") :"e-mail";
        case WebFieldType::Other: return localized ? i18nc("Web field with type different from 'text', 'password' or 'e-mail'", "other") :"other";
    }
    //Needed to make the compiler happy. You can't actually get here
    return QString();
}

WebEngineWallet::WebEngineWallet(WebEnginePart *parent, WId wid)
    : QObject(parent), d(new WebEngineWalletPrivate(this))
{
    d->wid = wid;
}

WebEngineWallet::~WebEngineWallet()
{
    delete d;
}

bool WebEngineWallet::isOpen() const
{
    return d->wallet && d->wallet->isOpen();
}

void WebEngineWallet::detectAndFillPageForms(WebEnginePage *page)
{
    QUrl url = page->url();

    //There are no forms in konq: URLs, so don't waste time looking for them
    if (Utils::isKonqUrl(url)) {
        return;
    }

    auto callback = [this, url, page](const WebFormList &forms) {
        emit formDetectionDone(url, !forms.isEmpty(), d->hasAutoFillableFields(forms));
        if (!WebEngineSettings::self()->isNonPasswordStorableSite(url.host())) {
            fillFormData(page, cacheableForms(url, forms, CacheOperation::Fill));
        }
    };
    WebEngineWalletPrivate::detectFormsInPage(page, callback);
}

void WebEngineWallet::fillFormData(WebEnginePage *page, const WebFormList &allForms)
{
    if (!page) return;
    QList<QUrl> urlList;
    if (!allForms.isEmpty()) {
        const QUrl url(page->url());
        if (d->pendingFillRequests.contains(url)) {
            qCWarning(WEBENGINEPART_LOG) << "Duplicate request rejected!";
        } else {
            WebEngineWalletPrivate::FormsData data;
            data.page = QPointer<WebEnginePage>(page);
            data.forms << allForms;
            d->pendingFillRequests.insert(url, data);
            urlList << url;
        }
    } else {
        emit fillFormRequestCompleted(false);
    }
    if (!urlList.isEmpty()) {
        fillFormDataFromCache(urlList);
    }
}

WebEngineWallet::WebFormList WebEngineWallet::cacheableForms(const QUrl& url, const WebEngineWallet::WebFormList& allForms, WebEngineWallet::CacheOperation op) const
{
    WebEngineSettings::WebFormInfoList customForms = WebEngineSettings::self()->customizedCacheableFieldsForPage(customFormsKey(url));
    if (customForms.isEmpty()) {
        return op == CacheOperation::Fill ? d->formsToFill(allForms) : d->formsToSave(allForms);
    }
    WebFormList forms;
    for (const WebForm &form : allForms) {
        auto sameForm = [form](const WebEngineSettings::WebFormInfo &info){return info.name == form.name && info.framePath == form.framePath;};
        auto it = std::find_if(customForms.constBegin(), customForms.constEnd(), sameForm);
        if (it == customForms.constEnd()) {
            continue;
        }
        QVector<WebForm::WebField> fields;
        auto filter = [it](const WebForm::WebField &f){return (*it).fields.contains(f.name);};
        std::copy_if(form.fields.constBegin(), form.fields.constEnd(), std::back_inserter(fields), filter);
        if (fields.isEmpty()) {
            continue;
        }
        WebForm f(form);
        f.fields = std::move(fields);
        forms.append(f);
    }
    return forms;
}

static void createSaveKeyFor(WebEnginePage *page, QString *key)
{
    QUrl pageUrl(page->url());
    pageUrl.setPassword(QString());

    QString keyStr = pageUrl.toString();

    *key = QString::number(qHash(keyStr), 16);
}

void WebEngineWallet::saveFormsInPage(WebEnginePage* page)
{
    if (!page) {
        return;
    }
    WebEngineWalletPrivate::detectFormsInPage(page, [this, page](const WebFormList &forms){saveFormData(page, forms);}, true);
}

void WebEngineWallet::saveFormData(WebEnginePage *page, const WebFormList &allForms, bool force)
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
    WebFormList formsToSave = cacheableForms(url, allForms, CacheOperation::Save);

    if (!formsToSave.isEmpty()) {
        d->pendingSaveRequests.insert(key, formsToSave);
    } else {
        return;
    }

    if (force) {
        saveFormDataToCache(key);
    } else if (std::all_of(formsToSave.constBegin(), formsToSave.constEnd(), [this](const WebForm &f){return hasCachedFormData(f);})) {
        d->confirmSaveRequestOverwrites.insert(url);
        saveFormDataToCache(key);
    } else {
        if (std::any_of(formsToSave.constBegin(), formsToSave.constEnd(), [](const WebForm &f){return f.hasFieldsWithWrittenValues();})) {
            emit saveFormDataRequested(key, url);
        }
    }
}

void WebEngineWallet::savePageDataNow(WebEnginePage* page)
{
    if (!page) {
        return;
    }
    QUrl url = page->url();
    WebEngineWalletPrivate::detectFormsInPage(page, [this, page](const WebFormList &forms){saveFormData(page, forms, true);});
}

void WebEngineWallet::removeFormData(WebEnginePage *page)
{
    if (page) {
    QUrl url = page->url();
        auto callback = [this, url](const WebFormList &forms){
            removeFormDataFromCache(forms);
            WebEngineSettings::self()->removeCacheableFieldsCustomizationForPage(customFormsKey(url));
        };
        WebEngineWalletPrivate::detectFormsInPage(page, callback);
    }
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
            QString value = field.value;
            value.replace(QL1C('\\'), QL1S("\\\\"));
            if (!field.value.isEmpty()) {
                script+= QString("fillFormElement(%1, '%2', '%3', '%4');")
                        .arg(form.framePath.isEmpty() ? "''" : form.framePath)
                        .arg((form.name.isEmpty() ? form.index : form.name))
                        .arg(field.name).arg(value);
            }
        }
    }
    if (!script.isEmpty()) {
        wasFilled = true;
        auto callback = [wasFilled, this](const QVariant &){emit fillFormRequestCompleted(wasFilled);};
        page.data()->runJavaScript(script, QWebEngineScript::ApplicationWorld, callback);
    }
}

WebEngineWallet::WebFormList WebEngineWallet::formsToFill(const QUrl &url) const
{
    return d->pendingFillRequests.value(url).forms;
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
            d->fillDataFromCache(list, hasCustomizedCacheableForms(url));
            fillWebForm(url, list);
        }
        d->pendingFillRequests.clear();
    }
    d->openWallet();
}

void WebEngineWallet::saveFormDataToCache(const QString &key)
{
    if (d->wallet) {
        bool removeEntry = d->saveDataToCache(key);
        if (removeEntry){
            d->pendingSaveRequests.remove(key);
        }
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

void WebEngineWallet::customizeFieldsToCache(WebEnginePage* page, QWidget* widget)
{
    if (!page) {
        return;
    }
    QUrl url = page->url();
    auto callback = [this, url, page, widget](const WebFormList &forms){
        WebEngineSettings::WebFormInfoList oldSettings = WebEngineSettings::self()->customizedCacheableFieldsForPage(customFormsKey(url));
        QMap<QString, QStringList> oldSettingsMap;
        for (const WebEngineSettings::WebFormInfo &info : oldSettings) {
            oldSettingsMap.insert(info.name, info.fields);
        }
        WebEngineCustomizeCacheableFieldsDlg dlg(forms, oldSettingsMap, widget);
        if (dlg.exec() == QDialog::Rejected) {
            return;
        }
        WebFormList selected = dlg.selectedFields();
        if (selected.isEmpty()) {
            return;
        }
        WebEngineSettings::WebFormInfoList vec;
        vec.reserve(selected.size());
        std::transform(selected.constBegin(), selected.constEnd(), std::back_inserter(vec), [](const WebForm &form){return form.toSettingsInfo();});
        WebEngineSettings::self()->setCustomizedCacheableFieldsForPage(customFormsKey(url), vec);
        if (dlg.immediatelyCacheData()) {
            //Pass only the selected fields to saveFormData instead of all the forms in the page, since
            //we already know they're the ones to be cached.
            saveFormData(page, selected, true);
            emit fillFormRequestCompleted(true);
        }
    };
    WebEngineWalletPrivate::detectFormsInPage(page, callback, true);
}

void WebEngineWallet::removeCustomizationForPage(const QUrl& url)
{
    WebEngineSettings::self()->removeCacheableFieldsCustomizationForPage(customFormsKey(url));
}

WebEngineWallet::WebFormList WebEngineWallet::pendingSaveData(const QString& key)
{
    return d->pendingSaveRequests.value(key);
}

QDebug operator<< (QDebug dbg, const WebEngineWallet::WebForm::WebFieldType type)
{
    dbg.maybeSpace() << WebEngineWallet::WebForm::fieldNameFromType(type);
    return dbg;
}

QDebug operator<< (QDebug dbg, const WebEngineWallet::WebForm::WebField field)
{
    QDebugStateSaver state(dbg);
    dbg.maybeSpace() << "WebField<";
    dbg.nospace() << "id: " << field.id;
    dbg.space() << "name: " << field.name;
    dbg << "type:" << field.type;
    dbg << "disabled:" << field.disabled;
    dbg << "readonly:" << field.readOnly;
    dbg << "autocompleteAllowed:" << field.autocompleteAllowed;
    dbg << "value:" << field.value;
    dbg.nospace() << ">";
    return dbg;
}

QDebug operator<< (QDebug dbg, const WebEngineWallet::WebForm form)
{
    QDebugStateSaver state(dbg);
    dbg.nospace() << "WebForm<name: " << form.name;
    dbg.space() << "URL:" << form.url;
    dbg << "index:" << form.index;
    dbg << "framePath:" << form.framePath;
    QStringList fieldNames;
    fieldNames.reserve(form.fields.size());
    std::transform(form.fields.constBegin(), form.fields.constEnd(), std::back_inserter(fieldNames), [](const WebEngineWallet::WebForm::WebField &f){return f.name;});
    dbg << "field names:" << fieldNames.join(", ");
    dbg << ">";
    return dbg;
}

#include "moc_webenginewallet.cpp"
