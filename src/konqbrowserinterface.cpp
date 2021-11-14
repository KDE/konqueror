/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#include <KParts/ReadOnlyPart>

#include "konqbrowserinterface.h"
#include "konqmainwindow.h"
#include "urlloader.h"
#include "konqview.h"

KonqBrowserInterface::KonqBrowserInterface(KonqMainWindow *mainWindow, KParts::ReadOnlyPart *part):
    KParts::BrowserInterface(mainWindow), m_mainWindow(mainWindow), m_part(part)
{
}

void KonqBrowserInterface::toggleCompleteFullScreen(bool on)
{
    m_mainWindow->toggleCompleteFullScreen(on);
}

// void KonqBrowserInterface::openUrl(const QUrl &url, const QString& mimetype, const QString &suggestedFileName)
// {
//     KParts::ReadOnlyPart *part = m_mainWindow->m_currentView ? m_mainWindow->m_currentView->part() : nullptr;
//     m_mainWindow->m_urlLoader->openUrl(part, url, mimetype, suggestedFileName);
// }

QString KonqBrowserInterface::partForLocalFile(const QString& path)
{
    return UrlLoader::partForLocalFile(path);
}
