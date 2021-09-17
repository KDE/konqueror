/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_FRAMEVISITOR_H
#define KONQ_FRAMEVISITOR_H

#include <QList>
#include "konqprivate_export.h"

class KonqFrameBase;
class KonqView;
class KonqFrame;
class KonqFrameContainer;
class KonqFrameTabs;
class KonqMainWindow;

class KonqFrameVisitor
{
public:
    enum VisitorBehavior { VisitAllTabs = 1, VisitCurrentTabOnly = 2 };
    KonqFrameVisitor(VisitorBehavior behavior = VisitAllTabs) : m_behavior(behavior) {}
    virtual ~KonqFrameVisitor() {}
    virtual bool visit(KonqFrame *)
    {
        return true;
    }
    virtual bool visit(KonqFrameContainer *)
    {
        return true;
    }
    virtual bool visit(KonqFrameTabs *)
    {
        return true;
    }
    virtual bool visit(KonqMainWindow *)
    {
        return true;
    }

    virtual bool endVisit(KonqFrameContainer *)
    {
        return true;
    }
    virtual bool endVisit(KonqFrameTabs *)
    {
        return true;
    }
    virtual bool endVisit(KonqMainWindow *)
    {
        return true;
    }

    bool visitAllTabs() const
    {
        return m_behavior & VisitAllTabs;
    }
private:
    VisitorBehavior m_behavior;
};

/**
 * Collects all views, recursively.
 */
class KONQ_TESTS_EXPORT KonqViewCollector : public KonqFrameVisitor
{
public:
    static QList<KonqView *> collect(KonqFrameBase *topLevel);
    bool visit(KonqFrame *frame) override;
    bool visit(KonqFrameContainer *) override
    {
        return true;
    }
    bool visit(KonqFrameTabs *) override
    {
        return true;
    }
    bool visit(KonqMainWindow *) override
    {
        return true;
    }
private:
    QList<KonqView *> m_views;
};

/**
 * Collects all views that can currently be linked; this excludes invisible tabs (#116714).
 */
class KonqLinkableViewsCollector : public KonqFrameVisitor
{
public:
    static QList<KonqView *> collect(KonqFrameBase *topLevel);
    bool visit(KonqFrame *frame) override;
    bool visit(KonqFrameContainer *) override
    {
        return true;
    }
    bool visit(KonqFrameTabs *) override
    {
        return true;
    }
    bool visit(KonqMainWindow *) override
    {
        return true;
    }
private:
    KonqLinkableViewsCollector() : KonqFrameVisitor(VisitCurrentTabOnly) {}
    QList<KonqView *> m_views;
};

/**
 * Returns the list of views that have modified data in them,
 * for the warning-before-closing-a-tab.
 */
class KonqModifiedViewsCollector : public KonqFrameVisitor
{
public:
    static QList<KonqView *> collect(KonqFrameBase *topLevel);
    bool visit(KonqFrame *frame) override;
    bool visit(KonqFrameContainer *) override
    {
        return true;
    }
    bool visit(KonqFrameTabs *) override
    {
        return true;
    }
    bool visit(KonqMainWindow *) override
    {
        return true;
    }
private:
    QList<KonqView *> m_views;
};

#endif /* KONQ_FRAMEVISITOR_H */

