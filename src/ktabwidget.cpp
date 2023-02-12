/* This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Stephan Binner <binner@kde.org>
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
    SPDX-FileCopyrightText: 2009 Urs Wolfer <uwolfer @ kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ktabwidget.h"

#include <QApplication>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QStyle>
#include <QStyleOption>
#include <QTextDocument>
#include <QWheelEvent>
#include <QList>

#include <ksharedconfig.h>
#include <kiconloader.h>
#include <kstringhandler.h>

#include "ktabbar.h"
#include "konqdebug.h"

#include <kconfiggroup.h>

class Q_DECL_HIDDEN KTabWidget::Private
{
public:
    enum {
        ResizeEnabled = 0,
        ResizeDisabled,
        ResizeLater
    } m_resizeSuspend;

    Private(KTabWidget *parent)
        : m_resizeSuspend(ResizeEnabled),
          m_parent(parent),
          m_automaticResizeTabs(false),
          m_tabBarHidden(false)
    {

        KConfigGroup cg(KSharedConfig::openConfig(), "General");
        m_maxLength = cg.readEntry("MaximumTabLength", 30);
        m_minLength = cg.readEntry("MinimumTabLength", 3);
        Q_ASSERT(m_maxLength >= m_minLength);
        m_currentTabLength = m_minLength;
    }

    KTabWidget *m_parent;
    bool m_automaticResizeTabs;
    bool m_tabBarHidden;
    int m_maxLength;
    int m_minLength;
    int m_currentTabLength;

    //holds the full names of the tab, otherwise all we
    //know about is the shortened name
    QStringList m_tabNames;

    bool isEmptyTabbarSpace(const QPoint &)  const;
    void resizeTabs(int changedTabIndex = -1);
    void updateTab(int index);
    void removeTab(int index);

    void slotTabMoved(int from, int to);
};

bool KTabWidget::Private::isEmptyTabbarSpace(const QPoint &point) const
{
    if (m_parent->count() == 0) {
        return true;
    }
    if (m_parent->tabBar()->isHidden()) {
        return false;
    }
    QSize size(m_parent->tabBar()->sizeHint());
    if ((m_parent->tabPosition() == QTabWidget::North && point.y() < size.height()) ||
            (m_parent->tabPosition() == QTabWidget::South && point.y() > (m_parent->height() - size.height()))) {

        QWidget *rightcorner = m_parent->cornerWidget(Qt::TopRightCorner);
        if (rightcorner && rightcorner->isVisible()) {
            if (point.x() >= m_parent->width() - rightcorner->width()) {
                return false;
            }
        }

        QWidget *leftcorner = m_parent->cornerWidget(Qt::TopLeftCorner);
        if (leftcorner && leftcorner->isVisible()) {
            if (point.x() <= leftcorner->width()) {
                return false;
            }
        }

        for (int i = 0; i < m_parent->count(); ++i)
            if (m_parent->tabBar()->tabRect(i).contains(m_parent->tabBar()->mapFromParent(point))) {
                return false;
            }

        return true;
    }

    return false;
}

void KTabWidget::Private::removeTab(int index)
{
    // prevent cascading resize slowness, not to mention crashes due to tab count()
    // and m_tabNames.count() being out of sync!
    m_resizeSuspend = ResizeDisabled;

    // Need to do this here, rather than in tabRemoved().  Calling
    // QTabWidget::removeTab() below may cause a relayout of the tab bar, which
    // will call resizeTabs() immediately.  If m_automaticResizeTabs is true,
    // that will use the m_tabNames[] list before it has been updated to reflect
    // the new tab arrangement.  See bug 190528.
    m_tabNames.removeAt(index);

    m_parent->QTabWidget::removeTab(index);

    const bool doResize = (m_resizeSuspend == ResizeLater) || m_automaticResizeTabs;
    m_resizeSuspend = ResizeEnabled;
    if (doResize) {
        resizeTabs();
    }

}

void KTabWidget::Private::resizeTabs(int changeTabIndex)
{
    if (m_resizeSuspend != ResizeEnabled) {
        m_resizeSuspend = ResizeLater;
        return;
    }

    int newTabLength = m_maxLength;

    if (m_automaticResizeTabs) {
        // Calculate new max length
        int lcw = 0, rcw = 0;

        const int tabBarHeight = m_parent->tabBar()->sizeHint().height();
        if (m_parent->cornerWidget(Qt::TopLeftCorner) &&
                m_parent->cornerWidget(Qt::TopLeftCorner)->isVisible()) {
            lcw = qMax(m_parent->cornerWidget(Qt::TopLeftCorner)->width(), tabBarHeight);
        }
        if (m_parent->cornerWidget(Qt::TopRightCorner) &&
                m_parent->cornerWidget(Qt::TopRightCorner)->isVisible()) {
            rcw = qMax(m_parent->cornerWidget(Qt::TopRightCorner)->width(), tabBarHeight);
        }

        const int maxTabBarWidth = m_parent->width() - lcw - rcw;

        // binary search for the best fitting tab title length; some wiggling was
        // required to make this behave in the face of rounding.
        int newTabLengthHi = m_maxLength + 1;
        int newTabLengthLo = m_minLength;
        int prevTabLengthMid = -1;
        while (true) {
            int newTabLengthMid = (newTabLengthHi + newTabLengthLo) / 2;
            if (prevTabLengthMid == newTabLengthMid) {
                // no change, we're stuck due to rounding.
                break;
            }
            prevTabLengthMid = newTabLengthMid;

            if (m_parent->tabBarWidthForMaxChars(newTabLengthMid) > maxTabBarWidth) {
                newTabLengthHi = newTabLengthMid;
            } else {
                newTabLengthLo = newTabLengthMid;
            }
        }
        newTabLength = qMin(newTabLengthLo, m_maxLength);
    }

    // Update hinted or all tabs
    if (m_currentTabLength != newTabLength) {
        m_currentTabLength = newTabLength;
        for (int i = 0; i < m_parent->count(); i++) {
            updateTab(i);
        }
    } else if (changeTabIndex != -1) {
        updateTab(changeTabIndex);
    }
}

void KTabWidget::Private::updateTab(int index)
{
    QString title = m_automaticResizeTabs ? m_tabNames[ index ] : m_parent->QTabWidget::tabText(index);
    m_parent->setTabToolTip(index, QString());

    if (title.length() > m_currentTabLength) {
        QString toolTipText = title;
        // Remove '&'s, which are indicators for keyboard shortcuts in tab titles. "&&" is replaced by '&'.
        for (int i = toolTipText.indexOf('&'); i >= 0 && i < toolTipText.length(); i = toolTipText.indexOf('&', i + 1)) {
            toolTipText.remove(i, 1);
        }

        if (Qt::mightBeRichText(toolTipText)) {
            m_parent->setTabToolTip(index, toolTipText.toHtmlEscaped());
        } else {
            m_parent->setTabToolTip(index, toolTipText);
        }
    }

    title = KStringHandler::rsqueeze(title, m_currentTabLength).leftJustified(m_minLength, ' ');

    if (m_parent->QTabWidget::tabText(index) != title) {
        m_parent->QTabWidget::setTabText(index, title);
    }
}

void KTabWidget::Private::slotTabMoved(int from, int to)
{
    /* called from Qt slot when Qt has moved the tab, so we only
       need to adjust the m_tabNames list */
    if (m_automaticResizeTabs) {
        QString movedName = m_tabNames.takeAt(from);
        m_tabNames.insert(to, movedName);
    }
}

