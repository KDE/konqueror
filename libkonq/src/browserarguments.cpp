/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserarguments.h"

struct BrowserArgumentsPrivate {
    QString contentType; // for POST
    bool doPost = false;
    bool redirectedRequest = false;
    bool lockHistory = false;
    bool newTab = false;
    bool forcesNewWindow = false;
};

BrowserArguments::BrowserArguments()
{
    softReload = false;
    trustedSource = false;
    d = nullptr; // Let's build it on demand for now
}

BrowserArguments::BrowserArguments(const BrowserArguments &args)
{
    d = nullptr;
    (*this) = args;
}

BrowserArguments &BrowserArguments::operator=(const BrowserArguments &args)
{
    if (this == &args) {
        return *this;
    }

    delete d;
    d = nullptr;

    softReload = args.softReload;
    postData = args.postData;
    frameName = args.frameName;
    docState = args.docState;
    trustedSource = args.trustedSource;

    if (args.d) {
        d = new BrowserArgumentsPrivate(*args.d);
    }

    return *this;
}

BrowserArguments::~BrowserArguments()
{
    delete d;
    d = nullptr;
}

void BrowserArguments::setContentType(const QString &contentType)
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    d->contentType = contentType;
}

void BrowserArguments::setRedirectedRequest(bool redirected)
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    d->redirectedRequest = redirected;
}

bool BrowserArguments::redirectedRequest() const
{
    return d ? d->redirectedRequest : false;
}

QString BrowserArguments::contentType() const
{
    return d ? d->contentType : QString();
}

void BrowserArguments::setDoPost(bool enable)
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    d->doPost = enable;
}

bool BrowserArguments::doPost() const
{
    return d ? d->doPost : false;
}

void BrowserArguments::setLockHistory(bool lock)
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    d->lockHistory = lock;
}

bool BrowserArguments::lockHistory() const
{
    return d ? d->lockHistory : false;
}

void BrowserArguments::setNewTab(bool newTab)
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    d->newTab = newTab;
}

bool BrowserArguments::newTab() const
{
    return d ? d->newTab : false;
}

void BrowserArguments::setForcesNewWindow(bool forcesNewWindow)
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    d->forcesNewWindow = forcesNewWindow;
}

bool BrowserArguments::forcesNewWindow() const
{
    return d ? d->forcesNewWindow : false;
}
