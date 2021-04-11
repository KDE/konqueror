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

#ifndef SPELLCHECKERMANAGER_H
#define SPELLCHECKERMANAGER_H

#include <QObject>
#include <QMap>
#include <QDateTime>

#include <Sonnet/Speller>

class QMenu;
class KActionCollection;
class QWidget;
class WebEnginePage;

class SpellCheckerManager : public QObject
{
    Q_OBJECT

public:
    SpellCheckerManager();
    ~SpellCheckerManager();

    static SpellCheckerManager* self();

    QMenu *spellCheckingMenu(const QStringList &suggestions, KActionCollection *coll, WebEnginePage *page);

    void setup();

public slots:
    void updateConfiguration(bool spellCheckingEnabled);

private:
    void removeLanguage(const QString &lang);
    void addLanguage(const QString &lang);
    void detectDictionaries();

private slots:
    void spellCheckingToggled(bool on);

private:
    bool m_setupDone = false;
    QString m_dictionaryDir;
    QMap<QString, QString> m_dicts;
    QStringList m_enabledDicts;
    Sonnet::Speller m_speller;
};

#endif // SPELLCHECKERMANAGER_H