KTabWidget::KTabWidget(QWidget *parent, Qt::WindowFlags flags)
    : QTabWidget(parent),
      d(new Private(this))
{
    setWindowFlags(flags);
    setTabBar(new KTabBar(this));
    setObjectName("tabbar");
    setAcceptDrops(true);

    connect(tabBar(), SIGNAL(contextMenu(int,QPoint)), SLOT(contextMenu(int,QPoint)));
    connect(tabBar(), SIGNAL(tabDoubleClicked(int)), SLOT(mouseDoubleClick(int)));
    connect(tabBar(), SIGNAL(newTabRequest()), this, SIGNAL(mouseDoubleClick())); // #185487
    connect(tabBar(), SIGNAL(mouseMiddleClick(int)), SLOT(mouseMiddleClick(int)));
    connect(tabBar(), SIGNAL(initiateDrag(int)), SLOT(initiateDrag(int)));
    connect(tabBar(), SIGNAL(testCanDecode(const QDragMoveEvent*,bool&)), SIGNAL(testCanDecode(const QDragMoveEvent*,bool&)));
    connect(tabBar(), SIGNAL(receivedDropEvent(int,QDropEvent*)), SLOT(receivedDropEvent(int,QDropEvent*)));
    connect(tabBar(), SIGNAL(tabMoved(int,int)), SLOT(slotTabMoved(int,int)));
}

