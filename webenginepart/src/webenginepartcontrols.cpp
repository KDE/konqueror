/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
#include "certificateerrordialogmanager.h"
#include "webenginewallet.h"
#include "webenginepart.h"
#include "webenginepage.h"
#include "navigationrecorder.h"

#include <KProtocolInfo>

#include <QWebEngineProfile>
#include <QWebEngineUrlScheme>
#include <QWebEngineSettings>
#include <QWebEngineScriptCollection>

WebEnginePartControls::WebEnginePartControls(): QObject(),
    m_profile(nullptr), m_cookieJar(nullptr), m_spellCheckerManager(nullptr), m_downloadManager(nullptr),
    m_certificateErrorDialogManager(new KonqWebEnginePart::CertificateErrorDialogManager(this)),
    m_navigationRecorder(new NavigationRecorder(this))
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

    m_profile->scripts()->insert({WebEngineWallet::formDetectorFunctionsScript(), WebEnginePart::detectRefreshScript()});

    m_profile->installUrlSchemeHandler("error", new WebEnginePartErrorSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("konq", new KonqUrlSchemeHandler(m_profile));
    m_profile->installUrlSchemeHandler("help", new WebEnginePartKIOHandler(m_profile));
    m_profile->installUrlSchemeHandler("tar", new WebEnginePartKIOHandler(m_profile));

    m_profile->setUrlRequestInterceptor(new WebEngineUrlRequestInterceptor(this));

    m_cookieJar = new WebEnginePartCookieJar(profile, this);
    m_spellCheckerManager = new SpellCheckerManager(profile, this);
    m_downloadManager= new WebEnginePartDownloadManager(profile, this);
    m_profile->settings()->setAttribute(QWebEngineSettings::ScreenCaptureEnabled, true);
}

WebEnginePartDownloadManager* WebEnginePartControls::downloadManager() const
{
    return m_downloadManager;
}

SpellCheckerManager* WebEnginePartControls::spellCheckerManager() const
{
    return m_spellCheckerManager;
}

bool WebEnginePartControls::handleCertificateError(const QWebEngineCertificateError& ce, WebEnginePage* page)
{
    return m_certificateErrorDialogManager->handleCertificateError(ce, page);
}

NavigationRecorder * WebEnginePartControls::navigationRecorder() const
{
    return m_navigationRecorder;
}
