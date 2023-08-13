/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#include <KParts/ReadOnlyPart>

#include "implementations/konqbrowserwindowinterface.h"
#include "konqmainwindow.h"
#include "urlloader.h"
#include "konqview.h"

KonqBrowserWindowInterface::KonqBrowserWindowInterface(KonqMainWindow *mainWindow, KParts::ReadOnlyPart *part):
    KParts::BrowserInterface(mainWindow), m_mainWindow(mainWindow), m_part(part)
{
}

void KonqBrowserWindowInterface::toggleCompleteFullScreen(bool on)
{
    m_mainWindow->toggleCompleteFullScreen(on);
}

bool KonqBrowserWindowInterface::isCorrectPartForLocalFile(KParts::ReadOnlyPart *part, const QString &path)
{
    return part->metaData().pluginId() == UrlLoader::partForLocalFile(path);
}
