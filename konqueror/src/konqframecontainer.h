/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
    Copyright 2007 David Faure <faure@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef KONQ_FRAMECONTAINER_H
#define KONQ_FRAMECONTAINER_H

#include "konqframe.h"
#include <QSplitter>

/**
 * Base class for containers
 * This implements the Composite pattern: a composite is a type of base element.
 */
class KONQ_TESTS_EXPORT KonqFrameContainerBase : public KonqFrameBase
{
public:
    virtual ~KonqFrameContainerBase() {}

    /**
     * Insert a new frame into the container.
     */
    virtual void insertChildFrame(KonqFrameBase *frame, int index = -1) = 0;
    /**
     * Replace a child frame with another
     */
    virtual void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame);
    /**
     * Split one of our child frames
     */
    KonqFrameContainer *splitChildFrame(KonqFrameBase *frame, Qt::Orientation orientation);

    /**
     * Call this before deleting one of our children.
     */
    virtual void childFrameRemoved(KonqFrameBase *frame) = 0;

    bool isContainer() const Q_DECL_OVERRIDE
    {
        return true;
    }

    KonqFrameBase::FrameType frameType() const Q_DECL_OVERRIDE
    {
        return KonqFrameBase::ContainerBase;
    }

    KonqFrameBase *activeChild() const
    {
        return m_pActiveChild;
    }

    virtual void setActiveChild(KonqFrameBase *activeChild)
    {
        m_pActiveChild = activeChild;
        m_pParentContainer->setActiveChild(this);
    }

    void activateChild() Q_DECL_OVERRIDE
    {
        if (m_pActiveChild) {
            m_pActiveChild->activateChild();
        }
    }

    KonqView *activeChildView() const Q_DECL_OVERRIDE
    {
        if (m_pActiveChild) {
            return m_pActiveChild->activeChildView();
        } else {
            return 0;
        }
    }

protected:
    KonqFrameContainerBase() {}

    KonqFrameBase *m_pActiveChild;
};

/**
 * With KonqFrameContainers and @refKonqFrames we can create a flexible
 * storage structure for the views. The top most element is a
 * KonqFrameContainer. It's a direct child of the MainView. We can then
 * build up a binary tree of containers. KonqFrameContainers are the nodes.
 * That means that they always have two children. Which are either again
 * KonqFrameContainers or, as leaves, KonqFrames.
 */
class KONQ_TESTS_EXPORT KonqFrameContainer : public QSplitter, public KonqFrameContainerBase   // TODO rename to KonqFrameContainerSplitter?
{
    Q_OBJECT
public:
    KonqFrameContainer(Qt::Orientation o,
                       QWidget *parent,
                       KonqFrameContainerBase *parentContainer);
    virtual ~KonqFrameContainer();

    bool accept(KonqFrameVisitor *visitor) Q_DECL_OVERRIDE;

    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) Q_DECL_OVERRIDE;
    void copyHistory(KonqFrameBase *other) Q_DECL_OVERRIDE;

    KonqFrameBase *firstChild()
    {
        return m_pFirstChild;
    }
    KonqFrameBase *secondChild()
    {
        return m_pSecondChild;
    }
    KonqFrameBase *otherChild(KonqFrameBase *child);

    void swapChildren();

    void setTitle(const QString &title, QWidget *sender) Q_DECL_OVERRIDE;
    void setTabIcon(const QUrl &url, QWidget *sender) Q_DECL_OVERRIDE;

    QWidget *asQWidget() Q_DECL_OVERRIDE
    {
        return this;
    }
    KonqFrameBase::FrameType frameType() const Q_DECL_OVERRIDE
    {
        return KonqFrameBase::Container;
    }

    /**
     * Insert a new frame into the splitter.
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) Q_DECL_OVERRIDE;
    /**
     * Call this before deleting one of our children.
     */
    void childFrameRemoved(KonqFrameBase *frame) Q_DECL_OVERRIDE;

    void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame) Q_DECL_OVERRIDE;

    void setAboutToBeDeleted()
    {
        m_bAboutToBeDeleted = true;
    }

protected:
    void childEvent(QChildEvent *) Q_DECL_OVERRIDE;

Q_SIGNALS:
    void setRubberbandCalled();

protected:
    KonqFrameBase *m_pFirstChild;
    KonqFrameBase *m_pSecondChild;
    bool m_bAboutToBeDeleted;
};

#endif /* KONQ_FRAMECONTAINER_H */

