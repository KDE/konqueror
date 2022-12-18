/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
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
class WebEngineView;
class WebEnginePage;
class QPrinter;

class KWEBENGINEPARTLIB_EXPORT WebEngineBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT

public:
    WebEngineBrowserExtension(WebEnginePart *parent, const QByteArray& cachedHistoryData);
    ~WebEngineBrowserExtension() override;

    int xOffset() override;
    int yOffset() override;
    void saveState(QDataStream &) override;
    void restoreState(QDataStream &) override;
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
    void slotSaveFullHTMLPage();
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
    void slotSaveLinkAs(const QUrl &url);
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

private Q_SLOTS:
    void slotHandlePagePrinted(bool result);
private:
    WebEngineView* view();
    WebEnginePage* page();

    QPointer<WebEnginePart> m_part;
    QPointer<WebEngineView> m_view;
    quint32 m_spellTextSelectionStart;
    quint32 m_spellTextSelectionEnd;
    QByteArray m_historyData;
    QPrinter *mCurrentPrinter;
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

    bool hasSelection() const override;
    QString selectedText(Format format) const override;
    QString completeText(Format format) const override;

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
    QUrl baseUrl() const override;
    bool hasSelection() const override;

    // SelectorInterface
    QueryMethods supportedQueryMethods() const override;
    Element querySelector(const QString& query, KParts::SelectorInterface::QueryMethod method) const override;
    QList<Element> querySelectorAll(const QString& query, KParts::SelectorInterface::QueryMethod method) const override;

    // HtmlSettingsInterface
    QVariant htmlSettingsProperty(HtmlSettingsType type) const override;
    bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant& value) override;

private:
    WebEnginePart* part() const;
};

class WebEngineScriptableExtension : public KParts::ScriptableExtension
{
  Q_OBJECT

public:
    WebEngineScriptableExtension(WebEnginePart* part);

    QVariant rootObject() override;

    QVariant get(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName) override;

    bool put(ScriptableExtension* callerPrincipal, quint64 objId, const QString& propName, const QVariant& value) override;

    bool setException(ScriptableExtension* callerPrincipal, const QString& message) override;

    QVariant evaluateScript(ScriptableExtension* callerPrincipal,
                                    quint64 contextObjectId,
                                    const QString& code,
                                    ScriptLanguage language = ECMAScript) override;

    bool isScriptLanguageSupported(ScriptLanguage lang) const override;

private:
     QVariant encloserForKid(KParts::ScriptableExtension* kid) override;
     WebEnginePart* part();
};

#endif // WEBENGINEPART_EXT_H
