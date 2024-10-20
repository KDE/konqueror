/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserarguments.h"

#include "interfaces/downloadjob.h"

#include <QDebug>
#include <QPointer>

using namespace Konq;

struct BrowserArgumentsPrivate {
    QString contentType; // for POST
    bool doPost = false;
    bool redirectedRequest = false;
    bool lockHistory = false;
    bool newTab = false;
    bool forcesNewWindow = false;
    QString suggestedDownloadName;
    QString embedWith;
    QString openWith;
    bool ignoreDefaultHtmlPart = false;
    QPointer<KonqInterfaces::DownloadJob> downloadJob;
    AllowedUrlActions allowedActions;
};

BrowserArguments::BrowserArguments()
{
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

    postData = args.postData;
    frameName = args.frameName;
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

BrowserArgumentsPrivate* BrowserArguments::ensureD()
{
    if (!d) {
        d = new BrowserArgumentsPrivate;
    }
    return d;
}


void BrowserArguments::setContentType(const QString &contentType)
{
    ensureD()->contentType = contentType;
}

void BrowserArguments::setRedirectedRequest(bool redirected)
{
    ensureD()->redirectedRequest = redirected;
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
    ensureD()->doPost = enable;
}

bool BrowserArguments::doPost() const
{
    return d ? d->doPost : false;
}

void BrowserArguments::setLockHistory(bool lock)
{
    ensureD()->lockHistory = lock;
}

bool BrowserArguments::lockHistory() const
{
    return d ? d->lockHistory : false;
}

void BrowserArguments::setNewTab(bool newTab)
{
    ensureD()->newTab = newTab;
}

bool BrowserArguments::newTab() const
{
    return d ? d->newTab : false;
}

void BrowserArguments::setForcesNewWindow(bool forcesNewWindow)
{
    ensureD()->forcesNewWindow = forcesNewWindow;
}

bool BrowserArguments::forcesNewWindow() const
{
    return d ? d->forcesNewWindow : false;
}

void BrowserArguments::setSuggestedDownloadName(const QString& name)
{
    ensureD()->suggestedDownloadName = name;
}

QString BrowserArguments::suggestedDownloadName() const
{
    return d ? d->suggestedDownloadName : QString();
}

KonqInterfaces::DownloadJob* BrowserArguments::downloadJob() const
{
    return d ? d->downloadJob : nullptr;
}

void BrowserArguments::setDownloadJob(KonqInterfaces::DownloadJob* job)
{
    ensureD();
    d->downloadJob = job;
}

QString BrowserArguments::embedWith() const
{
    return d ? d->embedWith : QString();
}

void BrowserArguments::setEmbedWith(const QString& partId)
{
    ensureD()->embedWith = partId;
}

QString BrowserArguments::openWith() const
{
    return d ? d->openWith : QString();
}

void BrowserArguments::setOpenWith(const QString& app)
{
    ensureD()->openWith = app;
}

bool BrowserArguments::ignoreDefaultHtmlPart() const
{
    return d ? d->ignoreDefaultHtmlPart : false;
}

void BrowserArguments::setIgnoreDefaultHtmlPart(bool ignore)
{
    ensureD()->ignoreDefaultHtmlPart = ignore;
}

Konq::AllowedUrlActions BrowserArguments::urlActions() const
{
    return d ? d->allowedActions : AllowedUrlActions();
}

void BrowserArguments::setAllowedUrlActions(const AllowedUrlActions &actions)
{
    ensureD()->allowedActions = actions;
}
