/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2020 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef KONQBROWSERINTERFACE_H
#define KONQBROWSERINTERFACE_H

#include <KParts/BrowserInterface>

class KonqMainWindow;

namespace KParts {
    class ReadOnlyPart;
}

/**
 * Implementation of KParts::BrowserInterface which redirects calls to KonqMainWindow
 */
class KonqBrowserInterface : public KParts::BrowserInterface
{
    Q_OBJECT

public:
    /**
     * Default constructor
     */
    KonqBrowserInterface(KonqMainWindow *mainWindow, KParts::ReadOnlyPart *part);
    ~KonqBrowserInterface() override {}

public slots:
    void toggleCompleteFullScreen(bool on);
//     void openUrl(const QUrl &url, const QString &mimetype, const QString &suggestedFileName=QString());
    QString partForLocalFile(const QString &path);

private:
    KonqMainWindow *m_mainWindow;
    KParts::ReadOnlyPart *m_part;

};

#endif // KONQBROWSERINTERFACE_H
