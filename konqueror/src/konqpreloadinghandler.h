/* This file is part of the KDE project
   Copyright (C) 2000-2016 David Faure <faure@kde.org>

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

#ifndef KONQPRELOADINGHANDLER_H
#define KONQPRELOADINGHANDLER_H

class KonqMainWindow;

class KonqPreloadingHandler
{
public:
    KonqPreloadingHandler();

    static KonqPreloadingHandler *self();

    /**
     * When the "preloaded" flag is set in this process, it means
     * this process is the one that has an empty window ready to be used.
     * The flag is set to false again as soon as the window is used.
     */
    bool isPreloaded() const;

    void makePreloadedWindow();

    KonqMainWindow *takePreloadedWindow();

private:
    void setPreloadedFlag(bool preloaded);

    bool m_preloaded = false;
    KonqMainWindow *m_preloadedWindow = nullptr;
};

#endif // KONQPRELOADINGHANDLER_H
