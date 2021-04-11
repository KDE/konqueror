/*
 * This file is part of the KDE project.
 *
 * Copyright 2021  Stefano Crocco <posta@stefanocrocco.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "spellcheckermanager.h"
#include "webenginepage.h"

#include <QDir>
#include <QStringList>
#include <QDebug>
#include <QWebEngineProfile>
#include <QMenu>
#include <QAction>

#include <KActionCollection>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <konq_spellcheckingconfigurationdispatcher.h>

#ifndef WEBENGINEPART_DICTIONARY_DIR
#define WEBENGINEPART_DICTIONARY_DIR ""
#endif


SpellCheckerManager::SpellCheckerManager(): QObject()
{
    m_dictionaryDir = QString(WEBENGINEPART_DICTIONARY_DIR);
    connect(KonqSpellCheckingConfigurationDispatcher::self(), &KonqSpellCheckingConfigurationDispatcher::spellCheckingConfigurationChanged,
            this, &SpellCheckerManager::updateConfiguration);
}

SpellCheckerManager::~SpellCheckerManager()
{
}

SpellCheckerManager * SpellCheckerManager::self()
{
    static SpellCheckerManager s_self;
    return &s_self;
}

void SpellCheckerManager::detectDictionaries()
{
    if (m_dictionaryDir.isEmpty()) {
        m_dicts.clear();
        m_enabledDicts.clear();
        return;
    }
    QStringList files = QDir(WEBENGINEPART_DICTIONARY_DIR).entryList({"*.bdic"});
    QStringList languages;
    std::transform(files.constBegin(), files.constEnd(), std::back_inserter(languages), [](const QString &f){return f.chopped(5);});
    QMap<QString, QString> dicts = m_speller.availableDictionaries();
    for (auto it = dicts.constBegin(); it != dicts.constEnd(); ++it) {
        if (languages.contains(it.value())) {
            m_dicts[it.value()] = it.key();
        }
    }
    QMap<QString, QString> preferred = m_speller.preferredDictionaries();
    for (auto it = preferred.constBegin(); it != preferred.constEnd(); ++it) {
        if (m_dicts.contains(it.value())) {
            m_enabledDicts << it.value();
        }
    }
}

void SpellCheckerManager::updateConfiguration(bool spellCheckingEnabled)
{
    detectDictionaries();
    QWebEngineProfile * prof = QWebEngineProfile::defaultProfile();
    prof->setSpellCheckEnabled(spellCheckingEnabled);
    prof->setSpellCheckLanguages(m_enabledDicts);
}

void SpellCheckerManager::setup()
{
    if (m_setupDone) {
        return;
    }
    m_setupDone = true;
    KSharedConfigPtr cfg = KSharedConfig::openConfig();
    KConfigGroup grp = cfg->group("General");
    updateConfiguration(grp.readEntry("SpellCheckingEnabled", false));
}

QMenu * SpellCheckerManager::spellCheckingMenu(const QStringList &suggestions, KActionCollection* coll, WebEnginePage* page)
{
    QMenu *menu = new QMenu();
    menu->setTitle(i18n("Spelling"));

    QWebEngineProfile *prof = QWebEngineProfile::defaultProfile();
    bool spellingEnabled = prof->isSpellCheckEnabled();

    QAction *a = new QAction(i18n("Spell Checking Enabled"), coll);
    a->setCheckable(true);
    a->setChecked(spellingEnabled);
    connect(a, &QAction::toggled, this, &SpellCheckerManager::spellCheckingToggled);
    menu->addAction(a);

    if (spellingEnabled) {
        if (!suggestions.isEmpty()) {
            menu->addSeparator();
            for (const QString &s : suggestions) {
                a = new QAction(s, menu);
                menu->addAction(a);
                connect(a, &QAction::triggered, page, [page, s](){page->replaceMisspelledWord(s);});
            }
        }

        menu->addSeparator();
        QMenu *langs = new QMenu(menu);
        langs->setTitle(i18n("&Languages"));
        menu->addMenu(langs);
        QStringList enabledLangs = prof->spellCheckLanguages();
        for (auto it = m_dicts.constBegin(); it != m_dicts.constEnd(); ++it) {
            a = new QAction(it.value(), coll);
            a->setCheckable(true);
            const QString lang = it.key();
            a->setChecked(enabledLangs.contains(lang));
            connect(a, &QAction::toggled, this, [this, lang](bool on){on ? addLanguage(lang) : removeLanguage(lang);});
            langs->addAction(a);
        }

    }
    return menu;
}

void SpellCheckerManager::addLanguage(const QString& lang)
{
    QWebEngineProfile *prof = QWebEngineProfile::defaultProfile();
    QStringList langs = prof->spellCheckLanguages();
    if (!langs.contains(lang)) {
        langs << lang;
        prof->setSpellCheckLanguages(langs);
    }
}

void SpellCheckerManager::removeLanguage(const QString& lang)
{
    QWebEngineProfile *prof = QWebEngineProfile::defaultProfile();
    QStringList langs = prof->spellCheckLanguages();
    langs.removeAll(lang);
    prof->setSpellCheckLanguages(langs);
}

void SpellCheckerManager::spellCheckingToggled(bool on)
{
    QWebEngineProfile::defaultProfile()->setSpellCheckEnabled(on);
}
