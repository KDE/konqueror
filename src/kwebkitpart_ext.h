/*
 * This file is part of the KDE project.
 *
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

#ifndef WEBKITPART_EXT_H
#define WEBKITPART_EXT_H

#include <KDE/KParts/BrowserExtension>
#include <KDE/KParts/TextExtension>
#include <KDE/KParts/HtmlExtension>
#include <kparts/scriptableextension.h>

class KUrl;
class KWebKitPart;
class WebView;
class KSaveFile;
class QWebFrame;

class WebKitBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    WebKitBrowserExtension(KWebKitPart *parent, const QByteArray& cachedHistoryData);
    ~WebKitBrowserExtension();

    virtual int xOffset();
    virtual int yOffset();
    virtual void saveState(QDataStream &);
    virtual void restoreState(QDataStream &);
    void saveHistory();

Q_SIGNALS:
    void saveUrl(const KUrl &);
    void saveHistory(QObject*, const QByteArray&);

public Q_SLOTS:
    void cut();
    void copy();
    void paste();
    void print();

    void slotSaveDocument();
    void slotSaveFrame();
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
    void slotCopyLinkText();
    void slotSaveLinkAs();
    void slotCopyEmailAddress();

    void slotViewDocumentSource();
    void slotViewFrameSource();

    void updateEditActions();
    void updateActions();

    void slotPlayMedia();
    void slotMuteMedia();
    void slotLoopMedia();
    void slotShowMediaControls();
    void slotSaveMedia();
    void slotCopyMedia();
    void slotTextDirectionChanged();
    void slotCheckSpelling();
    void slotSpellCheckSelection();
    void slotSpellCheckDone(const QString&);
    void spellCheckerCorrected(const QString&, int, const QString&);
    void spellCheckerMisspelling(const QString&, int);
    void slotPrintRequested(QWebFrame*);
    void slotPrintPreview();

    void slotOpenSelection();
    void slotLinkInTop();

private:
    WebView* view();
    QPointer<KWebKitPart> m_part;
    QPointer<WebView> m_view;
    quint32 m_spellTextSelectionStart;
    quint32 m_spellTextSelectionEnd;
    QByteArray m_historyData;
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

private:
    KWebKitPart* part() const;
};

/**
 * @internal
 * Implements the HtmlExtension interface
 */
class KWebKitHtmlExtension : public KParts::HtmlExtension,
                             public KParts::SelectorInterface,
                             public KParts::HtmlSettingsInterface
{
    Q_OBJECT
    Q_INTERFACES(KParts::SelectorInterface)
    Q_INTERFACES(KParts::HtmlSettingsInterface)

public:
    KWebKitHtmlExtension(KWebKitPart* part);

    // HtmlExtension
    virtual KUrl baseUrl() const;
    virtual bool hasSelection() const;

    // SelectorInterface
    virtual QueryMethods supportedQueryMethods() const;
    virtual Element querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const;
    virtual QList<Element> querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const;

    // HtmlSettingsInterface
    virtual QVariant htmlSettingsProperty(HtmlSettingsType type) const;
    virtual bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant& value);

private:
    KWebKitPart* part() const;
};

class KWebKitScriptableExtension : public KParts::ScriptableExtension
{
  Q_OBJECT

public:
    KWebKitScriptableExtension(KWebKitPart* part);

    virtual QVariant rootObject();

    virtual QVariant get(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName);

    virtual bool put(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName, const QVariant& value);

    virtual bool setException(ScriptableExtension* callerPrincipal, const QString& message);

    virtual QVariant evaluateScript(ScriptableExtension* callerPrincipal,
                                    quint64 contextObjectId,
                                    const QString& code,
                                    ScriptLanguage language = ECMAScript);

    virtual bool isScriptLanguageSupported(ScriptLanguage lang) const;

private:
     virtual QVariant encloserForKid(KParts::ScriptableExtension* kid);
     KWebKitPart* part();
};

#endif // WEBKITPART_EXT_H
