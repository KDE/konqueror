/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __konq_frame_h__
#define __konq_frame_h__

#include "konqfactory.h"
#include <KParts/ReadOnlyPart> // for the inline QPointer usage

#include <QPointer>
#include <QWidget>
#include <QCheckBox>
#include <QPixmap>
#include <QEvent>
#include <QList>

#include <KConfig>

class KonqFrameStatusBar;
class KonqFrameVisitor;
class QVBoxLayout;
class QUrl;

class KonqView;
class KonqFrameBase;
class KonqFrame;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KSeparator;

namespace KParts
{
class ReadOnlyPart;
}

typedef QList<KonqView *> ChildViewList;

/**
 * @brief Abstract base class for all kind of frames used by Konqueror
 *
 * A frame is a widget which can contain either other frames or a KonqView
 */
class KONQ_TESTS_EXPORT KonqFrameBase
{
public:
    /**
     * @brief Enum describing what information should be recorded when saving a frame
     */
    enum Option {
        None = 0x0, //!< Don't store any information
        SaveUrls = 0x01, //!< Store information about the currently shown URL but no information about history
        SaveHistoryItems = 0x02 //!< Store information about the currently shown URL and about history
    };
    Q_DECLARE_FLAGS(Options, Option)

    /**
     * @brief Enum for the different kind of frames
     */
    enum FrameType {
        View, //!< A frame which contains a single KonqView
        Tabs, //!< A frame which contains all the tabs in the window
        ContainerBase, //!< A generic frame which contains many frames
        Container, //!< A frame which contains many frames
        MainWindow //!< The main window
    };

    virtual ~KonqFrameBase() {} //!< Destructor

    /**
     * @brief Whether the frame is a container
     *
     * @return `true` if the frame is a container and `false` otherwise
     */
    virtual bool isContainer() const = 0;

    /**
     * @brief Implements the visitor pattern
     *
     * Implementations of this function should call the \link KonqFrameVisitor::visit() visit()\endlink
     * method of the visitor for the frame itself and each of its children (if any)
     *
     * @param visitor the visitor
     * @return `true` if @p visitor successfully visited the frame and `false` otherwise
     */
    virtual bool accept(KonqFrameVisitor *visitor) = 0;

    /**
     * @brief Writes to a config group information allowing to restore the frame
     *
     * @param config the group where to write the information
     * @param prefix a string to add to each key to make it unique in the group
     * @param options which information to include in the config group
     * @param docContainer the doc container
     * @param id an identifier to use
     * @param depth the level inside the frame hierarchy
     */
    virtual void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) = 0;

    /**
     * @brief Copies history information from another frame to this frame
     *
     * @param other the frame to copy history information from
     */
    virtual void copyHistory(KonqFrameBase *other) = 0;

    /**
     * @brief This frame parent frame
     *
     * @return the frame which contains this one or `nullptr` if this is a toplevel frame
     */
    KonqFrameContainerBase *parentContainer() const
    {
        return m_pParentContainer;
    }

    /**
     * @brief Changes the this frame parent frame
     *
     * @param parent the new parent frame
     */
    void setParentContainer(KonqFrameContainerBase *parent)
    {
        m_pParentContainer = parent;
    }

    /**
     * @brief Changes the frame title
     *
     * @param title the new title
     * @param sender the widget which requested to change the title
     */
    virtual void setTitle(const QString &title, QWidget *sender) = 0;

    /**
     * @brief Changes the frame icon
     *
     * @param url the url of the frame
     * @param sender the widget which requested to change the icon
     */
    virtual void setTabIcon(const QUrl &url, QWidget *sender) = 0;

    /**
     * @brief The `QWidget` representation of the frame
     *
     * @return the `QWidget` representing the frame. Implementations in classes
     * which derive from `QWidget` may just return `this`.
     */
    virtual QWidget *asQWidget() = 0;

    /**
     * @brief The type of the frame
     *
     * @return The type of the frame
     */
    virtual FrameType frameType() const = 0;

    /**
     * @brief Activates a child view
     *
     * If the frame only has one child view, that view is activated. If the frame
     * has multiple child views, it will activate the one which was last active
     */
    virtual void activateChild() = 0;

    /**
     * @brief The active child view
     *
     * @return The active child view or `nullptr` if no active view exists
     */
    virtual KonqView *activeChildView() const = 0;

    /**
     * @brief A string version of the given frame type
     *
     * @param frameType the frame type
     * @return The name of the given frame type
     */
    static QString frameTypeToString(const FrameType frameType);

    /**
     * @brief The frame type corresponding to a given frame type name
     * @param str the frame type name
     * @return the FrameType whose string representation is @p str or View if @p str
     * isn't a valid frame type name
     */
    static FrameType frameTypeFromString(const QString &str);

protected:

    /**
     * @brief Default constructor
     */
    KonqFrameBase();

    KonqFrameContainerBase *m_pParentContainer; //!< The container containing this frame
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KonqFrameBase::Options)

/**
 * @brief A frame which contains a single view and the corresponding statusbar
 *
 * It takes care of the widget handling i.e. it attaches/detaches the view widget and activates
 * them on click at the statusbar.
 *
 * @internal
 * We create a vertical layout in the frame, with the view and the KonqFrameStatusBar.
 * @endinternal
 */
