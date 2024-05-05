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
    switch (argument.metaType().id()) {
    case QMetaType::QString:
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(QString, argument.toString()));
        break;
    case QMetaType::QStringList: {
        QStringList strLst = argument.toStringList();
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(QStringList*, &strLst));
        break;
    }
    case QMetaType::Int:
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(int, argument.toInt()));
        break;
    case QMetaType::UInt: {
        unsigned int i = argument.toUInt();
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(uint*, &i));
        break;
    }
    case QMetaType::Bool:
        QMetaObject::invokeMethod(this, name,
                                  Q_ARG(bool, argument.toBool()));
        break;
    default:
        break;
    }
    // clang-format on
}

#include "moc_browserinterface.cpp"
