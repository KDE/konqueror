//  This file is part of the KDE project
//  SPDX-FileCopyrightText: 2024 Stefano Crocco <stefano.crocco@alice.it>
// 
//  SPDX-License-Identifier: LGPL-2.0-or-later

#ifndef WEBENGINEWALLET_KEYMIGRATOR_H
#define WEBENGINEWALLET_KEYMIGRATOR_H

#include "webenginewallet.h"

#include <QPointer>
#include <QUrl>

/**
 * Helper class to migrate KWallet form keys from the scheme used up to Konqueror-24.08
 * to the one used from Konqueror-24.12.
 *
 * The old scheme has the format: `url#form_name`, while the new is `url#form_name_or_id-index`,
 * so there are two differences:
 * - the new format includes the index of the form in the page (in the unlikely case there are
 * multiple forms with the same name/id)
 * - if a form only has an id, the id is used instead of the name (this was already done for elements)
 *
 * Because of this change, KWallet entries created by Konqueror 24.08 and earlier won't be used by
 * Konqueror 24.12 and later. To avoid inconveniencing the users, when a page with forms is loaded and
 * no KWallet entries with the new scheme are found, we look for keys in the old scheme and attempt to
 * migrate them. Unfortunately, this means that the attempt will also be made for pages for which the
 * user never stored login information (of course, in this case, it will always fail).
 *
 * It would have been better to migrate all keys at once the first time Konqueror 24.12 is launched:
 * this can't be done, however, because to create the new keys we need to load each page. Besides,
 * there's no way to list all the keys created by Konqueror in KWallet.
 */
class WebEngineWallet::KeyMigrator
{

public:
    /**
     * @brief Constructor
     * @param wallet the object managing the wallet whose keys should be migrated
     * @param url the url of the page to migrate
     * @param forms a list of the forms in the page
     *
     * This function detects the cached forms and whether a key migration is needed, as described in
     * findCachedForms().
     */
    KeyMigrator(WebEngineWallet *wallet, const QUrl &url, const WebEngineWallet::WebFormList &forms);

    /**
     * @brief The list of forms with cached information among those passed to findCachedForms()
     * @return the list of forms with cached information
     */
    WebFormList cachedForms() const;

    /**
     * @brief Whether a key migration is needed before filling forms in the page
     * @return `true` if a key migration is needed before filling forms in the page
     * and `false` if all the keys have already been migrated (including the case when there aren't KWallet
     * entries for the forms, both using the old or the new scheme)
     */
    bool keyMigrationRequired() const;

    /**
     * @brief Migrates the keys for the forms from the old scheme to the new schemes, then attempts to fill the forms again
     *
     * @note If the wallet is not open, a request to open the wallet is made and the migration and subsequent form filling is
     * made only after the wallet has been opened. If the wallet is already open, everything is synchronous (up to the point
     * when the javascript code to fill the form is run: that's always an asynchronous operation).
     */
    void performKeyMigration();

private:
    /**
     * @brief Finds the forms with cached entries among those given and determines whether some of them require migrating keys
     *
     * This function determines the information needed by cachedForms(), hasCachedForms(), keyMigrationRequired(), so it should
     * always be called before using them.
     *
     * @param allForms the available forms
     */
    void findCachedForms(const WebFormList &allForms);

    /**
     * @brief Migrates the wallet entry for a form from the old to the new scheme
     *
     * If KWallet only contains one old-scheme key which may correspond to @p form, that key is migrated. If instead there are
     * two keys, the decision of which key to use is made looking at the entry contents, in particular to the fields it lists.
     * If there's only one key whose fields are all in @p form, that key is used; if there are multiple such keys, the first
     * one is used (this should only happen very rarely).
     *
     * @param form the form whose entry should be migrated
     * @param keys a list of possible old-scheme keys for the form. It is assumed that an KWallet entry exists for
     * each key
     * @note This function assumes that there's a KWallet entry corresponding to each of @p keys, so the caller must make sure
     * of it
     */
    void migrateWalletEntry(const WebForm &form, const QStringList &keys);

    /**
     * @brief Migrates the custom settings for the forms
     *
     * This attempts to detect which forms have custom fields autofilled, and updates those settings so that the form names
     * use the new scheme. As for migrateWalletEntry(), each entry fields are used to match configuration entries with forms.
     */
    void migrateCustomSettings();

public:

    /**
     * @brief Class used to represent information about an entry which needs to be migrated
     *
     * The first element of the pair is the form, while the second element is a list of possible
     * old-scheme keys for the form
     */
    using KeyMigrationData = std::pair<WebForm, QStringList>;

    QPointer<WebEngineWallet> m_webEngineWallet; //!< The wallet object to use to fill forms
    QUrl m_url; //!< The URL of the forms belong to
    WebFormList m_cachedForms; //!< A list of form for which there are KWallet entries using the new scheme
    QList<KeyMigrationData> m_keyMigrations; //!< A list of forms whose entries should be migrated from the old to the new scheme
};

#endif // WEBENGINEWALLET_KEYMIGRATOR_H
