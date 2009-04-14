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

#include "konqanimatedlogo_p.h"

#include <KDE/KIconLoader>

#include <QtCore/QEvent>
#include <QtGui/QStyle>
#include <QtGui/QStyleOptionMenuItem>

KonqAnimatedLogo::KonqAnimatedLogo(QWidget *menuBar, QWidget *parent)
    : KAnimatedButton(parent)
    , m_menuBar(menuBar)
{
    setAutoRaise(true);
    setFocusPolicy(Qt::NoFocus);
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAnimatedLogoSize(maxThrobberHeight());
    m_menuBar->installEventFilter(this);
}

KonqAnimatedLogo::~KonqAnimatedLogo()
{
    m_menuBar->removeEventFilter(this);
}

QSize KonqAnimatedLogo::sizeHint() const
{
    return m_size;
}

bool KonqAnimatedLogo::eventFilter(QObject *watched, QEvent *event)
{
    if (qobject_cast<QWidget *>(watched) == m_menuBar) {
        if (event->type() == QEvent::StyleChange || event->type() == QEvent::FontChange
            || event->type() == QEvent::ApplicationFontChange) {
            // make sure the logo is resized before the menu bar gets the
            // font change event
            setAnimatedLogoSize(maxThrobberHeight());
        }
    }
    return KAnimatedButton::eventFilter(watched, event);
}

int KonqAnimatedLogo::maxThrobberHeight()
{
    // This comes from QMenuBar::sizeHint and QMenuBarPrivate::calcActionRects
    const QFontMetrics fm = m_menuBar->fontMetrics();
    QSize sz(100, fm.height());
    //let the style modify the above size..
    QStyleOptionMenuItem opt;
    opt.fontMetrics = fm;
    opt.state = QStyle::State_Enabled;
    opt.menuRect = m_menuBar->rect();
    opt.text = "dummy";
    sz = m_menuBar->style()->sizeFromContents(QStyle::CT_MenuBarItem, &opt, sz, m_menuBar);
    //kDebug() << "maxThrobberHeight=" << sz.height();
    return sz.height();
}

void KonqAnimatedLogo::setAnimatedLogoSize(int buttonHeight)
{
    // This gives the best results: we force a bigger icon size onto the style, and it'll just have to eat up its margin.
    // So we don't need to ask sizeFromContents at all.
    int iconSize = buttonHeight - 4;
#if 0
    QStyleOptionToolButton opt;
    opt.initFrom(m_paAnimatedLogo);
    const QSize finalSize = style()->sizeFromContents(QStyle::CT_ToolButton, &opt, opt.iconSize, m_paAnimatedLogo);
    //kDebug() << "throbberIconSize=" << buttonHeight << "-" << finalSize.height() - opt.iconSize.height();
    int iconSize = buttonHeight - (finalSize.height() - opt.iconSize.height());
#endif

    m_size = QSize(buttonHeight, buttonHeight);
    setFixedSize(m_size);

    //kDebug() << "buttonHeight=" << buttonHeight << "max iconSize=" << iconSize;
    if ( iconSize < KIconLoader::SizeSmallMedium )
        iconSize = KIconLoader::SizeSmall;
    else if ( iconSize < KIconLoader::SizeMedium  )
        iconSize = KIconLoader::SizeSmallMedium;
    else if ( iconSize < KIconLoader::SizeLarge )
        iconSize = KIconLoader::SizeMedium ;
    else if ( iconSize < KIconLoader::SizeHuge )
        iconSize = KIconLoader::SizeLarge;
    //kDebug() << "final iconSize=" << iconSize;
    if (iconDimensions() != iconSize) {
        setIconSize(QSize(iconSize, iconSize));
        if (!icons().isEmpty()) {
            // ### check why this crashes
            // updateIcons();
        }
    }
}

#include "konqanimatedlogo_p.moc"
