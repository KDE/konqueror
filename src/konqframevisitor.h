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

/**
 * @brief Base class for all frame visitors
 *
 * A frame visitor is a class which implements the visitor pattern, allowing to
 * recursively iterate on all frames inside a parent frame.
 *
 * Derived classes should override the visit() methods, either all or only some
 * of them, and do what they need with the frame.
 */
class KonqFrameVisitor
{
public:

    /**
     * @brief Enum describing how the visitor should behave when there are multiple tabs
     */
    enum VisitorBehavior {
        VisitAllTabs = 1, //!< The visitor should visit frames in all tabs
        VisitCurrentTabOnly = 2 //!< The visitor should only visit frames in the current tab
    };

    /**
     * @brief Constructor
     *
     * @param behavior how to behave when visiting a frame which contains multiple tabs
     */
    KonqFrameVisitor(VisitorBehavior behavior = VisitAllTabs) : m_behavior(behavior) {}

    virtual ~KonqFrameVisitor() {} //!< Destructor

    /**
     * @brief Visits a KonqFrame
     *
     * @param frm the frame
     * @return `true` if the visit was successful and `false` if something went wrong.
     * The base class implementation always returns `true`
     */
    virtual bool visit(KonqFrame *frm)
    {
        Q_UNUSED(frm);
        return true;
    }

    virtual bool visit(KonqFrameContainer *cont)
    {
        Q_UNUSED(cont)
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

    /**
     * @brief Method called after visiting the frame and all its children
     *
     * @param cont the frame container
     * @return `true` if everything was successful and `false` if something went wrong
     */
    virtual bool endVisit(KonqFrameContainer *cont)
    {
        Q_UNUSED(cont);
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

    /**
     * @brief Whether to visit child frames in all tabs or only in the current tab
     * @return `true` if frames in all tabs should be visited and `false` if they only
     * should be visited in the current tab
     */
    bool visitAllTabs() const
    {
        return m_behavior & VisitAllTabs;
    }
private:
    VisitorBehavior m_behavior;
};

/**
 * @brief Visitor which collects all views, recursively.
 */
class KONQ_TESTS_EXPORT KonqViewCollector : public KonqFrameVisitor
{
public:
    /**
     * @brief Frame visitor which collects all views in the given frame and all its children
     *
     * If @p topLevel contains multiple tabs, views are collected from all tabs
     *
     * @param topLevel the first frame to collect views from
     * @return a list of all views associated with @p topLevel and its children
     */
    static QList<KonqView *> collect(KonqFrameBase *topLevel);

    /**
     * @brief Override of KonqFrameVisitor::visit()
     *
     * It adds the view associated with @p frame to the list of all views.
     *
     * @return `true`
     */
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
    QList<KonqView *> m_views; //!< A list of all collected views
};

/**
 * @brief Frame visitor which collects all views that can currently be linked in the given frame and its children
 *
 * This excludes invisible tabs ([#116714](https://bugs.kde.org/show_bug.cgi?id=116714)).
 */
class KonqLinkableViewsCollector : public KonqFrameVisitor
{
public:

    /**
     * @brief A list of all linkable views
     *
     * If @p topLevel contains multiple tabs, views are collected from all tabs
     *
     * @param topLevel the first frame to collect views from
     * @return a list of all linkable views associated with @p topLevel and its children
     */
    static QList<KonqView *> collect(KonqFrameBase *topLevel);

    /**
     * @brief Override of KonqFrameVisitor::visit()
     *
     * It checks whether the view associated with @p frame is not already linked and,
     * if so, it adds to the list of such views.
     *
     * @return `true`
     */
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
    KonqLinkableViewsCollector() : KonqFrameVisitor(VisitCurrentTabOnly) {} //!< Constructor
    QList<KonqView *> m_views; //!< A list of all linkable views
};

/**
 * @brief Visitor which collects the list of views that have modified data in them
 *
 * This is used for the warning-before-closing-a-tab.
 */
class KonqModifiedViewsCollector : public KonqFrameVisitor
{
public:

    /**
     * @brief A list of all  views with modified data in them
     *
     * If @p topLevel contains multiple tabs, views are collected from all tabs
     *
     * @param topLevel the first frame to collect views from
     * @return a list of all views with modified data associated with @p topLevel and its children
     */
    static QList<KonqView *> collect(KonqFrameBase *topLevel);

    /**
     * @brief Override of KonqFrameVisitor::visit()
     *
     * It checks whether the view associated with @p frame has modified data and,
     * if so, it adds to the list of such views.
     *
     * @return `true`
     */
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
    QList<KonqView *> m_views; //!< A list of all views with modified data
};

#endif /* KONQ_FRAMEVISITOR_H */

