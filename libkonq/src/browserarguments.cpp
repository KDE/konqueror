/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserarguments.h"

#include <QDebug>

struct BrowserArgumentsPrivate {
    QString contentType; // for POST
    bool doPost = false;
    bool redirectedRequest = false;
    bool lockHistory = false;
    bool newTab = false;
    bool forcesNewWindow = false;
    QString suggestedDownloadName;
    BrowserArguments::MaybeInt downloadId = std::nullopt;
    QString embedWith;
    QString openWith;
    bool ignoreDefaultHtmlPart = false;
    BrowserArguments::Action forcedAction = BrowserArguments::Action::UnknownAction;
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

BrowserArguments::MaybeInt BrowserArguments::downloadId() const
{
    return d ? d->downloadId : std::nullopt;
}

void BrowserArguments::setDownloadId(MaybeInt id)
{
    ensureD()->downloadId = id;
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

BrowserArguments::Action BrowserArguments::forcedAction() const
{
    return d ? d->forcedAction : Action::UnknownAction;
}

void BrowserArguments::setForcedAction(Action act)
{
    ensureD()->forcedAction = act;
}

QDebug operator<<(QDebug dbg, BrowserArguments::Action action)
{
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    switch (action) {
        case BrowserArguments::Action::UnknownAction:
            dbg << "UnknownAction";
            break;
        case BrowserArguments::Action::DoNothing:
            dbg << "DoNothing";
            break;
        case BrowserArguments::Action::Save:
            dbg << "Save";
            break;
        case BrowserArguments::Action::Embed:
            dbg << "Embed";
            break;
        case BrowserArguments::Action::Open:
            dbg << "Open";
            break;
        case BrowserArguments::Action::Execute:
            dbg << "Execute";
            break;
    }
    return dbg;
}
