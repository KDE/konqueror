/*
   This file is part of the KDE project
   Copyright (C) 2008 David Faure <faure@kde.org>
   Copyright (C) 2009 Christoph Feck <christoph@maxiom.de>

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

#ifndef KONQANIMATEDLOGO_P_H
#define KONQANIMATEDLOGO_P_H

#include <KDE/KAnimatedButton>

class KonqAnimatedLogo : public KAnimatedButton
{
    Q_OBJECT

public:
    /**
     * Create an animated logo button suitable sized for @p menuBar
     *
     * The button is not added to the menu bar.
     */
    KonqAnimatedLogo(QWidget *parent = 0);
    ~KonqAnimatedLogo();

    QSize sizeHint() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void changeEvent(QEvent *event);

private:
    int maxThrobberHeight();
    void setAnimatedLogoSize(int buttonHeight);

    QSize m_size;
};

#endif // KONQANIMATEDLOGO_P_H
