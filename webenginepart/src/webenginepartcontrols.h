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

#ifndef WEBENGINEPARTCONTROLS_H
#define WEBENGINEPARTCONTROLS_H

#include <QObject>

class QWebEngineProfile;
class WebEnginePartCookieJar;
class SpellCheckerManager;
class WebEnginePartDownloadManager;

class WebEnginePartControls : public QObject
{
    Q_OBJECT

public:

    static WebEnginePartControls *self();

    ~WebEnginePartControls();

    bool isReady() const;

    void setup(QWebEngineProfile *profile);

    SpellCheckerManager* spellCheckerManager() const;

    WebEnginePartDownloadManager* downloadManager() const;

private:

    WebEnginePartControls();

    QWebEngineProfile *m_profile;
    WebEnginePartCookieJar *m_cookieJar;
    SpellCheckerManager *m_spellCheckerManager;
    WebEnginePartDownloadManager *m_downloadManager;
};

#endif // WEBENGINEPARTCONTROLS_H
