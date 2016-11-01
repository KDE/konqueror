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

#ifndef WEBENGINEPART_EXT_H
#define WEBENGINEPART_EXT_H

#include "kwebenginepartlib_export.h"

#include <QPointer>

#include <KParts/BrowserExtension>
#include <KParts/TextExtension>
#include <KParts/HtmlExtension>
#include <KParts/HtmlSettingsInterface>
#include <KParts/ScriptableExtension>
#include <KParts/SelectorInterface>

class QUrl;
class WebEnginePart;
class WebView;

class KWEBENGINEPARTLIB_EXPORT WebEngineBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    WebEngineBrowserExtension(WebEnginePart *parent, const QByteArray& cachedHistoryData);
    ~WebEngineBrowserExtension();

    virtual int xOffset() override;
    virtual int yOffset() override;
    virtual void saveState(QDataStream &) override;
    virtual void restoreState(QDataStream &) override;
    void saveHistory();

Q_SIGNALS:
    void saveUrl(const QUrl &);
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
    void toogleZoomToDPI();
    void slotSelectAll();

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
    //void slotPrintRequested(QWebFrame*);
    void slotPrintPreview();

    void slotOpenSelection();
    void slotLinkInTop();

private:
    WebView* view();
    QPointer<WebEnginePart> m_part;
    QPointer<WebView> m_view;
    quint32 m_spellTextSelectionStart;
    quint32 m_spellTextSelectionEnd;
    QByteArray m_historyData;
};

/**
 * @internal
 * Implements the TextExtension interface
 */
class WebEngineTextExtension : public KParts::TextExtension
{
    Q_OBJECT
public:
    WebEngineTextExtension(WebEnginePart* part);

    bool hasSelection() const Q_DECL_OVERRIDE;
    QString selectedText(Format format) const Q_DECL_OVERRIDE;
    QString completeText(Format format) const Q_DECL_OVERRIDE;

private:
    WebEnginePart* part() const;
};

/**
 * @internal
 * Implements the HtmlExtension interface
 */
class WebEngineHtmlExtension : public KParts::HtmlExtension,
                             public KParts::SelectorInterface,
                             public KParts::HtmlSettingsInterface
{
    Q_OBJECT
    Q_INTERFACES(KParts::SelectorInterface)
    Q_INTERFACES(KParts::HtmlSettingsInterface)

public:
    WebEngineHtmlExtension(WebEnginePart* part);

    // HtmlExtension
    QUrl baseUrl() const Q_DECL_OVERRIDE;
    bool hasSelection() const Q_DECL_OVERRIDE;

    // SelectorInterface
    QueryMethods supportedQueryMethods() const Q_DECL_OVERRIDE;
    Element querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const Q_DECL_OVERRIDE;
    QList<Element> querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const Q_DECL_OVERRIDE;

    // HtmlSettingsInterface
    QVariant htmlSettingsProperty(HtmlSettingsType type) const Q_DECL_OVERRIDE;
    bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant& value) Q_DECL_OVERRIDE;

private:
    WebEnginePart* part() const;
};

class WebEngineScriptableExtension : public KParts::ScriptableExtension
{
  Q_OBJECT

public:
    WebEngineScriptableExtension(WebEnginePart* part);

    QVariant rootObject() Q_DECL_OVERRIDE;

    QVariant get(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName) Q_DECL_OVERRIDE;

    bool put(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName, const QVariant& value) Q_DECL_OVERRIDE;

    bool setException(ScriptableExtension* callerPrincipal, const QString& message) Q_DECL_OVERRIDE;

    QVariant evaluateScript(ScriptableExtension* callerPrincipal,
                                    quint64 contextObjectId,
                                    const QString& code,
                                    ScriptLanguage language = ECMAScript) Q_DECL_OVERRIDE;

    bool isScriptLanguageSupported(ScriptLanguage lang) const Q_DECL_OVERRIDE;

private:
     QVariant encloserForKid(KParts::ScriptableExtension* kid) Q_DECL_OVERRIDE;
     WebEnginePart* part();
};

#endif // WEBENGINEPART_EXT_H
