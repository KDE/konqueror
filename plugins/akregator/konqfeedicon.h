/*
    This file is part of Akregator.

    SPDX-FileCopyrightText: 2004 Teemu Rytilahti <tpr@d5k.net>

    SPDX-License-Identifier: GPL-2.0-or-later

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#ifndef KONQFEEDICON_H
#define KONQFEEDICON_H

#include <qpointer.h>
#include <kparts/plugin.h>
#include <QMenu>
#include "feeddetector.h"

/**
@author Teemu Rytilahti
*/
class KUrlLabel;

namespace KParts
{
class StatusBarExtension;
class ReadOnlyPart;
}

namespace Akregator
{
class KonqFeedIcon : public KParts::Plugin
{
    Q_OBJECT

public:
    KonqFeedIcon(QObject *parent, const QVariantList &args);
    ~KonqFeedIcon() override;

private:
    /**
    * Tells you if there is feed(s) on the page.
    * @return true when there is feed(s) available
    */
    bool feedFound();

    QPointer<KParts::ReadOnlyPart> m_part;
    KUrlLabel *m_feedIcon;
    KParts::StatusBarExtension *m_statusBarEx;
    FeedDetectorEntryList m_feedList;
    QPointer<QMenu> m_menu;

private slots:
    void contextMenu();
    void addFeedIcon();
    void removeFeedIcon();

    void addAllFeeds();
    void addFeed();
};

}
#endif
