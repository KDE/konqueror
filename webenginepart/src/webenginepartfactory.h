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

#ifndef WEBENGINEPARTFACTORY
#define WEBENGINEPARTFACTORY

#include <KPluginFactory>

#include <QHash>

class QWidget;

class WebEngineFactory : public KPluginFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "webenginepart.json")
    Q_INTERFACES(KPluginFactory)
public:
    ~WebEngineFactory() override;
    QObject *create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList& args, const QString &keyword) override;

private Q_SLOTS:
    void slotDestroyed(QObject* object);
    void slotSaveHistory(QObject* widget, const QByteArray&);

private:
    QHash<QObject*, QByteArray> m_historyBufContainer;
};

#endif // WEBENGINEPARTFACTORY
