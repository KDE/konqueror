/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2007 Trolltech ASA
 * Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QPointer>
#include <KDE/KParts/BrowserExtension>

#include <QWebEngineView>
#include <QWebEngineContextMenuData>

class QUrl;
class WebEnginePart;

class WebView : public QWebEngineView
{
    Q_OBJECT
public:
    WebView(WebEnginePart* part, QWidget* parent);
    ~WebView();

    /**
     * Same as QWebPage::load, but with KParts style arguments instead.
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
    virtual void contextMenuEvent(QContextMenuEvent*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::keyPressEvent
     * @internal
     */
    virtual void keyPressEvent(QKeyEvent*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::keyReleaseEvent
     * @internal
     */
    virtual void keyReleaseEvent(QKeyEvent*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::mouseReleaseEvent
     * @internal
     */
    virtual void mouseReleaseEvent(QMouseEvent*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QObject::timerEvent
     * @internal
     */
    virtual void timerEvent(QTimerEvent*);

    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWidget::wheelEvent
     * @internal
     */
    virtual void wheelEvent(QWheelEvent*);

private Q_SLOTS:
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
};

#endif // WEBVIEW_H
