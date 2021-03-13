/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2021 Anthony Fieroni <bvbfan@abv.bg>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "settings/webenginesettings.h"
#include "webengineurlrequestinterceptor.h"

WebEngineUrlRequestInterceptor::WebEngineUrlRequestInterceptor(QObject* parent) :
    QWebEngineUrlRequestInterceptor(parent)
{
}

void WebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage) {
        info.block(WebEngineSettings::self()->isAdFiltered(info.requestUrl().url()));
    }
}
