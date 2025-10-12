/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPART_EXT_H
#define WEBENGINEPART_EXT_H

#include "kwebenginepartlib_export.h"

#include "interfaces/downloadjob.h"

#include <QPointer>
#include <QWebEngineDownloadRequest>

#include <KParts/NavigationExtension>

#include <interfaces/selectorinterface.h>
#include <htmlextension.h>
#include <htmlsettingsinterface.h>
#include <textextension.h>
#include <browserextension.h>

class QUrl;
class WebEnginePart;
class WebEngineView;
class WebEnginePage;
class QPrinter;
class QJsonObject;
class QWebEngineScript;

class WebEngineDownloadJob;

class KWEBENGINEPARTLIB_EXPORT WebEngineNavigationExtension : public BrowserExtension
{
    Q_OBJECT

public:
    WebEngineNavigationExtension(WebEnginePart *parent, const QByteArray& cachedHistoryData);
    ~WebEngineNavigationExtension() override;

    int xOffset() override;
    int yOffset() override;

    /**
     * @brief Override of BrowserExtension::saveState
     *
     * If #m_historyWorkaround is `true`, it doesn't save the state corresponding to the
     * current history item but the previous one. This is because #m_historyWorkaround should
     * be `true` only when this is called in response to a `openUrlNotify()` emitted from
     * WebEnginePart::slotUrlChanged rather than from WebEnginePart::slotLoadStarted. Since
     * when WebEnginePart::slotUrlChanged is called, the history already contains the new URL,
     * but we want to save the old URL, we need to save the previous item.
     *
     * @see WebEnginePart::slotUrlChanged
     */
    void saveState(QDataStream &) override;
    void restoreState(QDataStream &) override;
    void saveHistory();

    /**
     * @brief Calls a function with the history workaround enabled
     *
     * It sets #m_historyWorkaround to `true`, calls @p func, then sets `m_historyWorkaround`
     * to `false`. This ensures that #m_historyWorkaround is always `false` except
     * inside @p func.
     *
     * @p func the function to call. It should take no arguments have no return value
     *
     * @warning This should only be called from WebEnginePart::slotUrlChanged
     */
    void withHistoryWorkaround(std::function<void()> func);

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

    /**
     * @brief Performs the actual printing
     *
     * This calls `QWebEngineView::print()` after raising the printer resolution,
     * if it's not high enough.
     *
     * The resolution is only changed when printing to a file, since for physical
     * printers the user may have chosen a low resolution and we want to respect
     * that choice.
     *
     * @param printer the printer to use
     */
    void doPrinting(QPrinter* printer);

    QPointer<WebEnginePart> m_part;
    QPointer<WebEngineView> m_view;
    quint32 m_spellTextSelectionStart;
    quint32 m_spellTextSelectionEnd;
    QByteArray m_historyData;
    QPrinter *mCurrentPrinter;
    bool m_historyWorkaround = false; //!< Whether saveState() should apply the history workaround or not

    /**
     * @brief The minimum resolution when printing to a PDF from the preview dialog
     *
     * This is a compromise between the minimum resolution suggested in the documentation
     * for `QWebEnginePage::print()` and the high resolution value described in `QPrinter::HighResolution`.
     */
    static constexpr int s_previewMinResolution = 600;
};

/**
 * @internal
 * Implements the TextExtension interface
 */
class WebEngineTextExtension : public TextExtension
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
class WebEngineHtmlExtension : public HtmlExtension,
                             public KonqInterfaces::SelectorInterface,
                             public HtmlSettingsInterface
{
    Q_OBJECT
    Q_INTERFACES(KonqInterfaces::SelectorInterface)
    Q_INTERFACES(HtmlSettingsInterface)

public:
    WebEngineHtmlExtension(WebEnginePart* part);

    // HtmlExtension
    QUrl baseUrl() const override;
    bool hasSelection() const override;

    // SelectorInterface
    /**
     * @brief The async query methods supported by the part
     * @return A list containing only SelectorInterface::EntireContent
     */
    KonqInterfaces::SelectorInterface::QueryMethods supportedQueryMethods() const override;
    void querySelector(const QString& query, KonqInterfaces::SelectorInterface::QueryMethod method, SingleElementSelectorCallback& callback) override;
    void querySelectorAll(const QString & query, KonqInterfaces::SelectorInterface::QueryMethod method, MultipleElementSelectorCallback & callback) override;

    // HtmlSettingsInterface
    QVariant htmlSettingsProperty(HtmlSettingsType type) const override;
    bool setHtmlSettingsProperty(HtmlSettingsType type, const QVariant& value) override;

private:
    WebEnginePart* part() const;

    /**
     * @brief Converts the JSON array of elements returned by the javascript function `querySelectorAllToList()` in a list of KParts::SelectorInterface::Element
     * @param json A QVariant containing the JSON representation of the object returned by the javascript function `querySelectorAllToList()`
     * @return all the elements contained in @p json represented as KParts::SelectorInterface::Element
     */
    static QList<SelectorInterface::Element> jsonToElementList(const QVariant &json);

    /**
     * @brief Converts the JSON object returned by the javascript function `querySelectorToObject()` in a KParts::SelectorInterface::Element
     * @param json a QVariant containing the JSON representation of the object returned by the javascript function `querySelectorToObject()`
     * @return the element in @p json represented as KParts::SelectorInterface::Element
     */
    static SelectorInterface::Element jsonToElement(const QVariant &json);

    /**
     * @overload
     * @param obj the JSON object representing the element as returned by the javascript function `querySelectorToObject()`
     */
    static SelectorInterface::Element jsonToElement(const QJsonObject &obj);
};

#endif // WEBENGINEPART_EXT_H
