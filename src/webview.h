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

#include <KDE/KParts/BrowserExtension>
#include <KDE/KWebView>

#include <QWebHitTestResult>

class KUrl;
class KWebKitPart;
class QWebHitTestResult;
class QWebInspector;
class QLabel;

class WebView : public KWebView
{
    Q_OBJECT
public:
    WebView(KWebKitPart* part, QWidget* parent);
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
    void loadUrl(const KUrl& url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& bargs);

    QWebHitTestResult contextMenuResult() const;

protected:
    /**
     * Reimplemented for internal reasons, the API is not affected.
     *
     * @see QWebView::dropEvent
     * @internal
     */
    virtual void dropEvent(QDropEvent*);

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
    void hideAccessKeys();

private:
    void editableContentActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void selectActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void linkActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void partActionPopupMenu(KParts::BrowserExtension::ActionGroupMap &);
    void multimediaActionPopupMenu(KParts::BrowserExtension::ActionGroupMap&);
    void addSearchActions(QList<QAction*>& selectActions, QWebView*);

    void showAccessKeys();
    bool checkForAccessKey(QKeyEvent *event);
    void makeAccessKeyLabel(const QChar &accessKey, const QWebElement &element);

    KActionCollection* m_actionCollection;
    QWebHitTestResult m_result;
    QPointer<KWebKitPart> m_part;
    QWebInspector* m_webInspector;

    qint32 m_autoScrollTimerId;
    qint32 m_verticalAutoScrollSpeed;
    qint32 m_horizontalAutoScrollSpeed;

    enum AccessKeyState {
        NotActivated,
        PreActivated,
        Activated
    };
    AccessKeyState m_accessKeyActivated;
    QList<QLabel*> m_accessKeyLabels;
    QHash<QChar, QWebElement> m_accessKeyNodes;
    QHash<QString, QChar> m_duplicateLinkElements;
};

#endif // WEBVIEW_H
