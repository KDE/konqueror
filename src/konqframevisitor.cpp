/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqframevisitor.h"
#include "konqframe.h"
#include "konqview.h"

bool KonqViewCollector::visit(KonqFrame *frame)
{
    m_views.append(frame->childView());
    return true;
}

QList<KonqView *> KonqViewCollector::collect(KonqFrameBase *topLevel)
{
    KonqViewCollector collector;
    topLevel->accept(&collector);
    return collector.m_views;
}

bool KonqLinkableViewsCollector::visit(KonqFrame *frame)
{
    if (!frame->childView()->isFollowActive()) {
        m_views.append(frame->childView());
    }
    return true;
}

QList<KonqView *> KonqLinkableViewsCollector::collect(KonqFrameBase *topLevel)
{
    KonqLinkableViewsCollector collector;
    topLevel->accept(&collector);
    return collector.m_views;
}

bool KonqModifiedViewsCollector::visit(KonqFrame *frame)
{
    KonqView *view = frame->childView();
    if (view && view->isModified()) {
        m_views.append(view);
    }
    return true;
}

QList<KonqView *> KonqModifiedViewsCollector::collect(KonqFrameBase *topLevel)
{
    KonqModifiedViewsCollector collector;
    topLevel->accept(&collector);
    return collector.m_views;
}

