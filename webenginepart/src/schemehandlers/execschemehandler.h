/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2018 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef WEBENGINEPART_EXECSCHEMEHANDLER_H
#define WEBENGINEPART_EXECSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>

namespace WebEngine {

/**
 * @brief Class which handles URLs with the `exec` scheme
 *
 * The path of the URL will be interpreted as a command line and will be run
 * using `KIO::CommandLauncherJob`.
 *
 * @note For security reasons, only URLs with the `konq` scheme are allowed to use URLs with the `exec` scheme.
 * @todo Use a more fine-grained approach to deciding who is allowed to use the `exec` scheme
 */
class ExecSchemeHandler : public QWebEngineUrlSchemeHandler{
    
    Q_OBJECT
    
public:
    
    /**
     * @brief Constructor
     * @param parent the parent object
     */
    ExecSchemeHandler(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ExecSchemeHandler() override;
    
    /**
     * @brief Override of `QWebEngineUrlSchemeHandler::requestStarted`
     *
     * It launches the command, if the initiator has the `konq` scheme.
     *
     * @param job the object describing the request
     */
    void requestStarted(QWebEngineUrlRequestJob * job) override;
};

}

#endif //WEBENGINEPART_EXECSCHEMEHANDLER_H
