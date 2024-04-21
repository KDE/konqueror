/* This file is part of the KDE project

   Copyright (C) 2002 Patrick Charbonnier <pch@valleeurpe.net>
   Copyright (C) 2010 Matthias Fuchs <mat69@gmx.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/

#ifndef KGETPLUGIN_H
#define KGETPLUGIN_H

#include <konq_kpart_plugin.h>
#include "asyncselectorinterface.h"

#include <QPointer>

class KToggleAction;
class HtmlExtension;

class KGetPlugin : public KonqParts::Plugin
{
    Q_OBJECT
public:
    KGetPlugin(QObject *parent, const QVariantList &);
    ~KGetPlugin() override;

private Q_SLOTS:
    void slotShowDrop();
    void slotShowLinks();
    void slotShowSelectedLinks();
    void slotImportLinks();
    void showPopup();

private:
    void getLinks(bool selectedOnly = false);
    void fillLinkListFromHtml(const QUrl &baseUrl, const QList<AsyncSelectorInterface::Element> &elements);

    //TODO KF6: since the async interface doesn't exist anymore in KF6, the SelectorInterfaceType and SelectionInterface
    //could be removed/simplified
    /**
     * @brief The kind of html selector interface to use
     */
    enum class SelectorInterfaceType {
        None, /**< No interface type is supported by the part */
        Sync, /**< Use the synchronous interface */
        Async /**< Use the asynchronous interface */
    };

    /**
     * @brief Struct encapsulating the different selector interfaces supported by the plugin
     */
    struct SelectorInterface {
        /**
         * @brief Constructor
         * @param ext The HTML Extension
         */
        SelectorInterface(HtmlExtension *ext);
        /**
         * @brief The query methods supported by the HTML part
         * @return The query methods supported by the HTML part or KParts::SelectionInterface::None if no selector interface
         * is provided by the part
         */
        AsyncSelectorInterface::QueryMethods supportedMethods() const;
        /**
         * @brief Whether the HTML extension provides either the synchronous or the asynchronous interface
         * @return `true` if the HTML extension provides at least one of the two interfaces and `false` otherwise
         */
        bool hasInterface() const;

        /**
         * @brief The type of selector interface provided by the HTML extension
         */
        SelectorInterfaceType interfaceType = SelectorInterfaceType::None;

        /**
         * @brief A pointer to the AsyncSelectorInterface or `nullptr` if he HTML extension doesn't provide the
         * AsyncSelectorInterface interface
         **/
        AsyncSelectorInterface *asyncInterface = nullptr;
    };

    QStringList m_linkList;
    KToggleAction *m_dropTargetAction;
};

#endif
