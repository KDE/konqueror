/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef BROWSERINTERFACE_H
#define BROWSERINTERFACE_H

#include <libkonq_export.h>

#include <QObject>

class QVariant;

/**
 * @class BrowserInterface browserinterface.h
 *
 * @short The purpose of this interface is to allow a direct communication between
 * a KPart and the hosting browser shell (for example Konqueror) . A
 * shell implementing this interface can propagate it to embedded kpart
 * components by using the setBrowserInterface call of the part's
 * KParts::BrowserExtension object.
 *
 * This interface looks not very rich, but the main functionality is
 * implemented using the callMethod method for part->shell
 * communication and using Qt properties for allowing a part to
 * to explicitly query information from the shell.
 *
 * Konqueror in particular, as 'reference' implementation, provides
 * the following functionality through this interface:
 *
 * Qt properties:
 *   Q_PROPERTY( uint historyLength READ historyLength );
 *
 * Callable methods:
 *       void goHistory( int );
 *
 */
class LIBKONQ_EXPORT BrowserInterface : public QObject
{
    Q_OBJECT
public:
    explicit BrowserInterface(QObject *parent);
    ~BrowserInterface() override;

    /**
     * Perform a dynamic invocation of a method in the BrowserInterface
     * implementation. Methods are to be implemented as simple Qt slots.
     * You should only include the method name, and not the signature,
     * in the name argument.
     */
    void callMethod(const char *name, const QVariant &argument);
};

#endif
