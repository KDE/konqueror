/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2009 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPART_EXT_H
#define WEBENGINEPART_EXT_H

#include "kwebenginepartlib_export.h"

#include "interfaces/downloaderextension.h"

#include <QPointer>
#include <QWebEngineDownloadRequest>

#include <KParts/NavigationExtension>

#include <asyncselectorinterface.h>
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

class KWEBENGINEPARTLIB_EXPORT WebEngineNavigationExtension : public BrowserExtension
{
    Q_OBJECT

public:
    WebEngineNavigationExtension(WebEnginePart *parent, const QByteArray& cachedHistoryData);
    ~WebEngineNavigationExtension() override;

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
                             public AsyncSelectorInterface,
                             public HtmlSettingsInterface
{
    Q_OBJECT
    Q_INTERFACES(AsyncSelectorInterface)
    Q_INTERFACES(HtmlSettingsInterface)

public:
    WebEngineHtmlExtension(WebEnginePart* part);

    // HtmlExtension
    QUrl baseUrl() const override;
    bool hasSelection() const override;

    // AsyncSelectorInterface
    /**
     * @brief The async query methods supported by the part
     * @return A list containing only AsyncSelectorInterface::EntireContent
     */
    AsyncSelectorInterface::QueryMethods supportedAsyncQueryMethods() const override;
    void querySelectorAsync(const QString& query, AsyncSelectorInterface::QueryMethod method, SingleElementSelectorCallback& callback) override;
    void querySelectorAllAsync(const QString & query, AsyncSelectorInterface::QueryMethod method, MultipleElementSelectorCallback & callback) override;

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
    static QList<AsyncSelectorInterface::Element> jsonToElementList(const QVariant &json);

    /**
     * @brief Converts the JSON object returned by the javascript function `querySelectorToObject()` in a KParts::SelectorInterface::Element
     * @param json a QVariant containing the JSON representation of the object returned by the javascript function `querySelectorToObject()`
     * @return the element in @p json represented as KParts::SelectorInterface::Element
     */
    static AsyncSelectorInterface::Element jsonToElement(const QVariant &json);

    /**
     * @overload
     * @param obj the JSON object representing the element as returned by the javascript function `querySelectorToObject()`
     */
    static AsyncSelectorInterface::Element jsonToElement(const QJsonObject &obj);
};

class WebEngineDownloaderExtension : public KonqInterfaces::DownloaderExtension
{
    Q_OBJECT
public:
    WebEngineDownloaderExtension(WebEnginePart *parent);
    ~WebEngineDownloaderExtension();
    KonqInterfaces::DownloaderJob* downloadJob(const QUrl &url, quint32 id, QObject *parent=nullptr) override;
    void addDownloadRequest(QWebEngineDownloadRequest *req);
    KParts::ReadOnlyPart* part() const override;

private:
    QMultiHash<QUrl, QWebEngineDownloadRequest*> m_downloadRequests;
};

#endif // WEBENGINEPART_EXT_H
