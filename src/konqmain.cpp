/* This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 2016 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqapplication.h"
#include "konqmisc.h"
#include "konqfactory.h"
#include "konqmainwindow.h"
#include "konqmainwindowfactory.h"
#include "konqsessionmanager.h"
#include "konqview.h"
#include "konqsettingsxt.h"
#include "konqurl.h"
#include "konqclosedwindowsmanager.h"


#include <config-konqueror.h>

#include "konqdebug.h"
#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QProcess>

#include <KDBusService>
#include <KStartupInfo>
#include <KWindowSystem>
#include <kwindowsystem_version.h>

#include <KMessageBox>

extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // says QtWebEngine. Note: this must be set before creating the application
    KonquerorApplication app(argc, argv);
    return app.start();
}
