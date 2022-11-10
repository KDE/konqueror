/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2007 Trolltech ASA
    SPDX-FileCopyrightText: 2008 Urs Wolfer <uwolfer @ kde.org>
    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>
    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#ifndef WEBENGINEVIEW_H
#define WEBENGINEVIEW_H

#include <QPointer>
#include <KParts/BrowserExtension>

#include <QWebEngineView>
#include <QtWebEngine/QtWebEngineVersion>
#include <QWebEngineContextMenuData>

class QUrl;
class WebEnginePart;

class WebEngineView : public QWebEngineView
{
    Q_OBJECT
public:
    WebEngineView(WebEnginePart* part, QWidget* parent);
    ~WebEngineView() override;

    /**
     * Same as QWebEnginePage::load, but with KParts style arguments instead.
     *
     * @see KParts::OpenUrlArguments, KParts::BrowserArguments.
     *
     * @param url     the url to load.
     * @param args    reference to a OpenUrlArguments object.
     * @param bargs   reference to a BrowserArguments object.
     */
    void loadUrl(const QUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs);

    QWebEngineContextMenuData contextMenuResult() const;

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::contextMenuEvent
     * @internal
     */
    void contextMenuEvent(QContextMenuEvent*) override;

    /**
     * @brief Improve drag and drop functionality provided by `QWebEngineView`
     *
     * Before Qt 5.15.5, `QWebEngineView` didn't allow to open remote URLs by drag and drop. This function, together with
     * dragEnterEvent(), dragMoveEvent() and m_dragAndDropHandledBySuperclass allow that.
     *
     * In Qt 5.15.5, `QWebEngineView` allows opening remote URLs by drag and drop, but forces doing so in a new tab.
     * This reimplemented method, instead, opens it in the current view.
     *
     * @note Implementing this function with the behavior of `QWebEngineView` from Qt 5.15.5 is impossible without resorting
     * to an ugly hack. The problem is that we can't know whether the dropped URL should actually be opened or not because there
     * can be pages (or part of them) where dropping an URL has a special meaning (for example, uploading a file). In this case,
     * this function shouldn't interfere with the base class implementation. Unfortunately, `QWebEngineView` doesn't provide any
     * way to find out whether the URL should be opened or if the drop had
     * another effect, so we always should rely only on the base class implementation, which isn't configurable. To trick it,
     * we use WebEnginePage::setDropOperationStarted() to tell the page that a drop operation has started: this changes the way
     * WebEnginePage::createWindow() works so that it returns the page itself rather than creating a new page. The main problem,
     * however, is that there's no way to find out when the drop operation has ended, so we have to resort to a crude heuristic
     * to decide when this happens (as described in WebEnginePage::setDropOperationStarted())
     *
     * @see m_dragAndDropHandledBySuperclass
     * @see acceptDragMoveEventIfPossible()
     * @see WebEnginePage::setDropOperationStarted():
     */
    void dropEvent(QDropEvent *e) override;

#ifdef REMOTE_DND_NOT_HANDLED_BY_WEBENGINE
    void dragEnterEvent(QDragEnterEvent *e) override;

    void dragMoveEvent(QDragMoveEvent *e) override;
#endif

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::keyPressEvent
     * @internal
     */
    void keyPressEvent(QKeyEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::keyReleaseEvent
     * @internal
     */
    void keyReleaseEvent(QKeyEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mouseReleaseEvent
     * @internal
     */
    void mouseReleaseEvent(QMouseEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QObject::timerEvent
     * @internal
     */
    void timerEvent(QTimerEvent*) override;

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::wheelEvent
     * @internal
     */
    void wheelEvent(QWheelEvent*) override;

private Q_SLOTS:
    void slotConfigureWebShortcuts();
    void slotStopAutoScroll();

private:
    void editableContentActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &);
    void multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void addSearchActions(QList<QAction*>& selectActions, QWebEngineView*);

    KActionCollection* m_actionCollection;
    QWebEngineContextMenuData m_result;
    QPointer<WebEnginePart> m_part;

    qint32 m_autoScrollTimerId;
    qint32 m_verticalAutoScrollSpeed;
    qint32 m_horizontalAutoScrollSpeed;

    QHash<QString, QChar> m_duplicateLinkElements;
    QMenu *m_spellCheckMenu;

#ifdef REMOTE_DND_NOT_HANDLED_BY_WEBENGINE
    /**
     * @brief Whether a drop enter or move event should be accepted even if the superclass wants to reject it
     *
     * If the event hasn't already been accepted, it contains urls and Konqueror has been configured to allow opening
     * remote URLs by drag & drop, the event is accepted and #m_dragAndDropHandledBySuperclass is set to @c true;
     * otherwise the variable is set to @c false
     * @note This function should only be called from dragEnterEvent() or dragMoveEvent()
     *
     * @param e the event, which should have already been passed to `QWebEngineView::dragEnterEvent` or
     * `QWebEngineView::dragMoveEvent`, according to which method called this.
     */
    void acceptDragMoveEventIfPossible(QDragMoveEvent *e);

    /**
     * @brief Whether a drop action should be handled by `QWebEngineView` or not
     *
     * `QWebEngineView` rejects drop actions except for some cases, in particular local URLs. However,
     * since its documentation doesn't explicitly tell what it accepts and what it rejects, a way to keep
     * trace of whether WebEngineView should handle the drop action itself or not must be used. This is
     * the scope of this variable: if the drop action was rejected by `QWebEngine` but accepted by
     * acceptDragMoveEventIfPossible, this variable is set to @c false; otherwise it's set to @c true.
     */
    bool m_dragAndDropHandledBySuperclass = true;
#endif
};

#endif // WEBENGINEVIEW_H
