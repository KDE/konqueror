/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2022 Stefano Crocco <stefano.crocco@alice.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef NAVIGATIONRECORDER_H
#define NAVIGATIONRECORDER_H

#include <QObject>
#include <QUrl>
#include <QPointer>
#include <QWebEngineUrlRequestInfo>
#include <QMultiHash>

class WebEnginePage;

/**
 * Class used to integrate information from WebEnginePage::acceptNavigationRequest() and from
 * WebEngineUrlRequestInterceptor::interceptRequest()
 *
 * Sometimes it is important to know both which page requested navigating to a given URL and more
 * detailed information about the request itself
 * (for example, whether it is a GET or a POST request). Unfortunately, this kind information is only
 * available to WebEngineUrlRequestInterceptor::interceptRequest() which, in turn, does not have
 * information about which page made a given request. This class provides a workaround for such situation.
 *
 * When a WebEnginePage emits the \link WebEnginePage::mainFrameNavigationRequested mainFrameNavigationRequested() \endlink
 * signal, the corresponding URL, together with the page, is stored in the ::m_pendingNavigations instance variable.
 * WebEngineUrlRequestInterceptor::interceptRequest() calls recordRequestDetails() providing
 * information about the request. This information is associated which made a request for the given
 * URL and which is still pending and the relevant information is stored. That request is then
 * removed from ::m_pendingNavigations. When the page emits the `loadFinished()` signal, or when the
 * page is deleted, all information corresponding to its URL are removed.
 *
 * @note Currently, this class only records whether the request used the POST method.
 * @warning The algorithm described above is heuristic and it cannot guarantee that the association
 * between page and request information is always correct. In particular, if two different pages
 * request the same URL before \link WebEngineUrlRequestInterceptor::interceptRequest interceptRequest() \endlink
 * is called, when \link WebEngineUrlRequestInterceptor::interceptRequest interceptRequest() \endlink is called,
 * there will be no way to know which page the request refers to.
 * In this case, NavigationRecorder makes
 * the arbitrary assumption that the first call to \link WebEngineUrlRequestInterceptor::interceptRequest interceptRequest() \endlink
 * corresponds to the first call to \link WebEnginePage::acceptNavigationRequest() acceptNavigationRequest()\endlink.
 */
class NavigationRecorder : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent the parent object
     */
    NavigationRecorder(QObject *parent=nullptr);

    /**
     * @brief Destructor
     */
    ~NavigationRecorder();

    /**
     * @brief Registers a page so that the NavigationRecorder can connect to its signals
     * @param page the page to register
     */
    void registerPage(WebEnginePage *page);

    /**
     * @brief Records details about a navigation request
     *
     * This method is called by WebEngineUrlRequestInterceptor when it intercepts a request for
     * a main frame. It matches the URL of the request with the URL of a previous call to
     * \link WebEnginePage::acceptNavigationRequest acceptNavigationRequest()\endlink and stores
     * information about it.
     * @param info the information about the request
     */
    void recordRequestDetails(const QWebEngineUrlRequestInfo &info);

    /**
     * @brief Whether the request made by a given page for a given URl is a POST request or not
     * @param url the requested URL
     * @param page the page which made the request
     * @return @c true if a request for @p url by @p page with POST method has been found and `false`
     * otherwise
     * @note a `false` return value may mean that no request for @p url by @p page has been
     * encountered or that it used a different method (typically GET).
     * @warning as explained in the general description of the class, the algorithm to associate
     * requests with pages need not always be 100% accurate
     */
    bool isPostRequest(const QUrl &url, WebEnginePage *page) const;

public slots:
    /**
     * @brief Method called in response to the `WebEnginePage::loadFinished()` signal
     *
     * It removes all information about the given navigation.
     * @param page the page which finished loading
     * @param url the url
     */
    void recordNavigationFinished(WebEnginePage *page, const QUrl &url);

private slots:
    /**
     * @brief Removes all references to deleted pages from the stored data
     */
    void removePage(QObject*);

    /**
     * @brief Records a navigation request from a page
     * @param page the page which requested the navigation
     * @param url the requested URL
     */
    void recordNavigation(WebEnginePage *page, const QUrl &url);

private:
    /**
     * @brief A hash containing all POST requests and the pages which made them, grouped by URL
     */
    QMultiHash<QUrl, QPointer<WebEnginePage>> m_postNavigations;

    /**
     * @brief A hash containing all the navigation requests for which no call to
     * \link WebEnginePage::acceptNavigationRequest acceptNavigationRequest()\endlink has yet been
     * made, * grouped by URLs
     */
    QMultiHash<QUrl, QPointer<WebEnginePage>> m_pendingNavigations;
};

#endif // NAVIGATIONRECORDER_H
