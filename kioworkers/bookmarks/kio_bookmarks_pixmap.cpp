/*
SPDX-FileCopyrightText: 2008 Xavier Vello <xavier.vello@gmail.com>

SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "kio_bookmarks.h"

#include <stdio.h>
#include <stdlib.h>


#include <KImageCache>
#include <KConfig>
#include <KConfigGroup>

#include <QBuffer>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <KImageCache>

using namespace KIO;

void BookmarksProtocol::echoImage( const QString &type, const QString &string, const QString &sizestring )
{
    int size = sizestring.toInt();
    if (size == 0) {
        if (type == "icon")
            size = 16;
        else
            size = 128;
    }

    // Although KImageCache supports caching pixmaps, we need to send the data to the
    // destination process anyways so don't bother, just hold onto the image data.
    QImage image;
    bool ok = cache->findImage(type + string + QString::number(size), &image);
    if (!ok || image.isNull()) {
        const QIcon icon = QIcon::fromTheme(string);
        QPixmap pix; // QIcon can't give us a QImage anyways.

        if (type == "icon") {
            pix = icon.pixmap(size, size);
        } else {
            pix = QPixmap(size, size);
            pix.fill(Qt::transparent);

            QPainter painter(&pix);
            painter.setOpacity(0.3);
            painter.drawPixmap(pix.rect(), icon.pixmap(size, size), pix.rect());
        }

        image = pix.toImage();
        cache->insertImage(type + string + QString::number(size), image);
    }

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");

    WorkerBase::mimeType("image/png");
    data(buffer.buffer());
}
