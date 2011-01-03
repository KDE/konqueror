/* This file is part of Validators
 *
 *  Copyright (C) 2008 by  Pino Toscano <pino@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/

#include "clickiconlabel.h"

#include <qevent.h>
#include <qlabel.h>
#include <qlayout.h>

ClickIconLabel::ClickIconLabel(QWidget* parent)
  : QWidget(parent)
{
  QHBoxLayout *lay = new QHBoxLayout(this);
  lay->setMargin(0);
  lay->setSpacing(3);
  m_pixmap = new QLabel(this);
  lay->addWidget(m_pixmap);
  m_pixmap->show();
  m_text = new QLabel(this);
  lay->addWidget(m_text);
  m_text->show();
}

void ClickIconLabel::setText(const QString &text)
{
  m_text->setText(text);
}

void ClickIconLabel::setPixmap(const QPixmap &pixmap)
{
  m_pixmap->setPixmap(pixmap);
}

void ClickIconLabel::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
#if QT_VERSION < 0x040500
  switch (event->type())
  {
    case QEvent::PaletteChange:
      m_text->setPalette(palette());
      break;
    default:
      break;
  }
#endif
}

void ClickIconLabel::mouseReleaseEvent(QMouseEvent* event)
{
  switch (event->button())
  {
    case Qt::LeftButton:
      emit leftClicked();
      break;
    case Qt::MidButton:
      emit midClicked();
      break;
    case Qt::RightButton:
      emit rightClicked();
      break;
    default:
      break;
  }
}

#include "clickiconlabel.moc"
