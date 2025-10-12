/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 1999 Simon Hausmann <hausmann@kde.org>
    SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef BROWSEREXTENSION_H
#define BROWSEREXTENSION_H

#include "libkonq_export.h"

#include <kparts/openurlarguments.h>
#include <kparts/readonlypart.h>

#include <memory>

#include <QAction>
#include <qplatformdefs.h> //mode_t

#include <KParts/NavigationExtension>

#include "browserarguments.h"
#include "windowargs.h"
#include "browserinterface.h"

template<class Key, class T>
class QMap;
template<typename T>
class QList;

class KFileItem;
class KFileItemList;
class QPoint;
struct DelayedRequest;

class LIBKONQ_EXPORT BrowserExtension : public KParts::NavigationExtension
{
    Q_OBJECT
public:
    explicit BrowserExtension(KParts::ReadOnlyPart *parent);

    ~BrowserExtension() override;

    /**
     * Set the parameters to use for opening the next URL.
     * This is called by the "hosting" application, to pass parameters to the part.
     * @see BrowserArguments
     */
    void setBrowserArguments(const BrowserArguments &args);

    /**
     * Retrieve the set of parameters to use for opening the URL
     * (this must be called from openUrl() in the part).
     * @see BrowserArguments
     */
    BrowserArguments browserArguments() const;

    void setBrowserInterface(BrowserInterface *impl);

    BrowserInterface *browserInterface() const;

Q_SIGNALS:
    /**
     * Asks the host (browser) to open @p url.
     * To set a reload, the x and y offsets, the service type etc., fill in the
     * appropriate fields in the @p args structure.
     * Hosts should not connect to this signal but to openUrlRequestDelayed().
     */
    void browserOpenUrlRequest(const QUrl &url,
                        const KParts::OpenUrlArguments &arguments = KParts::OpenUrlArguments(),
                        const BrowserArguments &browserArguments = BrowserArguments(), bool temp = false);

    /**
     * @brief As browserOpenUrlRequest except that the browserOpenUrlRequestDelayed signal is emitted
     * synchronously
     *
     * While emitting the browserOpenUrlRequest causes the browserOpenUrlRequestDelayed signal to be emitted
     * after a 0-seconds signal, emitting this signal causes browserOpenUrlRequestDelayed to be emitted
     * immediately, before browserOpenUrlRequestSync returns.
     *
     * @note This should only be called when there's a reason not to want the 0-seconds delay
     *
     * The arguments are the same as browserOpenUrlRequest
     */
    void browserOpenUrlRequestSync(const QUrl &url, const KParts::OpenUrlArguments &arguments, const BrowserArguments &browserArguments, bool temp);

    /**
     * This signal is emitted when openUrlRequest() is called, after a 0-seconds timer.
     * This allows the caller to terminate what it's doing first, before (usually)
     * being destroyed. Parts should never use this signal, hosts should only connect
     * to this signal.
     */
    void browserOpenUrlRequestDelayed(const QUrl &url, const KParts::OpenUrlArguments &arguments, const BrowserArguments &browserArguments, bool temp = false);


    /**
     * Asks the hosting browser to open a new window for the given @p url
     * and return a reference to the content part.
     *
     * @p arguments is optional additional information about how to open the url,
     * @see KParts::OpenUrlArguments
     *
     * @p browserArguments is optional additional information for web browsers,
     * @see BrowserArguments
     *
     * The request for a pointer to the part is only fulfilled/processed
     * if the mimeType is set in the @p browserArguments.
     * (otherwise the request cannot be processed synchronously).
     */
    void browserCreateNewWindow(const QUrl &url,
                         const KParts::OpenUrlArguments &arguments = KParts::OpenUrlArguments(),
                         const BrowserArguments &browserArguments = BrowserArguments(),
                         const WindowArgs &windowArgs = WindowArgs(),
                         KParts::ReadOnlyPart **part = nullptr);


    /**
     * Emit this to make the browser show a standard popup menu for the files @p items.
     *
     * @param global global coordinates where the popup should be shown
     * @param items list of file items which the popup applies to
     * @param args OpenUrlArguments, mostly for metadata here
     * @param browserArguments BrowserArguments, mostly for referrer
     * @param flags enables/disables certain builtin actions in the popupmenu
     * @param actionGroups named groups of actions which should be inserted into the popup, see ActionGroupMap
     */
    void browserPopupMenuFromFiles(const QPoint &global,
                   const KFileItemList &items,
                   const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                   const BrowserArguments &browserArguments = BrowserArguments(),
                   KParts::NavigationExtension::PopupFlags flags = KParts::NavigationExtension::DefaultPopupItems,
                   const KParts::NavigationExtension::ActionGroupMap &actionGroups = ActionGroupMap());

    /**
     * Emit this to make the browser show a standard popup menu for the given @p url.
     *
     * Give as much information about this URL as possible,
     * like @p args.mimeType and the file type @p mode
     *
     * @param global global coordinates where the popup should be shown
     * @param url the URL this popup applies to
     * @param mode the file type of the url (S_IFREG, S_IFDIR...)
     * @param args OpenUrlArguments, set the mimetype of the URL using setMimeType()
     * @param browserArguments BrowserArguments, mostly for referrer
     * @param flags enables/disables certain builtin actions in the popupmenu
     * @param actionGroups named groups of actions which should be inserted into the popup, see ActionGroupMap
     */
    void browserPopupMenuFromUrl(const QPoint &global,
                   const QUrl &url,
                   mode_t mode = static_cast<mode_t>(-1),
                   const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                   const BrowserArguments &browserArguments = BrowserArguments(),
                   KParts::NavigationExtension::PopupFlags flags = KParts::NavigationExtension::DefaultPopupItems,
                   const KParts::NavigationExtension::ActionGroupMap &actionGroups = ActionGroupMap());

private Q_SLOTS:
    void slotCompleted();
    void slotEmitOpenUrlRequestDelayed();
    void slotOpenUrlRequest(const QUrl &url,
                                             const KParts::OpenUrlArguments &arguments,
                                             const BrowserArguments &browserArguments, bool temp);

private:
    BrowserArguments m_browserArgs;
    QList<DelayedRequest> m_requests;
    BrowserInterface *m_browserInterface = nullptr;
};

#endif
