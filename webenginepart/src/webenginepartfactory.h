/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2008 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
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
    QObject *create(const char* iface, QWidget *parentWidget, QObject *parent, const QVariantList& args) override;

private Q_SLOTS:
    void slotDestroyed(QObject* object);
    void slotSaveHistory(QObject* widget, const QByteArray&);

private:
    QHash<QObject*, QByteArray> m_historyBufContainer;
};

#endif // WEBENGINEPARTFACTORY
