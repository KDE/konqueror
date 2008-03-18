/*  This file is part of the KDE project

    Copyright (C) 2007 Fredrik Höglund <fredrik@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/

#include "konqproxystyle.h"

#include <QtGui/QWidget>

KonqProxyStyle::KonqProxyStyle(QWidget *parent)
    : QStyle(), parent(parent)
{
}

QStyle *KonqProxyStyle::style() const
{
    return parent->parentWidget()->style();
}

void KonqProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                        QPainter *painter, const QWidget *widget) const
{
    style()->drawComplexControl(control, option, painter, widget);
}

void KonqProxyStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                                 const QWidget *widget) const
{
    style()->drawControl(element, option, painter, widget);
}

void KonqProxyStyle::drawItemPixmap(QPainter *painter, const QRect &rectangle, int alignment,
                                    const QPixmap &pixmap) const
{
    style()->drawItemPixmap(painter, rectangle, alignment, pixmap);
}

void KonqProxyStyle::drawItemText(QPainter *painter, const QRect &rectangle, int alignment, const QPalette &palette,
                                  bool enabled, const QString &text, QPalette::ColorRole textRole) const
{
    style()->drawItemText(painter, rectangle, alignment, palette, enabled, text, textRole);
}

void KonqProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                                   const QWidget *widget) const
{
    style()->drawPrimitive(element, option, painter, widget);
}

QPixmap KonqProxyStyle::generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
                                            const QStyleOption *option) const
{
    return style()->generatedIconPixmap(iconMode, pixmap, option);
}

QStyle::SubControl KonqProxyStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                                                         const QPoint &position, const QWidget *widget) const
{
    return style()->hitTestComplexControl(control, option, position, widget);
}

QRect KonqProxyStyle::itemPixmapRect(const QRect &rectangle, int alignment, const QPixmap &pixmap) const
{
    return style()->itemPixmapRect(rectangle, alignment, pixmap);
}

QRect KonqProxyStyle::itemTextRect(const QFontMetrics &metrics, const QRect &rectangle, int alignment,
                                   bool enabled, const QString &text) const
{
    return style()->itemTextRect(metrics, rectangle, alignment, enabled, text);
}

int KonqProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    return style()->pixelMetric(metric, option, widget);
}

void KonqProxyStyle::polish(QWidget *widget)
{
    style()->polish(widget);
}

void KonqProxyStyle::polish(QApplication *application)
{
    style()->polish(application);
}

void KonqProxyStyle::polish(QPalette &palette)
{
    style()->polish(palette);
}

QSize KonqProxyStyle::sizeFromContents(ContentsType type, const QStyleOption *option,
                                       const QSize &contentsSize, const QWidget *widget) const
{
    return style()->sizeFromContents(type, option, contentsSize, widget);
}

QIcon KonqProxyStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption *option,
                                   const QWidget *widget) const
{
    return style()->standardIcon(standardIcon, option, widget);
}

QPixmap KonqProxyStyle::standardPixmap(StandardPixmap standardPixmap, const QStyleOption *option,
                                       const QWidget *widget) const
{
    return style()->standardPixmap(standardPixmap, option, widget);
}

QPalette KonqProxyStyle::standardPalette() const
{
    return style()->standardPalette();
}

int KonqProxyStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                              QStyleHintReturn *returnData) const
{
    return style()->styleHint(hint, option, widget, returnData);
}

QRect KonqProxyStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                     SubControl subControl, const QWidget *widget) const
{
    return style()->subControlRect(control, option, subControl, widget);
}

QRect KonqProxyStyle::subElementRect(SubElement element, const QStyleOption *option,
                                     const QWidget *widget) const
{
    return style()->subElementRect(element, option, widget);
}

void KonqProxyStyle::unpolish(QWidget *widget)
{
    style()->unpolish(widget);
}

void KonqProxyStyle::unpolish(QApplication *application)
{
    style()->unpolish(application);
}

