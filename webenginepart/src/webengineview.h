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
#ifndef WEBENGINEVIEW_H
#define WEBENGINEVIEW_H

#include <QPointer>
#include <KParts/BrowserExtension>

#include <QWebEngineView>
#include <QtWebEngine/QtWebEngineVersion>
#if QTWEBENGINE_VERSION >= QT_VERSION_CHECK(5, 7, 0)
#define HAVE_WEBENGINECONTEXTMENUDATA
#include <QWebEngineContextMenuData>
#else
typedef void* QWebEngineContextMenuData;
#endif

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
};

#endif // WEBENGINEVIEW_H
