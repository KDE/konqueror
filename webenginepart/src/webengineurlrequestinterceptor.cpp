/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Anthony Fieroni <bvbfan@abv.bg>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "settings/webenginesettings.h"
#include "webengineurlrequestinterceptor.h"
#include "webenginepartcontrols.h"
#include "navigationrecorder.h"

WebEngineUrlRequestInterceptor::WebEngineUrlRequestInterceptor(QObject* parent) :
    QWebEngineUrlRequestInterceptor(parent)
{
}

void WebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage) {
        if (info.requestUrl().scheme() == QLatin1String("http") && info.firstPartyUrl().scheme() == QLatin1String("https")) {
            info.block(true);
            return;
        }
        info.block(WebEngineSettings::self()->isAdFiltered(info.requestUrl().url()));
    }
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
        WebEnginePartControls::self()->navigationRecorder()->recordRequestDetails(info);
    }

    if (WebEngineSettings::self()->doNotTrack()) {
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));
    }
}
