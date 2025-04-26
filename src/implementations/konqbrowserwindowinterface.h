/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KONQBROWSERWINDOWINTERFACE_H
#define KONQBROWSERWINDOWINTERFACE_H

#include "browserinterface.h"

class KonqMainWindow;

namespace KParts {
    class ReadOnlyPart;
}

/**
 * Implementation of KParts::BrowserInterface which redirects calls to KonqMainWindow
 */
class KonqBrowserWindowInterface : public BrowserInterface
{
    Q_OBJECT

public:
    /**
     * @brief Default constructor
     *
     * @param mainWindow the main window associated with the interface
     * @param part the part associated with the interface
     */
    KonqBrowserWindowInterface(KonqMainWindow *mainWindow, KParts::ReadOnlyPart *part);
    ~KonqBrowserWindowInterface() override {} //!< Destructor

public slots:
    /**
     * @brief Toggles the complete full screen mode on or off
     *
     * @param on whether complete full screen should be turned on or off
     */
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
    KonqMainWindow *m_mainWindow; //!< The main window associated with this object
    KParts::ReadOnlyPart *m_part; //!< The main window associated with this object

};

#endif // KONQBROWSERWINDOWINTERFACE_H
