/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KONQBROWSERWINDOWINTERFACE_H
#define KONQBROWSERWINDOWINTERFACE_H

#include <KParts/BrowserInterface>

class KonqMainWindow;

namespace KParts {
    class ReadOnlyPart;
}

/**
 * Implementation of KParts::BrowserInterface which redirects calls to KonqMainWindow
 */
class KonqBrowserWindowInterface : public KParts::BrowserInterface
{
    Q_OBJECT

public:
    /**
     * Default constructor
     */
    KonqBrowserWindowInterface(KonqMainWindow *mainWindow, KParts::ReadOnlyPart *part);
    ~KonqBrowserWindowInterface() override {}

public slots:
    void toggleCompleteFullScreen(bool on);

    /**
     * @brief Whether the given part is the correct one to use to open a local file
     *
     * This use @c KPluginMetadata::pluginId() to compare parts.
     *
     * @param part the part to test
     * @param path the file path
     * @return @e true if @p part is the correct part to use and @e false otherwise
     */
    bool isCorrectPartForLocalFile(KParts::ReadOnlyPart *part, const QString &path);

private:
    KonqMainWindow *m_mainWindow;
    KParts::ReadOnlyPart *m_part;

};

#endif // KONQBROWSERWINDOWINTERFACE_H
