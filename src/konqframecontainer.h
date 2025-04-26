/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_FRAMECONTAINER_H
#define KONQ_FRAMECONTAINER_H

#include "konqframe.h"
#include <QSplitter>

/**
 * @brief Base class for container frames, that is frames which contain other frames
 *
 * This implements the Composite pattern: a composite is a type of base element.
 */
class KONQ_TESTS_EXPORT KonqFrameContainerBase : public KonqFrameBase
{
public:

    ~KonqFrameContainerBase() override {} //!< Destructor

    /**
     * @brief Inserts a new frame into the container
     *
     * @param frame the frame to insert
     * @param index an index representing where the new frame should be inserted.
     * A value of -1 means to insert the frame at the end
     */
    virtual void insertChildFrame(KonqFrameBase *frame, int index = -1) = 0;

    /**
     * @brief Replaces a child frame with another
     *
     * @param oldFrame the frame to replace
     * @param newFrame the frame to replace @p oldFrame with
     */
    virtual void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame);

    /**
     * @brief Splits one of the child frames
     *
     * @param frame the frame to split
     * @param orientation in which direction to split the child frame
     * @return the new frame resulting by splitting @p frame
     */
    KonqFrameContainer *splitChildFrame(KonqFrameBase *frame, Qt::Orientation orientation);

    /**
     * @brief Performs any operation needed before a child frame is deleted
     *
     * @param frame the frame which will be deleted
     * @warning This should be called before a child frame is deleted
     */
    virtual void childFrameRemoved(KonqFrameBase *frame) = 0;

    /**
     * @brief Override of KonqFrameBase::isContainer()
     * @return `true`
     */
    bool isContainer() const override
    {
        return true;
    }

    /**
     * @brief Override of KonqFrameBase::frameType()
     * @return KonqFrameBase::ContainerBase
     */
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::ContainerBase;
    }

    /**
     * @brief The active child frame
     *
     * Note that this doesn't need to be the active frame at the application level:
     * it's the frame that was last active.
     * @return the active child frame
     */
    KonqFrameBase *activeChild() const
    {
        return m_pActiveChild;
    }

    /**
     * @brief Sets which frame will be considered as active
     * @note This doesn't actually activate the child frame
     * @param activeChild the child to consider as active
     */
    virtual void setActiveChild(KonqFrameBase *activeChild)
    {
        m_pActiveChild = activeChild;
        m_pParentContainer->setActiveChild(this);
    }

    /**
     * @brief Override of KonqFrameBase::activateChild()
     *
     * It calls @link KonqFrameBase::activateChild() activateChild()@endlink on
     * the active child, if any
     */
    void activateChild() override
    {
        if (m_pActiveChild) {
            m_pActiveChild->activateChild();
        }
    }

    /**
     * @brief Override of KonqFrameBase::activeChildView()
     * @return the view associated with the active child frame or `nullptr` if there's no
     * active frame
     */
    KonqView *activeChildView() const override
    {
        if (m_pActiveChild) {
            return m_pActiveChild->activeChildView();
        } else {
            return nullptr;
        }
    }

protected:
    KonqFrameContainerBase() {} //!< Default constructor

    /**
     * @brief The child frame to consider as active
     *
     * @note This isn't necessarily the frame active at this moment.
     */
    KonqFrameBase *m_pActiveChild;
};

/**
 * @brief Class representing a frame which contains up to two other frames
 *
 * With KonqFrameContainer and KonqFrame we can create a flexible
 * storage structure for the views. The top most element is a KonqFrameContainer.
 * We can then build up a binary tree of containers. @link KonqFrameContainer KonqFrameContainers@endlink
 * are the nodes, which always have two children. Each child can be either another
 * KonqFrameContainer or, as leaf, a KonqFrame.
 */
