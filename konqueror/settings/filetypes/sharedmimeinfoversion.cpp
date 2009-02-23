/*  This file is part of the KDE project
    Copyright (C) 2009 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "sharedmimeinfoversion.h"
#include <kdebug.h>
#include <kprocess.h>

static int version()
{
    static int s_version = 0;
    if (s_version == 0) {
        KProcess proc;
        proc << "update-mime-database";
        proc << "-v";
        proc.setOutputChannelMode(KProcess::OnlyStderrChannel);
        proc.setReadChannel(KProcess::StandardError);
        const int exitCode = proc.execute();
        if (exitCode) {
            kWarning() << proc.program() << "exited with error code" << exitCode;
        }
        QByteArray versionLine = proc.readLine();
        const QByteArray expectedStart = "update-mime-database (shared-mime-info) ";
        if (versionLine.startsWith(expectedStart)) {
            versionLine = versionLine.mid(expectedStart.length());
            versionLine.chop(1); // \n
            const int dot = versionLine.indexOf('.');
            if (dot != -1) {
                const int major = versionLine.left(dot).toInt();
                const int minor = versionLine.mid(dot+1).toInt();
                s_version = major * 100 + minor; // 1.04 -> 104
            } else {
                kWarning() << "Unexpected version scheme from update-mime-database: got" << versionLine;
                s_version = -1;
            }
        } else {
            kWarning() << "Unexpected output from update-mime-database -v:" << versionLine;
            kWarning() << "Expected that it would start with:" << expectedStart;
            s_version = -1;
        }
    }
    return s_version;
}

bool SharedMimeInfoVersion::supportsIcon()
{
    return version() >= 40;
}
