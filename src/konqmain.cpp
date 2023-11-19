/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqapplication.h"

#if QT_VERSION_MAJOR < 6
extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // says QtWebEngine. Note: this must be set before creating the application
    KonquerorApplication app(argc, argv);
    return app.start();
}
