/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef KWEBPLUGINFACTORY_H
#define KWEBPLUGINFACTORY_H

#include <kdemacros.h>

#include <QtWebKit/QWebPluginFactory>
#include <QtCore/QList>
#include <QtGui/QWidget>

class QStringList;

class KDE_EXPORT KWebPluginFactory : public QWebPluginFactory
{
    Q_OBJECT
public:
    KWebPluginFactory(QObject *parent);
    KWebPluginFactory(QWebPluginFactory *delegate, QObject *parent);
    ~KWebPluginFactory();
    virtual QObject *create(const QString &mimeType,
                            const QUrl &url,
                            const QStringList &argumentNames,
                            const QStringList &argumentValues) const;
    virtual QList<Plugin> plugins() const;

private:
    class KWebPluginFactoryPrivate;
    KWebPluginFactoryPrivate* const d;
};

#endif // KWEBPLUGINFACTORY_H
