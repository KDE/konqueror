/*
    SPDX-FileCopyrightText: 2008 Xavier Vello <xavier.vello@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCM_BOOKMARKS_H
#define KCM_BOOKMARKS_H

// KDE
#include <kcmodule.h>

// Local
#include "ui_bookmarks.h"

class BookmarksConfigModule : public KCModule
{
    Q_OBJECT

public:
    //TODO KF6: when dropping compatibility with KF5, remove QVariantList argument
    BookmarksConfigModule(QObject *parent, const KPluginMetaData &md={}, const QVariantList &args={});
    ~BookmarksConfigModule() override;

    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void clearCache();
    void configChanged();

private:
    Ui::BookmarksConfigUI ui;
};

#endif // KCM_BOOKMARKS_H

