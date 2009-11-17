/*
    Copyright (c) 2009 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License or ( at
    your option ) version 3 or, at the discretion of KDE e.V. ( which shall
    act as a proxy as in section 14 of the GPLv3 ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "konqrmbeventfilter.h"
#include "konqframe.h"
#include "konqview.h"
#include "konqmainwindow.h"
#include "konqsettingsxt.h"
#include <kglobal.h>
#include <QApplication>

class KonqRmbEventFilterSingleton
{
public:
    KonqRmbEventFilter self;
};

K_GLOBAL_STATIC(KonqRmbEventFilterSingleton, globalRmbEventFilter)

KonqRmbEventFilter* KonqRmbEventFilter::self()
{
    return &globalRmbEventFilter->self;
}

KonqRmbEventFilter::KonqRmbEventFilter()
    : QObject(0)
{
    m_bBackRightClick = KonqSettings::backRightClick();
    if (m_bBackRightClick) {
        qApp->installEventFilter(this);
    }
}

static KonqFrame* parentFrame(QWidget* w)
{
    KonqFrame* frame = 0;
    while (w && !frame) {
        w = w->parentWidget(); // yes this fails if the initial widget itself is a KonqFrame, but this can't happen
        frame = qobject_cast<KonqFrame *>(w);
    }
    return frame;
}

bool KonqRmbEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    const int type = e->type();
    switch(type) {
    case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent *>(e)->button() == Qt::RightButton) {
            return true;
        }
        break;
    case QEvent::MouseButtonRelease:
        if (static_cast<QMouseEvent *>(e)->button() == Qt::RightButton) {
            QWidget* w = static_cast<QWidget *>(obj);
            if (KonqFrame* frame = parentFrame(w)) {
                frame->childView()->mainWindow()->slotBack();
                return true;
            }
        }
        break;
    case QEvent::MouseMove:
    {
        QMouseEvent* ev = static_cast<QMouseEvent *>(e);
        if (ev->buttons() & Qt::RightButton) {
            qApp->removeEventFilter( this );
            QMouseEvent me( QEvent::MouseButtonPress, ev->pos(), Qt::RightButton, Qt::RightButton, Qt::NoModifier );
            QApplication::sendEvent( obj, &me );
            QContextMenuEvent ce( QContextMenuEvent::Mouse, ev->pos(), ev->globalPos() );
            QApplication::sendEvent( obj, &ce );
            qApp->installEventFilter( this );
        }
        break;
    }
    case QEvent::ContextMenu:
    {
        QContextMenuEvent *ev = static_cast<QContextMenuEvent *>(e);
        if (ev->reason() == QContextMenuEvent::Mouse) {
            return true;
        }
        break;
    }
    default:
        break;
    }

    return false;
}

void KonqRmbEventFilter::reparseConfiguration()
{
    const bool oldBackRightClick = m_bBackRightClick;
    m_bBackRightClick = KonqSettings::backRightClick();
    if (!oldBackRightClick && m_bBackRightClick) {
        qApp->installEventFilter(this);
    } else if (oldBackRightClick && !m_bBackRightClick) {
        qApp->removeEventFilter(this);
    }
}
