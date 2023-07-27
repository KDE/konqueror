/*
    SPDX-FileCopyrightText: 2001 Dawit Alemayehu <adawit@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __UACHANGER_PLUGIN_H
#define __UACHANGER_PLUGIN_H

#include <QMap>
#include <QStringList>
#include <QUrl>

#include <konq_kpart_plugin.h>
#include <kparts/readonlypart.h>

class KActionMenu;
class QAction;
class QActionGroup;
class KConfig;

namespace KIO
{
}

class UAChangerPlugin : public KonqParts::Plugin
{
    Q_OBJECT

public:
    UAChangerPlugin(QObject* parent, const QVariantList&);
    ~UAChangerPlugin() override;

protected slots:
    void slotAboutToShow();
    void slotItemSelected(QAction *);

private:
    using TemplateMap = QMap<QString, QString>;

    void initMenu();
    QList<QAction*> fillMenu(const TemplateMap &templates);
    void clearMenu();

    KParts::ReadOnlyPart *m_part;
    KActionMenu *m_pUAMenu;
    QAction *m_defaultAction;
    QActionGroup *m_actionGroup;
};

#endif
