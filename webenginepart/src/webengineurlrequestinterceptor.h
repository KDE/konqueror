/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Anthony Fieroni <bvbfan@abv.bg>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef WEBENGINEURLREQUESTINTERCEPTOR
#define WEBENGINEURLREQUESTINTERCEPTOR

#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>

class WebEngineUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    WebEngineUrlRequestInterceptor(QObject* parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
};

#endif // WEBENGINEURLREQUESTINTERCEPTOR
