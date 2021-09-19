/*
    SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqsidebartest.h"

extern "C"
{
    Q_DECL_EXPORT void *create_konq_sidebartest(QWidget *parent, const QString &desktopname, const KConfigGroup &configGroup)
    {
        return new SidebarTest(parent, desktopname, configGroup);
    }
}
