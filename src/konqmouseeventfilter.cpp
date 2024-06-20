/*
    SPDX-FileCopyrightText: 2009 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2016 Anthony Fieroni <bvbfan@abv.bg>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "konqmouseeventfilter.h"
#include "konqframe.h"
#include "konqview.h"
#include "konqmainwindow.h"
#include "konqsettings.h"

#include <QApplication>
#include <QMouseEvent>

class KonqMouseEventFilterSingleton
{
public:
    KonqMouseEventFilter self;
};

Q_GLOBAL_STATIC(KonqMouseEventFilterSingleton, globalMouseEventFilter)

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
            QMouseEvent me(QEvent::MouseButtonPress, ev->pos(), ev->globalPosition(), Qt::RightButton, Qt::RightButton, Qt::NoModifier);
            QApplication::sendEvent(obj, &me);
            QContextMenuEvent ce(QContextMenuEvent::Mouse, ev->pos(), ev->globalPosition().toPoint());
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
    m_bBackRightClick = Konq::Settings::backRightClick();
}