class KONQ_TESTS_EXPORT KonqFrameContainer : public QSplitter, public KonqFrameContainerBase   // TODO rename to KonqFrameContainerSplitter?
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param o whether the two child frames should be arranged horizontally or vertically
     * @param parent the parent widget
     * @param parentContainer the container frame this frame belongs to
     */
    KonqFrameContainer(Qt::Orientation o,
                       QWidget *parent,
                       KonqFrameContainerBase *parentContainer);
    ~KonqFrameContainer() override; //!< Destructor

    /**
     * @brief Override of KonqFrameBase::accept()
     *
     * It makes the visitor visit itself and both of its children. If any of the calls
     * to @link KonqFrameVisitor::visit() visit()@endlink return `false`, no other calls
     * are made.
     * @return `true` if all calls to @link KonqFrameVisitor::visit() visit()@endlink succeed
     * and fals otherwise
     */
    bool accept(KonqFrameVisitor *visitor) override;

    /**
     * @brief Override of KonqFrameContainerBase::saveConfig()
     *
     * Besides information about the child frames, it saves information about the
     * orientation and the size of the splitter and the active child
     * @see KonqFrameContainerBase::saveConfig()
     */
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) override;

    /**
     * @brief Override of KonqFrameContainerBase::copyHistory()
     *
     * It copies the history of both children from the corresponding children of @p other
     *
     * @note This function assumes that @p other is another KonqFrameContainer
     *
     * @param other the frame to copy the history from. It must be a KonqFrameContainer
     */
    void copyHistory(KonqFrameBase *other) override;

    /**
     * @brief The first child frame
     * @return the first child frame
     */
    KonqFrameBase *firstChild()
    {
        return m_pFirstChild;
    }

    /**
     * @brief The second child frame
     * @return the second child frame
     */
    KonqFrameBase *secondChild()
    {
        return m_pSecondChild;
    }

    /**
     * @brief The child frame other than the given one
     *
     * @param child one of the two child frames
     * @return the second child if @p child is the first child and the first child
     * if @p child is the first frame
     */
    KonqFrameBase *otherChild(KonqFrameBase *child);

    /**
     * @brief Swaps the order of the two child frames
     *
     * @warning This won't change the order in which the children are shown but only the
     * order they're stored internally
     *
     * @internal
     * This function just swaps the contents of #m_pFirstChild and #m_pSecondChild
     * @endinternal
     */
    void swapChildren();

    /**
     * @brief Override of KonqFrameBase::setTitle()
     *
     * Sets the title of the parent container, but only if @p sender is the active
     * child.
     *
     * @see KonqFrameBase::setTitle()
     */
    void setTitle(const QString &title, QWidget *sender) override;

    /**
     * @brief Override of KonqFrameBase::setTabIcon()
     *
     * Sets the tab icon of the parent container, but only if @p sender is the active
     * child.
     *
     * @see KonqFrameBase::setTabIcon()
     */
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    /**
     * @brief Override of KonqFrameBase::asQWidget()
     *
     * @return `this`
     */
    QWidget *asQWidget() override
    {
        return this;
    }

    /**
     * @brief Override of KonqFrameBase::frameType()
     *
     * @return KonqFrameBase::Container
     */
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::Container;
    }

    /**
     * @brief Inserts a new child frame into the container
     *
     * If the container already has two children, nothing is done.
     *
     * @param frame the frame to insert
     * @param index where to insert the frame. If this is 0, the frame will be inserted
     * as first child, otherwise it will be inserted as second child if the container
     * already has one child and as first child otherwise
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;

    /**
     * @brief Override of KonqFrameBase::childFrameRemoved()
     *
     * @internal
     * It ensures that the variables storing the child frames are updated automatically
     * @endinternal
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    /**
     * @brief Override of KonqFrameContainerBase::replaceChildFrame()
     *
     * Replaces @p oldFrame with @p newFrame in the splitter.
     *
     * @param oldFrame the frame to replace
     * @param newFrame the frame to replace @p oldFrame with
     */
    void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame) override;

    /**
     * @brief Tells the frame that it's going to be deleted
     */
    void setAboutToBeDeleted()
    {
        m_bAboutToBeDeleted = true;
    }

protected:
    /**
     * @brief Override of `QSplitter::childEvent()`
     *
     * It works as the base class version except that it does nothing if the frame
     * is going to be deleted. Since child events can cause the layout to change,
     * there's no need to do so if the frame is going to be deleted.
     * @see setAboutToBeDeleted()
     * @param e the event
     */
    void childEvent(QChildEvent *e) override;

Q_SIGNALS:
    void setRubberbandCalled(); //!< Signal emitted when the splitter moves

protected:
    KonqFrameBase *m_pFirstChild; //!< The first child frame
    KonqFrameBase *m_pSecondChild; //!< The second child frame
    bool m_bAboutToBeDeleted; //!< A flag which is set to `true` when the object is about to be deleted
};

#endif /* KONQ_FRAMECONTAINER_H */

