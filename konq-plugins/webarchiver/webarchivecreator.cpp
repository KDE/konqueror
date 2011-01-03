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

#include <time.h>

#include <qpixmap.h>
#include <qimage.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QTimerEvent>
#include <QAbstractEventDispatcher>
#include <kapplication.h>
#include <khtml_part.h>

#include "webarchivecreator.moc"

extern "C"
{
	KDE_EXPORT ThumbCreator *new_creator()
	{
		return new WebArchiveCreator;
	}
}

WebArchiveCreator::WebArchiveCreator()
	: m_html(0)
{
}

WebArchiveCreator::~WebArchiveCreator()
{
	delete m_html;
}

bool WebArchiveCreator::create(const QString &path, int width, int height, QImage &img)
{
	if (!m_html)
	{
		m_html = new KHTMLPart;
		connect(m_html, SIGNAL(completed()), SLOT(slotCompleted()));
		m_html->setJScriptEnabled(false);
		m_html->setJavaEnabled(false);
		m_html->setPluginsEnabled(false);
	}
	KUrl url;
	url.setProtocol( "tar" );
	url.setPath( path );
	url.addPath( "index.html" );
	m_html->openUrl( url );
	m_completed = false;
	int timerId = startTimer(5000);
	while (!m_completed)
		kapp->processEvents(QEventLoop::WaitForMoreEvents);
	killTimer(timerId);
    
	// render the HTML page on a bigger pixmap and use smoothScale,
	// looks better than directly scaling with the QPainter (malte)
	QSize pixSize;
	if (width > 400 || height > 600)
	{
		if (height * 3 > width * 4)
			pixSize = QSize(width, width * 4 / 3);
		else
			pixSize = QSize(height * 3 / 4, height);
	}
	else
		pixSize = QSize(400, 600);
	QPixmap pix(pixSize);
	// light-grey background, in case loadind the page failed
	pix.fill( QColor( 245, 245, 245 ) );

	int borderX = pix.width() / width,
	borderY = pix.height() / height;
	QRect rc(borderX, borderY, pix.width() - borderX * 2, pix.height() - borderY *
2);

	QPainter p;
	p.begin(&pix);
	m_html->paint(&p, rc);
	p.end();

	img = pix.toImage();
	return true;
}

void WebArchiveCreator::timerEvent(QTimerEvent *)
{
	m_html->closeUrl();
	m_completed = true;
}

void WebArchiveCreator::slotCompleted()
{
	m_completed = true;
}

ThumbCreator::Flags WebArchiveCreator::flags() const
{
	return DrawFrame;
}

// vim: ts=4 sw=4 et
