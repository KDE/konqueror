/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2002 John Firebaugh <jfirebaugh@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqdraggablelabel.h"
#include "konqmainwindow.h"
#include "konqview.h"
#include <kiconloader.h>
#include <QMouseEvent>
#include <QApplication>
#include <QMimeData>
#include <QDrag>

#include <KUrlMimeData>

KonqDraggableLabel::KonqDraggableLabel(KonqMainWindow *mw, const QString &text)
    : QLabel(text)
    , m_mw(mw)
{
    setBackgroundRole(QPalette::Button);
    setAlignment((QApplication::isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft) |
                 Qt::AlignVCenter);
    setAcceptDrops(true);
    adjustSize();
    validDrag = false;
}

void KonqDraggableLabel::mousePressEvent(QMouseEvent *ev)
{
    validDrag = true;
    startDragPos = ev->pos();
}

void KonqDraggableLabel::mouseMoveEvent(QMouseEvent *ev)
{
    if ((startDragPos - ev->pos()).manhattanLength() > QApplication::startDragDistance()) {
        validDrag = false;
        if (m_mw->currentView()) {
            QList<QUrl> lst;
            //TODO KF6: check whether requestedUrl or realUrl is more suitable here
            lst.append(m_mw->currentView()->url());
            QDrag *drag = new QDrag(m_mw);
            QMimeData *md = new QMimeData;
            md->setUrls(lst);
            drag->setMimeData(md);
            QString iconName = KIO::iconNameForUrl(lst.first());

            const QIcon icon = QIcon::fromTheme(iconName, QIcon::fromTheme(QStringLiteral("application-octet-stream")));
            drag->setPixmap(icon.pixmap(KIconLoader::SizeSmall));

            drag->exec();
        }
    }
}

void KonqDraggableLabel::mouseReleaseEvent(QMouseEvent *)
{
    validDrag = false;
}

void KonqDraggableLabel::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasUrls()) {
        ev->accept();
    }
}

void KonqDraggableLabel::dropEvent(QDropEvent *ev)
{
    _savedLst.clear();
    _savedLst = KUrlMimeData::urlsFromMimeData(ev->mimeData());
    if (!_savedLst.isEmpty()) {
        QMetaObject::invokeMethod(this, "delayedOpenURL", Qt::QueuedConnection);
    }
}

void KonqDraggableLabel::delayedOpenURL()
{
    m_mw->openUrl(nullptr, _savedLst.first());
}
