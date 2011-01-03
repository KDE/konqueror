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

#ifndef CLICKICONLABEL_H
#define CLICKICONLABEL_H

#include <qwidget.h>

class QLabel;

class ClickIconLabel : public QWidget
{
  Q_OBJECT
public:
  ClickIconLabel(QWidget* parent = 0);

  void setText(const QString &text);
  void setPixmap(const QPixmap &pixmap);

signals:
  void leftClicked();
  void rightClicked();
  void midClicked();

protected:
  void changeEvent(QEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);

private:
  QLabel* m_text;
  QLabel* m_pixmap;
};

#endif
