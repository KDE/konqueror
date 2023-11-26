/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "schemehandlers/execschemehandler.h"

#include <webenginepart_debug.h>

#include <QWebEngineUrlRequestJob>

#include <KIO/CommandLauncherJob>
#include <KDialogJobUiDelegate>

#include <QDebug>

using namespace WebEngine;

ExecSchemeHandler::ExecSchemeHandler(QObject* parent): QWebEngineUrlSchemeHandler(parent)
{
}

ExecSchemeHandler::~ExecSchemeHandler() noexcept
{
}

void ExecSchemeHandler::requestStarted(QWebEngineUrlRequestJob* job)
{
    //TODO: devise a more fine-grained approach to allowing which URLs can request URLs with the exec scheme
    if (job->initiator().scheme() != QStringLiteral("konq")) {
        qCDebug(WEBENGINEPART_LOG) << "Exec URL not initiated from konq URL";
        job->fail(QWebEngineUrlRequestJob::RequestDenied);
        return;
    }
    //The path must start with /
    const QString cmdline = job->requestUrl().path();
    const QString exec = cmdline.left(cmdline.indexOf(' '));
    KIO::CommandLauncherJob *launcherJob = new KIO::CommandLauncherJob(cmdline, this);
    launcherJob->setExecutable(exec);
    launcherJob->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
    launcherJob->start();
    //It's not really a failure, but there's no other way to tell QtWebEngine that the request will be handled by someone else
    job->fail(QWebEngineUrlRequestJob::NoError);
}