class KONQ_TESTS_EXPORT KonqFrame : public QWidget, public KonqFrameBase
{
    Q_OBJECT

public:

    /**
     * @brief constructor
     *
     * @param parent the parent widget
     * @param parentContainer the frame which contains the new object
     */
    explicit KonqFrame(QWidget *parent, KonqFrameContainerBase *parentContainer = nullptr);
    ~KonqFrame() override; //!< Destructor

    /**
     * @brief Override of KonqFrameBase::isContainer()
     *
     * @return `false`
     */
    bool isContainer() const override
    {
        return false;
    }

    /**
     * @brief Override of KonqFrameBase::accept()
     *
     * It makes the visitor visit itself
     * @param visitor the visitor
     * @return the return value of `visitor->visit(this)`
     */
    bool accept(KonqFrameVisitor *visitor) override;

    /**
     * @brief Creates a new part from a KonqViewFactory and inserts it in the frame
     *
     * If @p viewFactory can't create a part, either because it's null or for any other
     * reason, the behavior depends on @p allowPlaceholder: if it's `true`, a placeholder
     * part will be used, otherwise a nothing will be done.
     *
     * @param viewFactory the factory to create the new part
     * @param allowPlaceholder whether a placeholder part can be used instead of the real part
     * @return the new part or `nullptr` if @p viewFactory couldn't create a part and @p allowPlaceholder
     * is `false`. If @p allowPlaceholder is `true`, the returned part could be a placeholder part
     */
    KParts::ReadOnlyPart *attach(const KonqViewFactory &viewFactory, bool allowPlaceholder = false);

    /**
     * @brief Inserts the widget and the statusbar into the layout
     *
     * @param widget the widget to insert
     */
    void attachWidget(QWidget *widget);

    /**
     * @brief Inserts a widget at the top of the part's widget in the layout
     *
     * @internal
     * This was used for the find functionality but it currently seem to be unused
     * @endinternal
     * @param widget the widget to insert
     */
    void insertTopWidget(QWidget *widget);

    /**
     * @brief The part that is currently connected to the frame
     * @return the part whose widget is contained in the frame
     */
    KParts::ReadOnlyPart *part()
    {
        return m_pPart;
    }

    /**
     * @brief returns the view that is currently connected to the frame
     * @return the view associated with the frame
     */
    KonqView *childView() const;

    /**
     * @brief Whether the part displayed in the frame is the active one
     * @return `true` if the part displayed in the frame is the active one and `false` otherwise
     */
    bool isActivePart();

    /**
     * @brief Sets the view associated with the frame
     * @param child the new view
     */
    void setView(KonqView *child);

    /**
     * @brief Override of KonqFrameBase::saveConfig()
     *
     * @see KonqFrameBase::saveConfig()
     */
    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) override;

    /**
     * @brief Override of KonqFrameBase::copyHistory()
     *
     * @param other the frame to copy history from
     */
    void copyHistory(KonqFrameBase *other) override;

    /**
     * @brief Override of KonqFrameBase::setTitle()
     *
     * @see KonqFrameBase::setTitle()
     */
    void setTitle(const QString &title, QWidget *sender) override;

    /**
     * @brief Override of KonqFrameBase::setTabIcon()
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
     * @return KonqFrameBase::View
     */
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::View;
    }

    /**
     * @brief The layout used by the frame
     *
     * @return the layout used by the frame
     */
    QVBoxLayout *layout()const
    {
        return m_pLayout;
    }

    /**
     * @brief The statusbar used by the frame
     * @return the statusbar used by the frame
     */
    KonqFrameStatusBar *statusbar() const
    {
        return m_pStatusBar;
    }

    /**
     * @brief Override of KonqFrameBase::activateChild()
     *
     * It activates the view associated with the frame, unless it's a passive view.
     *
     * @note If the URL of the view is empty, this function automatically gives focus
     * to the location bar
     */
    void activateChild() override;

    /**
     * @brief Override of KonqFrameBase::activeChildView()
     *
     * @return the view associated with the frame
     */
    KonqView *activeChildView() const override;

    /**
     * @brief The frame title
     *
     * @return the frame title
     */
    QString title() const
    {
        return m_title;
    }

public Q_SLOTS:

    /**
     * @brief Slot called when the statusbar has been clicked
     *
     * It activates the part associated with the frame
     */
    void slotStatusBarClicked();

    /**
     * @brief Slot called when the "link" checkbox on the statusbar is clicked
     *
     * It links or unlinks the view according to @p mode
     *
     * @param mode whether the checkbox was toggled on or off
     */
    void slotLinkedViewClicked(bool mode);

    /**
     * Is called when 'Remove View' is called from the popup menu
     */
    void slotRemoveView();

private:
    QVBoxLayout *m_pLayout; //!< The layout used by the frame
    QPointer<KonqView> m_pView; //!< The view associated with the frame

    QPointer<KParts::ReadOnlyPart> m_pPart; //!< The part displayed in the frame

    /**
     * @brief A separator
     *
     * This is most likely obsolete
     * @todo Remove it
     */
    KSeparator *m_separator;
    KonqFrameStatusBar *m_pStatusBar; //!< The statusbar

    QString m_title; //!< The frame title
};

#endif
