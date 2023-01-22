/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "spellcheckermanager.h"
#include "webenginepage.h"

#include <QDir>
#include <QStringList>
#include <QDebug>
#include <QWebEngineProfile>
#include <QMenu>
#include <QAction>
#include <QLibraryInfo>
#include <QApplication>

#include <KActionCollection>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>

#include <konq_spellcheckingconfigurationdispatcher.h>

QString SpellCheckerManager::dictionaryDir()
{
    static const char *varName = "QTWEBENGINE_DICTIONARIES_PATH";
    static QString s_dir;
    if (s_dir.isNull()) {
        if (!qEnvironmentVariableIsEmpty(varName)) {
            s_dir = qEnvironmentVariable(varName);
        } else {
            QLatin1String suffix("/qtwebengine_dictionaries");
            s_dir = QApplication::applicationDirPath() + suffix;
            if (!QDir(s_dir).exists()) {
                s_dir = QLibraryInfo::location(QLibraryInfo::DataPath) + suffix;
            }
        }
    }
    return s_dir;
}

SpellCheckerManager::SpellCheckerManager(QWebEngineProfile *profile, QObject *parent): QObject(parent), m_profile(profile)
{
    m_dictionaryDir = dictionaryDir();
    connect(KonqSpellCheckingConfigurationDispatcher::self(), &KonqSpellCheckingConfigurationDispatcher::spellCheckingConfigurationChanged,
            this, &SpellCheckerManager::updateConfiguration);
    KSharedConfigPtr cfg = KSharedConfig::openConfig();
    KConfigGroup grp = cfg->group("General");
    updateConfiguration(grp.readEntry("SpellCheckingEnabled", false));
}

SpellCheckerManager::~SpellCheckerManager()
{
}

void SpellCheckerManager::detectDictionaries()
{
    if (m_dictionaryDir.isEmpty()) {
        m_dicts.clear();
        m_enabledDicts.clear();
        return;
    }
    QStringList files = QDir(m_dictionaryDir).entryList({"*.bdic"});
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
    m_profile->setSpellCheckEnabled(spellCheckingEnabled);
    m_profile->setSpellCheckLanguages(m_enabledDicts);
}

QMenu * SpellCheckerManager::spellCheckingMenu(const QStringList &suggestions, KActionCollection* coll, WebEnginePage* page)
{
    QMenu *menu = new QMenu();
    menu->setTitle(i18n("Spelling"));

    bool spellingEnabled = m_profile->isSpellCheckEnabled();

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
        QStringList enabledLangs = m_profile->spellCheckLanguages();
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
    QStringList langs = m_profile->spellCheckLanguages();
    if (!langs.contains(lang)) {
        langs << lang;
        m_profile->setSpellCheckLanguages(langs);
    }
}

void SpellCheckerManager::removeLanguage(const QString& lang)
{
    QStringList langs = m_profile->spellCheckLanguages();
    langs.removeAll(lang);
    m_profile->setSpellCheckLanguages(langs);
}

void SpellCheckerManager::spellCheckingToggled(bool on)
{
    m_profile->setSpellCheckEnabled(on);
}
