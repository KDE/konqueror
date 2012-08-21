/*
 * This file is part of the KDE project.
 *
 * Copyright (C) 2012 Dawit Alemayehu <adawit@kde.org>
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

#ifndef WEBPLUGINFACTORY_H
#define WEBPLUGINFACTORY_H

#include <KDE/KWebPluginFactory>

#include <QList>
#include <QUrl>
#include <QWidget>
#include <QPointer>

class KWebKitPart;
class QPoint;

class FakePluginWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool swapping READ swapping)

public:
    FakePluginWidget(uint id, const QUrl& url, const QString& mimeType, QWidget* parent = 0);
    bool swapping() const { return m_swapping; }

Q_SIGNALS:
    void pluginLoaded(uint);

private Q_SLOTS:
    void loadAll();
    void load(bool loadAll = false);
    void showContextMenu(const QPoint&);
    void updateScrollPoisition(int, int, const QRect&);

private:
    bool m_swapping;
    bool m_updateScrollPosition;
    QString m_mimeType;
    uint m_id;
};

class WebPluginFactory : public KWebPluginFactory
{
    Q_OBJECT
public:
    WebPluginFactory (KWebKitPart* part, QObject* parent = 0);
    virtual QObject* create (const QString&, const QUrl&, const QStringList&, const QStringList&) const;
    void resetPluginOnDemandList();

private Q_SLOTS:
    void loadedPlugin(uint);

private:
    QPointer<KWebKitPart> mPart;
    mutable QList<uint> mPluginsLoadedOnDemand;
};

#endif