KTabWidget::~KTabWidget()
{
    delete d;
}

int KTabWidget::tabBarWidthForMaxChars(int maxLength)
{
    const int hframe  = tabBar()->style()->pixelMetric(QStyle::PM_TabBarTabHSpace, nullptr, tabBar());

    const QFontMetrics fm = tabBar()->fontMetrics();
    int x = 0;
    for (int i = 0; i < count(); ++i) {
        QString newTitle = d->m_tabNames.value(i);
        newTitle = KStringHandler::rsqueeze(newTitle, maxLength).leftJustified(d->m_minLength, ' ');

        int lw = fm.boundingRect(newTitle).width();
        int iw = 0;
        if (!tabBar()->tabIcon(i).isNull()) {
            iw = tabBar()->tabIcon(i).pixmap(style()->pixelMetric(QStyle::PM_SmallIconSize), QIcon::Normal).width() + 4;
        }
        if (tabsClosable()) {
            // FIXME: how to get the size of the close button directly from the tabBar()?
            iw += KIconLoader::SizeSmall * 3 / 2;
        }
        x += (tabBar()->style()->sizeFromContents(QStyle::CT_TabBarTab, nullptr,
                QSize(qMax(lw + hframe + iw, QApplication::globalStrut().width()), 0),
                this)).width();
    }

    return x;
}

QString KTabWidget::tabText(int index) const
{
    if (d->m_automaticResizeTabs) {
        if (index >= 0 && index < count()) {
            if (index >= d->m_tabNames.count()) {
                // Ooops, the tab exists, but tabInserted wasn't called yet.
                // This can happen when inserting the first tab,
                // and calling tabText from slotCurrentChanged,
                // see KTabWidget_UnitTest.
                const_cast<KTabWidget *>(this)->tabInserted(index);
            }
            return d->m_tabNames[ index ];
        } else {
            return QString();
        }
    } else {
        return QTabWidget::tabText(index);
    }
}

void KTabWidget::setTabText(int index, const QString &text)
{
    if (text == tabText(index)) {
        return;
    }

    if (d->m_automaticResizeTabs) {

        tabBar()->setUpdatesEnabled(false); //no flicker

        QTabWidget::setTabText(index, text);

        if (index != -1) {
            if (index >= d->m_tabNames.count()) {
                qCWarning(KONQUEROR_LOG) << "setTabText(" << index << ") called but d->m_tabNames has only" << d->m_tabNames.count() << "entries";
                while (index >= d->m_tabNames.count()) {
                    d->m_tabNames.append(QString());
                }
            }
            d->m_tabNames[ index ] = text;
            d->resizeTabs(index);
        }

        tabBar()->setUpdatesEnabled(true);

    } else {
        QTabWidget::setTabText(index, text);
    }
}

void KTabWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (d->isEmptyTabbarSpace(event->pos())) {
        bool accept = false;
        // The receivers of the testCanDecode() signal has to adjust
        // 'accept' accordingly.
        emit testCanDecode(event, accept);

        event->setAccepted(accept);
        return;
    }

    QTabWidget::dragEnterEvent(event);
}

