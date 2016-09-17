/* This file is part of the KDE project
   Copyright (C) 1998, 1999, 2016 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQMAINWINDOWFACTORY_H
#define KONQMAINWINDOWFACTORY_H

class KonqMainWindow;
class QUrl;

namespace KonqMainWindowFactory
{

/**
 * Create a new empty window.
 * This layer on top of the KonqMainWindow constructor allows to reuse preloaded windows,
 * and offers restoring windows after a crash.
 * Note: the caller must call show()
 */
KonqMainWindow *createEmptyWindow();

};

#endif // KONQMAINWINDOWFACTORY_H
