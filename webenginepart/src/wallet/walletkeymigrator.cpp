//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#include "walletkeymigrator.h"

#include "settings/webenginesettings.h"
#include "webenginepart.h"
#include "webenginepage.h"

#include <KWallet>

WebEngineWallet::KeyMigrator::KeyMigrator(WebEngineWallet* wallet, const QUrl& url, const WebEngineWallet::WebFormList& forms) : m_webEngineWallet{wallet}, m_url{url}
{
    findCachedForms(forms);
}


WebEngineWallet::WebFormList WebEngineWallet::KeyMigrator::cachedForms() const
{
    return m_cachedForms;
}

bool WebEngineWallet::KeyMigrator::keyMigrationRequired() const
{
    return !m_keyMigrations.isEmpty();
}

void WebEngineWallet::KeyMigrator::findCachedForms(const WebFormList& allForms)
{
    if (!m_webEngineWallet) {
        return;
    }

    auto oldKeysForForm = [this](const WebForm &f) {
        QString anonKey = f.url.toString(QUrl::RemoveQuery | QUrl::RemoveFragment) + QLatin1Char('#');
        //Order is important here: start from key including name (url#name) and only if
        //that doesn't exist try with key without name (url#)
        QString namedKey = anonKey + f.name;
        QStringList keys;
        if (m_webEngineWallet->hasCachedFormData(f, namedKey)) {
            keys.append(namedKey);
        }
        if (m_webEngineWallet->hasCachedFormData(f, anonKey)) {
            keys.append(anonKey);
        }
        return keys;
    };

    for (auto f : allForms) {
        if (m_webEngineWallet->hasCachedFormData(f)) {
            m_cachedForms.append(f);
            continue;
        } else if (QStringList oldKeys = oldKeysForForm(f); !oldKeys.isEmpty()){
            m_keyMigrations.append(std::make_pair(f, oldKeys));
        }
    }
}

void WebEngineWallet::KeyMigrator::performKeyMigration()
{
    if (!m_webEngineWallet) {
        return;
    }

    if (m_webEngineWallet->isOpen()) {
        for (auto fd : m_keyMigrations) {
            migrateWalletEntry(fd.first, fd.second);
        }
        migrateCustomSettings();

        if (!WebEngineSettings::self()->isNonPasswordStorableSite(m_url.host())) {
            m_webEngineWallet->fillFormData(m_webEngineWallet->part()->page(), m_webEngineWallet->cacheableForms(m_url, m_cachedForms, CacheOperation::Fill));
        }
    } else {
        //By the time the walletOpened signal has been emitted, this object will have gone out of scope
        //so calling the lambda will cause a crash. To avoid this, we create a copy of this object on the heap
        //so that it'll exist after this function ends and use *its* performKeyMigration from the lambda. After
        //that we delete the object
        KeyMigrator *delayed = new KeyMigrator(std::move(*this));
        auto migrateDelayed = [delayed](){
            delayed->performKeyMigration();
            delete delayed;
        };
        connect(delayed->m_webEngineWallet, &WebEngineWallet::walletOpened, delayed->m_webEngineWallet.get(), migrateDelayed, Qt::SingleShotConnection);
        delayed->m_webEngineWallet->openWallet();
    }
}

void WebEngineWallet::KeyMigrator::migrateWalletEntry(const WebForm &form, const QStringList &keys)
{
    if (!m_webEngineWallet) {
        return;
    }

    QString key = form.walletKey();
    //We assume keys is not empty
    QString oldKey = keys.first();
    QStringList elementNames = form.fieldNames();
    if (keys.count() > 1) {
        auto entriesMatch = [form, this, elementNames](const QString &k) {
            QMap<QString, QString> contents;
            if(m_webEngineWallet->wallet()->readMap(k, contents) != 0) {
                return false;
            }
            return std::all_of(contents.keyBegin(), contents.keyEnd(), [elementNames](const QString &n){return elementNames.contains(n);});
        };
        auto it = std::find_if(keys.constBegin(), keys.constEnd(), entriesMatch);
        if (it == keys.constEnd()) {
            return;
        }
        oldKey = *it;
    }
    if (m_webEngineWallet->wallet()->renameEntry(oldKey, key) == 0) {
        m_cachedForms.append(form);
    }
}

void WebEngineWallet::KeyMigrator::migrateCustomSettings()
{
    if (!m_webEngineWallet) {
        return;
    }

    QString customSettingsKey = m_webEngineWallet->customFormsKey(m_url);
    WebEngineSettings::WebFormInfoList customSettings = WebEngineSettings::self()->customizedCacheableFieldsForPage(customSettingsKey);
    WebEngineSettings::WebFormInfoList newSettings;
    newSettings.reserve(customSettings.count());
    for (auto fi : customSettings) {
        if (!fi.name.isEmpty()) {
            newSettings.append(fi);
            continue;
        }
        auto it = std::find_if(m_cachedForms.constBegin(), m_cachedForms.constEnd(), [fi](const WebForm &f){return f.framePath == fi.framePath && f.hasFields(fi.fields);});
        if (it != m_cachedForms.constEnd()) {
            newSettings.append(WebEngineSettings::WebFormInfo{(*it).name, fi.framePath, fi.fields});
        }
    }
    WebEngineSettings::self()->setCustomizedCacheableFieldsForPage(customSettingsKey, newSettings);
}