void KTabWidget::dragMoveEvent(QDragMoveEvent *event)
{
    if (d->isEmptyTabbarSpace(event->pos())) {
        bool accept = false;
        // The receivers of the testCanDecode() signal has to adjust
        // 'accept' accordingly.
        emit testCanDecode(event, accept);

        event->setAccepted(accept);
        return;
    }

    QTabWidget::dragMoveEvent(event);
}

void KTabWidget::dropEvent(QDropEvent *event)
{
    if (d->isEmptyTabbarSpace(event->pos())) {
        emit(receivedDropEvent(event));
        return;
    }

    QTabWidget::dropEvent(event);
}

#ifndef QT_NO_WHEELEVENT
void KTabWidget::wheelEvent(QWheelEvent *event)
{
    if (d->isEmptyTabbarSpace(event->position().toPoint())) {
        QCoreApplication::sendEvent(tabBar(), event);
    } else {
        QTabWidget::wheelEvent(event);
    }
}

void KTabWidget::wheelDelta(int delta)
{
    if (count() < 2) {
        return;
    }

    int page = currentIndex();
    if (delta < 0) {
        page = (page + 1) % count();
    } else {
        page--;
        if (page < 0) {
            page = count() - 1;
        }
    }
    setCurrentIndex(page);
}
#endif

void KTabWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (d->isEmptyTabbarSpace(event->pos())) {
        emit(mouseDoubleClick());
        return;
    }

    QTabWidget::mouseDoubleClickEvent(event);
}

void KTabWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        if (d->isEmptyTabbarSpace(event->pos())) {
            emit(contextMenu(mapToGlobal(event->pos())));
            return;
        }
    }

    QTabWidget::mousePressEvent(event);
}

void KTabWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        if (d->isEmptyTabbarSpace(event->pos())) {
            emit(mouseMiddleClick());
            return;
        }
    }

    QTabWidget::mouseReleaseEvent(event);
}

void KTabWidget::receivedDropEvent(int index, QDropEvent *event)
{
    emit(receivedDropEvent(widget(index), event));
}

void KTabWidget::initiateDrag(int index)
{
    emit(initiateDrag(widget(index)));
}

void KTabWidget::contextMenu(int index, const QPoint &point)
{
    emit(contextMenu(widget(index), point));
}

void KTabWidget::mouseDoubleClick(int index)
{
    emit(mouseDoubleClick(widget(index)));
}

void KTabWidget::mouseMiddleClick(int index)
{
    emit(mouseMiddleClick(widget(index)));
}

void KTabWidget::removeTab(int index)
{
    if (d->m_automaticResizeTabs) {
        const bool wasUpdatesEnabled = updatesEnabled();
        setUpdatesEnabled(false);
        d->removeTab(index);
        setUpdatesEnabled(wasUpdatesEnabled);
    } else {
        d->removeTab(index);
    }
}

void KTabWidget::setAutomaticResizeTabs(bool enabled)
{
    if (d->m_automaticResizeTabs == enabled) {
        return;
    }

    setUpdatesEnabled(false);

    d->m_automaticResizeTabs = enabled;
    if (enabled) {
        d->m_tabNames.clear();
        for (int i = 0; i < count(); ++i) {
            d->m_tabNames.append(tabBar()->tabText(i));
        }
    } else
        for (int i = 0; i < count(); ++i) {
            tabBar()->setTabText(i, d->m_tabNames[ i ]);
        }

    d->resizeTabs();

    setUpdatesEnabled(true);
}

bool KTabWidget::automaticResizeTabs() const
{
    return d->m_automaticResizeTabs;
}

void KTabWidget::resizeEvent(QResizeEvent *event)
{
    QTabWidget::resizeEvent(event);
    d->resizeTabs();
}

void KTabWidget::tabInserted(int idx)
{
    d->m_tabNames.insert(idx, tabBar()->tabText(idx));
}

#include "moc_ktabwidget.cpp"
