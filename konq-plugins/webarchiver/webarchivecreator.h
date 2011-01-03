/* 
   Copyright (C) 2001 Malte Starostik <malte@kde.org>

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

// $Id$
// 
#ifndef __webarchivecreator_h__
#define __webarchivecreator_h__

#include <kio/thumbcreator.h>
//Added by qt3to4:
#include <QTimerEvent>

class KHTMLPart;

class WebArchiveCreator : public QObject, public ThumbCreator
{
	Q_OBJECT
public:
	WebArchiveCreator();
	virtual ~WebArchiveCreator();
	virtual bool create(const QString &path, int width, int height, QImage &img);
	virtual Flags flags() const;

protected:
	virtual void timerEvent(QTimerEvent *);

private slots:
	void slotCompleted();

private:
	KHTMLPart *m_html;
	bool m_completed;
};

#endif

// vim: ts=4 sw=4 et
