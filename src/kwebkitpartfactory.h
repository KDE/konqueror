/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2008 Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef WEBKITPARTFACTORY
#define WEBKITPARTFACTORY

#include <kpluginfactory.h>

#include <QHash>

class QWidget;

class KWebKitFactory : public KPluginFactory
{
    Q_OBJECT
public:
    virtual ~KWebKitFactory();
    virtual QObject *create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList& args, const QString &keyword);

private Q_SLOTS:
    void slotDestroyed(QObject* object);
    void slotSaveHistory(QObject* widget, const QByteArray&);

private:
    QHash<QObject*, QByteArray> m_historyBufContainer;
};

#endif // WEBKITPARTFACTORY
