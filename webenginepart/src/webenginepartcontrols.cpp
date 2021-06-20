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

#include "webenginepartcontrols.h"
#include "webengineparterrorschemehandler.h"
#include "webenginepartcookiejar.h"
#include "webengineurlrequestinterceptor.h"
#include "webenginepartkiohandler.h"
#include "about/konq_aboutpage.h"
#include "webenginepartcookiejar.h"
#include "spellcheckermanager.h"
#include "webenginepartdownloadmanager.h"

#include <KProtocolInfo>

#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>

WebEnginePartControls::WebEnginePartControls(): QObject(),
    m_profile(nullptr), m_cookieJar(nullptr), m_spellCheckerManager(nullptr), m_downloadManager(nullptr)
{
        QVector<QByteArray> localSchemes = {"error", "konq", "tar"};
        const QStringList protocols = KProtocolInfo::protocols();
        for(const QString &prot : protocols){
            if (KProtocolInfo::defaultMimetype(prot) == "text/html") {
                localSchemes.append(QByteArray(prot.toLatin1()));
            }
        }
        for (const QByteArray &name : qAsConst(localSchemes)){
            QWebEngineUrlScheme scheme(name);
            scheme.setFlags(QWebEngineUrlScheme::LocalScheme|QWebEngineUrlScheme::LocalAccessAllowed);
            scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
            QWebEngineUrlScheme::registerScheme(scheme);
        }
}

WebEnginePartControls::~WebEnginePartControls()
{
}

WebEnginePartControls* WebEnginePartControls::self()
{
    static WebEnginePartControls s_self;
    return &s_self;
}

bool WebEnginePartControls::isReady() const
{
    return m_profile;
}

void WebEnginePartControls::setup(QWebEngineProfile* profile)
{
    if (!profile || isReady()) {
        return;
    }
    m_profile = profile;

    m_profile->installUrlSchemeHandler("error", new WebEnginePartErrorSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("konq", new KonqUrlSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("help", new WebEnginePartKIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("tar", new WebEnginePartKIOHandler(m_profile));

    m_profile->setUrlRequestInterceptor(new WebEngineUrlRequestInterceptor(this));

    m_cookieJar = new WebEnginePartCookieJar(profile, this);
    m_spellCheckerManager = new SpellCheckerManager(profile, this);
    m_downloadManager= new WebEnginePartDownloadManager(profile, this);
}

WebEnginePartDownloadManager* WebEnginePartControls::downloadManager() const
{
    return m_downloadManager;
}

SpellCheckerManager* WebEnginePartControls::spellCheckerManager() const
{
    return m_spellCheckerManager;
}
