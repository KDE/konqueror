/*
    Copyright (c) 2009 David Faure <faure@kde.org>
    Copyright (c) 2016 Anthony Fieroni <bvbfan@abv.bg>

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

#include "konqmouseeventfilter.h"
#include "konqframe.h"
#include "konqview.h"
#include "konqmainwindow.h"
#include "konqsettingsxt.h"
#include <kglobal.h>

#include <QApplication>
#include <QMouseEvent>

class KonqMouseEventFilterSingleton
{
public:
    KonqMouseEventFilter self;
};

K_GLOBAL_STATIC(KonqMouseEventFilterSingleton, globalMouseEventFilter)

KonqMouseEventFilter *KonqMouseEventFilter::self()
{
    return &globalMouseEventFilter->self;
}

KonqMouseEventFilter::KonqMouseEventFilter()
    : QObject(nullptr)
{
    reparseConfiguration();
    qApp->installEventFilter(this);
}

static KonqMainWindow* parentWindow(QWidget *w)
{
    KonqFrame *frame = nullptr;
    while (w && !frame) {
        w = w->parentWidget(); // yes this fails if the initial widget itself is a KonqFrame, but this can't happen
        frame = qobject_cast<KonqFrame *>(w);
    }
    if (!frame) {
        return nullptr;
    }
    if (auto view = frame->childView()) {
        return view->mainWindow();
    }
    return nullptr;
}

bool KonqMouseEventFilter::eventFilter(QObject *obj, QEvent *e)
{
    const int type = e->type();
    switch (type) {
    case QEvent::MouseButtonPress:
        switch (static_cast<QMouseEvent*>(e)->button()) {
        case Qt::RightButton:
            if (m_bBackRightClick) {
                return true;
            }
            break;
        case Qt::ForwardButton:
            if (auto window = parentWindow(qobject_cast<QWidget*>(obj))) {
                window->slotForward();
                return true;
            }
            break;
        case Qt::BackButton:
            if (auto window = parentWindow(qobject_cast<QWidget*>(obj))) {
                window->slotBack();
                return true;
            }
            break;
        default:
            break;
        }
        break;
    case QEvent::MouseButtonRelease:
        if (!m_bBackRightClick) {
            break;
        }
        if (static_cast<QMouseEvent*>(e)->button() == Qt::RightButton) {
            if (auto window = parentWindow(qobject_cast<QWidget*>(obj))) {
                window->slotBack();
                return true;
            }
        }
        break;
    case QEvent::MouseMove: {
        QMouseEvent *ev = static_cast<QMouseEvent*>(e);
        if (m_bBackRightClick && ev->buttons() & Qt::RightButton) {
            qApp->removeEventFilter(this);
            QMouseEvent me(QEvent::MouseButtonPress, ev->pos(), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
            QApplication::sendEvent(obj, &me);
            QContextMenuEvent ce(QContextMenuEvent::Mouse, ev->pos(), ev->globalPos());
            QApplication::sendEvent(obj, &ce);
            qApp->installEventFilter(this);
        }
        break;
    }
    case QEvent::ContextMenu: {
        QContextMenuEvent *ev = static_cast<QContextMenuEvent*>(e);
        if (m_bBackRightClick && ev->reason() == QContextMenuEvent::Mouse) {
            return true;
        }
        break;
    }
    default:
        break;
    }

    return false;
}

void KonqMouseEventFilter::reparseConfiguration()
{
    m_bBackRightClick = KonqSettings::backRightClick();
}
