/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2001 Simon Hausmann <hausmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "browserinterface.h"

#include <QStringList>
#include <QVariant>

BrowserInterface::BrowserInterface(QObject *parent)
    : QObject(parent)
{
}

BrowserInterface::~BrowserInterface()
{
}

void BrowserInterface::callMethod(const char *name, const QVariant &argument)
{
    // clang-format off
    switch (argument.type()) {
    case QVariant::Invalid:
        break;
    case QVariant::String:
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(QString, argument.toString()));
        break;
    case QVariant::StringList: {
        QStringList strLst = argument.toStringList();
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(QStringList*, &strLst));
        break;
    }
    case QVariant::Int:
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(int, argument.toInt()));
        break;
    case QVariant::UInt: {
        unsigned int i = argument.toUInt();
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(uint*, &i));
        break;
    }
    case QVariant::Bool:
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(bool, argument.toBool()));
        break;
    default:
        break;
    }
    // clang-format on
}

#include "moc_browserinterface.cpp"
