/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2009 Dawit Alemayehu <adawit@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WEBKITPART_EXT_H
#define WEBKITPART_EXT_H

#include <KDE/KParts/BrowserExtension>
#include <KDE/KParts/TextExtension>
#include <KDE/KParts/HtmlExtension>

class KUrl;
class KWebKitPart;
class WebView;
class KSaveFile;

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    WebKitBrowserExtension(KWebKitPart *parent,
                           const QString& historyFileName);
    ~WebKitBrowserExtension();

    virtual int xOffset();
    virtual int yOffset();
    virtual void saveState(QDataStream &);
    virtual void restoreState(QDataStream &);

Q_SIGNALS:
    void saveUrl(const KUrl &);

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void slotSaveDocument();
    void slotSaveFrame();
    void print();
    void printFrame();
    void searchProvider();
    void reparseConfiguration();
    void disableScrolling();

    void zoomIn();
    void zoomOut();
    void zoomNormal();
    void toogleZoomTextOnly();
    void slotSelectAll();

    void slotFrameInWindow();
    void slotFrameInTab();
    void slotFrameInTop();
    void slotReloadFrame();
    void slotBlockIFrame();

    void slotSaveImageAs();
    void slotSendImage();
    void slotCopyImageURL();
    void slotCopyImage();
    void slotViewImage();
    void slotBlockImage();
    void slotBlockHost();

    void slotCopyLinkURL();
    void slotSaveLinkAs();

    void slotViewDocumentSource();
    void slotViewFrameSource();

    void updateEditActions();
    void slotPlayMedia();
    void slotMuteMedia();
    void slotLoopMedia();
    void slotShowMediaControls();
    void slotSaveMedia();
    void slotCopyMedia();

private:
    WebView* view();
    QWeakPointer<KWebKitPart> m_part;
    QWeakPointer<WebView> m_view;
    KSaveFile* m_historyContentSaver;
};

/**
 * @internal
 * Implements the TextExtension interface
 */
class KWebKitTextExtension : public KParts::TextExtension
{
    Q_OBJECT
public:
    KWebKitTextExtension(KWebKitPart* part);

    virtual bool hasSelection() const;
    virtual QString selectedText(Format format) const;
    virtual QString completeText(Format format) const;

    KWebKitPart* part() const;
};

/**
 * @internal
 * Implements the HtmlExtension interface
 */
class KWebKitHtmlExtension : public KParts::HtmlExtension,
                             public KParts::SelectorInterface
{
    Q_OBJECT
    Q_INTERFACES(KParts::SelectorInterface)

public:
    KWebKitHtmlExtension(KWebKitPart* part);

    // HtmlExtension
    virtual KUrl baseUrl() const;
    virtual bool hasSelection() const;

    // SelectorInterface
    virtual QueryMethods supportedQueryMethods() const;
    virtual Element querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const;
    virtual QList<Element> querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const;

    KWebKitPart* part() const;
};

#endif // WEBKITPART_EXT_H
